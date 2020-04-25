/*	@(#)syslocal.c	UniPlus 2.1.5	*/

#include "compat.h"
#include "sys/param.h"
#ifdef PAGING
#include "sys/uconfig.h"
#endif PAGING
#include "sys/types.h"
#include "sys/mmu.h"
#ifdef PAGING
#include "sys/page.h"
#include "sys/region.h"
#endif PAGING
#include "sys/sysmacros.h"
#include "sys/seg.h"
#include "sys/time.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/user.h"
#include "sys/systm.h"
#include "sys/vnode.h"
#include "sys/vfs.h"
#include "sys/file.h"
#include "sys/conf.h"
#include "sys/buf.h"
#include "sys/map.h"
#include "sys/var.h"

#ifdef PAGING
#include "sys/reg.h"
#include "sys/sysinfo.h"
#include "sys/swap.h"
#include "sys/debug.h"
#include "sys/buserr.h"
#include "sys/tuneable.h"
#endif PAGING

#include "sys/protosw.h"
#include "sys/socket.h"
#include "sys/socketvar.h"
#include "net/if.h"
#include "net/netisr.h"

#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"

#include "vax/vaxque.h"

#include "vaxuba/ubavar.h"

/*
 * These routines implement local system calls
 */

short   netoff = 0;
int	nselcoll;
int	setrun();

/*
 * Initialize network code.  Called from main().
 */
netinit()
{
	register int s;
	extern struct uba_device ubdinit[];
	register struct uba_driver *udp;
	register struct uba_device *ui = &ubdinit[0];
	extern int nulldev();

	if (netoff)
		return;
#ifdef	HOWFAR
	printf("netinit calling mbinit\n");
#endif
	mbinit();
	for(ui = &ubdinit[0] ; udp = ui->ui_driver ; ui++) {
		if (ui->ui_alive)
			continue;
		if (*udp->ud_probe != nulldev) {
#ifdef	HOWFAR
			printf("netinit calling 0x%x (ud_probe for unit %d)\n", *udp->ud_probe, ui->ui_unit);
#endif
			if ((*udp->ud_probe)(ui)) {
				ui->ui_alive = 1;
				udp->ud_dinfo[ui->ui_unit] = ui;
#ifdef	HOWFAR
				printf("netinit calling 0x%x (ud_attach for unit %d)\n", *udp->ud_attach, ui->ui_unit);
#endif
				(*udp->ud_attach)(ui);
			}
		}
	}
#ifdef INET
#ifdef	HOWFAR
	printf("netinit calling loattach\n");
#endif
	loattach();			/* XXX */
#ifdef	HOWFAR
	printf("netinit calling ifinit\n");
#endif
	/*
	 * Block reception of incoming packets
	 * until protocols have been initialized.
	 */
	s = splimp();
	ifinit();
#ifdef	HOWFAR
	printf("netinit calling domaininit\n");
#endif
	domaininit();			/* must follow interfaces */
	splx(s);
#endif
}

/*
 * Select system call.
 */
select()
{
	register struct uap  {
		int	nd;
		long	*in, *ou, *ex;
		struct	timeval *tv;
	} *uap = (struct uap *)u.u_ap;
	int ibits[3], obits[3];
	struct timeval atv;
	int s, ncoll, mask;
	label_t lqsav;

	obits[0] = obits[1] = obits[2] = 0;
	if (uap->nd > NOFILE)
		uap->nd = NOFILE;	/* forgiving, if slightly wrong */
	if (uap->nd < 0) {
		u.u_error = EBADF;
		return;
	}
	mask = (1 << uap->nd) - 1;

#define	getbits(name, x) \
	if (uap->name) { \
		u.u_error = copyin((caddr_t)uap->name, (caddr_t)&ibits[x], \
		    sizeof (ibits[x])); \
		if (u.u_error) \
			goto done; \
		ibits[x] &= mask; \
	} else \
		ibits[x] = 0;
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	if (uap->tv) {
		u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
			sizeof (atv));
		if (u.u_error)
			goto done;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			goto done;
		}
		s = spl7(); timevaladd(&atv, &time); splx(s);
	}
