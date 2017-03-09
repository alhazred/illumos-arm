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
/*
 * Copyright (c) 2012, Joyent, Inc.  All rights reserved.
 * Copyright 2013 Nexenta Systems, Inc.  All rights reserved.
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
#include <sys/fp.h>
#include <sys/reboot.h>
#include <sys/kdi_machimpl.h>
#include <vm/vm_dep.h>
#include <sys/memnode.h>
#include <sys/sysmacros.h>
#include <sys/ontrap.h>
#include <sys/promif.h>
#include <sys/platmod.h>
#include <sys/irq.h>

struct cpu	cpus[1];			/* CPU data */
struct cpu	*cpu[NCPU] = {&cpus[0]};	/* pointers to all CPUs */
struct cpu	*cpu_free_list;			/* list for released CPUs */
cpu_core_t	cpu_core[NCPU];			/* cpu_core structures */
cpuset_t	cpu_ready_set;
cpuset_t	mp_cpus;
uint64_t	wakeup_hartmask;

static cpuset_t procset_slave, procset_master;

static void
mp_startup_wait(cpuset_t *sp, processorid_t cpuid)
{
	cpuset_t tempset;

	for (tempset = *sp; !CPU_IN_SET(tempset, cpuid);
	    tempset = *(volatile cpuset_t *)sp) {
		asm volatile ("nop":::"memory");
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
		asm volatile ("nop":::"memory");
	}
}

void
init_cpu_info(struct cpu *cp)
{
	processor_info_t *pi = &cp->cpu_type_info;

	/*
	 * Get clock-frequency property for the CPU.
	 */
	pi->pi_clock = (plat_get_cpu_clock(cp->cpu_id) + 500000) / 1000000;

	strcpy(pi->pi_processor_type, "RISC-V");

	strcat(pi->pi_fputypes, "f");
	strcat(pi->pi_fputypes, "d");
	//strcat(pi->pi_fputypes, "q");

	/*
	 * Current frequency in Hz.
	 */
	cp->cpu_curr_clock = plat_get_cpu_clock(cp->cpu_id);

	cp->cpu_idstr = kmem_zalloc(CPU_IDSTRLEN, KM_SLEEP);
	snprintf(cp->cpu_idstr, CPU_IDSTRLEN - 1, "RISC-V 64bit hart=%d", __builtin_ctzl(cp->cpu_hartmask));

	cp->cpu_brandstr = kmem_zalloc(strlen(plat_get_cpu_str()) + 1, KM_SLEEP);
	strcpy(cp->cpu_brandstr, plat_get_cpu_str());

	cp->cpu_isa = kmem_zalloc(32, KM_SLEEP);
	strcat(cp->cpu_isa, "rv64imafdc");

	cp->cpu_mmu = kmem_zalloc(32, KM_SLEEP);
	strcat(cp->cpu_mmu, "riscv,sv39");

	cp->cpu_uarch = kmem_zalloc(32, KM_SLEEP);
	strcat(cp->cpu_uarch, "sifive,rocket0");

	/*
	 * Supported frequencies.
	 */
	if (cp->cpu_supp_freqs == NULL) {
		cpu_set_supp_freqs(cp, NULL);
	}
}

/*
 * Dummy functions - no aarch64 platforms support dynamic cpu allocation.
 */
/*ARGSUSED*/
int
mp_cpu_configure(int cpuid)
{
	return (ENOTSUP);		/* not supported */
}

/*ARGSUSED*/
int
mp_cpu_unconfigure(int cpuid)
{
	return (ENOTSUP);		/* not supported */
}

/*
 * Power on CPU.
 */
/*ARGSUSED*/
int
mp_cpu_poweron(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	return (ENOTSUP);		/* not supported */
}

/*
 * Power off CPU.
 */
