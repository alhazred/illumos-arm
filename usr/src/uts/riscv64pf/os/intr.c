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
#include <sys/csr.h>
#include <sys/x_call.h>
#include <sys/irq.h>
#include <sys/cbe.h>
#include <sys/byteorder.h>

static inline uint_t
bsrw_insn(uint16_t mask)
{
	return 31 - __builtin_clz(mask);
}

/*
 * Set cpu's base SPL level to the highest active interrupt level
 */
void
set_base_spl(void)
{
	struct cpu *cpu = CPU;
	uint16_t active = (uint16_t)cpu->cpu_intr_actv;

	cpu->cpu_base_spl = active == 0 ? 0 : bsrw_insn(active);
}

/*
 * Do all the work necessary to set up the cpu and thread structures
 * to dispatch a high-level interrupt.
 *
 * Returns 0 if we're -not- already on the high-level interrupt stack,
 * (and *must* switch to it), non-zero if we are already on that stack.
 *
 * Called with interrupts masked.
 * The 'pil' is already set to the appropriate level for rp->r_trapno.
 */
int
hilevel_intr_prolog(struct cpu *cpu, uint_t pil, uint_t oldpil, struct regs *rp, int irq)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t mask;
	hrtime_t intrtime;
	hrtime_t now = csr_read_time();

	ASSERT(pil > LOCK_LEVEL);

	if (pil == CBE_HIGH_PIL) {
		cpu->cpu_profile_pil = oldpil;
		if (USERMODE(rp->r_ssr)) {
			cpu->cpu_profile_pc = 0;
			cpu->cpu_profile_upc = rp->r_pc;
			cpu->cpu_cpcprofile_pc = 0;
			cpu->cpu_cpcprofile_upc = rp->r_pc;
		} else {
			cpu->cpu_profile_pc = rp->r_pc;
			cpu->cpu_profile_upc = 0;
			cpu->cpu_cpcprofile_pc = rp->r_pc;
			cpu->cpu_cpcprofile_upc = 0;
		}
	}

	mask = cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK;
	if (mask != 0) {
		int nestpil;

		/*
		 * We have interrupted another high-level interrupt.
		 * Load starting timestamp, compute interval, update
		 * cumulative counter.
		 */
		nestpil = bsrw_insn((uint16_t)mask);
		if (nestpil >= pil) {
			//prom_printf("%s:%d %d %d %d\n",__func__,__LINE__,nestpil,pil,oldpil);
			//prom_printf("%s:%d %d %x %x %x\n",__func__,__LINE__,irq,mask, csr_read_sie(), csr_read_sip());
		}
		ASSERT(nestpil < pil);
		intrtime = now -
		    mcpu->pil_high_start[nestpil - (LOCK_LEVEL + 1)];
		mcpu->intrstat[nestpil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		/*
		 * Another high-level interrupt is active below this one, so
		 * there is no need to check for an interrupt thread.  That
		 * will be done by the lowest priority high-level interrupt
		 * active.
		 */
	} else {
		kthread_t *t = cpu->cpu_thread;

		/*
		 * See if we are interrupting a low-level interrupt thread.
		 * If so, account for its time slice only if its time stamp
		 * is non-zero.
		 */
		if ((t->t_flag & T_INTR_THREAD) != 0 && t->t_intr_start != 0) {
			intrtime = now - t->t_intr_start;
			mcpu->intrstat[t->t_pil][0] += intrtime;
			cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
			t->t_intr_start = 0;
		}
	}

	/*
	 * Store starting timestamp in CPU structure for this PIL.
	 */
	mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)] = now;

	ASSERT((cpu->cpu_intr_actv & (1 << pil)) == 0);

	mask = cpu->cpu_intr_actv;
	cpu->cpu_intr_actv |= (1 << pil)|CPU_INTR_ACTV_INTR_STACK_MASK;

	return (mask & CPU_INTR_ACTV_INTR_STACK_MASK);
}

/*
 * Does most of the work of returning from a high level interrupt.
 *
 * Returns 0 if there are no more high level interrupts (in which
 * case we must switch back to the interrupted thread stack) or
 * non-zero if there are more (in which case we should stay on it).
 *
 * Called with interrupts masked
 */
