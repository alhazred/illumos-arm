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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/thread.h>
#include <sys/cpuvar.h>
#include <sys/x_call.h>
#include <sys/cpu.h>
#include <sys/psw.h>
#include <sys/sunddi.h>
#include <sys/debug.h>
#include <sys/systm.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/mutex_impl.h>
#include <sys/stack.h>
#include <sys/promif.h>
#include <sys/spl.h>
#include <sys/irq.h>
#include <sys/smp_impldefs.h>

struct	xc_mbox {
	xc_func_t	func;
	xc_arg_t	arg0;
	xc_arg_t	arg1;
	xc_arg_t	arg2;
	cpuset_t	set;
};

enum {
	XC_DONE,	/* x-call session done */
	XC_HOLD,	/* spin doing nothing */
	XC_SYNC_OP,	/* perform a synchronous operation */
	XC_CALL_OP,	/* perform a call operation */
	XC_WAIT,	/* capture/release. callee has seen wait */
};
static int	xc_initialized = 0;
extern cpuset_t	cpu_ready_set;
static kmutex_t	xc_mbox_lock;
static struct	xc_mbox xc_mbox;

static uint_t
xc_poke(caddr_t arg0, caddr_t arg1)
{
	return (DDI_INTR_UNCLAIMED);
}

void
xc_init()
{
	ASSERT(xc_initialized == 0);
	mutex_init(&xc_mbox_lock, NULL,  MUTEX_SPIN, (void *)ipltospl(XC_HI_PIL));
	xc_initialized = 1;
}

uint_t
xc_serv(void)
{
	int	op;
	struct cpu *cpup = CPU;

	if (cpup->cpu_m.xc_pend == 0) {
		return (DDI_INTR_UNCLAIMED);
	}
	cpup->cpu_m.xc_pend = 0;
	asm volatile ("fence":::"memory");

	op = cpup->cpu_m.xc_state;

	/*
	 * Don't invoke a null function.
	 */
	if (xc_mbox.func != NULL) {
		cpup->cpu_m.xc_retval = (*xc_mbox.func)(xc_mbox.arg0, xc_mbox.arg1, xc_mbox.arg2);
	} else {
		cpup->cpu_m.xc_retval = 0;
	}

	/*
	 * Acknowledge that we have completed the x-call operation.
	 */
	cpup->cpu_m.xc_ack = 1;
	asm volatile ("fence":::"memory");

	if (op == XC_CALL_OP) {
		return (DDI_INTR_CLAIMED);
	}

	/*
	 * for (op == XC_SYNC_OP)
	 * Wait for the initiator of the x-call to indicate
	 * that all CPUs involved can proceed.
	 */
	while (cpup->cpu_m.xc_wait) {
	}

	while (cpup->cpu_m.xc_state != XC_DONE) {
	}

	/*
	 * Acknowledge that we have received the directive to continue.
	 */
	ASSERT(cpup->cpu_m.xc_ack == 0);
	cpup->cpu_m.xc_ack = 1;
	asm volatile ("fence":::"memory");

	return (DDI_INTR_CLAIMED);
}