/*ARGSUSED*/
int
mp_cpu_poweroff(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	return (ENOTSUP);		/* not supported */
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
/* ARGSUSED */
int
mp_cpu_stop(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	return (0);
}

void
mp_cpu_faulted_enter(struct cpu *cp)
{
}

void
mp_cpu_faulted_exit(struct cpu *cp)
{
}

/*
 * Take the specified CPU out of participation in interrupts.
 */
int
cpu_disable_intr(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	return (EBUSY);
}

/*
 * Allow the specified CPU to participate in interrupts.
 */
void
cpu_enable_intr(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	cp->cpu_flags |= CPU_ENABLE;
}


static uint64_t get_cpu_hartmask(pnode_t node)
{
	uint32_t reg[1] = {0};
	int address_cells = prom_get_address_cells(node);
	ASSERT(address_cells == 1);
	prom_getprop(node, "reg", (caddr_t)reg);
	return 1ul << ntohl(reg[0]);
}

static pnode_t get_cpu_node(int cpun, uint64_t ignore_hartmask)
{
	pnode_t cpus = prom_finddevice("/cpus");
	if (cpus <= 0)
		return cpus;

	pnode_t node = prom_childnode(cpus);
	int id = 1;
	for (;;) {
		if (node <= 0)
			return node;
		if (cpun == 0) {
			if (get_cpu_hartmask(node) == ignore_hartmask)
				return node;
		} else {
			uint64_t hartmask = get_cpu_hartmask(node);
			if ((hartmask & ignore_hartmask) == 0 && (hartmask & enable_hartmask) != 0) {
				if (id == cpun)
					return node;
				id++;
			}
		}
		node = prom_nextnode(node);
	}
}
static inline int l2_pteidx(uintptr_t vaddr) { return ((vaddr >> (PAGESHIFT+2*NPTESHIFT)) & ((1<<NPTESHIFT)-1));}
static inline int l1_pteidx(uintptr_t vaddr) { return ((vaddr >> (PAGESHIFT+1*NPTESHIFT)) & ((1<<NPTESHIFT)-1));}
static inline int l0_pteidx(uintptr_t vaddr) { return ((vaddr >> (PAGESHIFT+0*NPTESHIFT)) & ((1<<NPTESHIFT)-1));}

static bool
vtop(uintptr_t vaddr, uint64_t *pap)
{
	uint64_t satp = csr_read_satp();
	ASSERT((satp & SATP_MODE_MASK) == SATP_MODE_SV39);
	pte_t *l2pt = (pte_t *)pa_to_kseg((satp & SATP_PPN_MASK) << MMU_PAGESHIFT);
	pte_t l2pte = l2pt[l2_pteidx(vaddr)];
	if ((l2pte & PTE_V) == 0)
		return false;
	if (!IS_TABLE(l2pte)) {
		*pap = (PTE_TO_PA(l2pte) | (vaddr & LEVEL_OFFSET(2)));
		return true;
	}
	pte_t *l1pt = (pte_t *)(PTE_TO_PA(l2pte) + SEGKPM_BASE);
	pte_t l1pte = l1pt[l1_pteidx(vaddr)];
	if ((l1pte & PTE_V) == 0)
		return false;
	if (!IS_TABLE(l1pte)) {
		*pap = (PTE_TO_PA(l1pte) | (vaddr & LEVEL_OFFSET(1)));
		return true;
	}
	pte_t *l0pt = (pte_t *)(PTE_TO_PA(l1pte) + SEGKPM_BASE);
	pte_t l0pte = l0pt[l0_pteidx(vaddr)];
	if ((l0pte & PTE_V) == 0)
		return false;
	*pap = (PTE_TO_PA(l0pte) | (vaddr & LEVEL_OFFSET(0)));
	return true;
}

static int
wakeup_cpu(int who, uint64_t ignore_hartmask)
{
	cpu_t *cp = cpu[who];
	uint64_t hartmask = cp->cpu_m.mcpu_hartmask;
	asm volatile ("fence":::"memory");
	wakeup_hartmask |= hartmask;
	return 0;
}

static void
mp_startup_boot(void)
{
	cpu_t *cp = CPU;

	extern void cpu_event_init_cpu(cpu_t *);
	extern void trap_entry(void);

	/* Let the control CPU continue into tsc_sync_master() */
	mp_startup_signal(&procset_slave, cp->cpu_id);

	intr_slave_init();
	csr_write_stvec((uintptr_t)trap_entry);
	csr_write_sscratch(0);

	/*
	 * Enable interrupts with spl set to LOCK_LEVEL. LOCK_LEVEL is the
	 * highest level at which a routine is permitted to block on
	 * an adaptive mutex (allows for cpu poke interrupt in case
	 * the cpu is blocked on a mutex and halts). Setting LOCK_LEVEL blocks
	 * device interrupts that may end up in the hat layer issuing cross
	 * calls before CPU_READY is set.
	 */
	splx(ipltospl(LOCK_LEVEL));

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

	init_cpu_info(cp);

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
	csr_set_sstatus(SSR_SIE);

	pghw_physid_create(cp);
	/*
	 * Delegate initialization tasks, which need to access the cpu_lock,
	 * to mp_start_cpu_common() because we can't acquire the cpu_lock here
	 * during CPU DR operations.
	 */
	mp_startup_signal(&procset_slave, cp->cpu_id);
	mp_startup_wait(&procset_master, cp->cpu_id);
	pg_cmt_cpu_startup(cp);

	mutex_enter(&cpu_lock);
	cp->cpu_flags &= ~CPU_OFFLINE;
	cpu_enable_intr(cp);
	cpu_add_active(cp);
	mutex_exit(&cpu_lock);

	/* Enable interrupts */
	(void) spl0();

	/*
	 * Setting the bit in cpu_ready_set must be the last operation in
	 * processor initialization; the boot CPU will continue to boot once
	 * it sees this bit set for all active CPUs.
	 */
	CPUSET_ATOMIC_ADD(cpu_ready_set, cp->cpu_id);

	cmn_err(CE_CONT, "cpu%d: %s\n", cp->cpu_id, cp->cpu_idstr);
	cmn_err(CE_CONT, "cpu%d: %s\n", cp->cpu_id, cp->cpu_brandstr);
	cmn_err(CE_CONT, "cpu%d initialization complete - online\n", cp->cpu_id);

	/*
	 * Now we are done with the startup thread, so free it up.
	 */
	thread_exit();
	panic("mp_startup: cannot return");
	/*NOTREACHED*/
}

static struct cpu *
mp_cpu_configure_common(int cpun, uint64_t ignore_hartmask)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	ASSERT(cpun < NCPU && cpu[cpun] == NULL);
	extern void idle();

	pnode_t node = get_cpu_node(cpun, ignore_hartmask);
	if (node <= 0) {
		return NULL;
	}

	uint64_t hartmask = get_cpu_hartmask(node);

	struct cpu *cp;

	kthread_id_t tp;
	caddr_t	sp;
	proc_t *procp;

	cp = kmem_zalloc(sizeof (*cp), KM_SLEEP);

	cp->cpu_m.mcpu_hartmask = hartmask;

	procp = &p0;

	disp_cpu_init(cp);
	cpu_vm_data_init(cp);
	tp = thread_create(NULL, 0, NULL, NULL, 0, procp, TS_STOPPED, maxclsyspri);

	THREAD_ONPROC(tp, cp);
	tp->t_preempt = 1;
	tp->t_bound_cpu = cp;
	tp->t_affinitycnt = 1;
	tp->t_cpu = cp;
	tp->t_disp_queue = cp->cpu_disp;

	sp = tp->t_stk;
	tp->t_sp = (uintptr_t)(sp - MINFRAME);
	tp->t_sp -= STACK_ENTRY_ALIGN;		/* fake a call */
	tp->t_pc = (uintptr_t)mp_startup_boot;

	cp->cpu_id = cpun;
	cp->cpu_self = cp;
	cp->cpu_thread = tp;
	cp->cpu_lwp = NULL;
	cp->cpu_dispthread = tp;
	cp->cpu_dispatch_pri = DISP_PRIO(tp);

	cp->cpu_base_spl = ipltospl(LOCK_LEVEL);

	tp = thread_create(NULL, PAGESIZE, idle, NULL, 0, procp, TS_ONPROC, -1);

	cp->cpu_idle_thread = tp;

	tp->t_preempt = 1;
	tp->t_bound_cpu = cp;
	tp->t_affinitycnt = 1;
	tp->t_cpu = cp;
	tp->t_disp_queue = cp->cpu_disp;

	pg_cpu_bootstrap(cp);

	kcpc_hw_init(cp);

	cpu_intr_alloc(cp, NINTR_THREADS);

	cp->cpu_flags = CPU_OFFLINE | CPU_QUIESCED | CPU_POWEROFF;
	cpu_set_state(cp);

	cpu_add_unit(cp);

	return (cp);
}