int
hilevel_intr_epilog(struct cpu *cpu, uint_t pil, uint_t oldpil, uint_t vecnum)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t mask;
	hrtime_t intrtime;
	hrtime_t now = csr_read_time();

	ASSERT(mcpu->mcpu_pri == pil);

	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));

	cpu->cpu_intr_actv &= ~(1 << pil);
	ASSERT(mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)] != 0);

	intrtime = now - mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)];
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;

	/*
	 * Check for lower-pil nested high-level interrupt beneath
	 * current one.  If so, place a starting timestamp in its
	 * pil_high_start entry.
	 */
	mask = cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK;
	if (mask != 0) {
		int nestpil;

		/*
		 * find PIL of nested interrupt
		 */
		nestpil = bsrw_insn((uint16_t)mask);
		ASSERT(nestpil < pil);
		mcpu->pil_high_start[nestpil - (LOCK_LEVEL + 1)] = now;
		/*
		 * (Another high-level interrupt is active below this one,
		 * so there is no need to check for an interrupt
		 * thread.  That will be done by the lowest priority
		 * high-level interrupt active.)
		 */
	} else {
		/*
		 * Check to see if there is a low-level interrupt active.
		 * If so, place a starting timestamp in the thread
		 * structure.
		 */
		kthread_t *t = cpu->cpu_thread;

		if (t->t_flag & T_INTR_THREAD)
			t->t_intr_start = now;
	}

	if ((mask = cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK) == 0) {
		cpu->cpu_intr_actv &= ~CPU_INTR_ACTV_INTR_STACK_MASK;
	}

	mcpu->mcpu_pri = oldpil;
	setlvlx(oldpil, vecnum);

	return (int)mask;
}

/*
 * Set up the cpu, thread and interrupt thread structures for
 * executing an interrupt thread.  The new stack pointer of the
 * interrupt thread (which *must* be switched to) is returned.
 */
caddr_t
intr_thread_prolog(struct cpu *cpu, caddr_t stackptr, uint_t pil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t, *volatile it;
	hrtime_t now = csr_read_time();

	ASSERT(pil > 0);
	ASSERT((cpu->cpu_intr_actv & (1 << pil)) == 0);
	cpu->cpu_intr_actv |= (1 << pil);

	/*
	 * Get set to run an interrupt thread.
	 * There should always be an interrupt thread, since we
	 * allocate one for each level on each CPU.
	 *
	 * t_intr_start could be zero due to cpu_intr_swtch_enter.
	 */
	t = cpu->cpu_thread;
	if ((t->t_flag & T_INTR_THREAD) && t->t_intr_start != 0) {
		hrtime_t intrtime = now - t->t_intr_start;
		mcpu->intrstat[t->t_pil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		t->t_intr_start = 0;
	}

	ASSERT(SA((uintptr_t)stackptr) == (uintptr_t)stackptr);

	t->t_sp = (uintptr_t)stackptr;	/* mark stack in curthread for resume */

	/*
	 * unlink the interrupt thread off the cpu
	 *
	 * Note that the code in kcpc_overflow_intr -relies- on the
	 * ordering of events here - in particular that t->t_lwp of
	 * the interrupt thread is set to the pinned thread *before*
	 * curthread is changed.
	 */
	it = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it->t_link;
	it->t_intr = t;
	it->t_lwp = t->t_lwp;

	/*
	 * (threads on the interrupt thread free list could have state
	 * preset to TS_ONPROC, but it helps in debugging if
	 * they're TS_FREE.)
	 */
	it->t_state = TS_ONPROC;

	asm volatile ("mv tp, %0"::"r"(it):"tp");
	cpu->cpu_thread = it;		/* new curthread on this cpu */
	it->t_pil = (uchar_t)pil;
	it->t_pri = intr_pri + (pri_t)pil;
	it->t_intr_start = now;

	return (it->t_stk);
}


#ifdef DEBUG
int intr_thread_cnt;
#endif

/*
 * Called with interrupts disabled
 */
void
intr_thread_epilog(struct cpu *cpu, uint_t vec, uint_t oldpil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t;
	kthread_t *it = cpu->cpu_thread;	/* curthread */
	uint_t pil, basespl;
	hrtime_t intrtime;
	hrtime_t now = csr_read_time();

	pil = it->t_pil;
	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(it->t_intr_start != 0);
	intrtime = now - it->t_intr_start;
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));
	cpu->cpu_intr_actv &= ~(1 << pil);

	/*
	 * If there is still an interrupted thread underneath this one
	 * then the interrupt was never blocked and the return is
	 * fairly simple.  Otherwise it isn't.
	 */
	if ((t = it->t_intr) == NULL) {
		/*
		 * The interrupted thread is no longer pinned underneath
		 * the interrupt thread.  This means the interrupt must
		 * have blocked, and the interrupted thread has been
		 * unpinned, and has probably been running around the
		 * system for a while.
		 *
		 * Since there is no longer a thread under this one, put
		 * this interrupt thread back on the CPU's free list and
		 * resume the idle thread which will dispatch the next
		 * thread to run.
		 */
#ifdef DEBUG
		intr_thread_cnt++;
#endif
		cpu->cpu_stats.sys.intrblk++;
		/*
		 * Set CPU's base SPL based on active interrupts bitmask
		 */
		set_base_spl();
		basespl = cpu->cpu_base_spl;
		mcpu->mcpu_pri = basespl;
		setlvlx(basespl, vec);
		(void) splhigh();
		csr_set_sstatus(SSR_SIE);

		it->t_state = TS_FREE;
		/*
		 * Return interrupt thread to pool
		 */
		it->t_link = cpu->cpu_intr_thread;
		cpu->cpu_intr_thread = it;
		swtch();
		panic("intr_thread_epilog: swtch returned");
		/*NOTREACHED*/
	}

	/*
	 * Return interrupt thread to the pool
	 */
	it->t_link = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it;
	it->t_state = TS_FREE;

	basespl = cpu->cpu_base_spl;
	pil = MAX(oldpil, basespl);
	mcpu->mcpu_pri = pil;
	setlvlx(pil, vec);
	t->t_intr_start = now;
	asm volatile ("mv tp, %0"::"r"(t):"tp");
	cpu->cpu_thread = t;
}

