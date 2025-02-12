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
#include <vm/hat_alpha.h>
#include <sys/pal.h>
#include <sys/spl.h>
#include <sys/irq.h>
#include <sys/xc_levels.h>

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
static int
hilevel_intr_prolog(struct cpu *cpu, uint_t pil, uint_t oldpil, struct regs *rp)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t mask;
	uint32_t intrtime;
	uint32_t now = tsc_read();

	ASSERT(pil > LOCK_LEVEL);

	if (pil == CBE_HIGH_PIL) {
		cpu->cpu_profile_pil = oldpil;
		if (USERMODE(rp->r_ps)) {
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
		ASSERT(nestpil < pil);
		intrtime = now -
		    mcpu->pil_high_start[nestpil - (LOCK_LEVEL + 1)];
		mcpu->intrstat[nestpil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		ASSERT(intrtime > 0);
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
			ASSERT(intrtime > 0);
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

	if (pil == 15) {
		/*
		 * To support reentrant level 15 interrupts, we maintain a
		 * recursion count in the top half of cpu_intr_actv.  Only
		 * when this count hits zero do we clear the PIL 15 bit from
		 * the lower half of cpu_intr_actv.
		 */
		uint16_t *refcntp = (uint16_t *)&cpu->cpu_intr_actv + 1;
		(*refcntp)++;
	}

	mask = cpu->cpu_intr_actv;

	cpu->cpu_intr_actv |= (1 << pil);

	return (mask & CPU_INTR_ACTV_HIGH_LEVEL_MASK);
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
static int
hilevel_intr_epilog(struct cpu *cpu, uint_t pil, uint_t oldpil, uint_t vecnum)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t mask;
	uint32_t intrtime;
	uint32_t now = tsc_read();

	ASSERT(mcpu->mcpu_pri == pil);

	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));

	if (pil == 15) {
		/*
		 * To support reentrant level 15 interrupts, we maintain a
		 * recursion count in the top half of cpu_intr_actv.  Only
		 * when this count hits zero do we clear the PIL 15 bit from
		 * the lower half of cpu_intr_actv.
		 */
		uint16_t *refcntp = (uint16_t *)&cpu->cpu_intr_actv + 1;

		ASSERT(*refcntp > 0);

		if (--(*refcntp) == 0)
			cpu->cpu_intr_actv &= ~(1 << pil);
	} else {
		cpu->cpu_intr_actv &= ~(1 << pil);
	}

	ASSERT(mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)] != 0);

	intrtime = now - mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)];
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
	ASSERT(intrtime > 0);

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

	mcpu->mcpu_pri = oldpil;
	(void) (*setlvlx)(oldpil, vecnum);

	return (cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK);
}

/*
 * Set up the cpu, thread and interrupt thread structures for
 * executing an interrupt thread.  The new stack pointer of the
 * interrupt thread (which *must* be switched to) is returned.
 */
