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
 * Copyright (c) 2004, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, Joyent, Inc.  All rights reserverd.
 */

#include <sys/cpuvar.h>
#include <sys/cpu_event.h>
#include <sys/regset.h>
#include <sys/psw.h>
#include <sys/types.h>
#include <sys/thread.h>
#include <sys/systm.h>
#include <sys/pcb.h>
#include <sys/trap.h>
#include <sys/ftrace.h>
#include <sys/clock.h>
#include <sys/panic.h>
#include <sys/disp.h>
#include <vm/seg_kp.h>
#include <sys/stack.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/kstat.h>
#include <sys/smp_impldefs.h>
#include <sys/pool_pset.h>
#include <sys/zone.h>
#include <sys/bitmap.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/ontrap.h>
#include <sys/promif.h>
#include <sys/fault.h>
#include <sys/procfs.h>
#include <sys/fp.h>
#include <sys/contract/process_impl.h>
#include <sys/aio_impl.h>
#include <sys/prsystm.h>
#include <vm/hat_riscv64.h>
#include <sys/frame.h>

extern void print_msg_hwerr(ctid_t ct_id, proc_t *p);

void
panic_showtrap(struct panic_trap_info *tip)
{
}

void
panic_savetrap(panic_data_t *pdp, struct panic_trap_info *tip)
{
}
static inline int l2_pteidx(uintptr_t vaddr) { return ((vaddr >> (PAGESHIFT+2*NPTESHIFT)) & ((1<<NPTESHIFT)-1));}
static inline int l1_pteidx(uintptr_t vaddr) { return ((vaddr >> (PAGESHIFT+1*NPTESHIFT)) & ((1<<NPTESHIFT)-1));}
static inline int l0_pteidx(uintptr_t vaddr) { return ((vaddr >> (PAGESHIFT+0*NPTESHIFT)) & ((1<<NPTESHIFT)-1));}

