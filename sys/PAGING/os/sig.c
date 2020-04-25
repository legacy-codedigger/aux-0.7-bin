/*	@(#)sig.c	UniPlus VVV.2.1.10	*/

#ifdef HOWFAR
extern int T_signal;
#endif HOWFAR
#ifdef lint
#include "sys/sysinclude.h"
#else lint
#include "compat.h"
#include "sys/types.h"
#include "sys/mmu.h"
#include "sys/seg.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/pfdat.h"
#include "sys/page.h"
#include "sys/systm.h"
#include "sys/time.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/vnode.h"
#include "sys/uio.h"
#include "sys/file.h"
#include "sys/psl.h"
#include "sys/var.h"
#include "sys/ipc.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/debug.h"
#include "sys/reg.h"
#include	<svfs/inode.h>
#endif lint

#define mask(sig)	(1<<(sig-1))
#define	stops	(mask(SIGSTOP)|mask(SIGTSTP)|mask(SIGTTIN)|mask(SIGTTOU))
#define cantmask        (mask(SIGKILL)|mask(SIGCONT)|mask(SIGSTOP))

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct ipc
{
	int	ip_data;
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
} ipc;

/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 * Called by tty.c for quits and
 * interrupts.
 */
signal(pgrp, sig)
register pgrp;
{
	register struct proc *p;

	if (pgrp == 0)
		return;
	for(p = &proc[1]; p < (struct proc *)v.ve_proc; p++)
		if (p->p_pgrp == pgrp)
			psignal(p, sig);
}

/*
 * Send the specified signal to
 * the specified process.
 */
