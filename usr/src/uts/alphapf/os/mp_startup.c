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
/*
 * Copyright (c) 2010, Intel Corporation.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/thread.h>
#include <sys/cpuvar.h>
#include <sys/cpu.h>
#include <sys/t_lock.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/class.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/note.h>
#include <sys/asm_linkage.h>
#include <sys/x_call.h>
#include <sys/systm.h>
#include <sys/var.h>
#include <sys/vtrace.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg_kmem.h>
#include <vm/seg_kp.h>
#include <sys/kmem.h>
#include <sys/stack.h>
#include <sys/smp_impldefs.h>
#include <sys/machsystm.h>
#include <sys/clock.h>
#include <sys/cpc_impl.h>
#include <sys/pg.h>
#include <sys/cmt.h>
#include <sys/dtrace.h>
#include <sys/archsystm.h>
#include <sys/reboot.h>
#include <sys/kdi_machimpl.h>
#include <vm/hat_alpha.h>
#include <vm/vm_dep.h>
#include <sys/memnode.h>
#include <sys/sysmacros.h>
#include <sys/cpu_module.h>
#include <sys/spl.h>
#include <sys/promif.h>

struct cpu	cpus[1];			/* CPU data */
struct cpu	*cpu[NCPU] = {&cpus[0]};	/* pointers to all CPUs */
struct cpu	*cpu_free_list;			/* list for released CPUs */
cpu_core_t	cpu_core[NCPU];			/* cpu_core structures */

#define	cpu_next_free	cpu_prev

/*
 * to be set by a PSM to indicate what cpus
 * are sitting around on the system.
 */
cpuset_t mp_cpus;

/*
 * This variable is used by the hat layer to decide whether or not
 * critical sections are needed to prevent race conditions.  For sun4m,
 * this variable is set once enough MP initialization has been done in
 * order to allow cross calls.
 */
int flushes_require_xcalls;

cpuset_t cpu_ready_set;		/* initialized in startup() */

static void mp_startup_boot(void);
static void mp_startup_hotplug(void);

/*
 * Init CPU info - get CPU type info for processor_info system call.
 */
void
init_cpu_info(struct cpu *cp)
{
	processor_info_t *pi = &cp->cpu_type_info;

	/*
	 * Get clock-frequency property for the CPU.
	 */
	pi->pi_clock = cpu_freq;

	/*
	 * Current frequency in Hz.
	 */
	cp->cpu_curr_clock = cpu_freq_hz;

	/*
	 * Supported frequencies.
	 */
	if (cp->cpu_supp_freqs == NULL) {
		cpu_set_supp_freqs(cp, NULL);
	}
}

/*
 * Multiprocessor initialization.
 *
 * Allocate and initialize the cpu structure, TRAPTRACE buffer, and the
 * startup and idle threads for the specified CPU.
 * Parameter boot is true for boot time operations and is false for CPU
 * DR operations.
 */