/*
 * intr_get_time() is a resource for interrupt handlers to determine how
 * much time has been spent handling the current interrupt. Such a function
 * is needed because higher level interrupts can arrive during the
 * processing of an interrupt.  intr_get_time() only returns time spent in the
 * current interrupt handler.
 *
 * The caller must be calling from an interrupt handler running at a pil
 * below or at lock level. Timings are not provided for high-level
 * interrupts.
 *
 * The first time intr_get_time() is called while handling an interrupt,
 * it returns the time since the interrupt handler was invoked. Subsequent
 * calls will return the time since the prior call to intr_get_time(). Time
 * is returned as ticks. Use scalehrtimef() to convert ticks to nsec.
 *
 * Theory Of Intrstat[][]:
 *
 * uint64_t intrstat[pil][0..1] is an array indexed by pil level, with two
 * uint64_ts per pil.
 *
 * intrstat[pil][0] is a cumulative count of the number of ticks spent
 * handling all interrupts at the specified pil on this CPU. It is
 * exported via kstats to the user.
 *
 * intrstat[pil][1] is always a count of ticks less than or equal to the
 * value in [0]. The difference between [1] and [0] is the value returned
 * by a call to intr_get_time(). At the start of interrupt processing,
 * [0] and [1] will be equal (or nearly so). As the interrupt consumes
 * time, [0] will increase, but [1] will remain the same. A call to
 * intr_get_time() will return the difference, then update [1] to be the
 * same as [0]. Future calls will return the time since the last call.
 * Finally, when the interrupt completes, [1] is updated to the same as [0].
 *
 * Implementation:
 *
 * intr_get_time() works much like a higher level interrupt arriving. It
 * "checkpoints" the timing information by incrementing intrstat[pil][0]
 * to include elapsed running time, and by setting t_intr_start to rdtsc.
 * It then sets the return value to intrstat[pil][0] - intrstat[pil][1],
 * and updates intrstat[pil][1] to be the same as the new value of
 * intrstat[pil][0].
 *
 * In the normal handling of interrupts, after an interrupt handler returns
 * and the code in intr_thread() updates intrstat[pil][0], it then sets
 * intrstat[pil][1] to the new value of intrstat[pil][0]. When [0] == [1],
 * the timings are reset, i.e. intr_get_time() will return [0] - [1] which
 * is 0.
 *
 * Whenever interrupts arrive on a CPU which is handling a lower pil
 * interrupt, they update the lower pil's [0] to show time spent in the
 * handler that they've interrupted. This results in a growing discrepancy
 * between [0] and [1], which is returned the next time intr_get_time() is
 * called. Time spent in the higher-pil interrupt will not be returned in
 * the next intr_get_time() call from the original interrupt, because
 * the higher-pil interrupt's time is accumulated in intrstat[higherpil][].
 */
uint64_t
intr_get_time(void)
{
	struct cpu *cpu;
	struct machcpu *mcpu;
	kthread_t *t;
	uint64_t time, delta, ret;
	uint_t pil;

	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	cpu = CPU;
	mcpu = &cpu->cpu_m;
	t = cpu->cpu_thread;
	pil = t->t_pil;
	ASSERT((cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK) == 0);
	ASSERT(t->t_flag & T_INTR_THREAD);
	ASSERT(pil != 0);
	ASSERT(t->t_intr_start != 0);

	time = csr_read_time();
	delta = time - t->t_intr_start;
	t->t_intr_start = time;

	time = mcpu->intrstat[pil][0] + delta;
	ret = time - mcpu->intrstat[pil][1];
	mcpu->intrstat[pil][0] = time;
	mcpu->intrstat[pil][1] = time;
	cpu->cpu_intracct[cpu->cpu_mstate] += delta;

	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
	return (ret);
}

