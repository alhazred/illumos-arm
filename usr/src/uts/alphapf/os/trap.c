/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2017 Hayashi Naoyuki
 * Copyright (c) 1992, 2010, Oracle and/or its affiliates. All rights reserved.
 */

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc. */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T   */
/*		All Rights Reserved   				*/
/*								*/
/*	Copyright (c) 1987, 1988 Microsoft Corporation  	*/
/*		All Rights Reserved   				*/
/*								*/

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/class.h>
#include <sys/core.h>
#include <sys/syscall.h>
#include <sys/cpuvar.h>
#include <sys/vm.h>
#include <sys/sysinfo.h>
#include <sys/fault.h>
#include <sys/stack.h>
#include <sys/psw.h>
#include <sys/regset.h>
#include <sys/trap.h>
#include <sys/kmem.h>
#include <sys/vtrace.h>
#include <sys/cmn_err.h>
#include <sys/prsystm.h>
#include <sys/mutex_impl.h>
#include <sys/machsystm.h>
#include <sys/archsystm.h>
#include <sys/sdt.h>
#include <sys/avintr.h>
#include <sys/kobj.h>

#include <vm/hat.h>

#include <vm/seg_kmem.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/hat_pte.h>
#include <vm/hat_alpha.h>

#include <sys/procfs.h>

#include <sys/reboot.h>
#include <sys/debug.h>
#include <sys/modctl.h>
#include <sys/aio_impl.h>
#include <sys/cred.h>
#include <sys/mman.h>
#include <sys/copyops.h>
#include <c2/audit.h>
#include <sys/ftrace.h>
#include <sys/panic.h>
#include <sys/ontrap.h>
#include <sys/cpc_impl.h>
#include <sys/bootconf.h>
#include <sys/bootinfo.h>
#include <sys/promif.h>
#include <sys/fp.h>
#include <sys/contract/process_impl.h>
#include <sys/ddi.h>

/*
 * Called from the trap handler when a processor trap occurs.
 *
 * Note: All user-level traps that might call stop() must exit
 * trap() by 'goto out' or by falling through.
 * Note Also: trap() is usually called with interrupts enabled, (PS_IE == 1)
 * however, there are paths that arrive here with PS_IE == 0 so special care
 * must be taken in those cases.
 */

void memory_management_fault(uint64_t a0, uint64_t a1, uint64_t a2, struct regs *rp)
{
	caddr_t vaddr = (caddr_t)a0;
//	prom_printf("%s(): a0=%lx a1=%lx a2=%ld PC=%lx rp=%lx\n", __FUNCTION__, a0, a1, a2, rp->r_pc, rp);
	if (!USERMODE(rp->r_ps) || vaddr < (caddr_t)kernelbase) {
		switch (a1) {
		case 2: case 4:
			if (hat_page_falt(
				    vaddr < (caddr_t)kernelbase ?
				    curproc->p_as->a_hat: kas.a_hat, vaddr, (int)a2) == 0) {
				return;
			}
		}
	}
	trap(a0, a1, a2, T_MM, rp);
}

static void
die (uint64_t a0, uint64_t a1, uint64_t a2, uint64_t type, struct regs *rp)
{
	prom_printf("%s(): a0=%lx a1=%lx a2=%ld type=%lx\n", __FUNCTION__, a0, a1, a2, type);
	prom_printf("      v0=%lx\n", rp->r_v0);
	prom_printf("      t0=%lx\n", rp->r_t0);
	prom_printf("      t1=%lx\n", rp->r_t1);
	prom_printf("      t2=%lx\n", rp->r_t2);
	prom_printf("      t3=%lx\n", rp->r_t3);
	prom_printf("      t4=%lx\n", rp->r_t4);
	prom_printf("      t5=%lx\n", rp->r_t5);
	prom_printf("      t6=%lx\n", rp->r_t6);
	prom_printf("      t7=%lx\n", rp->r_t7);
	prom_printf("      s0=%lx\n", rp->r_s0);
	prom_printf("      s1=%lx\n", rp->r_s1);
	prom_printf("      s2=%lx\n", rp->r_s2);
	prom_printf("      s3=%lx\n", rp->r_s3);
	prom_printf("      s4=%lx\n", rp->r_s4);
	prom_printf("      s5=%lx\n", rp->r_s5);
	prom_printf("      s6=%lx\n", rp->r_s6);
	prom_printf("      a0=%lx\n", rp->r_a0);
	prom_printf("      a1=%lx\n", rp->r_a1);
	prom_printf("      a2=%lx\n", rp->r_a2);
	prom_printf("      a3=%lx\n", rp->r_a3);
	prom_printf("      a4=%lx\n", rp->r_a4);
	prom_printf("      a5=%lx\n", rp->r_a5);
	prom_printf("      t8=%lx\n", rp->r_t8);
	prom_printf("      t9=%lx\n", rp->r_t9);
	prom_printf("      t10=%lx\n", rp->r_t10);
	prom_printf("      t11=%lx\n", rp->r_t11);
	prom_printf("      ra=%lx\n", rp->r_ra);
	prom_printf("      t12(pv)=%lx\n", rp->r_pv);
	prom_printf("      at=%lx\n", rp->r_at);
	prom_printf("      gp=%lx\n", rp->r_gp);
	prom_printf("      ps=%lx\n", rp->r_ps);
	prom_printf("      pc=%lx\n", rp->r_pc);

//	pal_bpt();
//	prom_panic("\n");
}