static struct cpu *
mp_cpu_configure_common(int cpun, boolean_t boot)
{
	struct cpu *cp;
	kthread_id_t tp;
	caddr_t	sp;
	proc_t *procp;
	extern int idle_cpu_prefer_mwait;
	extern void cpu_idle_mwait();
	extern void idle();
	extern void cpu_idle();

	ASSERT(MUTEX_HELD(&cpu_lock));
	ASSERT(cpun < NCPU && cpu[cpun] == NULL);

	if (cpu_free_list == NULL) {
		cp = kmem_zalloc(sizeof (*cp), KM_SLEEP);
	} else {
		cp = cpu_free_list;
		cpu_free_list = cp->cpu_next_free;
	}

	cp->cpu_m.mcpu_istamp = cpun << 16;

	/* Create per CPU specific threads in the process p0. */
	procp = &p0;

	/*
	 * Initialize the dispatcher first.
	 */
	disp_cpu_init(cp);

	cpu_vm_data_init(cp);

	/*
	 * Allocate and initialize the startup thread for this CPU.
	 * Interrupt and process switch stacks get allocated later
	 * when the CPU starts running.
	 */
	tp = thread_create(NULL, 0, NULL, NULL, 0, procp,
	    TS_STOPPED, maxclsyspri);

	/*
	 * Set state to TS_ONPROC since this thread will start running
	 * as soon as the CPU comes online.
	 *
	 * All the other fields of the thread structure are setup by
	 * thread_create().
	 */
	THREAD_ONPROC(tp, cp);
	tp->t_preempt = 1;
	tp->t_bound_cpu = cp;
	tp->t_affinitycnt = 1;
	tp->t_cpu = cp;
	tp->t_disp_queue = cp->cpu_disp;

	/*
	 * Setup thread to start in mp_startup_common.
	 */
	sp = tp->t_stk;
	tp->t_sp = (uintptr_t)(sp - MINFRAME);

	/*
	 * Setup thread start entry point for boot or hotplug.
	 */
	if (boot) {
		tp->t_pc = (uintptr_t)mp_startup_boot;
	} else {
		tp->t_pc = (uintptr_t)mp_startup_hotplug;
	}

	cp->cpu_id = cpun;
	cp->cpu_self = cp;
	cp->cpu_thread = tp;
	cp->cpu_lwp = NULL;
	cp->cpu_dispthread = tp;
	cp->cpu_dispatch_pri = DISP_PRIO(tp);

	/*
	 * cpu_base_spl must be set explicitly here to prevent any blocking
	 * operations in mp_startup_common from causing the spl of the cpu
	 * to drop to 0 (allowing device interrupts before we're ready) in
	 * resume().
	 * cpu_base_spl MUST remain at LOCK_LEVEL until the cpu is CPU_READY.
	 * As an extra bit of security on DEBUG kernels, this is enforced with
	 * an assertion in mp_startup_common() -- before cpu_base_spl is set
	 * to its proper value.
	 */
	cp->cpu_base_spl = ipltospl(LOCK_LEVEL);

	/*
	 * Now, initialize per-CPU idle thread for this CPU.
	 */
	tp = thread_create(NULL, PAGESIZE, idle, NULL, 0, procp, TS_ONPROC, -1);

	cp->cpu_idle_thread = tp;

	tp->t_preempt = 1;
	tp->t_bound_cpu = cp;
	tp->t_affinitycnt = 1;
	tp->t_cpu = cp;
	tp->t_disp_queue = cp->cpu_disp;

	/*
	 * Bootstrap the CPU's PG data
	 */
	pg_cpu_bootstrap(cp);

	/*
	 * Perform CPC initialization on the new CPU.
	 */
	kcpc_hw_init(cp);

	/*
	 * Get interrupt priority data from cpu 0.
	 */
	cp->cpu_pri_data = CPU->cpu_pri_data;

	/*
	 * alloc space for cpuid info
	 */
	init_cpu_info(cp);

	xc_init_cpu(cp);

	/*
	 * Record that we have another CPU.
	 */
	/*
	 * Initialize the interrupt threads for this CPU
	 */
	cpu_intr_alloc(cp, NINTR_THREADS);

	cp->cpu_flags = CPU_OFFLINE | CPU_QUIESCED | CPU_POWEROFF;
	cpu_set_state(cp);

	/*
	 * Add CPU to list of available CPUs.  It'll be on the active list
	 * after mp_startup_common().
	 */
	cpu_add_unit(cp);

	return (cp);
}

/*
 * Undo what was done in mp_cpu_configure_common
 */