caddr_t
dosoftint_prolog(
	struct cpu *cpu,
	caddr_t stackptr,
	uint32_t st_pending,
	uint_t oldpil)
{
	kthread_t *t, *volatile it;
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t pil;
	hrtime_t now;

	ASSERT(st_pending == mcpu->mcpu_softinfo.st_pending);
	ASSERT((csr_read_sstatus() & SSR_SIE) == 0);

	pil = bsrw_insn((uint16_t)st_pending);
	if (pil <= oldpil || pil <= cpu->cpu_base_spl)
		return (0);
	if ((cpu->cpu_intr_actv & ~((1u << pil) - 1)) != 0)
		return (0);

	atomic_and_32((uint32_t *)&mcpu->mcpu_softinfo.st_pending, ~(1 << pil));

	ASSERT((cpu->cpu_intr_actv & (1 << pil)) == 0);
	mcpu->mcpu_pri = pil;
	setlvlx(pil, -1);

	now = csr_read_time();

	/*
	 * Get set to run interrupt thread.
	 * There should always be an interrupt thread since we
	 * allocate one for each level on the CPU.
	 */
	it = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it->t_link;

	/* t_intr_start could be zero due to cpu_intr_swtch_enter. */
	t = cpu->cpu_thread;
	if ((t->t_flag & T_INTR_THREAD) && t->t_intr_start != 0) {
		hrtime_t intrtime = now - t->t_intr_start;
		mcpu->intrstat[pil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		t->t_intr_start = 0;
	}

	/*
	 * Note that the code in kcpc_overflow_intr -relies- on the
	 * ordering of events here - in particular that t->t_lwp of
	 * the interrupt thread is set to the pinned thread *before*
	 * curthread is changed.
	 */
	it->t_lwp = t->t_lwp;
	it->t_state = TS_ONPROC;

	/*
	 * Push interrupted thread onto list from new thread.
	 * Set the new thread as the current one.
	 * Set interrupted thread's T_SP because if it is the idle thread,
	 * resume() may use that stack between threads.
	 */

	ASSERT(SA((uintptr_t)stackptr) == (uintptr_t)stackptr);
	t->t_sp = (uintptr_t)stackptr;

	it->t_intr = t;
	asm volatile ("mv tp, %0"::"r"(it):"tp");
	cpu->cpu_thread = it;

	/*
	 * Set bit for this pil in CPU's interrupt active bitmask.
	 */
	ASSERT((cpu->cpu_intr_actv & (1 << pil)) == 0);
	cpu->cpu_intr_actv |= (1 << pil);

	/*
	 * Initialize thread priority level from intr_pri
	 */
	it->t_pil = (uchar_t)pil;
	it->t_pri = (pri_t)pil + intr_pri;
	it->t_intr_start = now;

	return (it->t_stk);
}

void
dosoftint_epilog(struct cpu *cpu, uint_t oldpil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t, *it;
	uint_t pil, basespl;
	hrtime_t intrtime;
	hrtime_t now = csr_read_time();

	it = cpu->cpu_thread;
	pil = it->t_pil;

	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));
	cpu->cpu_intr_actv &= ~(1 << pil);
	intrtime = now - it->t_intr_start;
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;

	/*
	 * If there is still an interrupted thread underneath this one
	 * then the interrupt was never blocked and the return is
	 * fairly simple.  Otherwise it isn't.
	 */
	if ((t = it->t_intr) == NULL) {
		/*
		 * Put thread back on the interrupt thread list.
		 * This was an interrupt thread, so set CPU's base SPL.
		 */
		set_base_spl();
		it->t_state = TS_FREE;
		it->t_link = cpu->cpu_intr_thread;
		cpu->cpu_intr_thread = it;
		(void) splhigh();
		csr_set_sstatus(SSR_SIE);
		swtch();
		/*NOTREACHED*/
		panic("dosoftint_epilog: swtch returned");
	}
	it->t_link = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it;
	it->t_state = TS_FREE;
	asm volatile ("mv tp, %0"::"r"(t):"tp");
	cpu->cpu_thread = t;
	if (t->t_flag & T_INTR_THREAD)
		t->t_intr_start = now;
	basespl = cpu->cpu_base_spl;
	pil = MAX(oldpil, basespl);
	mcpu->mcpu_pri = pil;
	setlvlx(pil, -1);
}


/*
 * Make the interrupted thread 'to' be runnable.
 *
 * Since t->t_sp has already been saved, t->t_pc is all
 * that needs to be set in this function.
 *
 * Returns the interrupt level of the interrupt thread.
 */
int
intr_passivate(
	kthread_t *it,		/* interrupt thread */
	kthread_t *t)		/* interrupted thread */
{
	extern void _sys_rtt();

	ASSERT(it->t_flag & T_INTR_THREAD);
	ASSERT(SA(t->t_sp) == t->t_sp);

	t->t_pc = (uintptr_t)_sys_rtt;
	return (it->t_pil);
}