void
dump_trap(uint64_t scause, uint64_t stval, struct regs *rp)
{
	static volatile int exclusion;

	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	while (__sync_lock_test_and_set(&exclusion, 1)) {}

	kthread_t *ct = curthread;
	proc_t *p = ttoproc(ct);
	if (p) {
		prom_printf("%s(): pid=%d\n", __FUNCTION__, p->p_pid);
		if (p->p_exec && p->p_exec->v_path) {
			prom_printf("%s(): path=%s\n", __FUNCTION__, p->p_exec->v_path);
		}
	}

	prom_printf("%s(): scause=0x%lx stval=%p cpu_id=%d\n", __FUNCTION__, scause, stval, CPU->cpu_id);
	prom_printf("      RA=%016lx\n", rp->r_ra);
	prom_printf("      SP=%016lx\n", rp->r_sp);
	prom_printf("      GP=%016lx\n", rp->r_gp);
	prom_printf("      TP=%016lx\n", rp->r_tp);
	prom_printf("      T0=%016lx\n", rp->r_t0);
	prom_printf("      T1=%016lx\n", rp->r_t1);
	prom_printf("      T2=%016lx\n", rp->r_t2);
	prom_printf("      S0=%016lx\n", rp->r_s0);
	prom_printf("      S1=%016lx\n", rp->r_s1);
	prom_printf("      A0=%016lx\n", rp->r_a0);
	prom_printf("      A1=%016lx\n", rp->r_a1);
	prom_printf("      A2=%016lx\n", rp->r_a2);
	prom_printf("      A3=%016lx\n", rp->r_a3);
	prom_printf("      A4=%016lx\n", rp->r_a4);
	prom_printf("      A5=%016lx\n", rp->r_a5);
	prom_printf("      A6=%016lx\n", rp->r_a6);
	prom_printf("      A7=%016lx\n", rp->r_a7);
	prom_printf("      S2=%016lx\n", rp->r_s2);
	prom_printf("      S3=%016lx\n", rp->r_s3);
	prom_printf("      S4=%016lx\n", rp->r_s4);
	prom_printf("      S5=%016lx\n", rp->r_s5);
	prom_printf("      S6=%016lx\n", rp->r_s6);
	prom_printf("      S7=%016lx\n", rp->r_s7);
	prom_printf("      S8=%016lx\n", rp->r_s8);
	prom_printf("      S9=%016lx\n", rp->r_s9);
	prom_printf("      S10=%016lx\n", rp->r_s10);
	prom_printf("      S11=%016lx\n", rp->r_s11);
	prom_printf("      T2=%016lx\n", rp->r_t2);
	prom_printf("      T3=%016lx\n", rp->r_t3);
	prom_printf("      T4=%016lx\n", rp->r_t4);
	prom_printf("      T5=%016lx\n", rp->r_t5);
	prom_printf("      T6=%016lx\n", rp->r_t6);
	prom_printf("      PC=%016lx\n", rp->r_pc);
	prom_printf("      SSR=%016lx\n", rp->r_ssr);


	if (USERMODE(rp->r_ssr)) {
		struct frame *fp = (struct frame *)rp->r_s0 - 1;
		struct frame fr;
		int i = 0;
		for (;;) {
			if (copyin(fp, &fr, sizeof(fr)) != 0) {
				break;
			}
			prom_printf("  frame[%d]= %016lx\n", i++, fr.fr_savpc);
			fp = (struct frame *)fr.fr_savfp - 1;
		}
	}

	{
		uintptr_t vaddr = ALIGN2PAGE(stval);

		uint64_t satp = csr_read_satp();
		ASSERT((satp & SATP_MODE_MASK) == SATP_MODE_SV39);
		pte_t *l2pt = (pte_t *)pa_to_kseg((satp & SATP_PPN_MASK) << MMU_PAGESHIFT);
		pte_t l2pte = l2pt[l2_pteidx(vaddr)];
		prom_printf("  l2pte = %016lx\n", l2pte);
		if ((l2pte & PTE_V) != 0 && IS_TABLE(l2pte)) {
			pte_t *l1pt = (pte_t *)(PTE_TO_PA(l2pte) + SEGKPM_BASE);
			pte_t l1pte = l1pt[l1_pteidx(vaddr)];
			prom_printf("  l1pte = %016lx\n", l1pte);
			if ((l1pte & PTE_V) != 0 && IS_TABLE(l1pte)) {
				pte_t *l0pt = (pte_t *)(PTE_TO_PA(l1pte) + SEGKPM_BASE);
				pte_t l0pte = l0pt[l0_pteidx(vaddr)];
				prom_printf("  l0pte = %016lx\n", l0pte);
			}
		}
	}

	__sync_lock_release(&exclusion);
	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
}
void
dump_svc(uint64_t sysnum, struct regs *rp)
{
	prom_printf("SVC=%d\n", (int)sysnum);
	prom_printf("      RA=%016lx\n", rp->r_ra);
	prom_printf("      SP=%016lx\n", rp->r_sp);
	prom_printf("      GP=%016lx\n", rp->r_gp);
	prom_printf("      TP=%016lx\n", rp->r_tp);
	prom_printf("      T0=%016lx\n", rp->r_t0);
	prom_printf("      T1=%016lx\n", rp->r_t1);
	prom_printf("      T2=%016lx\n", rp->r_t2);
	prom_printf("      S0=%016lx\n", rp->r_s0);
	prom_printf("      S1=%016lx\n", rp->r_s1);
	prom_printf("      A0=%016lx\n", rp->r_a0);
	prom_printf("      A1=%016lx\n", rp->r_a1);
	prom_printf("      A2=%016lx\n", rp->r_a2);
	prom_printf("      A3=%016lx\n", rp->r_a3);
	prom_printf("      A4=%016lx\n", rp->r_a4);
	prom_printf("      A5=%016lx\n", rp->r_a5);
	prom_printf("      A6=%016lx\n", rp->r_a6);
	prom_printf("      A7=%016lx\n", rp->r_a7);
	prom_printf("      S2=%016lx\n", rp->r_s2);
	prom_printf("      S3=%016lx\n", rp->r_s3);
	prom_printf("      S4=%016lx\n", rp->r_s4);
	prom_printf("      S5=%016lx\n", rp->r_s5);
	prom_printf("      S6=%016lx\n", rp->r_s6);
	prom_printf("      S7=%016lx\n", rp->r_s7);
	prom_printf("      S8=%016lx\n", rp->r_s8);
	prom_printf("      S9=%016lx\n", rp->r_s9);
	prom_printf("      S10=%016lx\n", rp->r_s10);
	prom_printf("      S11=%016lx\n", rp->r_s11);
	prom_printf("      T2=%016lx\n", rp->r_t2);
	prom_printf("      T3=%016lx\n", rp->r_t3);
	prom_printf("      T4=%016lx\n", rp->r_t4);
	prom_printf("      T5=%016lx\n", rp->r_t5);
	prom_printf("      T6=%016lx\n", rp->r_t6);
	prom_printf("      PC=%016lx\n", rp->r_pc);
	prom_printf("      SSR=%016lx\n", rp->r_ssr);
}
static void
die(uint64_t scause, uint64_t stval, struct regs *rp)
{
	dump_trap(scause, stval, rp);
	prom_panic("die");
}