psignal(p, sig)
	register struct proc *p;
	register sig;
{
	register int s;
	register int (*action)();
	int sigmask;

	sigmask = 1 << (sig-1);
    TRACE(T_signal, ("psignal proc %d sig %d\n", p->p_pid, sig));
    if ( (p->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
	if (sig <= 0 || sig > NSIG)
		return;
	if ((p->p_sigignore & sigmask) && sig != SIGCLD )
		return;
	p->p_sig |= sigmask;
	if (p->p_sigcatch & sigmask)
		action = SIG_CATCH;
	else
		action = SIG_DFL;
	switch (sig) {
	case SIGKILL:
		p->p_flag &= ~SWTED;
		s = spl6();
		if(p->p_stat == SSTOP) {
			if(p->p_wchan)
				p->p_stat = SSLEEP;
			else
				setrun(p);
		}
		splx(s);
		break;
	case SIGCONT:
		s = spl6();
		p->p_flag &= ~SWTED;
		p->p_sig &= ~stops;
		if(p->p_cursig == SIGSTOP || p->p_cursig == SIGTSTP ||
		    p->p_cursig == SIGTTIN || p->p_cursig == SIGTTOU)
			p->p_cursig = 0;
		if (p->p_stat == SSTOP) {
			if (action == SIG_DFL && p->p_wchan)
				p->p_stat = SSLEEP;
			else
				setrun(p);
			splx(s);
			return;
		}
		splx(s);
		break;
	case SIGTTIN:
	case SIGTTOU:
	case SIGTSTP:
		if((p->p_flag & SPGRP42) == 0) {/* if no job control */
			p->p_sig &= ~sigmask;
			return;
		}
		/* Else fall through to */
	case SIGSTOP:
		if(p->p_flag & STRC)
			break;
		p->p_sig &= ~mask(SIGCONT);
		s = spl6();
		switch (p->p_stat) {
		case SSLEEP:
			if (action == SIG_DFL) {
				if (sig != SIGSTOP && p->p_ppid == 1) {
					p->p_sig &= ~sigmask;
					splx(s);
					psignal(p, SIGKILL);
					return;
				}
				p->p_sig &= ~sigmask;
				p->p_cursig = sig;	/* for wait */
				splx(s);
				stop(p);
				return;
			}
			break;
		case SSTOP:
			p->p_sig &= ~sigmask;
			return;
		}
		splx(s);
		break;
	case SIGWINCH:
		s = spl6();
		if (p->p_stat == SSLEEP && action == SIG_DFL) {
			p->p_sig &= ~sigmask;
			return;
		}
		splx(s);
		break;
	}
	s = spl6();
	if (p->p_pri > PZERO) {
		switch(p->p_stat) {
		case SSLEEP:
			setrun(p);
			break;
		case SSTOP:
			if(p->p_wchan)	/* XXX ok for traced procs ? */
				unsleep(p);
			break;
		}
	}
	splx(s);
    } else {
	if ((unsigned)sig >= NSIG)
		return;

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & STRC)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 */
		if (p->p_sigignore & sigmask) {
			if(sig == SIGCLD)
				wakeup(p);
			return;
		}
		if (p->p_sigmask & sigmask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & sigmask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}
	if (sig) {
		p->p_sig |= sigmask;
		switch (sig) {

		case SIGTERM:
			if ((p->p_flag&STRC) || action != SIG_DFL)
				break;
			/* fall into ... */

		case SIGKILL:
			if (p->p_nice > NZERO)
				p->p_nice = NZERO;
			break;

		case SIGCONT:
			p->p_sig &= ~stops;
			if(p->p_cursig == SIGSTOP || p->p_cursig == SIGTSTP ||
			    p->p_cursig == SIGTTIN || p->p_cursig == SIGTTOU)
				p->p_cursig = 0;
			p->p_flag &= ~SWTED;
			break;

		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			if((p->p_flag & SPGRP42) == 0) {/* if no job control */
				p->p_sig &= ~sigmask;
				return;
			}
			/* Else fall through to */
		case SIGSTOP:
			p->p_sig &= ~mask(SIGCONT);
			break;
		}
	}
	/*
	 * Defer further processing for signals which are held.
	 */
	if (action == SIG_HOLD)
		return;
	s = spl6();
	switch (p->p_stat) {

	case SSLEEP:
		/*
		 * If process cannot be interrupted, the signal will
		 * be noticed when the process returns through
		 * trap() or syscall().
		 */
		if (p->p_pri <= PZERO)
			goto out;
		/*
		 * Process is sleeping and traced... make it runnable
		 * so it can discover the signal in issig() and stop
		 * for the parent.
		 */
		if (p->p_flag&STRC)
			goto run;
		switch (sig) {

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * These are the signals which by default
			 * stop a process.
			 */
			if (action != SIG_DFL)
				goto run;
			/*
			 * Don't clog system with children of init
			 * stopped from the keyboard.
			 */
			if (sig != SIGSTOP && p->p_pptr == &proc[1]) {
				psignal(p, SIGKILL);
				p->p_sig &= ~sigmask;
				splx(s);
				return;
			}
			p->p_sig &= ~sigmask;
			p->p_cursig = sig;
			stop(p);
			goto out;

		case SIGIO:
		case SIGURG:
		case SIGWINCH:
			/*
			 * These signals are special in that they
			 * don't get propogated... if the process
			 * isn't interested, forget it.
			 */
			if (action != SIG_DFL)
				goto run;
			p->p_sig &= ~sigmask;		/* take it away */
			goto out;

		case SIGCLD:
			if(action != SIG_DFL)
				goto run;
			p->p_sig &= ~sigmask;
			wakeup(p);
			goto out;

		default:
			/*
			 * All other signals cause the process to run
			 */
			goto run;
		}
		/*NOTREACHED*/

	case SSTOP:
		/*
		 * If traced process is already stopped,
		 * then no further action is necessary.
		 */
		if (p->p_flag&STRC)
			goto out;
		switch (sig) {

		case SIGKILL:
			/*
			 * Kill signal always sets processes running.
			 */
			goto run;

		case SIGCONT:
			/*
			 * If the process catches SIGCONT, let it handle
			 * the signal itself.  If it isn't waiting on
			 * an event, then it goes back to run state.
			 * Otherwise, process goes back to sleep state.
			 */
			if (action != SIG_DFL || p->p_wchan == 0)
				goto run;
			p->p_stat = SSLEEP;
			goto out;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			p->p_sig &= ~sigmask;		/* take it away */
			goto out;

		default:
			/*
			 * If process is sleeping interruptibly, then
			 * unstick it so that when it is continued
			 * it can look at the signal.
			 * But don't setrun the process as its not to
			 * be unstopped by the signal alone.
			 */
			if (p->p_wchan && p->p_pri > PZERO)
				unsleep(p);
			goto out;
		}
		/*NOTREACHED*/

	default:
		/* SRUN, SIDL, SZOMB do nothing with the signal */
		goto out;
	}
	/*NOTREACHED*/
run:
	/*
	 * Raise priority to at least PUSER.
	 */
	if (p->p_pri > PUSER)
		if ((p != u.u_procp || curpri == PIDLE) && p->p_stat == SRUN &&
		    (p->p_flag & SLOAD)) {
			remrq(p);
			p->p_pri = PUSER;
			setrq(p);
		} else
			p->p_pri = PUSER;
	setrun(p);
out:
	splx(s);
    }
}