/*
 * Create interrupt kstats for this CPU.
 */
void
cpu_create_intrstat(cpu_t *cp)
{
	int		i;
	kstat_t		*intr_ksp;
	kstat_named_t	*knp;
	char		name[KSTAT_STRLEN];
	zoneid_t	zoneid;

	ASSERT(MUTEX_HELD(&cpu_lock));

	if (pool_pset_enabled())
		zoneid = GLOBAL_ZONEID;
	else
		zoneid = ALL_ZONES;

	intr_ksp = kstat_create_zone("cpu", cp->cpu_id, "intrstat", "misc",
	    KSTAT_TYPE_NAMED, PIL_MAX * 2, 0, zoneid);

	/*
	 * Initialize each PIL's named kstat
	 */
	if (intr_ksp != NULL) {
		intr_ksp->ks_update = cpu_kstat_intrstat_update;
		knp = (kstat_named_t *)intr_ksp->ks_data;
		intr_ksp->ks_private = cp;
		for (i = 0; i < PIL_MAX; i++) {
			(void) snprintf(name, KSTAT_STRLEN, "level-%d-time",
			    i + 1);
			kstat_named_init(&knp[i * 2], name, KSTAT_DATA_UINT64);
			(void) snprintf(name, KSTAT_STRLEN, "level-%d-count",
			    i + 1);
			kstat_named_init(&knp[(i * 2) + 1], name,
			    KSTAT_DATA_UINT64);
		}
		kstat_install(intr_ksp);
	}
}

/*
 * Delete interrupt kstats for this CPU.
 */
void
cpu_delete_intrstat(cpu_t *cp)
{
	kstat_delete_byname_zone("cpu", cp->cpu_id, "intrstat", ALL_ZONES);
}

/*
 * Convert interrupt statistics from CPU ticks to nanoseconds and
 * update kstat.
 */
int
cpu_kstat_intrstat_update(kstat_t *ksp, int rw)
{
	kstat_named_t	*knp = ksp->ks_data;
	cpu_t		*cpup = (cpu_t *)ksp->ks_private;
	int		i;
	hrtime_t	hrt;

	if (rw == KSTAT_WRITE)
		return (EACCES);

	for (i = 0; i < PIL_MAX; i++) {
		hrt = (hrtime_t)cpup->cpu_m.intrstat[i + 1][0];
		scalehrtime(&hrt);
		knp[i * 2].value.ui64 = (uint64_t)hrt;
		knp[(i * 2) + 1].value.ui64 = cpup->cpu_stats.sys.intr[i];
	}

	return (0);
}

/*
 * An interrupt thread is ending a time slice, so compute the interval it
 * ran for and update the statistic for its PIL.
 */
void
cpu_intr_swtch_enter(kthread_id_t t)
{
	uint64_t	interval;
	uint64_t	start;
	cpu_t		*cpu;

	ASSERT((t->t_flag & T_INTR_THREAD) != 0);
	ASSERT(t->t_pil > 0 && t->t_pil <= LOCK_LEVEL);

	/*
	 * We could be here with a zero timestamp. This could happen if:
	 * an interrupt thread which no longer has a pinned thread underneath
	 * it (i.e. it blocked at some point in its past) has finished running
	 * its handler. intr_thread() updated the interrupt statistic for its
	 * PIL and zeroed its timestamp. Since there was no pinned thread to
	 * return to, swtch() gets called and we end up here.
	 *
	 * Note that we use atomic ops below (atomic_cas_64 and
	 * atomic_add_64), which we don't use in the functions above,
	 * because we're not called with interrupts blocked, but the
	 * epilog/prolog functions are.
	 */
	if (t->t_intr_start) {
		do {
			start = t->t_intr_start;
			interval = csr_read_time() - start;
		} while (atomic_cas_64(&t->t_intr_start, start, 0) != start);
		cpu = CPU;
		cpu->cpu_m.intrstat[t->t_pil][0] += interval;

		atomic_add_64((uint64_t *)&cpu->cpu_intracct[cpu->cpu_mstate],
		    interval);
	} else
		ASSERT(t->t_intr == NULL);
}

/*
 * An interrupt thread is returning from swtch(). Place a starting timestamp
 * in its thread structure.
 */
void
cpu_intr_swtch_exit(kthread_id_t t)
{
	uint64_t ts;

	ASSERT((t->t_flag & T_INTR_THREAD) != 0);
	ASSERT(t->t_pil > 0 && t->t_pil <= LOCK_LEVEL);

	do {
		ts = t->t_intr_start;
	} while (atomic_cas_64(&t->t_intr_start, ts, csr_read_time()) != ts);
}