retry:
	ncoll = nselcoll;
	u.u_procp->p_flag |= SSEL;
	u.u_rval1 = selscan(ibits, obits);
	if (u.u_error || u.u_rval1)
		goto done;
	s = spl6();
	/* this should be timercmp(&time, &atv, >=) */
	if (uap->tv && (time.tv_sec > atv.tv_sec ||
	    time.tv_sec == atv.tv_sec && time.tv_usec >= atv.tv_usec)) {
		splx(s);
		goto done;
	}
	if ((u.u_procp->p_flag & SSEL) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~SSEL;
		splx(s);
		goto retry;
	}
	u.u_procp->p_flag &= ~SSEL;
	if (uap->tv) {
		bcopy((caddr_t) u.u_qsav, (caddr_t) lqsav, sizeof(label_t));
		if (save(u.u_qsav)) {
			untimeout(setrun, (caddr_t)u.u_procp);
			u.u_error = EINTR;
			splx(s);
			goto done;
		}
		timeout(setrun, (caddr_t)u.u_procp, hzto(&atv));
	}
	(void) sleep((caddr_t)&selwait, PZERO+1);
	if (uap->tv) {
		bcopy((caddr_t) lqsav, (caddr_t) u.u_qsav, sizeof(label_t));
		untimeout(setrun, (caddr_t)u.u_procp);
	}
	splx(s);
	goto retry;
done:
#define	putbits(name, x) \
	if (uap->name) { \
		int error = copyout((caddr_t)&obits[x], (caddr_t)uap->name, \
		    sizeof (obits[x])); \
		if (error) \
			u.u_error = error; \
	}
	putbits(in, 0);
	putbits(ou, 1);
	putbits(ex, 2);
#undef putbits
}

selscan(ibits, obits)
	int *ibits, *obits;
{
	register int which, bits, i;
	int flag;
	struct file *fp;
	int n = 0;

	for (which = 0; which < 3; which++) {
		bits = ibits[which];
		obits[which] = 0;
		switch (which) {

		case 0:
			flag = FREAD; break;

		case 1:
			flag = FWRITE; break;

		case 2:
			flag = 0; break;
		}
		while (i = ffs((long)bits)) {
			bits &= ~(1<<(i-1));
			fp = u.u_ofile[i-1];
			if (fp == NULL) {
				u.u_error = EBADF;
				break;
			}
			if ((*fp->f_ops->fo_select)(fp, flag)) {
				obits[which] |= (1<<(i-1));
				n++;
			}
		}
	}
	return (n);
}

/*ARGSUSED*/
seltrue(dev, flag)
	dev_t dev;
	int flag;
{

	return (1);
}

selwakeup(p, coll)
	register struct proc *p;
	int coll;
{

	if (coll) {
		nselcoll++;
		wakeup((caddr_t)&selwait);
	}
	if (p) {
		int s = spl6();
		if (p->p_wchan == (caddr_t)&selwait)
			setrun(p);
		else if (p->p_flag & SSEL)
			p->p_flag &= ~SSEL;
		splx(s);
	}
}

#ifndef	mc68020
#define	loint(l)	((int) (l) & 0177777)
#define	hiint(l)	(((int) ((l) >> 16)) & 0xffff)

ffs(mask)
	long mask;
{
	register int i;
	register imask;

	if (mask == 0)
		return (0);
	imask = loint(mask);
	for (i = 1; i <= 16; i++) {
		if (imask & 1)
			return (i);
		imask >>= 1;
	}
	imask = hiint(mask);
	for(; i <= 32; i++) {
		if (imask & 1)
			return (i);
		imask >>= 1;
	}
	return (0);     /* can't get here anyway! */
}
#endif	mc68020

char	hostname[32];
int	hostnamelen;

gethostname()
{
	register struct a {
		char	*hostname;
		int	len;
	} *uap = (struct a *)u.u_ap;
	register u_int len;

	len = uap->len;
	if (len > hostnamelen + 1)
		len = hostnamelen + 1;
	u.u_error = copyout((caddr_t)hostname, (caddr_t)uap->hostname, len);
}

sethostname()
{
	register struct a {
		char	*hostname;
		u_int	len;
	} *uap = (struct a *)u.u_ap;

	if (!suser())
		return;
	if (uap->len > sizeof (hostname) - 1) {
		u.u_error = EINVAL;
		return;
	}
	hostnamelen = uap->len;
	u.u_error = copyin((caddr_t)uap->hostname, hostname, uap->len);
	hostname[hostnamelen] = 0;
}

long	hostid;

gethostid()
{
	u.u_rval1 = hostid;
}

sethostid()
{
	struct a {
		int	hostid;
	} *uap = (struct a *)u.u_ap;

	if (suser())
		hostid = uap->hostid;
}

char domainname[32];
int domainnamelen;