/*
 * Returns true if the current
 * process has a signal to process.
 * This is asked at least once
 * each time a process enters the
 * system.
 * A signal does not do anything
 * directly to a process; it sets
 * a flag that asks the process to
 * do something to itself.
 */
issig()
{
	register struct proc *p, *q;
	register int sig;
	register int sigbits, sigmask;

	p = u.u_procp;
    if ( (p->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
	while(p->p_sig) {
		sig = fsig(p);
		switch (sig) {
		case SIGCLD:
			if ((int)u.u_signal[SIGCLD-1]&01) {
				for (q = p->p_child; q != NULL; q = q->p_sibling)
				{
					if (q->p_stat == SZOMB)
						freeproc(q, 0);
				}
			} else if (u.u_signal[SIGCLD-1])
				return(sig);
			break;
		case SIGPWR:
			if (u.u_signal[SIGPWR-1] &&
			    ((int)u.u_signal[SIGPWR-1]&1)==0)
				return(sig);
			break;
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			if (u.u_signal[sig-1] == 0) {
				if (p->p_ppid == 1) {
					psignal(p, SIGKILL);
					break;
				}
			} else
				goto chk;
			/* fall through */
		case SIGSTOP:
			if (u.u_signal[sig-1] == 0) {
				if (p->p_flag & STRC)
					break;
				p->p_cursig = sig;
				sigmask = 1 << (sig-1);
				p->p_sig &= ~sigmask;
				stop(p);
				swtch();
				break;
			}
			goto chk;
		case SIGCONT:
		case SIGWINCH:
			if (u.u_signal[sig-1] == 0)
				break;
			goto chk;
		default:
	chk:
			if (((int)u.u_signal[sig-1]&1) == 0 || (p->p_flag&STRC))
				return(sig);
			break;
		}
		p->p_sig &= ~(1L<<(sig-1));
	}
	return(0);
    } else {
	if(p->p_cursig != 0)
		return(p->p_cursig);
	for (;;) {
		sigbits = p->p_sig &~ p->p_sigmask;
		if ((p->p_flag&STRC) == 0)
			sigbits &= ~p->p_sigignore;
		if (sigbits == 0)
			break;
		sig = ffs((long)sigbits);
		sigmask = 1 << (sig-1);
		p->p_sig &= ~sigmask;		/* take the signal! */
		p->p_cursig = sig;
		if (p->p_flag&STRC) {
			/*
			 * If traced, always stop, and stay
			 * stopped until released by the parent.
			 */
			do {
				stop(p);
				swtch();
			} while (!procxmt() && p->p_flag&STRC);

			/*
			 * If the traced bit got turned off,
			 * then put the signal taken above back into p_sig
			 * and go back up to the top to rescan signals.
			 * This ensures that p_sig* and u_signal are consistent.
			 */
			if ((p->p_flag&STRC) == 0) {
				p->p_sig |= sigmask;
				continue;
			}

			/*
			 * If parent wants us to take the signal,
			 * then it will leave it in p->p_cursig;
			 * otherwise we just look for signals again.
			 */
			sig = p->p_cursig;
			if (sig == 0)
				continue;

			/*
			 * If signal is being masked put it back
			 * into p_sig and look for other signals.
			 */
			sigmask = 1 << (sig-1);
			if (p->p_sigmask & sigmask) {
				p->p_sig |= sigmask;
				continue;
			}
		}
		switch ((int)u.u_signal[sig-1]) {

		case SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_ppid == 0)
				break;
			switch (sig) {

			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				/*
				 * Children of init aren't allowed to stop
				 * on signals from the keyboard.
				 */
				if (p->p_pptr == &proc[1]) {
					psignal(p, SIGKILL);
					continue;
				}
				/* fall into ... */

			case SIGSTOP:
				if (p->p_flag&STRC)
					continue;
				stop(p);
				swtch();
				continue;

			case SIGCONT:
			case SIGCLD:
			case SIGURG:
			case SIGIO:
			case SIGWINCH:
				/*
				 * These signals are normally not
				 * sent if the action is the default.
				 */
				continue;		/* == ignore */

			default:
				goto send;
			}
			/*NOTREACHED*/

		case SIG_HOLD:
		case SIG_IGN:
			/*
			 * Masking above should prevent us
			 * ever trying to take action on a held
			 * or ignored signal, unless process is traced.
			 */
			if ((p->p_flag&STRC) == 0)
				printf("issig\n");
			continue;

		default:
			/*
			 * This signal has an action, let
			 * psig process it.
			 */
			goto send;
		}
		/*NOTREACHED*/
	}
	/*
	 * Didn't find a signal to send.
	 */
	p->p_cursig = 0;
	return (0);

send:
	/*
	 * Let psig process the signal.
	 */
	return (sig);
    }
}

