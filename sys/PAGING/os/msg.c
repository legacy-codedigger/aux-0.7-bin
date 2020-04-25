/*	@(#)msg.c	UniPlus VVV.2.1.1	*/

/*
**	Inter-Process Communication Message Facility.
*/

#ifdef lint
#include "sys/sysinclude.h"
#else lint
#include	"sys/types.h"
#include	"sys/mmu.h"
#include	"sys/seg.h"
#include	"sys/param.h"
#include	"sys/signal.h"
#include	"sys/time.h"
#include	"sys/user.h"
#include	"sys/page.h"
#include	"sys/region.h"
#include	"sys/proc.h"
#include	"sys/buf.h"
#include	"sys/errno.h"
#include	"sys/map.h"
#include	"sys/ipc.h"
#include	"sys/msg.h"
#include	"sys/systm.h"
#include	"sys/sysmacros.h"
#include	"sys/uio.h"
#endif lint

extern struct map	msgmap[];	/* msg allocation map */
extern struct msqid_ds	msgque[];	/* msg queue headers */
extern struct msg	msgh[];		/* message headers */
extern struct msginfo	msginfo;	/* message parameters */
struct msg		*msgfp;		/* ptr to head of free header list */
paddr_t			msg;		/* base address of message buffer */
extern unsigned int malloc();

struct ipc_perm		*ipcget();
struct msqid_ds		*msgconv();

/* Convert bytes to msg segments. */
#define	btoq(X)	((X + msginfo.msgssz - 1) / msginfo.msgssz)

/*
**	msgconv - Convert a user supplied message queue id into a ptr to
**		  a locked msqid_ds structure.
*/