static void
mp_cpu_unconfigure_common(struct cpu *cp, int error)
{
	ASSERT(MUTEX_HELD(&cpu_lock));

	/*
	 * Remove the CPU from the list of available CPUs.
	 */
	cpu_del_unit(cp->cpu_id);

	if (error == ETIMEDOUT) {
		/*
		 * The cpu was started, but never *seemed* to run any
		 * code in the kernel; it's probably off spinning in its
		 * own private world, though with potential references to
		 * our kmem-allocated IDTs and GDTs (for example).
		 *
		 * Worse still, it may actually wake up some time later,
		 * so rather than guess what it might or might not do, we
		 * leave the fundamental data structures intact.
		 */
		cp->cpu_flags = 0;
		return;
	}

	/*
	 * At this point, the only threads bound to this CPU should
	 * special per-cpu threads: it's idle thread, it's pause threads,
	 * and it's interrupt threads.  Clean these up.
	 */
	cpu_destroy_bound_threads(cp);
	cp->cpu_idle_thread = NULL;

	/*
	 * Free the interrupt stack.
	 */
	segkp_release(segkp,
	    cp->cpu_intr_stack - (INTR_STACK_SIZE - SA(MINFRAME)));
	cp->cpu_intr_stack = NULL;

	/* Free CPU ID string and brand string. */
	if (cp->cpu_idstr) {
		kmem_free(cp->cpu_idstr, CPU_IDSTRLEN);
		cp->cpu_idstr = NULL;
	}
	if (cp->cpu_brandstr) {
		kmem_free(cp->cpu_brandstr, CPU_IDSTRLEN);
		cp->cpu_brandstr = NULL;
	}

	if (cp->cpu_supp_freqs != NULL) {
		size_t len = strlen(cp->cpu_supp_freqs) + 1;
		kmem_free(cp->cpu_supp_freqs, len);
		cp->cpu_supp_freqs = NULL;
	}

	kcpc_hw_fini(cp);

	cp->cpu_dispthread = NULL;
	cp->cpu_thread = NULL;	/* discarded by cpu_destroy_bound_threads() */

	cpu_vm_data_destroy(cp);

	xc_fini_cpu(cp);
	disp_cpu_fini(cp);

	ASSERT(cp != CPU0);
	bzero(cp, sizeof (*cp));
	cp->cpu_next_free = cpu_free_list;
	cpu_free_list = cp;
}

/*
 * The procset_slave and procset_master are used to synchronize
 * between the control CPU and the target CPU when starting CPUs.
 */
static cpuset_t procset_slave, procset_master;

static void
mp_startup_wait(cpuset_t *sp, processorid_t cpuid)
{
	cpuset_t tempset;

	for (tempset = *sp; !CPU_IN_SET(tempset, cpuid);
	    tempset = *(volatile cpuset_t *)sp) {
		SMT_PAUSE();
	}
	CPUSET_ATOMIC_DEL(*(cpuset_t *)sp, cpuid);
}

static void
mp_startup_signal(cpuset_t *sp, processorid_t cpuid)
{
	cpuset_t tempset;

	CPUSET_ATOMIC_ADD(*(cpuset_t *)sp, cpuid);
	for (tempset = *sp; CPU_IN_SET(tempset, cpuid);
	    tempset = *(volatile cpuset_t *)sp) {
		SMT_PAUSE();
	}
}