static void
xc_common(
	xc_func_t func,
	xc_arg_t arg0,
	xc_arg_t arg1,
	xc_arg_t arg2,
	cpuset_t set,
	int sync)
{
	int cix;
	int lcx = (int)(CPU->cpu_id);
	struct cpu *cpup;
	cpuset_t cpuset;

	ASSERT(panicstr == NULL);

	ASSERT(MUTEX_HELD(&xc_mbox_lock));
	ASSERT(CPU->cpu_flags & CPU_READY);

	CPUSET_ZERO(cpuset);

	/*
	 * Set up the service definition mailbox.
	 */
	xc_mbox.func = func;
	xc_mbox.arg0 = arg0;
	xc_mbox.arg1 = arg1;
	xc_mbox.arg2 = arg2;

	/*
	 * Request service on all remote processors.
	 */
	for (cix = 0; cix < NCPU; cix++) {
		if ((cpup = cpu[cix]) == NULL || (cpup->cpu_flags & CPU_READY) == 0) {
			/*
			 * In case the non-local CPU is not ready but becomes
			 * ready later, take it out of the set now. The local
			 * CPU needs to remain in the set to complete the
			 * requested function.
			 */
			if (cix != lcx)
				CPUSET_DEL(set, cix);
		} else if (cix != lcx && CPU_IN_SET(set, cix)) {
			CPU_STATS_ADDQ(CPU, sys, xcalls, 1);
			cpup->cpu_m.xc_ack = 0;
			cpup->cpu_m.xc_wait = sync;
			if (sync)
				cpup->cpu_m.xc_state = XC_SYNC_OP;
			else
				cpup->cpu_m.xc_state = XC_CALL_OP;
			cpup->cpu_m.xc_pend = 1;
			CPUSET_ADD(cpuset, cix);
		}
	}

	asm volatile ("fence":::"memory");

	/*
	 * Send IPI to requested cpu sets.
	 */
	if (!CPUSET_ISNULL(cpuset)) {
		send_ipi(cpuset, IRQ_IPI_HI);
	}

	/*
	 * Run service locally
	 */
	if (CPU_IN_SET(set, lcx) && func != NULL)
		CPU->cpu_m.xc_retval = (*func)(arg0, arg1, arg2);

	/*
	 * Wait here until all remote calls acknowledge.
	 */
	for (cix = 0; cix < NCPU; cix++) {
		if (lcx != cix && CPU_IN_SET(set, cix)) {
			cpup = cpu[cix];
			while (cpup->cpu_m.xc_ack == 0) {
			}
			cpup->cpu_m.xc_ack = 0;
		}
	}
	asm volatile ("fence":::"memory");

	if (sync == 0)
		return;

	/*
	 * Release any waiting CPUs
	 */
	for (cix = 0; cix < NCPU; cix++) {
		if (lcx != cix && CPU_IN_SET(set, cix)) {
			cpup = cpu[cix];
			if (cpup != NULL && (cpup->cpu_flags & CPU_READY)) {
				cpup->cpu_m.xc_wait = 0;
				cpup->cpu_m.xc_state = XC_DONE;
			}
		}
	}
	asm volatile ("fence":::"memory");

	/*
	 * Wait for all CPUs to acknowledge completion before we continue.
	 * Without this check it's possible (on a VM or hyper-threaded CPUs
	 * or in the presence of Service Management Interrupts which can all
	 * cause delays) for the remote processor to still be waiting by
	 * the time xc_common() is next invoked with the sync flag set
	 * resulting in a deadlock.
	 */
	for (cix = 0; cix < NCPU; cix++) {
		if (lcx != cix && CPU_IN_SET(set, cix)) {
			cpup = cpu[cix];
			if (cpup != NULL && (cpup->cpu_flags & CPU_READY)) {
				while (cpup->cpu_m.xc_ack == 0) {
				}
				cpup->cpu_m.xc_ack = 0;
			}
		}
	}
	asm volatile ("fence":::"memory");
}

void
xc_call(
	xc_arg_t arg0,
	xc_arg_t arg1,
	xc_arg_t arg2,
	cpuset_t set,
	xc_func_t func)
{
	mutex_enter(&xc_mbox_lock);
	xc_common(func, arg0, arg1, arg2, set, 0);
	mutex_exit(&xc_mbox_lock);
}

void
xc_sync(
	xc_arg_t arg0,
	xc_arg_t arg1,
	xc_arg_t arg2,
	cpuset_t set,
	xc_func_t func)
{
	mutex_enter(&xc_mbox_lock);
	xc_common(func, arg0, arg1, arg2, set, 1);
	mutex_exit(&xc_mbox_lock);
}