static uint64_t *get_regs(struct regs *rp, int index)
{
	int offset[] = {
		offsetof(struct regs, r_v0),  offsetof(struct regs, r_t0),  offsetof(struct regs, r_t1), offsetof(struct regs, r_t2),
		offsetof(struct regs, r_t3),  offsetof(struct regs, r_t4),  offsetof(struct regs, r_t5), offsetof(struct regs, r_t6),
		offsetof(struct regs, r_t7),  offsetof(struct regs, r_s0),  offsetof(struct regs, r_s1), offsetof(struct regs, r_s2),
		offsetof(struct regs, r_s3),  offsetof(struct regs, r_s4),  offsetof(struct regs, r_s5), offsetof(struct regs, r_s6),
		offsetof(struct regs, r_a0),  offsetof(struct regs, r_a1),  offsetof(struct regs, r_a2), offsetof(struct regs, r_a3),
		offsetof(struct regs, r_a4),  offsetof(struct regs, r_a5),  offsetof(struct regs, r_t8), offsetof(struct regs, r_t9),
		offsetof(struct regs, r_t10), offsetof(struct regs, r_t11), offsetof(struct regs, r_ra), offsetof(struct regs, r_pv),
		offsetof(struct regs, r_at),  offsetof(struct regs, r_gp),  /* sp    , zero */
	};
	return (uint64_t *)((uintptr_t)rp + offset[index]);
}

static void write_reg(struct regs *rp, int index, uint64_t value)
{
	switch (index) {
	case 31:
		// do nothing
		break;
	case 30:
		pal_wrusp(value);
		break;
	default:
		*get_regs(rp, index) = value;
		break;
	}
}

static uint64_t read_reg(struct regs *rp, int index)
{
	uint64_t ret;
	switch (index) {
	case 31:
		ret = 0;
		break;
	case 30:
		ret = pal_rdusp();
		break;
	default:
		ret = *get_regs(rp, index);
		break;
	}
	return ret;
}

void unaligned_fault(uint64_t a0, uint64_t a1, uint64_t a2, struct regs *rp)
{
	//prom_printf("%s(): a0=%lx a1=%lx a2=%ld PC=%lx rp=%lx\n", __FUNCTION__, a0, a1, a2, rp->r_pc, rp);
	void *va = (void *)a0;
	uint64_t opcode = a1;
	uint64_t index = a2;
	if ((USERMODE(rp->r_ps) && va < (void *)kernelbase)) {
		uint16_t w;
		int32_t dw;
		uint64_t qw;
		int success = 0;
		switch (opcode) {
		case 0x0c:	/* ldwu */
			if (copyin(va, &w, sizeof(w)) == 0) {
				write_reg(rp, index, (uint64_t)w);
				success = 1;
			}
			break;
		case 0x28:	/* ldl */
			if (copyin(va, &dw, sizeof(dw)) == 0) {
				write_reg(rp, index, (uint64_t)dw);
				success = 1;
			}
			break;
		case 0x29:	/* ldq */
			if (copyin(va, &qw, sizeof(qw)) == 0) {
				write_reg(rp, index, qw);
				success = 1;
			}
			break;
		case 0x0d:	/* stw */
			w = (uint16_t)read_reg(rp, index);
			if (copyout(&w, va, sizeof(w)) == 0) {
				success = 1;
			}
			break;
		case 0x2c:	/* stl */
			dw = (int32_t)read_reg(rp, index);
			if (copyout(&dw, va, sizeof(dw)) == 0) {
				success = 1;
			}
			break;
		case 0x2d:	/* stq */
			qw = read_reg(rp, index);
			if (copyout(&qw, va, sizeof(qw)) == 0) {
				success = 1;
			}
			break;
		}
		if (success)
			return;
		trap(a0, a1, a2, T_UNA, rp);
	} else {
		trap(a0, a1, a2, T_UNA, rp);
	}
}