struct msqid_ds *
msgconv(id)
register int	id;
{
	register struct msqid_ds	*qp;	/* ptr to associated q slot */

	if (id < 0) {
		u.u_error = EINVAL;
		return(NULL);
	}
	qp = &msgque[id % msginfo.msgmni];
	if ((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
		id / msginfo.msgmni != qp->msg_perm.seq) {
		u.u_error = EINVAL;
		return(NULL);
	}
	return(qp);
}

/*
**	msgctl - Msgctl system call.
*/

msgctl()
{
	register struct a {
		int		msgid,
				cmd;
		struct msqid_ds	*buf;
	}		*uap = (struct a *)u.u_ap;
	struct msqid_ds			ds;	/* queue work area */
	register struct msqid_ds	*qp;	/* ptr to associated q */

	if ((qp = msgconv(uap->msgid)) == NULL)
		return;
	u.u_rval1 = 0;
	switch(uap->cmd) {
	case IPC_RMID:
		if (u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
			&& !suser())
			return;
		while(qp->msg_first)
			msgfree(qp, (struct msg *)NULL, qp->msg_first);
		qp->msg_cbytes = 0;
		if (uap->msgid + msginfo.msgmni < 0)
			qp->msg_perm.seq = 0;
		else
			qp->msg_perm.seq++;
		if (qp->msg_perm.mode & MSG_RWAIT)
			wakeup((caddr_t)&qp->msg_qnum);
		if (qp->msg_perm.mode & MSG_WWAIT)
			wakeup((caddr_t)qp);
		qp->msg_perm.mode = 0;
		break;

	case IPC_SET:
		if (u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
			&& !suser())
			return;
		if (copyin(uap->buf, &ds, sizeof(ds))) {
			u.u_error = EFAULT;
			return;
		}
		if (ds.msg_qbytes > qp->msg_qbytes && !suser())
			return;
		qp->msg_perm.uid = ds.msg_perm.uid;
		qp->msg_perm.gid = ds.msg_perm.gid;
		qp->msg_perm.mode = (qp->msg_perm.mode & ~0777) |
			(ds.msg_perm.mode & 0777);
		qp->msg_qbytes = ds.msg_qbytes;
		qp->msg_ctime = time.tv_sec;
		break;

	case IPC_STAT:
		if (ipcaccess(&qp->msg_perm, MSG_R))
			return;
		if (copyout(qp, uap->buf, sizeof(*qp))) {
			u.u_error = EFAULT;
			return;
		}
		break;

	default:
		u.u_error = EINVAL;
		return;
	}
}

/*
**	msgfree - Free up space and message header, relink pointers on
**	q, and wakeup anyone waiting for resources.  Called with *qp
**	locked.
*/

msgfree(qp, pmp, mp)
register struct msqid_ds	*qp;	/* ptr to q of mesg being freed */
register struct msg		*mp,	/* ptr to msg being freed */
				*pmp;	/* ptr to mp's predecessor */
{
	/* Unlink message from the q. */
	if (pmp == NULL)
		qp->msg_first = mp->msg_next;
	else
		pmp->msg_next = mp->msg_next;
	if (mp->msg_next == NULL)
		qp->msg_last = pmp;
	qp->msg_qnum--;
	if (qp->msg_perm.mode & MSG_WWAIT) {
		qp->msg_perm.mode &= ~MSG_WWAIT;
		wakeup((caddr_t)qp);
	}

	/* Free up message text. */
	if (mp->msg_ts)
		mfree(msgmap, btoq(mp->msg_ts), mp->msg_spot + 1);

	/* Free up header */
	mp->msg_next = msgfp;
	if (msgfp == NULL)
		wakeup((caddr_t)&msgfp);
	msgfp = mp;
}

/*
**	msgget - Msgget system call.
*/

msgget()
{
	register struct a {
		key_t	key;
		int	msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int				s;	/* ipcget status return */
	int	msgipcget();

	if ((qp = (struct msqid_ds *)
		ipcget(uap->key, uap->msgflg, msgipcget, &s)) == NULL)
		return;

	if (s) {
		/* This is a new queue.  Finish initialization. */
		qp->msg_first = qp->msg_last = NULL;
		qp->msg_qnum = 0;
		qp->msg_qbytes = msginfo.msgmnb;
		qp->msg_lspid = qp->msg_lrpid = 0;
		qp->msg_stime = qp->msg_rtime = 0;
		qp->msg_ctime = time.tv_sec;
	}
	u.u_rval1 = qp->msg_perm.seq * msginfo.msgmni + (qp - msgque);
}

/*
**	msginit - Called by main(main.c) to initialize message queues.
*/

msginit()
{
	register int			i;	/* loop control */
	register struct msg		*mp;	/* ptr to msg begin linked */
	register int bs;			/* message buffer size */

	/* Allocate physical memory for message buffer.
	 */

	bs = btop(msginfo.msgseg * msginfo.msgssz);
	msg = (paddr_t) sptalloc(bs, PG_V | PG_RW, 0);
	if (msg == NULL) {
		printf("Can't allocate message buffer.\n");
		msginfo.msgseg = 0;
	}
	mapinit(msgmap, msginfo.msgmap);
	mfree(msgmap, (int)msginfo.msgseg, 1);
	for(i = 0, mp = msgfp = msgh;++i < msginfo.msgtql;mp++)
		mp->msg_next = mp + 1;
}

/*
**	msgrcv - Msgrcv system call.
*/

msgrcv()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		long		msgtyp;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msg		*mp,	/* ptr to msg on q */
					*pmp,	/* ptr to mp's predecessor */
					*smp,	/* ptr to best msg on q */
					*spmp;	/* ptr to smp's predecessor */
	register struct msqid_ds	*qp, *qp1;	/* ptr to associated q */
	int				sz;	/* transfer byte count */
	register struct user *up;

	up = &u;
	if ((qp=msgconv(uap->msqid)) == NULL)
		return;
	if (ipcaccess(&qp->msg_perm, MSG_R))
		return;
	if (uap->msgsz < 0) {
		up->u_error = EINVAL;
		return;
	}
	smp = spmp = NULL;
findmsg:
	if (((qp1 = msgconv(uap->msqid)) == NULL) || (qp1 != qp)) {
		up->u_error = EIDRM;
		return;
	}
	pmp = NULL;
	mp = qp->msg_first;
	if (uap->msgtyp == 0)
		smp = mp;
	else
		for(;mp;pmp = mp, mp = mp->msg_next) {
			if (uap->msgtyp > 0) {
				if (uap->msgtyp != mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
				break;
			}
			if (mp->msg_type <= -uap->msgtyp) {
				if (smp && smp->msg_type <= mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
			}
		}
	if (smp) {
		if (uap->msgsz < smp->msg_ts)
			if (!(uap->msgflg & MSG_NOERROR)) {
				up->u_error = E2BIG;
				return;
			} else
				sz = uap->msgsz;
		else
			sz = smp->msg_ts;
		if (copyout((caddr_t)&smp->msg_type, uap->msgp,
			sizeof(smp->msg_type))) {
			up->u_error = EFAULT;
			return;
		}
		if (sz) {
			struct uio uio;
			struct iovec iov;

			iov.iov_base = (caddr_t)uap->msgp + sizeof(smp->msg_type);
			iov.iov_len = sz;
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_offset = 0;
			uio.uio_seg = UIOSEG_USER;
			uio.uio_resid = sz;
			up->u_error = uiomove((caddr_t)(msg + msginfo.msgssz * smp->msg_spot),
				sz, UIO_READ, &uio);
			if (up->u_error)
				return;
		}
		up->u_rval1 = sz;
		qp->msg_cbytes -= smp->msg_ts;
		qp->msg_lrpid = up->u_procp->p_pid;
		qp->msg_rtime = time.tv_sec;
		curpri = PMSG;
		msgfree(qp, spmp, smp);
		return;
	}
	if (uap->msgflg & IPC_NOWAIT) {
		up->u_error = ENOMSG;
		return;
	}
	qp->msg_perm.mode |= MSG_RWAIT;
	if (sleep((caddr_t)&qp->msg_qnum, PMSG | PCATCH)) {
		up->u_error = EINTR;
		return;
	}
	if (msgconv(uap->msqid) == NULL) {
		up->u_error = EIDRM;
		return;
	}
	goto findmsg;
}

/*
**	msgsnd - Msgsnd system call.
*/

msgsnd()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp, *qp1; /* ptr to associated q */
	register struct msg		*mp;	/* ptr to allocated msg hdr */
	register int			cnt,	/* byte count */
					spot;	/* msg pool allocation spot */
	long				type;	/* msg type */
	register struct user *up;


	up = &u;
	if ((qp=msgconv(uap->msqid)) == NULL)
		return;
	if (ipcaccess(&qp->msg_perm, MSG_W))
		return;
	if ((cnt = uap->msgsz) < 0 || cnt > msginfo.msgmax) {
		up->u_error = EINVAL;
		return;
	}
	if (copyin(uap->msgp, (caddr_t)&type, sizeof(type))) {
		up->u_error = EFAULT;
		return;
	}
	if (type < 1) {
		up->u_error = EINVAL;
		return;
	}

getres:
	/* Be sure that q has not been removed. */
	if (((qp1=msgconv(uap->msqid)) == NULL) || qp1 != qp) {
		up->u_error = EIDRM;
		return;
	}

	/* Allocate space on q, message header, & buffer space. */
	if (cnt + qp->msg_cbytes > qp->msg_qbytes) {
		if (uap->msgflg & IPC_NOWAIT) {
			up->u_error = EAGAIN;
			return;
		}
		qp->msg_perm.mode |= MSG_WWAIT;
		if (sleep((caddr_t)qp, PMSG | PCATCH)) {
			up->u_error = EINTR;
			qp->msg_perm.mode &= ~MSG_WWAIT;
			wakeup((caddr_t)qp);
			return;
		}
		goto getres;
	}
	if (cnt && (spot = malloc(msgmap, btoq(cnt))) == NULL || msgfp == NULL) {
		if (uap->msgflg & IPC_NOWAIT) {
			up->u_error = EAGAIN;
			return;
		}
		mapwant(msgmap)++;
		if (sleep((caddr_t)msgmap, PMSG | PCATCH)) {
			up->u_error = EINTR;
			return;
		}
		goto getres;
	}

	/* Everything is available, copy in text and put msg on q. */
	if (cnt) {
		struct uio uio;
		struct iovec iov;

		iov.iov_base = (caddr_t)uap->msgp + sizeof(type);
		iov.iov_len = cnt;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_seg = UIOSEG_USER;
		uio.uio_resid = cnt;
		up->u_error = uiomove((caddr_t)(msg + msginfo.msgssz * --spot),
			cnt, UIO_WRITE, &uio);
		if (up->u_error) {
			mfree(msgmap, btoq(cnt), spot + 1);
			return;
		}
	}
	qp->msg_qnum++;
	qp->msg_cbytes += cnt;
	qp->msg_lspid = up->u_procp->p_pid;
	qp->msg_stime = time.tv_sec;
	mp = msgfp;
	msgfp = mp->msg_next;
	mp->msg_next = NULL;
	mp->msg_type = type;
	mp->msg_ts = cnt;
	mp->msg_spot = cnt ? spot : -1;
	if (qp->msg_last == NULL)
		qp->msg_first = qp->msg_last = mp;
	else {
		qp->msg_last->msg_next = mp;
		qp->msg_last = mp;
	}
	if (qp->msg_perm.mode & MSG_RWAIT) {
		qp->msg_perm.mode &= ~MSG_RWAIT;
		curpri = PMSG;
		wakeup((caddr_t)&qp->msg_qnum);
	}
	up->u_rval1 = 0;
}

/*
	routine to return info to ipcget
*/

msgipcget(i, ip)
register int i;
register struct ipc_perm **ip;
{
	if (i < 0 || i >= msginfo.msgmni)
		return(0);
	*ip = &(msgque[i].msg_perm);
	return(1);
}

/*
**	msgsys - System entry point for msgctl, msgget, msgrcv, and
**		 msgsnd system calls.
*/

msgsys()
{
	int		msgctl(),
			msgget(),
			msgrcv(),
			msgsnd();
	static int	(*calls[])() = { msgget, msgctl, msgrcv, msgsnd };
	register struct a {
		unsigned	id;	/* function code id */
		int		*ap;	/* arg pointer for recvmsg */
	}		*uap = (struct a *)u.u_ap;

	if (uap->id > 3) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*calls[uap->id])();
}

/* <@(#)msg.c	6.3.1.1> */