int
mp_start_cpu_common(cpu_t *cp, boolean_t boot)
{
	_NOTE(ARGUNUSED(boot));

	void *ctx;
	int delays;
	int error = 0;
	cpuset_t tempset;
	processorid_t cpuid;

	ASSERT(cp != NULL);
	cpuid = cp->cpu_id;
	ctx = mach_cpucontext_alloc(cp);
	if (ctx == NULL) {
		cmn_err(CE_WARN,
		    "cpu%d: failed to allocate context", cp->cpu_id);
		return (EAGAIN);
	}
	error = mach_cpu_start(cp, ctx);
	if (error != 0) {
		cmn_err(CE_WARN,
		    "cpu%d: failed to start, error %d", cp->cpu_id, error);
		mach_cpucontext_free(cp, ctx, error);
		return (error);
	}

	for (delays = 0, tempset = procset_slave; !CPU_IN_SET(tempset, cpuid);
	    delays++) {
		if (delays == 500) {
			/*
			 * After five seconds, things are probably looking
			 * a bit bleak - explain the hang.
			 */
			cmn_err(CE_NOTE, "cpu%d: started, "
			    "but not running in the kernel yet", cpuid);
		} else if (delays > 2000) {
			/*
			 * We waited at least 20 seconds, bail ..
			 */
			error = ETIMEDOUT;
			cmn_err(CE_WARN, "cpu%d: timed out", cpuid);
			mach_cpucontext_free(cp, ctx, error);
			return (error);
		}

		/*
		 * wait at least 10ms, then check again..
		 */
		delay(USEC_TO_TICK_ROUNDUP(10000));
		tempset = *((volatile cpuset_t *)&procset_slave);
	}
	CPUSET_ATOMIC_DEL(procset_slave, cpuid);

	mach_cpucontext_free(cp, ctx, 0);

	if (dtrace_cpu_init != NULL) {
		(*dtrace_cpu_init)(cpuid);
	}

	/*
	 * During CPU DR operations, the cpu_lock is held by current
	 * (the control) thread. We can't release the cpu_lock here
	 * because that will break the CPU DR logic.
	 * On the other hand, CPUPM and processor group initialization
	 * routines need to access the cpu_lock. So we invoke those
	 * routines here on behalf of mp_startup_common().
	 *
	 * CPUPM and processor group initialization routines depend
	 * on the cpuid probing results. Wait for mp_startup_common()
	 * to signal that cpuid probing is done.
	 */
	mp_startup_wait(&procset_slave, cpuid);
	(void) pg_cpu_init(cp, B_FALSE);
	cpu_set_state(cp);
	mp_startup_signal(&procset_master, cpuid);

	return (0);
}

/*
 * Start a single cpu, assuming that the kernel context is available
 * to successfully start another cpu.
 *
 * (For example, real mode code is mapped into the right place
 * in memory and is ready to be run.)
 */
int
start_cpu(processorid_t who)
{
	cpu_t *cp;
	int error = 0;
	cpuset_t tempset;

	ASSERT(who != 0);

	/*
	 * Check if there's at least a Mbyte of kmem available
	 * before attempting to start the cpu.
	 */
	if (kmem_avail() < 1024 * 1024) {
		/*
		 * Kick off a reap in case that helps us with
		 * later attempts ..
		 */
		kmem_reap();
		return (ENOMEM);
	}

	/*
	 * First configure cpu.
	 */
	cp = mp_cpu_configure_common(who, B_TRUE);
	ASSERT(cp != NULL);

	/*
	 * Then start cpu.
	 */
	error = mp_start_cpu_common(cp, B_TRUE);
	if (error != 0) {
		mp_cpu_unconfigure_common(cp, error);
		return (error);
	}

	mutex_exit(&cpu_lock);
	tempset = cpu_ready_set;
	while (!CPU_IN_SET(tempset, who)) {
		drv_usecwait(1);
		tempset = *((volatile cpuset_t *)&cpu_ready_set);
	}
	mutex_enter(&cpu_lock);

	return (0);
}

void
start_other_cpus(int cprboot)
{
	_NOTE(ARGUNUSED(cprboot));

	uint_t who;
	uint_t bootcpuid = 0;

	/*
	 * Initialize our own cpu_info.
	 */
	init_cpu_info(CPU);

	cmn_err(CE_CONT, "?cpu%d: %s\n", CPU->cpu_id, CPU->cpu_idstr);
	cmn_err(CE_CONT, "?cpu%d: %s\n", CPU->cpu_id, CPU->cpu_brandstr);

	/*
	 * Take the boot cpu out of the mp_cpus set because we know
	 * it's already running.  Add it to the cpu_ready_set for
	 * precisely the same reason.
	 */
	CPUSET_DEL(mp_cpus, bootcpuid);
	CPUSET_ADD(cpu_ready_set, bootcpuid);

	/*
	 * skip the rest of this if
	 * . only 1 cpu dectected and system isn't hotplug-capable
	 */
	if (CPUSET_ISNULL(mp_cpus))
		return;

	/*
	 * perform such initialization as is needed
	 * to be able to take CPUs on- and off-line.
	 */
	cpu_pause_init();

	xc_init_cpu(CPU);		/* initialize processor crosscalls */

	if (mach_cpucontext_init() != 0)
		return;

	flushes_require_xcalls = 1;

	/*
	 * We lock our affinity to the master CPU to ensure that all slave CPUs
	 * do their TSC syncs with the same CPU.
	 */
	affinity_set(CPU_CURRENT);

	for (who = 0; who < NCPU; who++) {
		if (!CPU_IN_SET(mp_cpus, who))
			continue;
		ASSERT(who != bootcpuid);

		mutex_enter(&cpu_lock);
		if (start_cpu(who) != 0)
			CPUSET_DEL(mp_cpus, who);
		cpu_state_change_notify(who, CPU_SETUP);
		mutex_exit(&cpu_lock);
	}

	affinity_clear();

	mach_cpucontext_fini();
}