#define	USER	0x10000
void
trap(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t type, struct regs *rp)
{
	kthread_t *ct = curthread;
	enum seg_rw rw;
	proc_t *p = ttoproc(ct);
	klwp_t *lwp = ttolwp(ct);
	uintptr_t lofault;
	faultcode_t pagefault(), res, errcode;
	enum fault_type fault_type;
	k_siginfo_t siginfo;
	uint_t fault = 0;
	int mstate;
	int sicode = 0;
	int watchcode;
	int watchpage;
	caddr_t vaddr;
	int singlestep_twiddle;
	size_t sz;
	int ta;

//	prom_printf("%s(): a0=%lx a1=%lx a2=%ld type=%lx PC=%lx rp=%lx\n", __FUNCTION__, a0, a1, a2, type, rp->r_pc, rp);
	caddr_t addr = (caddr_t)a0;

	ASSERT_STACK_ALIGNED();

	CPU_STATS_ADDQ(CPU, sys, trap, 1);
	ASSERT(ct->t_schedflag & TS_DONT_SWAP);

	if (type == T_MM) {
		//prom_printf("%s(): a0=%lx a1=%lx a2=%ld type=%lx PC=%lx pfn=%lx\n", __FUNCTION__, a0, a1, a2, type, rp->r_pc, hat_getpfnum(p->p_as->a_hat, (caddr_t)a0));
		if (a1 == 0 || hat_getpfnum(p->p_as->a_hat, (caddr_t)a0) == PFN_INVALID) {
			fault_type = F_INVAL;
			//prom_printf("%s(): %d\n", __FUNCTION__, __LINE__);
		} else {
			fault_type = F_PROT;
			//prom_printf("%s(): %d\n", __FUNCTION__, __LINE__);
		}
		switch (a2) {
		case -1: rw = S_EXEC; break;
		case 0: rw = S_READ; break;
		case 1: rw = S_WRITE; break;
		default:
			prom_printf("%s(): %d\n", __FUNCTION__ ,__LINE__);
			die (a0, a1, a2, type, rp);
		}
	}

	if (USERMODE(rp->r_ps)) {
		/*
		 * Set up the current cred to use during this trap. u_cred
		 * no longer exists.  t_cred is used instead.
		 * The current process credential applies to the thread for
		 * the entire trap.  If trapping from the kernel, this
		 * should already be set up.
		 */
		if (ct->t_cred != p->p_cred) {
			cred_t *oldcred = ct->t_cred;
			/*
			 * DTrace accesses t_cred in probe context.  t_cred
			 * must always be either NULL, or point to a valid,
			 * allocated cred structure.
			 */
			ct->t_cred = crgetcred();
			crfree(oldcred);
		}
		ASSERT(lwp != NULL);
		type |= USER;
		ASSERT(lwptoregs(lwp) == rp);
		lwp->lwp_state = LWP_SYS;

		switch (type) {
		case T_MM + USER:
			if ((caddr_t)rp->r_pc == addr)
				mstate = LMS_TFAULT;
			else
				mstate = LMS_DFAULT;
			break;
		default:
			mstate = LMS_TRAP;
			break;
		}
		/* Kernel probe */
		mstate = new_mstate(ct, mstate);

		bzero(&siginfo, sizeof (siginfo));
	}

	switch (type) {
	default:
		prom_printf("%s(): %d a0=%lx a1=%lx a2=%lx type=%lx rp=%lx PC=%lx\n", __FUNCTION__ ,__LINE__,a0, a1, a2, type, rp, rp->r_pc);
		die (a0, a1, a2, type, rp);
		if (type & USER) {
			siginfo.si_signo = SIGILL;
			siginfo.si_code  = ILL_ILLTRP;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			siginfo.si_trapno = type & ~USER;
			fault = FLTILL;
			break;
		} else {
			prom_printf("%s(): %d\n", __FUNCTION__ ,__LINE__);
			die (a0, a1, a2, type, rp);
		}

	case T_MM:		/* system page fault */
		/*
		 * If we're under on_trap() protection (see <sys/ontrap.h>),
		 * set ot_trap and bounce back to the on_trap() call site
		 * via the installed trampoline.
		 */
		if ((ct->t_ontrap != NULL) &&
		    (ct->t_ontrap->ot_prot & OT_DATA_ACCESS)) {
			ct->t_ontrap->ot_trap |= OT_DATA_ACCESS;
			rp->r_pc = ct->t_ontrap->ot_trampoline;
			goto cleanup;
		}

		/*
		 * See if we can handle as pagefault. Save lofault
		 * across this. Here we assume that an address
		 * less than KERNELBASE is a user fault.
		 * We can do this as copy.s routines verify that the
		 * starting address is less than KERNELBASE before
		 * starting and because we know that we always have
		 * KERNELBASE mapped as invalid to serve as a "barrier".
		 */
		lofault = ct->t_lofault;
		ct->t_lofault = 0;

		mstate = new_mstate(ct, LMS_KFAULT);

		if (addr < (caddr_t)kernelbase) {
			res = pagefault(addr, fault_type, rw, 0);
			if (res == FC_NOMAP && addr < p->p_usrstack && grow(addr))
				res = 0;
		} else {
			res = pagefault(addr, fault_type, rw, 1);
		}
		(void) new_mstate(ct, mstate);

		/*
		 * Restore lofault. If we resolved the fault, exit.
		 * If we didn't and lofault wasn't set, die.
		 */
		ct->t_lofault = lofault;
		if (res == 0)
			goto cleanup;

		if (lofault == 0) {
			prom_printf("%s(): %d\n", __FUNCTION__ ,__LINE__);
			die (a0, a1, a2, type, rp);
		}

		/*
		 * Cannot resolve fault.  Return to lofault.
		 */
		if (FC_CODE(res) == FC_OBJERR)
			res = FC_ERRNO(res);
		else
			res = EFAULT;
		rp->r_v0 = res;
		rp->r_pc = ct->t_lofault;
		goto cleanup;

	case T_MM + USER:	/* user page fault */
		ASSERT(!(curthread->t_flag & T_WATCHPT));
		res = pagefault(addr, fault_type, rw, 0);

		/*
		 * If pagefault() succeeded, ok.
		 * Otherwise attempt to grow the stack.
		 */
		if (res == 0 ||
		    (res == FC_NOMAP && addr < p->p_usrstack && grow(addr))) {
			lwp->lwp_lastfault = FLTPAGE;
			lwp->lwp_lastfaddr = addr;
			if (prismember(&p->p_fltmask, FLTPAGE)) {
				bzero(&siginfo, sizeof (siginfo));
				siginfo.si_addr = addr;
				(void) stop_on_fault(FLTPAGE, &siginfo);
			}
			goto out;
		} else if (res == FC_PROT && addr < p->p_usrstack && rw == S_EXEC) {
			report_stack_exec(p, addr);
		}

		// For debug
		die (a0, a1, a2, type, rp);

		/*
		 * In the case where both pagefault and grow fail,
		 * set the code to the value provided by pagefault.
		 * We map all errors returned from pagefault() to SIGSEGV.
		 */
		bzero(&siginfo, sizeof (siginfo));
		siginfo.si_addr = addr;
		switch (FC_CODE(res)) {
		case FC_HWERR:
		case FC_NOSUPPORT:
			siginfo.si_signo = SIGBUS;
			siginfo.si_code = BUS_ADRERR;
			fault = FLTACCESS;
			prom_printf("%s(): %d\n", __FUNCTION__, __LINE__);
			break;
		case FC_ALIGN:
			siginfo.si_signo = SIGBUS;
			siginfo.si_code = BUS_ADRALN;
			fault = FLTACCESS;
			prom_printf("%s(): %d\n", __FUNCTION__, __LINE__);
			break;
		case FC_OBJERR:
			if ((siginfo.si_errno = FC_ERRNO(res)) != EINTR) {
				siginfo.si_signo = SIGBUS;
				siginfo.si_code = BUS_OBJERR;
				fault = FLTACCESS;
				prom_printf("%s(): %d\n", __FUNCTION__, __LINE__);
			}
			prom_printf("%s(): %d\n", __FUNCTION__, __LINE__);
			break;
		default:	/* FC_NOMAP or FC_PROT */
			siginfo.si_signo = SIGSEGV;
			siginfo.si_code =
			    (res == FC_NOMAP)? SEGV_MAPERR : SEGV_ACCERR;
			fault = FLTBOUNDS;
			prom_printf("%s(): %d\n", __FUNCTION__, __LINE__);
			break;
		}
		break;

	case T_FAULT + USER:
		switch (a0) {
		case T_IF_GENTRAP:
			if (rp->r_a0 == -2) {
				// div by zero. raise by software div call.
				siginfo.si_signo = SIGFPE;
				siginfo.si_code  = FPE_FLTDIV;
				siginfo.si_addr  = (caddr_t)rp->r_pc;
				fault = FLTFPE;
				break;
			}
		case T_IF_BPT:
		case T_IF_BUGCHK:
		default:
			die (a0, a1, a2, type, rp);
			break;
		case T_IF_OPDEC:
			siginfo.si_signo = SIGILL;
			siginfo.si_code  = ILL_ILLOPC;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			fault = FLTILL;
			break;
		case T_IF_FEN:
			if (fp_fenflt()) {
				siginfo.si_signo = SIGILL;
				siginfo.si_code  = ILL_ILLOPC;
				siginfo.si_addr  = (caddr_t)rp->r_pc;
				fault = FLTILL;
			} else {
				goto out;
			}
			break;
		}
		break;

	case T_ARITH + USER:
		switch (a0) {
		case T_ARITH_IOV:
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_INTOVF;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			fault = FLTFPE;
			break;

		case T_ARITH_DZE:
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTDIV;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			fault = FLTFPE;
			break;

		case T_ARITH_UNF:
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTUND;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			fault = FLTFPE;
			break;

		case T_ARITH_INE:
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTRES;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			fault = FLTFPE;
			break;

		case T_ARITH_OVF:
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTOVF;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			fault = FLTFPE;
			break;

		case T_ARITH_INV:
		case T_ARITH_SWC:
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTINV;
			siginfo.si_addr  = (caddr_t)rp->r_pc;
			fault = FLTFPE;
			break;
		}
		break;

	case T_AST:
		/*
		 * This occurs only after the cs register has been made to
		 * look like a kernel selector, either through debugging or
		 * possibly by functions like setcontext().  The thread is
		 * about to cause a general protection fault at common_iret()
		 * in locore.  We let that happen immediately instead of
		 * doing the T_AST processing.
		 */
		goto cleanup;

	case T_AST + USER:	/* profiling, resched, h/w error pseudo trap */
		if (lwp->lwp_pcb.pcb_flags & ASYNC_HWERR) {
			proc_t *p = ttoproc(curthread);
			extern void print_msg_hwerr(ctid_t ct_id, proc_t *p);

			lwp->lwp_pcb.pcb_flags &= ~ASYNC_HWERR;
			print_msg_hwerr(p->p_ct_process->conp_contract.ct_id,
			    p);
			contract_process_hwerr(p->p_ct_process, p);
			siginfo.si_signo = SIGKILL;
			siginfo.si_code = SI_NOINFO;
		} else if (lwp->lwp_pcb.pcb_flags & CPC_OVERFLOW) {
			lwp->lwp_pcb.pcb_flags &= ~CPC_OVERFLOW;
			if (kcpc_overflow_ast()) {
				/*
				 * Signal performance counter overflow
				 */
				bzero(&siginfo, sizeof (siginfo));
				siginfo.si_signo = SIGEMT;
				siginfo.si_code = EMT_CPCOVF;
				siginfo.si_addr = (caddr_t)rp->r_pc;
				fault = FLTCPCOVF;
			}
		}

		break;
	}

	/*
	 * We can't get here from a system trap
	 */
	ASSERT(type & USER);

	if (fault) {
		/* We took a fault so abort single step. */
		lwp->lwp_pcb.pcb_flags &= ~(NORMAL_STEP|WATCH_STEP);
		/*
		 * Remember the fault and fault adddress
		 * for real-time (SIGPROF) profiling.
		 */
		lwp->lwp_lastfault = fault;
		lwp->lwp_lastfaddr = siginfo.si_addr;

		DTRACE_PROC2(fault, int, fault, ksiginfo_t *, &siginfo);

		/*
		 * If a debugger has declared this fault to be an
		 * event of interest, stop the lwp.  Otherwise just
		 * deliver the associated signal.
		 */
		if (siginfo.si_signo != SIGKILL &&
		    prismember(&p->p_fltmask, fault) &&
		    stop_on_fault(fault, &siginfo) == 0)
			siginfo.si_signo = 0;
	}

	if (siginfo.si_signo)
		trapsig(&siginfo, (fault != FLTFPE && fault != FLTCPCOVF));

	if (lwp->lwp_oweupc)
		profil_tick(rp->r_pc);

	if (ct->t_astflag | ct->t_sig_check) {
		/*
		 * Turn off the AST flag before checking all the conditions that
		 * may have caused an AST.  This flag is on whenever a signal or
		 * unusual condition should be handled after the next trap or
		 * syscall.
		 */
		astoff(ct);

		ct->t_sig_check = 0;

		mutex_enter(&p->p_lock);
		if (curthread->t_proc_flag & TP_CHANGEBIND) {
			timer_lwpbind();
			curthread->t_proc_flag &= ~TP_CHANGEBIND;
		}
		mutex_exit(&p->p_lock);

		/*
		 * for kaio requests that are on the per-process poll queue,
		 * aiop->aio_pollq, they're AIO_POLL bit is set, the kernel
		 * should copyout their result_t to user memory. by copying
		 * out the result_t, the user can poll on memory waiting
		 * for the kaio request to complete.
		 */
		if (p->p_aio)
			aio_cleanup(0);
		/*
		 * If this LWP was asked to hold, call holdlwp(), which will
		 * stop.  holdlwps() sets this up and calls pokelwps() which
		 * sets the AST flag.
		 *
		 * Also check TP_EXITLWP, since this is used by fresh new LWPs
		 * through lwp_rtt().  That flag is set if the lwp_create(2)
		 * syscall failed after creating the LWP.
		 */
		if (ISHOLD(p))
			holdlwp();

		/*
		 * All code that sets signals and makes ISSIG evaluate true must
		 * set t_astflag afterwards.
		 */
		if (ISSIG_PENDING(ct, lwp, p)) {
			if (issig(FORREAL))
				psig();
			ct->t_sig_check = 1;
		}

		if (ct->t_rprof != NULL) {
			realsigprof(0, 0, 0);
			ct->t_sig_check = 1;
		}
	}

out:	/* We can't get here from a system trap */
	ASSERT(type & USER);

	if (ISHOLD(p))
		holdlwp();

	/*
	 * Set state to LWP_USER here so preempt won't give us a kernel
	 * priority if it occurs after this point.  Call CL_TRAPRET() to
	 * restore the user-level priority.
	 *
	 * It is important that no locks (other than spinlocks) be entered
	 * after this point before returning to user mode (unless lwp_state
	 * is set back to LWP_SYS).
	 */
	lwp->lwp_state = LWP_USER;

	if (ct->t_trapret) {
		ct->t_trapret = 0;
		thread_lock(ct);
		CL_TRAPRET(ct);
		thread_unlock(ct);
	}
	if (CPU->cpu_runrun || curthread->t_schedflag & TS_ANYWAITQ)
		preempt();
	prunstop();
	(void) new_mstate(ct, mstate);

	/* Kernel probe */

	return;

cleanup:	/* system traps end up here */
	ASSERT(!(type & USER));
}