/*
 * Enter the tracing STOP state.
 * In this state, the parent is
 * informed and the process is able to
 * receive commands from the parent.
 */
stop(p)
register struct proc *p;
{
	register struct user *up;
	register proc_t	*pp;
	register proc_t	*cp;
	register preg_t	*prp;

	pp = p->p_parent;
	TRACE(T_signal, ("stop proc %d\n", p->p_pid));
	if ((p->p_flag & SPGRP42) || !(p->p_flag & STRC)) {

		p->p_stat = SSTOP;
		p->p_flag &= ~SWTED;
		wakeup((caddr_t)pp);
		/*
		 * Avoid sending signal to parent if process is traced
		 */
		if (p->p_flag&STRC)
			return;
		psignal(pp, SIGCLD);
		return;
	}

	/*	Put the region sizes into the u-block for the
	 *	parent to look at if he wishes.
	 */
	
	up = &u;
	pp = up->u_procp;
	if(p != pp)	/* check not about to over-write a stranger's u dot. */
		panic("bad stop() call");	/* XXX debug only */
	if(prp = findpreg(pp, PT_TEXT))
		up->u_tsize = prp->p_reg->r_pgsz;
	else
		up->u_tsize = 0;
	if(prp = findpreg(pp, PT_DATA))
		up->u_dsize = prp->p_reg->r_pgsz;
	else
		up->u_dsize = 0;
	if(prp = findpreg(pp, PT_STACK))
		up->u_ssize = prp->p_reg->r_pgsz;
	else
		up->u_ssize = 0;


	/* we got rid of a costly search loop every time by
	 * finding parent first time in function, then every iteration
	 * thru loop check that proc 1 is not parent and parent has
	 * not changed.
	 * Does ANYBODY out there really care?
	*/
	cp = up->u_procp;
	/* no need to protect following assignment: if someone changes it
	 * simultaneously, we get either the first or last value here, same
	 * as if it were protected
	 */
	pp = cp->p_parent;
	if (cp->p_ppid == 1 || cp->p_ppid != pp->p_pid)
		exit(fsig(cp));

	cp->p_stat = SSTOP;
	cp->p_flag &= ~SWTED;
	/* following wakes up parent if it was sleeping in
	 * wait, waiting for child to enter SSTOP state.
	*/
	wakeup((caddr_t)pp);
}


/*
 * Perform the action specified by
 * the current signal.
 * The usual sequence is:
 *	if (issig())
 * 		psig();
 */