int
mp_cpu_configure(int cpuid)
{
	cpu_t *cp;

	prom_printf("%s(): called\n", __FUNCTION__);
	cp = cpu_get(cpuid);
	if (cp != NULL) {
		return (EALREADY);
	}

	/*
	 * Check if there's at least a Mbyte of kmem available
	 * before attempting to start the cpu.
	 */
	if (kmem_avail() < 1024 * 1024) {
		/*
		 * Kick off a reap in case that helps us with
		 * later attempts ..
		 */
		kmem_reap();
		return (ENOMEM);
	}

	cp = mp_cpu_configure_common(cpuid, B_FALSE);
	ASSERT(cp != NULL && cpu_get(cpuid) == cp);

	return (cp != NULL ? 0 : EAGAIN);
}

int
mp_cpu_unconfigure(int cpuid)
{
	cpu_t *cp;

	if (cpuid < 0 || cpuid >= max_ncpus) {
		return (EINVAL);
	}

	cp = cpu_get(cpuid);
	if (cp == NULL) {
		return (ENODEV);
	}
	mp_cpu_unconfigure_common(cp, 0);

	return (0);
}

/*
 * Startup function for 'other' CPUs (besides boot cpu).
 * Called from real_mode_start.
 *
 * WARNING: until CPU_READY is set, mp_startup_common and routines called by
 * mp_startup_common should not call routines (e.g. kmem_free) that could call
 * hat_unload which requires CPU_READY to be set.
 */