getdomainname()
{
	register struct a {
		char    *domainname;
		int     len;
	} *uap = (struct a *)u.u_ap;
	register u_int len;

	len = uap->len;
	if (len > domainnamelen + 1)
		len = domainnamelen + 1;
	u.u_error = copyout((caddr_t)domainname,(caddr_t)uap->domainname,len);
}
										setdomainname()
{
	register struct a {
		char    *domainname;
		u_int   len;
	} *uap = (struct a *)u.u_ap;

	if (!suser())
		return;
	if (uap->len > sizeof (domainname) - 1) {
		u.u_error = EINVAL;
		return;
	}
	domainnamelen = uap->len;
	u.u_error = copyin((caddr_t)uap->domainname, domainname, uap->len);
	domainname[domainnamelen] = 0;
}

/*
 * compare bytes; same result as VAX cmpc3.
 */
bcmp(s1, s2, n)
register char *s1, *s2;
register unsigned	n;
{
	do
		if (*s1++ != *s2++)
			break;
	while (--n);

	return(n);
}

/*
 * Insert an entry onto queue.
 */
_insque(elem, prev)
	register struct qelem *elem, *prev;
{
	register s = spl7();

	elem->q_forw = prev->q_forw;
	elem->q_back = prev;
	if (prev->q_forw)
		prev->q_forw->q_back = elem;
	prev->q_forw = elem;
	splx(s);
}

/*
 * Remove an entry from queue.
 */
_remque(elem)
	register struct qelem *elem;
{
	register s = spl7();

	elem->q_back->q_forw = elem->q_forw;
	if (elem->q_forw)
		elem->q_forw->q_back = elem->q_back;
	splx(s);
}

/*
 * Is p an inferior of the current process?
 * Due to the lack of a parent/child chain,
 * this function is gross! Hopefully, it will
 * not be used often.
 */
inferior(p)
register struct proc *p;
{
	register struct proc *q;

	for (; p != u.u_procp; p = p->p_pptr)
		if (p->p_ppid == 0)
			return(0);
	return(1);
}

struct proc *
pfind(pid)
register int	pid;
{
	register struct proc *p;

	for (p = &proc[1]; p < (struct proc *) v.ve_proc; p++)
		if (p->p_pid == pid)
			return (p);

	return ((struct proc *) NULL);
}

/*
 * System calls related to processes and protection
 */

getgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--)
		if (gp[-1] >= 0)
			break;
	if (uap->gidsetsize < gp - u.u_groups) {
		u.u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u.u_groups;
	u.u_error = copyout((caddr_t)u.u_groups, (caddr_t)uap->gidset,
	    uap->gidsetsize * sizeof (u.u_groups[0]));
	if (u.u_error)
		return;
	u.u_rval1 = uap->gidsetsize;
}

setreuid()
{
	struct a {
		int	ruid;
		int	euid;
	} *uap;
	register int ruid, euid;

	if ((u.u_procp->p_compatflags & COMPAT_BSDPROT) == 0) {
		/*
		 *	Pretend this is setuid():  in .../gen/setuid.c,
		 *	setuid(uid) is emulated as setreuid(uid, uid),
		 *	so we may simply call setuid() directly.
		 */
		(void) setuid();
		return;
		/*NOTREACHED*/
	}

	uap = (struct a *)u.u_ap;
	ruid = uap->ruid;
	if (ruid == -1)
		ruid = u.u_ruid;
	if (u.u_ruid != ruid && u.u_uid != ruid && !suser())
		return;
	euid = uap->euid;
	if (euid == -1)
		euid = u.u_uid;
	if (u.u_ruid != euid && u.u_uid != euid && !suser())
		return;
	/*
	 * Everything's okay, do it.
	 */
	u.u_cred = crcopy(u.u_cred);
#ifdef QUOTA
	if (u.u_quota->q_uid != ruid) {
		qclean();
		qstart(getquota(ruid, 0, 0));
	}
#endif
	u.u_procp->p_uid = ruid;
	u.u_ruid = ruid;
	u.u_uid = euid;
}