static caddr_t
intr_thread_prolog(struct cpu *cpu, caddr_t stackptr, uint_t pil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t, *volatile it;
	uint32_t now = tsc_read();

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
		uint32_t intrtime = now - t->t_intr_start;
		mcpu->intrstat[t->t_pil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		t->t_intr_start = 0;
		ASSERT(intrtime > 0);
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
static void
intr_thread_epilog(struct cpu *cpu, uint_t vec, uint_t oldpil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t;
	kthread_t *it = cpu->cpu_thread;	/* curthread */
	uint_t pil, basespl;
	uint32_t intrtime;
	uint32_t now = tsc_read();

	pil = it->t_pil;
	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(it->t_intr_start != 0);
	intrtime = now - it->t_intr_start;
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
	ASSERT(intrtime > 0);

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
		(*setlvlx)(basespl, vec);
		(void) splhigh();
		it->t_state = TS_FREE;
		/*
		 * Return interrupt thread to pool
		 */
		it->t_link = cpu->cpu_intr_thread;
		cpu->cpu_intr_thread = it;
		pal_swpipl(0);
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
	(*setlvlx)(pil, vec);
	t->t_intr_start = now;
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
	uint32_t time, ret, delta;
	uint_t pil;
	uint64_t flag;

	flag = pal_swpipl(6);
	cpu = CPU;
	mcpu = &cpu->cpu_m;
	t = cpu->cpu_thread;
	pil = t->t_pil;
	ASSERT((cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK) == 0);
	ASSERT(t->t_flag & T_INTR_THREAD);
	ASSERT(pil != 0);
	ASSERT(t->t_intr_start != 0);

	time = tsc_read();
	delta = time - t->t_intr_start;
	t->t_intr_start = time;
	ASSERT(delta > 0);

	time = mcpu->intrstat[pil][0] + delta;
	ret = time - mcpu->intrstat[pil][1];
	mcpu->intrstat[pil][0] = time;
	mcpu->intrstat[pil][1] = time;
	cpu->cpu_intracct[cpu->cpu_mstate] += delta;
	ASSERT(delta > 0);

	pal_swpipl(flag);
	return (ret);
}

static caddr_t
dosoftint_prolog(
	struct cpu *cpu,
	caddr_t stackptr,
	uint_t oldpil)
{
	kthread_t *t, *volatile it;
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t pil;
	uint32_t now;
	uint32_t st_pending;

	st_pending = mcpu->mcpu_softinfo.st_pending;
top:
	pil = bsrw_insn((uint16_t)st_pending);
	if (pil <= oldpil || pil <= cpu->cpu_base_spl)
		return (0);

	if (atomic_cas_32((uint32_t *)&mcpu->mcpu_softinfo.st_pending,
		    st_pending, st_pending & ~(1<<pil)) != st_pending) {
		st_pending = mcpu->mcpu_softinfo.st_pending;
		goto top;
	}

	mcpu->mcpu_pri = pil;
	(*setspl)(pil);

	now = tsc_read();

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
		uint32_t intrtime = now - t->t_intr_start;
		mcpu->intrstat[pil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		t->t_intr_start = 0;
		ASSERT(intrtime > 0);
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

static void
dosoftint_epilog(struct cpu *cpu, uint_t oldpil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t, *it;
	uint_t pil, basespl;
	uint32_t intrtime;
	uint32_t now = tsc_read();

	it = cpu->cpu_thread;
	pil = it->t_pil;

	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));
	cpu->cpu_intr_actv &= ~(1 << pil);
	intrtime = now - it->t_intr_start;
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
	ASSERT(intrtime > 0);

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
		pal_swpipl(0);
		swtch();
		/*NOTREACHED*/
		panic("dosoftint_epilog: swtch returned");
	}
	it->t_link = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it;
	it->t_state = TS_FREE;
	cpu->cpu_thread = t;
	if (t->t_flag & T_INTR_THREAD)
		t->t_intr_start = now;
	basespl = cpu->cpu_base_spl;
	pil = MAX(oldpil, basespl);
	mcpu->mcpu_pri = pil;
	(*setspl)(pil);
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
	uint32_t	interval;
	uint32_t	start;
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
	 * Note that we use atomic ops below (cas64 and atomic_add_64), which
	 * we don't use in the functions above, because we're not called
	 * with interrupts blocked, but the epilog/prolog functions are.
	 */
	if (t->t_intr_start) {
		do {
			start = t->t_intr_start;
			interval = tsc_read() - start;
		} while (cas64(&t->t_intr_start, start, 0) != start);
		cpu = CPU;
		cpu->cpu_m.intrstat[t->t_pil][0] += interval;

		atomic_add_64((uint64_t *)&cpu->cpu_intracct[cpu->cpu_mstate],
		    interval);
		ASSERT(interval > 0);
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
	} while (cas64(&t->t_intr_start, ts, tsc_read()) != ts);
}

/*
 * Dispatch a hilevel interrupt (one above LOCK_LEVEL)
 */
/*ARGSUSED*/
static void
dispatch_hilevel(uint_t vector, uint_t arg2)
{
	struct cpu *cpu = CPU;
	pal_swpipl(0);
	av_dispatch_autovect(vector);
	pal_swpipl(6);
}

/*
 * Dispatch a normal interrupt
 */
static void
dispatch_hardint(uint_t vector, uint_t oldipl)
{
	struct cpu *cpu = CPU;

	pal_swpipl(0);
	av_dispatch_autovect(vector);
	pal_swpipl(6);

	/*
	 * Must run intr_thread_epilog() on the interrupt thread stack, since
	 * there may not be a return from it if the interrupt thread blocked.
	 */
	intr_thread_epilog(cpu, vector, oldipl);
}

/*
 * Dispatch a soft interrupt
 */
/*ARGSUSED*/
static void
dispatch_softint(uint_t oldpil, uint_t arg2)
{
	struct cpu *cpu = CPU;

	pal_swpipl(0);
	av_dispatch_softvect((int)cpu->cpu_thread->t_pil);
	pal_swpipl(6);

	/*
	 * Must run softint_epilog() on the interrupt thread stack, since
	 * there may not be a return from it if the interrupt thread blocked.
	 */
	dosoftint_epilog(cpu, oldpil);
}

/*
 * Deliver any softints the current interrupt priority allows.
 * Called with interrupts disabled.
 */
void
dosoftint(struct regs *regs)
{
	struct cpu *cpu = CPU;
	int oldipl;
	caddr_t newsp;

	while (cpu->cpu_softinfo.st_pending) {
		oldipl = cpu->cpu_pri;
		newsp = dosoftint_prolog(cpu, (caddr_t)regs, oldipl);
		/*
		 * If returned stack pointer is NULL, priority is too high
		 * to run any of the pending softints now.
		 * Break out and they will be run later.
		 */
		if (newsp == NULL)
			break;
		switch_sp_and_call(newsp, dispatch_softint, oldipl, 0);
	}
}

/*
 * Interrupt service routine, called with interrupts disabled.
 */
void
do_interrupt(uint64_t itype, uint64_t ivec, uint64_t logoutarea, struct regs *rp)
{
	struct cpu *cpu = CPU;
	int newipl, oldipl = cpu->cpu_pri;
	caddr_t newsp;
	int irqno;
	int oldflag;

	oldflag = pal_swpipl(6);
#if 0
	if (itype != 1 && !(itype==3 && ivec==0x800)) {
		prom_printf("%s() itype=%ld ivec=%lx pri=%d\n", __FUNCTION__, itype, ivec, oldipl);
		prom_printf("%s() %02x %02x\n", __FUNCTION__, *(uint8_t *)0xFFFFFC8900000021UL, *(uint8_t *)0xFFFFFC89000000a1UL);
	}
	if ((itype==3 && ivec==0x800)) {
		//prom_printf("%s() itype=%ld ivec=%lx pri=%d flag=%d\n", __FUNCTION__, itype, ivec, oldipl, oldflag);
	}
#endif
	if (itype != T_INT_SFT && itype != T_INT_IO)
		prom_printf("%s() itype=%ld ivec=%lx pri=%d flag=%d\n", __FUNCTION__, itype, ivec, oldipl, oldflag);

	cpu_idle_exit(CPU_IDLE_CB_FLAG_INTR);

	++*(uint16_t *)&cpu->cpu_m.mcpu_istamp;

	/*
	 * If it's a softint go do it now.
	 */
	if (itype == T_INT_SFT) {
		dosoftint(rp);
		ASSERT(!interrupts_enabled());
		return;
	} else if (itype == T_INT_IPI) {
		return;
	} else if (itype == T_INT_CLK) {
		return;
	} else {
		irqno = (ivec - 0x800)>>4;
	}

	/*
	 * Raise the interrupt priority.
	 */
	newipl = setlvl(oldipl, irqno);

	cpu->cpu_pri = newipl;
	if (newipl > LOCK_LEVEL) {
		/*
		 * High priority interrupts run on this cpu's interrupt stack.
		 */
		if (hilevel_intr_prolog(cpu, newipl, oldipl, rp) == 0) {
			newsp = cpu->cpu_intr_stack;
			switch_sp_and_call(newsp, dispatch_hilevel, irqno, 0);
		} else { /* already on the interrupt stack */
			dispatch_hilevel(irqno, 0);
		}
		(void) hilevel_intr_epilog(cpu, newipl, oldipl, irqno);
	} else {
		/*
		 * Run this interrupt in a separate thread.
		 */
		newsp = intr_thread_prolog(cpu, (caddr_t)rp, newipl);
		switch_sp_and_call(newsp, dispatch_hardint, irqno, oldipl);
	}
	//ASSERT(oldipl == cpu->cpu_pri);

	/*
	 * Deliver any pending soft interrupts.
	 */
	if (cpu->cpu_softinfo.st_pending)
		dosoftint(rp);
}


/*
 * Common tasks always done by _sys_rtt, called with interrupts disabled.
 * Returns 1 if returning to userland, 0 if returning to system mode.
 */
int
sys_rtt_common(struct regs *rp)
{
	kthread_t *tp;
	extern void mutex_exit_critical_start();
	extern uint64_t mutex_exit_critical_size;
	extern void mutex_owner_running_critical_start();
	extern uint64_t mutex_owner_running_critical_size;

loop:

	/*
	 * Check if returning to user
	 */
	tp = CPU->cpu_thread;
	if (USERMODE(rp->r_ps)) {
		/*
		 * Check if AST pending.
		 */
		if (tp->t_astflag) {
			/*
			 * Let trap() handle the AST
			 */
			//rp->r_trapno = T_AST;
			pal_swpipl(0);
			trap(0, 0, 0, T_AST, rp);
			pal_swpipl(6);
			goto loop;
		}
		return (1);
	}

	/*
	 * Here if we are returning to supervisor mode.
	 * Check for a kernel preemption request.
	 */
	if (CPU->cpu_kprunrun && (rp->r_ps & PSR_PIL)) {
		/*
		 * Do nothing if already in kpreempt
		 */
		if (!tp->t_preempt_lk) {
			tp->t_preempt_lk = 1;
			pal_swpipl(0);
			kpreempt(1); /* asynchronous kpreempt call */
			pal_swpipl(6);
			tp->t_preempt_lk = 0;
		}
	}

	if ((uintptr_t)rp->r_pc - (uintptr_t)mutex_owner_running_critical_start < mutex_owner_running_critical_size) {
		rp->r_pc = (greg_t)mutex_owner_running_critical_start;
	}
	if ((uintptr_t)rp->r_pc - (uintptr_t)mutex_exit_critical_start < mutex_exit_critical_size) {
		rp->r_pc = (greg_t)mutex_exit_critical_start;
	}

	return (0);
}

void
send_dirint(int cpuid, int int_level)
{
	(*send_dirintf)(cpuid, int_level);
}

#define	IS_FAKE_SOFTINT(flag, newpri)		\
	(((flag) == 0) &&				\
	 bsrw_insn((uint16_t)cpu->cpu_softinfo.st_pending) > (newpri))


/*
 * do_splx routine, takes new ipl to set
 * returns the old ipl.
 * We are careful not to set priority lower than CPU->cpu_base_pri,
 * even though it seems we're raising the priority, it could be set
 * higher at any time by an interrupt routine, so we must block interrupts
 * and look at CPU->cpu_base_pri
 */
int
do_splx(int newpri)
{
	ulong_t	flag;
	cpu_t	*cpu;
	int	curpri, basepri;

	flag = pal_swpipl(6);
	cpu = CPU; /* ints are disabled, now safe to cache cpu ptr */
	curpri = cpu->cpu_m.mcpu_pri;
	basepri = cpu->cpu_base_spl;
	if (newpri < basepri)
		newpri = basepri;
	cpu->cpu_m.mcpu_pri = newpri;
	(*setspl)(newpri);
	if (IS_FAKE_SOFTINT(flag, newpri))
		fakesoftint();
	pal_swpipl(flag);
	return (curpri);
}

/*
 * Common spl raise routine, takes new ipl to set
 * returns the old ipl, will not lower ipl.
 */
int
splr(int newpri)
{
	ulong_t	flag;
	cpu_t	*cpu;
	int	curpri, basepri;

	flag = pal_swpipl(6);
	cpu = CPU;
	curpri = cpu->cpu_m.mcpu_pri;
	if (newpri > curpri) {
		basepri = cpu->cpu_base_spl;
		if (newpri < basepri)
			newpri = basepri;
		cpu->cpu_m.mcpu_pri = newpri;
		(*setspl)(newpri);
		if (IS_FAKE_SOFTINT(flag, newpri))
			fakesoftint();
	}

	pal_swpipl(flag);
	return (curpri);
}

int
getpil(void)
{
	return (CPU->cpu_m.mcpu_pri);
}

int
spl_xcall(void)
{
	return (splr(ipltospl(XCALL_PIL)));
}

int
interrupts_enabled(void)
{
	return pal_rdps() == 0;
}