psig(arg)
{
	register int sig;
	register struct proc *rp;
	register struct user *up;
	register int (*action)();
	register int sigmask;
	int returnmask;

	up = &u;
	rp = up->u_procp;
#ifdef FLOAT
	if (up->u_fpinuse && up->u_fpsaved==0) {
		savfp();
		up->u_fpsaved = 1;
	}
#endif
#ifdef mc68881		/* MC68881 floating-point coprocessor */
	fpsave();
#endif
    if ( (rp->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
	if (rp->p_flag&STRC)
		do {
			stop(rp);
			swtch();
		} while (rp->p_flag & STRC && !procxmt());
	sig = fsig(rp);
	if (sig == 0)
		return;
	rp->p_sig &= ~(1L<<(sig-1));
	if ((action=up->u_signal[sig-1]) != 0) {
		if ((int)action & 1)
			return;
		up->u_error = 0;
		if (sig != SIGILL && sig != SIGTRAP)
			up->u_signal[sig-1] = 0;
		sendsig((caddr_t) action, sig, arg);
		return;
	}
	switch(sig) {

	case SIGQUIT:
	case SIGILL:
	case SIGTRAP:
	case SIGIOT:
	case SIGEMT:
	case SIGFPE:
	case SIGBUS:
	case SIGSEGV:
	case SIGSYS:
		TRACE(T_signal,("calling core pid %d sig %d\n", rp->p_pid,sig));
		if (core())
			sig += 0200;
	}
	exit(sig);
    } else {
	sig = rp->p_cursig;
	rp->p_cursig = 0;
	if (sig == 0)
		panic("psig");;
	sigmask = 1 << (sig - 1);
	action = up->u_signal[sig-1];
	if (action != SIG_DFL) {
		if (action == SIG_IGN || rp->p_sigmask & sigmask)
			panic("psig action");
		up->u_error = 0;
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal (unless we're simulating
		 * the old signal facilities). 
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want restored
		 * after the signal processing is completed.
		 */
		SPL6();
		if (rp->p_flag & SOMASK) {
			returnmask = up->u_oldmask;
			rp->p_flag &= ~SOMASK;
		} else
			returnmask = rp->p_sigmask;
		rp->p_sigmask |= up->u_sigmask[sig-1] | sigmask;
		SPL0();
		TRACE(T_signal,("psig: pid %d\n",rp->p_pid));
		sendsig(action, sig, returnmask);
		return;
	}
	switch(sig) {

	case SIGQUIT:
	case SIGILL:
	case SIGTRAP:
	case SIGIOT:
	case SIGEMT:
	case SIGFPE:
	case SIGBUS:
	case SIGSEGV:
	case SIGSYS:
		up->u_arg[0] = sig;
		TRACE(T_signal,("calling core pid %d sig %d\n", rp->p_pid,sig));
		if (core())
			sig += 0200;
	}
	exit(sig);
    }
}


/*
 * find the signal in bit-position
 * representation in p_sig.
 * since we only call fsig if we know there is a signal, no need to
 * protect: who cares if we miss posting of a newer signal? get it
 * next time.
 */
fsig(p)
	struct proc *p;
{
	register i;
	register n;

	n = p->p_sig;
	for(i=1; i<=NSIG; i++) {
		if (n & 1L)
			return(i);
		n >>= 1;
	}
	return(0);
}

/*
 * Create a core image on the file "core"
 *
 * It writes USIZE block of the
 * user.h area followed by the entire
 * data+stack segments.
 */
core()
{
	struct vnode *vp;
	struct vattr vattr;

	if (u.u_uid != u.u_ruid || u.u_gid != u.u_rgid)
		return (0);
	u.u_error = 0;

	vattr_null(&vattr);
	vattr.va_type = VREG;
	vattr.va_mode = 0666;
	u.u_error =
	    vn_create("core", UIOSEG_KERNEL, &vattr, NONEXCL, VWRITE, &vp);
	if (u.u_error)
		return (0);
	if (vattr.va_nlink != 1) {
		u.u_error = EFAULT;
		goto out;
	}
	vattr_null(&vattr);
	vattr.va_size = 0;
	VOP_SETATTR(vp, &vattr, u.u_cred);

	coredump(vp);

out:
	VN_RELE(vp);
	return (u.u_error == 0);

}

/*
 * sys-trace system call.
 */
ptrace()
{
	register struct ipc *ipcp;
	register struct user *up;
	register struct proc *p;
	register struct a {
		int	req;
		int	pid;
		int	*addr;
		int	data;
	} *uap;

	up = &u;
	uap = (struct a *)up->u_ap;
	if (uap->req <= 0) {
		/* child gets here before exec'ing the process to be traced */
		up->u_procp->p_flag |= STRC;
		return;
	}
	for (p = up->u_procp->p_child;   p != NULL;   p = p->p_sibling)
		if (p->p_stat==SSTOP && p->p_pid == uap->pid)
			goto found;
	up->u_error = ESRCH;
	return;

    found:		/* p is the child (traced) process */
	ipcp = &ipc;
	while (ipcp->ip_lock)
		(void) sleep((caddr_t)ipcp, IPCPRI);
	ipcp->ip_lock = p->p_pid;
	ipcp->ip_data = uap->data;
	ipcp->ip_addr = uap->addr;
	ipcp->ip_req = uap->req;
	p->p_flag &= ~SWTED;
	/* wake up child sleeping in stop routine */
	setrun(p);

	/* parent sleeps until child returns with data and restarts it */
	while (ipcp->ip_req > 0)
		(void) sleep((caddr_t)ipcp, IPCPRI);
	up->u_rval1 = ipcp->ip_data;
	if (ipcp->ip_req < 0)
		up->u_error = EIO;
	ipcp->ip_lock = 0;
	wakeup((caddr_t)ipcp);
}

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
	register struct ipc *ipcp;
	register struct user *up;
	register int i;
	register int *p;

	up = &u;
	ipcp = &ipc;
	if (ipcp->ip_lock != up->u_procp->p_pid)
		return(0);
	i = ipcp->ip_req;
	ipcp->ip_req = 0;
	wakeup((caddr_t)ipcp);
	switch (i) {

	/* read user I */
	case 1:
		if ((int)ipcp->ip_addr & 1)
			goto error;
		ipcp->ip_data = fuiword((caddr_t)ipcp->ip_addr);
		break;

	/* read user D */
	case 2:
		if ((int)ipcp->ip_addr & 1)
			goto error;
		ipcp->ip_data = fuword((caddr_t)ipcp->ip_addr);
		break;

	/* read u */
	case 3:
		i = (int)ipcp->ip_addr;
		if (i<0 || i >= ptob(v.v_usize))
			goto error;

		ipcp->ip_data = *((int *) (((int) (((char *) up) + i)) & ~1));
		break;

	/* write user I */
	/* Must set up to allow writing */
	case 4:{
		register preg_t	*prp;
		register reg_t	*rp;
		register pte_t	*pt;
		register dbd_t	*dbd;
		register pfd_t	*pfd;
		preg_t *vtopreg();


		prp = vtopreg(u.u_procp, (caddr_t) ipcp->ip_addr);
		if (prp == NULL)
			goto error;
		rp = prp->p_reg;
		reglock(rp);
		if(prp->p_type == PT_TEXT){
			rp = prp->p_reg;
			if((rp->r_flags & RG_NOFREE)  ||
			   (rp->r_refcnt > 1)){
				regrele(rp);
				goto error;
			}
			rp->r_flags |= RG_NOSHARE;
			if(prp->p_flags & PF_RDONLY){
				chgprot(prp, SEG_RW);
			} else {
				prp = NULL;
			}
		}

		regrele(rp);
		i = suiword((caddr_t)ipc.ip_addr, ipcp->ip_data);
		reglock(rp);
		if(prp->p_type == PT_TEXT){
			chgprot(prp, SEG_RO);
		}
		if(i >= 0){
			i = btotp(ipcp->ip_addr - (int)prp->p_regva);
			pt = &rp->r_list[i/NPGPT][i%NPGPT];
			dbd = dbdget(pt);
			pfd = &pfdat[pftopfi(pt->pgm[0].pg_pfn)];
			memlock();
			if (pfd->pf_flags & P_HASH)
				(void)premove(pfd);
			if (dbd->dbd_type == DBD_SWAP)
				(void)swfree1(dbd);
			memunlock();
			dbd->dbd_type = DBD_NONE;
		} else {
			regrele(rp);
			goto error;
		}
		regrele(rp);
		break;
	}

	/* write user D */
	case 5:
		if (suword((caddr_t)ipcp->ip_addr, 0) < 0)
			goto error;
		(void) suword((caddr_t)ipc.ip_addr, ipcp->ip_data);
		break;

	/* write u */
	case 6:
		i = (int)ipcp->ip_addr;
		p = (int *) (((int) (((char *) up) + i)) & ~1);
		for (i=0; i<16; i++)
			if (p == &up->u_ar0[regloc[i]])
				goto ok;
		if (p == &up->u_ar0[RPS]) {
			/* assure user space and priority 0 */
			ipcp->ip_data &= ~0x2700;
			goto ok;
		}
		goto error;

	ok:
		*p = ipcp->ip_data;
		break;

	/* set signal and continue */
	case 9:
		up->u_ar0[RPS] |= PS_T;
	case 7:
		if ((int)ipcp->ip_addr != 1)
			up->u_ar0[PC] = (int)ipcp->ip_addr;
		SPLHI();
		up->u_procp->p_sig = 0L;
		SPL0();
		i = ipcp->ip_data;
		if(i < 0 || i >= NSIG)
			goto error;
		if(i)
			psignal(up->u_procp, i);
		return(1);

	/* force exit */
	case 8:
		exit(fsig(up->u_procp));

	/* read u registers */
	case 10:
		if ((i = (int)ipcp->ip_addr) < 0 || i > 17)
			goto error;
		if (i == 17)
			ipcp->ip_data = up->u_ar0[regloc[17]] & 0xFFFF;
		else
			ipcp->ip_data = up->u_ar0[regloc[i]];
		break;

	/* write u registers */
	case 11:
		
		if ((i = (int)ipcp->ip_addr) < 0 || i > 17)
			goto error;
		if (i == 17) {
			ipcp->ip_data &= ~0x2700;	/* user only */
			up->u_ar0[regloc[17]] =
				(up->u_ar0[regloc[17]] & ~0xFFFF) |
				(ipcp->ip_data & 0xFFFF);
		} else
			up->u_ar0[regloc[i]] = ipcp->ip_data;
		break;

	default:
	error:
		ipcp->ip_req = -1;
	}
	return(0);
}