int
getpil(void)
{
	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	int pil = CPU->cpu_m.mcpu_pri;
	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
	return pil;
}

static int
do_splx(int newpri)
{
	uint64_t old = csr_read_clear_sstatus(SSR_SIE);

	cpu_t *cpu = CPU;
	int curpri = cpu->cpu_m.mcpu_pri;
	int basepri = cpu->cpu_base_spl;
	if (newpri < basepri)
		newpri = basepri;
	cpu->cpu_m.mcpu_pri = newpri;
	setlvlx(newpri, -1);

	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
	return curpri;
}

void
splx(int newpri)
{
	do_splx(newpri);
}

int
splr(int newpri)
{
	uint64_t old = csr_read_clear_sstatus(SSR_SIE);

	cpu_t *cpu = CPU;
	int curpri = cpu->cpu_m.mcpu_pri;
	if (newpri > curpri) {
		int basepri = cpu->cpu_base_spl;
		if (newpri < basepri)
			newpri = basepri;
		cpu->cpu_m.mcpu_pri = newpri;
		setlvlx(newpri, -1);
	}

	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
	return curpri;
}

int splhigh(void)
{
	return splr(DISP_LEVEL);
}
int splhi(void)
{
	return splr(DISP_LEVEL);
}
int spl6(void)
{
	return splr(DISP_LEVEL);
}
int spl0(void)
{
	return do_splx(0);
}
int splzs(void)
{
	return do_splx(12);
}
int spl7(void)
{
	return splr(13);
}
int spl8(void)
{
	return splr(15);
}
int
interrupts_enabled(void)
{
	return (csr_read_sstatus() & SSR_SIE) != 0;
}

int
spl_xcall(void)
{
	return (splr(ipltospl(XCALL_PIL)));
}

static uint32_t ipi_pending[NCPU];
static cpuset_t ipi_cpuset;
void
send_dirint(int cpuid, int irq)
{
	uint64_t old = csr_read_clear_sstatus(SSR_SIE);

	__sync_fetch_and_or(&ipi_pending[cpuid], 1ul << irq);

	sbi_send_ipi(&cpu[cpuid]->cpu_hartmask);

	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
}

void
send_ipi(cpuset_t cpuset, uint32_t irq)
{
	uint64_t target = 0;
	CPUSET_AND(cpuset, ipi_cpuset);
	while (!CPUSET_ISNULL(cpuset)) {
		uint_t cpuid;
		CPUSET_FIND(cpuset, cpuid);
		target |= cpu[cpuid]->cpu_hartmask;
		CPUSET_DEL(cpuset, cpuid);
		__sync_fetch_and_or(&ipi_pending[cpuid], 1ul << irq);
	}

	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	sbi_send_ipi(&target);
	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
}

static uint_t
ipi_handler(caddr_t arg0, caddr_t arg1)
{
	csr_clear_sip(SIE_SSIE);

	int id = CPU->cpu_id;
	uint32_t ipi = __sync_lock_test_and_set(&ipi_pending[id], 0);
	if (ipi == 0)
		return (DDI_INTR_UNCLAIMED);

	if (ipi & (1u << IRQ_IPI_HI))
		xc_serv();

	if (ipi & (1u << IRQ_IPI_CBE))
		cbe_fire_slave();

	if (ipi & (1u << IRQ_IPI_CPUPOKE)) {
		// do nothing
	}

	if (ipi & (1u << IRQ_IPI_STOP)) {
		for (;;) {
			asm volatile ("wfi":::"memory");
		}
	}

	return (DDI_INTR_CLAIMED);
}

static int plic_id_table[NCPU];
static uint32_t plic_max_lvl = 1;
static uint32_t plic_ndev = 0;

uint32_t plic_get_priority(int irq)
{
	ASSERT(0 < irq && irq < 512);
	return *(volatile uint32_t *)(SEGKPM_BASE + 0x0C000000 + irq * 4);
}
void plic_set_priority(int irq, uint32_t lvl)
{
	ASSERT(0 < irq && irq < 512);
	ASSERT(lvl <= plic_max_lvl);
	*(volatile uint32_t *)(SEGKPM_BASE + 0x0C000000 + irq * 4) = lvl;
}
bool plic_is_pending(int irq)
{
	ASSERT(0 < irq && irq < 512);
	return (*(volatile uint32_t *)(SEGKPM_BASE + 0x0C001000 + (irq / 32) * 4) & (1u << (irq % 32))) != 0;
}