static void
mp_startup_common(boolean_t boot)
{
	cpu_t *cp = CPU;
	extern void cpu_event_init_cpu(cpu_t *);

	prom_printf("%s(): called\n", __FUNCTION__);
	/*
	 * We need to get TSC on this proc synced (i.e., any delta
	 * from cpu0 accounted for) as soon as we can, because many
	 * many things use gethrtime/pc_gethrestime, including
	 * interrupts, cmn_err, etc.
	 */

	/* Let the control CPU continue into tsc_sync_master() */
	mp_startup_signal(&procset_slave, cp->cpu_id);

	/*
	 * Once this was done from assembly, but it's safer here; if
	 * it blocks, we need to be able to swtch() to and from, and
	 * since we get here by calling t_pc, we need to do that call
	 * before swtch() overwrites it.
	 */
	(void) (*ap_mlsetup)();

	/*
	 * Enable interrupts with spl set to LOCK_LEVEL. LOCK_LEVEL is the
	 * highest level at which a routine is permitted to block on
	 * an adaptive mutex (allows for cpu poke interrupt in case
	 * the cpu is blocked on a mutex and halts). Setting LOCK_LEVEL blocks
	 * device interrupts that may end up in the hat layer issuing cross
	 * calls before CPU_READY is set.
	 */
	splx(ipltospl(LOCK_LEVEL));
	pal_swpipl(6);

	/*
	 * We can touch cpu_flags here without acquiring the cpu_lock here
	 * because the cpu_lock is held by the control CPU which is running
	 * mp_start_cpu_common().
	 * Need to clear CPU_QUIESCED flag before calling any function which
	 * may cause thread context switching, such as kmem_alloc() etc.
	 * The idle thread checks for CPU_QUIESCED flag and loops for ever if
	 * it's set. So the startup thread may have no chance to switch back
	 * again if it's switched away with CPU_QUIESCED set.
	 */
	cp->cpu_flags &= ~(CPU_POWEROFF | CPU_QUIESCED);
	cp->cpu_flags |= CPU_RUNNING | CPU_READY | CPU_EXISTS;

	cpu_event_init_cpu(cp);

	/*
	 * Enable preemption here so that contention for any locks acquired
	 * later in mp_startup_common may be preempted if the thread owning
	 * those locks is continuously executing on other CPUs (for example,
	 * this CPU must be preemptible to allow other CPUs to pause it during
	 * their startup phases).  It's safe to enable preemption here because
	 * the CPU state is pretty-much fully constructed.
	 */
	curthread->t_preempt = 0;

	/* The base spl should still be at LOCK LEVEL here */
	ASSERT(cp->cpu_base_spl == ipltospl(LOCK_LEVEL));
	set_base_spl();		/* Restore the spl to its proper value */

	pghw_physid_create(cp);
	/*
	 * Delegate initialization tasks, which need to access the cpu_lock,
	 * to mp_start_cpu_common() because we can't acquire the cpu_lock here
	 * during CPU DR operations.
	 */
	mp_startup_signal(&procset_slave, cp->cpu_id);
	mp_startup_wait(&procset_master, cp->cpu_id);
	pg_cmt_cpu_startup(cp);

	if (boot) {
		mutex_enter(&cpu_lock);
		cp->cpu_flags &= ~CPU_OFFLINE;
		cpu_enable_intr(cp);
		cpu_add_active(cp);
		mutex_exit(&cpu_lock);
	}

	/* Enable interrupts */
	(void) spl0();

	/*
	 * Setting the bit in cpu_ready_set must be the last operation in
	 * processor initialization; the boot CPU will continue to boot once
	 * it sees this bit set for all active CPUs.
	 */
	CPUSET_ATOMIC_ADD(cpu_ready_set, cp->cpu_id);

	(void) mach_cpu_create_device_node(cp, NULL);

	cmn_err(CE_CONT, "?cpu%d: %s\n", cp->cpu_id, cp->cpu_idstr);
	cmn_err(CE_CONT, "?cpu%d: %s\n", cp->cpu_id, cp->cpu_brandstr);
	cmn_err(CE_CONT, "?cpu%d initialization complete - online\n",
	    cp->cpu_id);

	/*
	 * Now we are done with the startup thread, so free it up.
	 */
	thread_exit();
	panic("mp_startup: cannot return");
	/*NOTREACHED*/
}

/*
 * Startup function for 'other' CPUs at boot time (besides boot cpu).
 */
static void
mp_startup_boot(void)
{
	mp_startup_common(B_TRUE);
}

/*
 * Startup function for hotplug CPUs at runtime.
 */
void
mp_startup_hotplug(void)
{
	mp_startup_common(B_FALSE);
}

/*
 * Start CPU on user request.
 */
/* ARGSUSED */
int
mp_cpu_start(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	return (0);
}

/*
 * Stop CPU on user request.
 */
int
mp_cpu_stop(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));

	/*
	 * If TIMER_PERIODIC mode is used, CPU0 is the one running it;
	 * can't stop it.  (This is true only for machines with no TSC.)
	 */

	if (cp->cpu_id == 0)
		return (EBUSY);

	return (0);
}

/*
 * Take the specified CPU out of participation in interrupts.
 */
int
cpu_disable_intr(struct cpu *cp)
{
	if (psm_disable_intr(cp->cpu_id) != DDI_SUCCESS)
		return (EBUSY);

	cp->cpu_flags &= ~CPU_ENABLE;
	return (0);
}

/*
 * Allow the specified CPU to participate in interrupts.
 */
void
cpu_enable_intr(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	cp->cpu_flags |= CPU_ENABLE;
	psm_enable_intr(cp->cpu_id);
}

void
mp_cpu_faulted_enter(struct cpu *cp)
{
}

void
mp_cpu_faulted_exit(struct cpu *cp)
{
}