void
trap(uint64_t scause, uint64_t stval, struct regs *rp)
{
	kthread_t *ct = curthread;
	enum seg_rw rw;
	proc_t *p = ttoproc(ct);
	klwp_t *lwp = ttolwp(ct);
	uintptr_t lofault;
	label_t *onfault;
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
	caddr_t addr = (caddr_t)stval;

	ASSERT_STACK_ALIGNED();

	CPU_STATS_ADDQ(CPU, sys, trap, 1);
	ASSERT(ct->t_schedflag & TS_DONT_SWAP);

	switch (scause) {
	case T_IACCESS:
	case T_IPGFLT:
		mstate = LMS_TFAULT;
		rw = S_EXEC;
		break;

	case T_LACCESS:
	case T_LPGFLT:
		mstate = LMS_DFAULT;
		rw = S_READ;
		break;

	case T_SACCESS:
	case T_SPGFLT:
		mstate = LMS_DFAULT;
		rw = S_WRITE;
		break;
	case T_IALIGN:
	case T_AALIGN:
	default:
		mstate = LMS_TRAP;
		break;
	}
	if (mstate != LMS_TRAP) {
		if ((caddr_t)_userlimit <= addr && addr < (caddr_t)kernelbase) {
			fault_type = F_INVAL;
		} else if (hat_page_falt(addr < (caddr_t)kernelbase ?  p->p_as->a_hat: kas.a_hat, addr, rw) == 0) {
			return;
		} else {
			fault_type = (hat_getpfnum(addr < (caddr_t)_userlimit ?  p->p_as->a_hat: kas.a_hat, addr) == PFN_INVALID)? F_INVAL: F_PROT;
		}
	}

	if (USERMODE(rp->r_ssr)) {
		if (ct->t_cred != p->p_cred) {
			cred_t *oldcred = ct->t_cred;
			ct->t_cred = crgetcred();
			crfree(oldcred);
		}
		ASSERT(lwp != NULL);
		ASSERT(lwptoregs(lwp) == rp);
		lwp->lwp_state = LWP_SYS;

		mstate = new_mstate(ct, mstate);

		bzero(&siginfo, sizeof (siginfo));

		switch (scause) {
		default:
			{
				siginfo.si_signo = SIGILL;
				siginfo.si_code  = ILL_ILLTRP;
				siginfo.si_addr  = (caddr_t)rp->r_pc;
				siginfo.si_trapno = scause;
				fault = FLTILL;
			}
			break;
		case T_IALIGN:
		case T_AALIGN:
			{
				siginfo.si_signo = SIGBUS;
				siginfo.si_code = BUS_ADRALN;
				siginfo.si_addr = (caddr_t)rp->r_pc;
				fault = FLTACCESS;
			}
			break;
		case T_IACCESS:
		case T_IPGFLT:
		case T_LACCESS:
		case T_LPGFLT:
		case T_SACCESS:
		case T_SPGFLT:
			{
				ASSERT(!(curthread->t_flag & T_WATCHPT));
				res = pagefault(addr, fault_type, rw, 0);

				if (res == 0 ||
				    (res == FC_NOMAP && addr < p->p_usrstack && grow(addr))) {
					lwp->lwp_lastfault = FLTPAGE;
					lwp->lwp_lastfaddr = addr;
					if (prismember(&p->p_fltmask, FLTPAGE)) {
						siginfo.si_addr = addr;
						(void) stop_on_fault(FLTPAGE, &siginfo);
					}
					goto out;
				} else if (res == FC_PROT && addr < p->p_usrstack && rw == S_EXEC) {
					report_stack_exec(p, addr);
				}

				siginfo.si_addr = addr;
				switch (FC_CODE(res)) {
				case FC_HWERR:
				case FC_NOSUPPORT:
					siginfo.si_signo = SIGBUS;
					siginfo.si_code = BUS_ADRERR;
					fault = FLTACCESS;
					break;
				case FC_ALIGN:
					siginfo.si_signo = SIGBUS;
					siginfo.si_code = BUS_ADRALN;
					fault = FLTACCESS;
					break;
				case FC_OBJERR:
					if ((siginfo.si_errno = FC_ERRNO(res)) != EINTR) {
						siginfo.si_signo = SIGBUS;
						siginfo.si_code = BUS_OBJERR;
						fault = FLTACCESS;
					}
					break;
				default:
					siginfo.si_signo = SIGSEGV;
					siginfo.si_code = ((res == FC_NOMAP)? SEGV_MAPERR : SEGV_ACCERR);
					fault = FLTBOUNDS;
					break;
				}
			}
			break;
		case T_ILEXC:
			{
				siginfo.si_signo = SIGILL;
				siginfo.si_code  = ILL_ILLOPC;
				siginfo.si_addr  = (caddr_t)rp->r_pc;
				fault = FLTILL;
			}
			break;
		case T_AST:
			{
				if (lwp->lwp_pcb.pcb_flags & ASYNC_HWERR) {
					proc_t *p = ttoproc(curthread);
					extern void print_msg_hwerr(ctid_t ct_id, proc_t *p);

					lwp->lwp_pcb.pcb_flags &= ~ASYNC_HWERR;
					print_msg_hwerr(p->p_ct_process->conp_contract.ct_id, p);
					contract_process_hwerr(p->p_ct_process, p);
					siginfo.si_signo = SIGKILL;
					siginfo.si_code = SI_NOINFO;
				} else if (lwp->lwp_pcb.pcb_flags & CPC_OVERFLOW) {
					lwp->lwp_pcb.pcb_flags &= ~CPC_OVERFLOW;
					if (kcpc_overflow_ast()) {
						siginfo.si_signo = SIGEMT;
						siginfo.si_code = EMT_CPCOVF;
						siginfo.si_addr = (caddr_t)rp->r_pc;
						fault = FLTCPCOVF;
					}
				}
			}
			break;
		case T_BRK:
			{
				siginfo.si_signo = SIGTRAP;
				siginfo.si_code  = TRAP_BRKPT;
				siginfo.si_addr  = (caddr_t)rp->r_pc;
				fault = FLTBPT;
			}
			break;
		}

		if (fault) {
			dump_trap(scause, stval, rp);
			lwp->lwp_pcb.pcb_flags &= ~(NORMAL_STEP|WATCH_STEP);
			lwp->lwp_lastfault = fault;
			lwp->lwp_lastfaddr = siginfo.si_addr;

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
			astoff(ct);

			if (lwp->lwp_pcb.pcb_flags & DEBUG_PENDING)
				deferred_singlestep_trap((caddr_t)rp->r_pc);

			ct->t_sig_check = 0;

			if (curthread->t_proc_flag & TP_CHANGEBIND) {
				mutex_enter(&p->p_lock);
				if (curthread->t_proc_flag & TP_CHANGEBIND) {
					timer_lwpbind();
					curthread->t_proc_flag &= ~TP_CHANGEBIND;
				}
				mutex_exit(&p->p_lock);
			}

			if (p->p_aio)
				aio_cleanup(0);

			if (ISHOLD(p))
				holdlwp();

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
out:
		if (ISHOLD(p))
			holdlwp();

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
		new_mstate(ct, mstate);
	} else {
		switch (scause) {
		default:
			die(scause, stval, rp);
			break;
		case T_IACCESS:
		case T_IPGFLT:
		case T_LACCESS:
		case T_LPGFLT:
		case T_SACCESS:
		case T_SPGFLT:
			{
				if ((ct->t_ontrap != NULL) &&
				    (ct->t_ontrap->ot_prot & OT_DATA_ACCESS)) {
					ct->t_ontrap->ot_trap |= OT_DATA_ACCESS;
					rp->r_pc = ct->t_ontrap->ot_trampoline;
					goto cleanup;
				}
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
				new_mstate(ct, mstate);

				ct->t_lofault = lofault;
				if (res == 0)
					goto cleanup;

				if (lofault == 0) {
					die(scause, stval, rp);
				}

				if (FC_CODE(res) == FC_OBJERR)
					res = FC_ERRNO(res);
				else
					res = EFAULT;
				rp->r_a0 = res;
				rp->r_pc = ct->t_lofault;
				goto cleanup;
			}
			break;
		case T_AST:
			goto cleanup;
			break;
		}
	}
cleanup:
	return;
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