static volatile int plic_exclusion;
void plic_enable(int irq, int cpuid)
{
	ASSERT(0 < irq && irq < 512);
	int plic_id = plic_id_table[cpuid];
	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	while (__sync_lock_test_and_set(&plic_exclusion, 1)) {}

	*(volatile uint32_t *)(SEGKPM_BASE + 0x0C002000 + 0x80 * plic_id + (irq / 32) * 4) |= (1u << (irq % 32));

	__sync_lock_release(&plic_exclusion);
	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
}
void plic_disable(int irq, int cpuid)
{
	ASSERT(0 < irq && irq < 512);
	int plic_id = plic_id_table[cpuid];
	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	while (__sync_lock_test_and_set(&plic_exclusion, 1)) {}

	*(volatile uint32_t *)(SEGKPM_BASE + 0x0C002000 + 0x80 * plic_id + (irq / 32) * 4) &= ~(1u << (irq % 32));

	__sync_lock_release(&plic_exclusion);
	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
}
uint32_t plic_get_level(void)
{
	int plic_id = plic_id_table[CPU->cpu_id];
	return *(volatile uint32_t *)(SEGKPM_BASE + 0x0C200000 + 0x1000 * plic_id);
}
void plic_set_level(uint32_t lvl)
{
	int plic_id = plic_id_table[CPU->cpu_id];
	*(volatile uint32_t *)(SEGKPM_BASE + 0x0C200000 + 0x1000 * plic_id) = lvl;
}
int plic_get_irq(void)
{
	int plic_id = plic_id_table[CPU->cpu_id];
	return *(volatile uint32_t *)(SEGKPM_BASE + 0x0C200004 + 0x1000 * plic_id);
}
void plic_eol(int irq)
{
	int plic_id = plic_id_table[CPU->cpu_id];
	*(volatile uint32_t *)(SEGKPM_BASE + 0x0C200004 + 0x1000 * plic_id) = irq;
}

static int
plic_addspl(int irq, int ipl, int min_ipl, int max_ipl)
{
	if (irq >= 0x10) {
		plic_set_priority(irq - 0x10, ipl >= plic_max_lvl? plic_max_lvl: ipl);
		plic_enable(irq - 0x10, CPU->cpu_id);
	}

	return 0;
}

static int
plic_delspl(int irq, int ipl, int min_ipl, int max_ipl)
{
	if (irq >= 0x10) {
		if (autovect[irq].avh_hi_pri == 0) {
			plic_disable(irq - 0x10, CPU->cpu_id);
			plic_set_priority(irq - 0x10, 0);
		}
	}

	return 0;
}

int (*addspl)(int, int, int, int) = plic_addspl;
int (*delspl)(int, int, int, int) = plic_delspl;

int get_irq(uint64_t scause)
{
	scause &= ((1ul << 63) - 1);
	switch (scause) {
	case 1: return 1;
	case 5: return 5;
	case 9: break;
	default:
		prom_printf("%s: unknown scause %lx\n", __func__, scause);
		return -1;
	}
	int irq = plic_get_irq();
	if (irq == 0)
		return -1;
	return irq + 0x10;
}

void mask_irq(int irq)
{
	if (irq > 0x10) {
		plic_set_priority(irq - 0x10, 0);
		plic_eol(irq - 0x10);
	}
}
void unmask_irq(int irq)
{
	if (irq > 0x10) {
		int ipl = autovect[irq].avh_hi_pri;
		plic_set_priority(irq - 0x10, ipl >= plic_max_lvl? plic_max_lvl: ipl);
	}
}

int
setlvl(int irq)
{
	ASSERT((csr_read_sstatus() & SSR_SIE) == 0);
	int ipl = autovect[irq].avh_hi_pri;
	int curpri = CPU->cpu_m.mcpu_pri;

	if (ipl != 0) {
		ASSERT(curpri < ipl);

		ASSERT(irq != 1 || (csr_read_sie() & SIE_SSIE) != 0);
		ASSERT(irq != 5 || (csr_read_sie() & SIE_STIE) != 0);
		if (ipl >= autovect[1].avh_hi_pri) {
			csr_clear_sie(SIE_SSIE);
		} else {
			csr_set_sie(SIE_SSIE);
		}
		if (ipl >= autovect[5].avh_hi_pri) {
			csr_clear_sie(SIE_STIE);
		} else {
			csr_set_sie(SIE_STIE);
		}
		plic_set_level(ipl >= plic_max_lvl? plic_max_lvl: ipl);
		csr_clear_sie(SIE_SEIE);
	}

	return ipl;
}

void
setlvlx(int ipl, int irq)
{
	ASSERT((csr_read_sstatus() & SSR_SIE) == 0);
	unmask_irq(irq);

	if (ipl >= autovect[1].avh_hi_pri) {
		csr_clear_sie(SIE_SSIE);
	} else {
		csr_set_sie(SIE_SSIE);
	}
	if (ipl >= autovect[5].avh_hi_pri) {
		csr_clear_sie(SIE_STIE);
	} else {
		csr_set_sie(SIE_STIE);
	}
	plic_set_level(ipl >= plic_max_lvl? plic_max_lvl: ipl);
	if (ipl > 0) {
		csr_clear_sie(SIE_SEIE);
	} else {
		csr_set_sie(SIE_SEIE);
	}
}