/*
 * Patch non-zero to disable preemption of threads in the kernel.
 */
int IGNORE_KERNEL_PREEMPTION = 0;	/* XXX - delete this someday */

struct kpreempt_cnts {		/* kernel preemption statistics */
	int	kpc_idle;	/* executing idle thread */
	int	kpc_intr;	/* executing interrupt thread */
	int	kpc_clock;	/* executing clock thread */
	int	kpc_blocked;	/* thread has blocked preemption (t_preempt) */
	int	kpc_notonproc;	/* thread is surrendering processor */
	int	kpc_inswtch;	/* thread has ratified scheduling decision */
	int	kpc_prilevel;	/* processor interrupt level is too high */
	int	kpc_apreempt;	/* asynchronous preemption */
	int	kpc_spreempt;	/* synchronous preemption */
} kpreempt_cnts;

/*
 * kernel preemption: forced rescheduling, preempt the running kernel thread.
 *	the argument is old PIL for an interrupt,
 *	or the distingished value KPREEMPT_SYNC.
 */
void
kpreempt(int asyncspl)
{
	kthread_t *ct = curthread;

	if (IGNORE_KERNEL_PREEMPTION) {
		aston(CPU->cpu_dispthread);
		return;
	}

	/*
	 * Check that conditions are right for kernel preemption
	 */
	do {
		if (ct->t_preempt) {
			/*
			 * either a privileged thread (idle, panic, interrupt)
			 * or will check when t_preempt is lowered
			 * We need to specifically handle the case where
			 * the thread is in the middle of swtch (resume has
			 * been called) and has its t_preempt set
			 * [idle thread and a thread which is in kpreempt
			 * already] and then a high priority thread is
			 * available in the local dispatch queue.
			 * In this case the resumed thread needs to take a
			 * trap so that it can call kpreempt. We achieve
			 * this by using siron().
			 * How do we detect this condition:
			 * idle thread is running and is in the midst of
			 * resume: curthread->t_pri == -1 && CPU->dispthread
			 * != CPU->thread
			 * Need to ensure that this happens only at high pil
			 * resume is called at high pil
			 * Only in resume_from_idle is the pil changed.
			 */
			if (ct->t_pri < 0) {
				kpreempt_cnts.kpc_idle++;
				if (CPU->cpu_dispthread != CPU->cpu_thread)
					siron();
			} else if (ct->t_flag & T_INTR_THREAD) {
				kpreempt_cnts.kpc_intr++;
				if (ct->t_pil == CLOCK_LEVEL)
					kpreempt_cnts.kpc_clock++;
			} else {
				kpreempt_cnts.kpc_blocked++;
				if (CPU->cpu_dispthread != CPU->cpu_thread)
					siron();
			}
			aston(CPU->cpu_dispthread);
			return;
		}
		if (ct->t_state != TS_ONPROC ||
		    ct->t_disp_queue != CPU->cpu_disp) {
			/* this thread will be calling swtch() shortly */
			kpreempt_cnts.kpc_notonproc++;
			if (CPU->cpu_thread != CPU->cpu_dispthread) {
				/* already in swtch(), force another */
				kpreempt_cnts.kpc_inswtch++;
				siron();
			}
			return;
		}
		if (getpil() >= DISP_LEVEL) {
			/*
			 * We can't preempt this thread if it is at
			 * a PIL >= DISP_LEVEL since it may be holding
			 * a spin lock (like sched_lock).
			 */
			siron();	/* check back later */
			kpreempt_cnts.kpc_prilevel++;
			return;
		}
		if (!interrupts_enabled()) {
			/*
			 * Can't preempt while running with ints disabled
			 */
			kpreempt_cnts.kpc_prilevel++;
			return;
		}
		if (asyncspl != KPREEMPT_SYNC)
			kpreempt_cnts.kpc_apreempt++;
		else
			kpreempt_cnts.kpc_spreempt++;

		ct->t_preempt++;
		preempt();
		ct->t_preempt--;
	} while (CPU->cpu_kprunrun);
}


void
panic_showtrap(struct panic_trap_info *tip)
{
}

void
panic_savetrap(panic_data_t *pdp, struct panic_trap_info *tip)
{
}

static uint64_t fasttrap_null(void)
{
	return (uint64_t)-1;
}

static uint64_t fasttrap_gethrtime(void)
{
	return (uint64_t)gethrtime();
}

static uint64_t fasttrap_gethrvtime(void)
{
	hrtime_t hrt = gethrtime_unscaled();
	kthread_t *ct = curthread;
	klwp_t *lwp = ttolwp(ct);
	hrt -= lwp->lwp_mstate.ms_state_start;
	hrt += lwp->lwp_mstate.ms_acct[LWP_USER];
	scalehrtime(&hrt);
	return (uint64_t)hrt;
}

extern uint64_t fasttrap_gethrestime(void);
extern uint64_t getlgrp(void);

uint64_t (*fasttrap_table[])() = {
	fasttrap_null,
	fasttrap_gethrtime,
	fasttrap_gethrvtime,
	fasttrap_gethrestime,
	getlgrp,
};