setsigvec(sig, sv)
	int sig;
	register struct sigvec *sv;
{
	register struct proc *p;
	register int bit;

	p = u.u_procp;
	if ( (p->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
		u.u_error = EINVAL;
		return;
	}
	bit = mask(sig);
	/*
	 * Change setting atomically.
	 */
	(void) spl6();
	u.u_signal[sig-1] = sv->sv_handler;
	u.u_sigmask[sig-1] = sv->sv_mask &~ cantmask;
	if (sv->sv_onstack)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	if (sv->sv_handler == SIG_IGN) {
		p->p_sig &= ~bit;		/* never to be seen again */
		p->p_sigignore |= bit;
		p->p_sigcatch &= ~bit;
	} else {
		p->p_sigignore &= ~bit;
		if (sv->sv_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	(void) spl0();
}

sigblock()
{
	struct a {
		int	sigmask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	if ( (p->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
		u.u_error = EINVAL;
		return;
	}
	(void) spl6();
	u.u_rval1 = p->p_sigmask;
	p->p_sigmask |= uap->sigmask &~ cantmask;
	(void) spl0();
}

sigpause()
{
	struct a {
		int	sigmask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	if ( (p->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
		u.u_error = EINVAL;
		return;
	}
	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in u.).
	 */
	u.u_oldmask = p->p_sigmask;
	p->p_flag |= SOMASK;
	p->p_sigmask = uap->sigmask &~ cantmask;
	for (;;)
		sleep((caddr_t)&u, PSLEP);
	/*NOTREACHED*/
}

sigsetmask()
{
	struct a {
		int	sigmask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	if ( (p->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
		u.u_error = EINVAL;
		return;
	}
	(void) spl6();
	u.u_rval1 = p->p_sigmask;
	p->p_sigmask = uap->sigmask &~ cantmask;
	(void) spl0();
}

sigstack()
{
	register struct a {
		struct	sigstack *nss;
		struct	sigstack *oss;
	} *uap = (struct a *)u.u_ap;
	struct sigstack ss;

	if ( (u.u_procp->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->oss) {
		u.u_error = copyout((caddr_t)&u.u_sigstack, (caddr_t)uap->oss, 
		    sizeof (struct sigstack));
		if (u.u_error)
			return;
	}
	if (uap->nss) {
		u.u_error =
		    copyin((caddr_t)uap->nss, (caddr_t)&ss, sizeof (ss));
		if (u.u_error == 0)
			u.u_sigstack = ss;
	}
}

sigvec()
{
	register struct a {
		int	signo;
		struct	sigvec *nsv;
		struct	sigvec *osv;
	} *uap = (struct a  *)u.u_ap;
	struct sigvec vec;
	register struct sigvec *sv;
	register int sig;

	if ( (u.u_procp->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
		u.u_error = EINVAL;
		return;
	}
	sig = uap->signo;
	if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) {
		u.u_error = EINVAL;
		return;
	}
	sv = &vec;
	if (uap->osv) {
		sv->sv_handler = u.u_signal[sig-1];
		sv->sv_mask = u.u_sigmask[sig-1];
		sv->sv_onstack = (u.u_sigonstack & mask(sig)) != 0;
		u.u_error =
		    copyout((caddr_t)sv, (caddr_t)uap->osv, sizeof (vec));
		if (u.u_error)
			return;
	}
	if (uap->nsv) {
		u.u_error =
		    copyin((caddr_t)uap->nsv, (caddr_t)sv, sizeof (vec));
		if (u.u_error)
			return;
		if (sig == SIGCONT && sv->sv_handler == SIG_IGN) {
			u.u_error = EINVAL;
			return;
		}
		setsigvec(sig, sv);
	}
}

/* <@(#)sig.c	6.3> */