static int
mp_start_cpu_common(cpu_t *cp, uint64_t ignore_hartmask)
{
	void *ctx;
	int delays;
	int error = 0;
	cpuset_t tempset;
	processorid_t cpuid;

	ASSERT(cp != NULL);
	cpuid = cp->cpu_id;

	error = wakeup_cpu(cpuid, ignore_hartmask);
	if (error != 0) {
		cmn_err(CE_WARN,
		    "cpu%d: failed to start, error %d", cp->cpu_id, error);
		return (error);
	}

	for (delays = 0, tempset = procset_slave; !CPU_IN_SET(tempset, cpuid); delays++) {
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
			return (error);
		}

		/*
		 * wait at least 10ms, then check again..
		 */
		delay(USEC_TO_TICK_ROUNDUP(10000));
		tempset = *((volatile cpuset_t *)&procset_slave);
	}
	CPUSET_ATOMIC_DEL(procset_slave, cpuid);

	mp_startup_wait(&procset_slave, cpuid);

	(void) pg_cpu_init(cp, B_FALSE);
	cpu_set_state(cp);
	mp_startup_signal(&procset_master, cpuid);

	return (0);
}

static int
start_cpu(processorid_t who, uint64_t ignore_hartmask)
{
	cpu_t *cp;
	int error = 0;
	cpuset_t tempset;

	ASSERT(who != 0);

	error = mp_start_cpu_common(cpu[who], ignore_hartmask);
	if (error != 0) {
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
	init_cpu_info(CPU);

	cmn_err(CE_CONT, "cpu%d: %s\n", CPU->cpu_id, CPU->cpu_idstr);

	uint_t bootcpuid = CPU->cpu_id;

	CPUSET_DEL(mp_cpus, bootcpuid);
	CPUSET_ADD(cpu_ready_set, bootcpuid);

	cpu_pause_init();
	xc_init();
	ipi_handler_init();

	affinity_set(CPU_CURRENT);

	mutex_enter(&cpu_lock);
	for (int who = 0; who < NCPU; who++) {
		if (!CPU_IN_SET(mp_cpus, who))
			continue;
		cpu_t *cp;
		cp = mp_cpu_configure_common(who, CPU->cpu_m.mcpu_hartmask);
		ASSERT(cp != NULL);
	}
	for (int who = 0; who < NCPU; who++) {
		if (!CPU_IN_SET(mp_cpus, who))
			continue;
		ASSERT(who != bootcpuid);

		if (start_cpu(who, CPU->cpu_m.mcpu_hartmask) != 0)
			CPUSET_DEL(mp_cpus, who);
		cpu_state_change_notify(who, CPU_SETUP);
		mutex_exit(&cpu_lock);
		mutex_enter(&cpu_lock);
	}
	for (int who = 0; who < NCPU; who++) {
		if (!CPU_IN_SET(mp_cpus, who))
			continue;
		cpu_t *cp = cpu[who];
	}

	mutex_exit(&cpu_lock);

	affinity_clear();
}

