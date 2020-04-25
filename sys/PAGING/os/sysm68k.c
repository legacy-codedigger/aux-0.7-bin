/*	@(#)sysm68k.c	UniPlus VVV.2.1.4	*/

#ifdef lint
#include "sys/sysinclude.h"
#else lint
#include "compat.h"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/mmu.h"
#include "sys/page.h"
#include "sys/systm.h"
#include "sys/time.h"
#include "sys/signal.h"
#include "sys/seg.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/reg.h"
#include "sys/var.h"
#include "sys/buserr.h"
#include "sys/swap.h"
#include "sys/sysinfo.h"
#endif lint

/*
 * sysm68k - system call
 * 1) Called to grow the user stack on the MC68000 OBSOLETE
 * 2) Called to perform instruction continuation after signal handlers
 * 3-n) implement 3b system calls
 */
sysm68k ()
{
	register  struct  a {
		int	cmd;
		int	arg1, arg2, arg3, arg4;
	} *uap = (struct a *) u.u_ap;

	register struct user *up;
	register i, kind, *regs, *usp;

	up = &u;
	switch (((struct a *)up->u_ap)->cmd) {

 	case 1:		/* grow/shrink stack */
		i = ((struct a *)up->u_ap)->arg1;

	        if (grow((unsigned)(v.v_uend-ptob(up->u_ssize)-i)) == NULL) {
			up->u_error = ENOMEM;
			return;
		}
		up->u_rval1 = v.v_uend - ptob(up->u_ssize);
		return;

	case 2:		
		/* continue from signal handler */

	    if ( (up->u_procp->p_compatflags & COMPAT_BSDSIGNALS) == 0) {
		regs = up->u_ar0;
		usp = (int *) regs[SP];

		/* find the return address for the 'rtr' */
		/* in the user's stack area */
		/* stack format: */
		/*   0 - _sysm68k return addr */
		/*   1 - command */
		/*   2 - old %d0 */
		/*   3 - <kind of resume, previous trap process status reg> */
		/*   4 - previous trap PC */

 		/* get resume kind */
		kind = (fuword((caddr_t) (usp+3)) >> 16) & 0xffff;
		up->u_traptype = kind; /* make it appear that we just got here */
				     /* for the first time for psignal */

		regs[PC] = fuword((caddr_t) (usp+4)); 
		/* make sure we don't allow transfer to supervisor mode */
		regs[RPS] = (regs[RPS] & 0xff00) |
			   (fuword((caddr_t) (usp+3)) & 0xff);
		up->u_rval1 = fuword((caddr_t) (usp+2));	/* user's D0 */
		usp += 5;
		regs[SP] = (int) usp;

		if (cputype == 68010) {
			if (kind & TRAPLONG)
				((struct buserr *)regs)->ber_format = 0x8;
		}
		else if (cputype == 68020) {
			if (kind & TRAPLONG)
				((struct buserr *)regs)->ber_format = FMT_LONG;
			else if (kind & TRAPSHORT)
				((struct buserr *)regs)->ber_format = FMT_SHORT;
		}
		up->u_traptype = kind;
		if (kind & TRAPADDR) /* addr error */
			psignal(up->u_procp, SIGBUS);
		else if (kind & TRAPBUS) /* bus error */
			psignal(up->u_procp, SIGSEGV);
		else			/* no error */
			up->u_traptype = 0;
	    } else {
		if (sigcleanup())
			up->u_error = EFAULT;
	    }

		/* else we can just resume here */
		break;

	case 3:	/* Swap function. */
	{
		u.u_error = swapfunc((swpi_t *)uap->arg1);
		break;
	}

	case 4:	/* namelist interface */
	{
		extern	char	putbuf[];
		extern	int	putindx;

		switch(uap->arg1) {
		case 1:
			up->u_rval1 = (int)proc;
			break;
		case 2:
			up->u_rval1 = (int)putbuf;
			break;
		case 3:
			up->u_rval1 = putindx;
			break;
		case 4:
			up->u_rval1 = (int) &v;
			break;
		case 5:
			up->u_rval1 = swaplow;
			up->u_rval2 = swapdev;
			break;
		case 6:
			up->u_rval1 = (int) &sysinfo;
			break;

		default:
			up->u_error = EINVAL;
			break;
		}
		break;
	}

	case 9: /* Change Field Test Set Utility ID */

		if(uap->arg1 < 0 || uap->arg1 > (int)0xFFFF)
			up->u_error = EINVAL;
		else
			up->u_utilid = 0xFF0000 | uap->arg1;
		break;

	case 99: {

			for(i = 0; i < uap->arg1; i++)
				clratb(USRATB);
			break;
	}
	case 100: {

			for(i = 0; i < uap->arg1; i++)
				invsatb(USRATB, 0, 1);
			break;
	}
	case 101: {

			for(i = 0; i < uap->arg1; i++)
				;
			break;
	}
	case 102: {
			int nullsys();

			for(i = 0; i < uap->arg1; i++)
				nullsys();
			break;
	}
	case 103: {
			int nullsys();

			for(i = 0; i < uap->arg1; i++)
				nullsys();
			break;
	}

	default:
		up->u_error = EINVAL;
	}
}