static pnode_t
find_compatible(pnode_t node, const char *compatible)
{
	if (prom_is_compatible(node, compatible)) {
		return node;
	}

	pnode_t child = prom_childnode(node);
	while (child > 0) {
		pnode_t n = find_compatible(child, compatible);
		if (n > 0)
			return n;
		child = prom_nextnode(child);
	}
	return -1;
}

static void
plic_map(void)
{
	pnode_t ic = find_compatible(prom_rootnode(), "riscv,plic0");
	if (ic < 0) {
		ic = find_compatible(prom_rootnode(), "sifive,plic-1.0.0");
	}
	ASSERT(ic >= 0);
	int len = prom_getproplen(ic, "interrupts-extended");
	if (len >= 0) {
		uint32_t *extended = __builtin_alloca(len);
		prom_getprop(ic, "interrupts-extended", (caddr_t)extended);
		for (int i = 0; i < len / (sizeof(uint32_t) * 2); i++) {
			pnode_t parent = prom_findnode_by_phandle(htonl(extended[2 * i]));
			int arg = (int)extended[2 * i + 1];
			ASSERT(parent >= 0);
			ASSERT(prom_is_compatible(parent, "riscv,cpu-intc"));
			pnode_t pcpu = prom_parentnode(parent);
			ASSERT(pcpu >= 0);
			ASSERT(prom_getproplen(pcpu, "reg") > 0);
			int hartid;
			ASSERT(prom_get_address_cells(pcpu) >= 0);
			prom_getprop(pcpu, "reg", (caddr_t)&hartid);
			hartid = ntohl(hartid);
			if (arg < 0) {
				//prom_printf("%s: index %d is not used. hart%d\n",__func__,i,hartid);
				continue;
			}
			//prom_printf("%s: plic index %d for hart%d\n",__func__,i,hartid);
			if (CPU->cpu_hartmask == (1ul << hartid)) {
				plic_id_table[CPU->cpu_id] = i;
			}
		}
	}
}
void intr_slave_init(void)
{
	plic_map();
	plic_set_level(plic_max_lvl);
	for (int i = 1; i < plic_ndev; i++) {
		plic_disable(i, CPU->cpu_id);
	}
	csr_set_sie(SIE_SEIE);

	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	while (__sync_lock_test_and_set(&plic_exclusion, 1)) {}
	CPUSET_ADD(ipi_cpuset, CPU->cpu_id);
	__sync_lock_release(&plic_exclusion);
	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
}
void intr_init(void)
{
	pnode_t ic = find_compatible(prom_rootnode(), "riscv,plic0");
	if (ic < 0) {
		ic = find_compatible(prom_rootnode(), "sifive,plic-1.0.0");
	}
	ASSERT(ic >= 0);
	int len = prom_getproplen(ic, "riscv,ndev");
	if (len >= 0) {
		prom_getprop(ic, "riscv,ndev", (caddr_t)&plic_ndev);
		plic_ndev = ntohl(plic_ndev);
	}
	len = prom_getproplen(ic, "riscv,max-priority");
	if (len >= 0) {
		prom_getprop(ic, "riscv,max-priority", (caddr_t)&plic_max_lvl);
		plic_max_lvl = ntohl(plic_max_lvl);
	}

	//prom_printf("%s:%d riscv,max-priority=%d\n",__func__,__LINE__,plic_max_lvl);
	//prom_printf("%s:%d riscv,ndev=%d\n",__func__,__LINE__,plic_ndev);

	for (int i = 1; i < plic_ndev; i++) {
		plic_set_priority(i, 0);
	}

	plic_map();
	plic_set_level(plic_max_lvl);
	for (int i = 1; i < plic_ndev; i++) {
		plic_disable(i, CPU->cpu_id);
	}
	csr_set_sie(SIE_SEIE);

	uint64_t old = csr_read_clear_sstatus(SSR_SIE);
	while (__sync_lock_test_and_set(&plic_exclusion, 1)) {}
	CPUSET_ADD(ipi_cpuset, CPU->cpu_id);
	__sync_lock_release(&plic_exclusion);
	if (old & SSR_SIE)
		csr_set_sstatus(SSR_SIE);
}

void ipi_handler_init(void)
{
	add_avintr((void *)NULL, XC_HI_PIL, ipi_handler, "ipi_handler", IRQ_IPI_HI, NULL, NULL, NULL, NULL);
}