setregid()
{
	register struct a {
		int	rgid;
		int	egid;
	} *uap;
	register int rgid, egid;

	if ((u.u_procp->p_compatflags & COMPAT_BSDPROT) == 0) {
		/*
		 *	Pretend this is setgid():  in .../gen/setgid.c,
		 *	setgid(gid) is emulated as setregid(gid, gid),
		 *	so we may simply call setgid() directly.
		 */
		(void) setgid();
		return;
		/*NOTREACHED*/
	}

	uap = (struct a *)u.u_ap;
	rgid = uap->rgid;
	if (rgid == -1)
		rgid = u.u_rgid;
	if (u.u_rgid != rgid && u.u_gid != rgid && !suser())
		return;
	egid = uap->egid;
	if (egid == -1)
		egid = u.u_gid;
	if (u.u_rgid != egid && u.u_gid != egid && !suser())
		return;
	u.u_cred = crcopy(u.u_cred);
	if (u.u_rgid != rgid) {
		leavegroup(u.u_rgid);
		(void) entergroup(rgid);
		u.u_rgid = rgid;
	}
	u.u_gid = egid;
}

setgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;
	struct ucred *newcr, *tmpcr;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u.u_groups) / sizeof (u.u_groups[0])) {
		u.u_error = EINVAL;
		return;
	}
	newcr = crdup(u.u_cred);
	u.u_error = copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    uap->gidsetsize * sizeof (newcr->cr_groups[0]));
	if (u.u_error) {
		crfree(newcr);
		return;
	}
	tmpcr = u.u_cred;
	u.u_cred = newcr;
	crfree(tmpcr);
	for (gp = &u.u_groups[uap->gidsetsize]; gp < &u.u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;
}

/*
 * Group utility functions.
 */

/*
 * Delete gid from the group set.
 */
leavegroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			goto found;
	return;
found:
	for (; gp < &u.u_groups[NGROUPS-1]; gp++)
		*gp = *(gp+1);
	*gp = NOGROUP;
}

/*
 * Add gid to the group set.
 */
entergroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			return (0);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp < 0) {
			*gp = gid;
			return (0);
		}
	return (-1);
}

/*
 * Check if gid is a member of the group set.
 */
groupmember(gid)
	int gid;
{
	register int *gp;

	if (u.u_gid == gid)
		return (1);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

/*	@(#)kern_prot.c 1.1 86/02/03 SMI; from UCB 5.17 83/05/27        */
/*	NFSSRC @(#)kern_prot.c	2.3 86/05/14 */
/*
 * Routines to allocate and free credentials structures
 */

int cractive = 0;

struct credlist {
	union {
		struct ucred cru_cred;
		struct credlist *cru_next;
	} cl_U;
#define	 cl_cred cl_U.cru_cred
#define	 cl_next cl_U.cru_next
};

struct credlist *crfreelist = NULL;

/*
 * Allocate a zeroed cred structure and crhold it.
 */
struct ucred *
crget()
{
	register struct ucred *cr;

	if (crfreelist) {
		cr = &crfreelist->cl_cred;
		crfreelist = ((struct credlist *)cr)->cl_next;
	} else {
		cr = (struct ucred *)kmem_alloc((u_int)sizeof(*cr));
	}
	bzero((char *) cr, sizeof(*cr));
	crhold(cr);
	cractive++;
	return(cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
crfree(cr)
	struct ucred *cr;
{
	if (--cr->cr_ref != 0) {
		return;
	}
	((struct credlist *)cr)->cl_next = crfreelist;
	crfreelist = (struct credlist *)cr;
	cractive--;
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	crfree(cr);
	newcr->cr_ref = 1;
	return(newcr);
}

/*
 * Dup cred struct to a new held one.
 */
struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
	return(newcr);
}

getcompat()
{
	register struct user *up = &u;

	up->u_rval1 = up->u_procp->p_compatflags;
}

setcompat(uap)
register struct a {
	int	compatflags;
	int	(*sigcode)();
} *uap;
{
	register struct user *up = &u;
	register struct proc *p = up->u_procp;
	register int i;

	if ( (uap->compatflags & COMPAT_BSDSIGNALS) !=
	     (p->p_compatflags & COMPAT_BSDSIGNALS) ) {
		if (p->p_sigcatch) {
			up->u_error = EINVAL;
			return;
		}
		if (uap->compatflags & COMPAT_BSDSIGNALS) {
			up->u_sigcode = uap->sigcode;
		}
	}
	if((uap->compatflags & (COMPAT_SYSCALLS | COMPAT_BSDTTY)) &&
			!(uap->compatflags & COMPAT_BSDSIGNALS)) {
		up->u_error = EINVAL;
		return;
	}
	if(uap->compatflags & COMPAT_CLRPGROUP)
		p->p_flag &= ~SPGRP42;
	else if(uap->compatflags & COMPAT_BSDTTY)
		p->p_flag |= SPGRP42;
	p->p_compatflags = uap->compatflags & ~COMPAT_CLRPGROUP;
}
