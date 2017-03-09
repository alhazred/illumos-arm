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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sys/errno.h>
#include "assym.h"

	/*
	 * +-----------
	 * |  fp (0)
	 * +-----------
	 * |  lr (0)
	 * +----------- <- SP, FP when calling func
	 * |  dummy(0)
	 * +-----------
	 * |  func
	 * +-----------
	 * |  arg1
	 * +-----------
	 * |  arg0
	 * +----------- <- SP
	 */
	ENTRY(thread_start)
	ld	a0, 0 * 8(sp)
	ld	a1, 1 * 8(sp)
	ld	t0, 2 * 8(sp)
	addi	fp, sp, 6 * 8
	addi	sp, sp, 4 * 8

	la	ra, thread_exit
	jr	t0
	SET_SIZE(thread_start)

	ENTRY(resume)
	sd	ra, T_LABEL_PC(tp)
	sd	sp, T_LABEL_SP(tp)
	sd	s0, T_LABEL_S0(tp)
	sd	s1, T_LABEL_S1(tp)
	sd	s2, T_LABEL_S2(tp)
	sd	s3, T_LABEL_S3(tp)
	sd	s4, T_LABEL_S4(tp)
	sd	s5, T_LABEL_S5(tp)
	sd	s6, T_LABEL_S6(tp)
	sd	s7, T_LABEL_S7(tp)
	sd	s8, T_LABEL_S8(tp)
	sd	s9, T_LABEL_S9(tp)
	sd	s10, T_LABEL_S10(tp)
	sd	s11, T_LABEL_S11(tp)

	ld	a1, T_CPU(tp)
	mv	a2, tp

	mv	s0, a0		// s0 <- new thread
	mv	s1, a1		// s1 <- CPU
	mv	s2, a2		// s2 <- curthread

	call	__dtrace_probe___sched_off__cpu

	// Call savectx if thread has installed context ops.
	ld	t0, T_CTX(s2)
	beqz	t0, .nosavectx
	mv	a0, s2
	call	savectx
.nosavectx:

	// Call savepctx if process has installed context ops.
	ld	a0, T_PROCP(s2)
	ld	a1, P_PCTX(a0)
	beqz	a1, .nosavepctx
	call	savepctx
.nosavepctx:

	// Temporarily switch to the idle thread's stack
	ld	a0, CPU_IDLE_THREAD(s1)
	mv	tp, a0
	sd	a0, CPU_THREAD(s1)
	ld	sp, T_LABEL_SP(a0)

	// get hatp of new thread
	ld	a0, T_PROCP(s0)
	ld	a0, P_AS(a0)
	ld	a0, A_HAT(a0)
	call	hat_switch

	// Clear and unlock previous thread's t_lock
	addi	a0, s2, T_LOCK
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xff
	sllw	t1, t1, t0	// t1 = 0xff << (8 * ((uintptr_t)lp & 3));
	not	t1, t1
	andi	a0, a0, -4
	amoand.w.rl t1, t1, (a0)

	ALTENTRY(_resume_from_idle)

	/*
	 * spin until dispatched thread's mutex has
	 * been unlocked. this mutex is unlocked when
	 * it becomes safe for the thread to run.
	 */
	addi	a0, s0, T_LOCK
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, LOCK_HELD_VALUE
	sllw	t1, t1, t0	// t1 = LOCK_HELD_VALUE << (8 * ((uintptr_t)lp & 3));

	andi	a0, a0, -4
1:	amoor.w.aq t2, t1, (a0)
	srlw	t2, t2, t0
	andi	t2, t2, 0xFF
	bnez	t2, 1b

	ld	a0, T_CPU(s0)
	beq	a0, s1, .nocpumigrate

	// cp->cpu_stats.sys.cpumigrate++
	ld	a0, CPU_STATS_SYS_CPUMIGRATE(s1)
	addi	a0, a0, 1
	sd	a0, CPU_STATS_SYS_CPUMIGRATE(s1)
	sd	s1, T_CPU(s0)

.nocpumigrate:
	mv	tp, s0
	sd	s0, CPU_THREAD(s1)
	ld	a0, T_LWP(s0)
	sd	a0, CPU_LWP(s1)


	 // Switch to new thread's stack
	ld	sp, T_LABEL_SP(tp)

	// Call restorectx if thread has installed context ops.
	ld	t0, T_CTX(s0)
	beqz	t0, .norestorectx
	mv	a0, s0
	call	restorectx
.norestorectx:

	// Call restorepctx if process has installed context ops.
	ld	a0, T_PROCP(s0)
	ld	a1, P_PCTX(a0)
	beqz	a1, .norestorepctx
	call	restorepctx
.norestorepctx:

	// store t_intr_start
	lh	a0, T_FLAG(s0)
	andi	a0, a0, T_INTR_THREAD
	beqz	a0, 1f
	rdtime	a0
	sd	a0, T_INTR_START(s0)
1:
	call	__dtrace_probe___sched_on__cpu

	ld	ra, T_LABEL_PC(tp)
	ld	s0, T_LABEL_S0(tp)
	ld	s1, T_LABEL_S1(tp)
	ld	s2, T_LABEL_S2(tp)
	ld	s3, T_LABEL_S3(tp)
	ld	s4, T_LABEL_S4(tp)
	ld	s5, T_LABEL_S5(tp)
	ld	s6, T_LABEL_S6(tp)
	ld	s7, T_LABEL_S7(tp)
	ld	s8, T_LABEL_S8(tp)
	ld	s9, T_LABEL_S9(tp)
	ld	s10, T_LABEL_S10(tp)
	ld	s11, T_LABEL_S11(tp)
	tail	spl0

	SET_SIZE(_resume_from_idle)
	SET_SIZE(resume)


	ENTRY(resume_from_zombie)
	sd	ra, T_LABEL_PC(tp)
	sd	sp, T_LABEL_SP(tp)
	sd	s0, T_LABEL_S0(tp)
	sd	s1, T_LABEL_S1(tp)
	sd	s2, T_LABEL_S2(tp)
	sd	s3, T_LABEL_S3(tp)
	sd	s4, T_LABEL_S4(tp)
	sd	s5, T_LABEL_S5(tp)
	sd	s6, T_LABEL_S6(tp)
	sd	s7, T_LABEL_S7(tp)
	sd	s8, T_LABEL_S8(tp)
	sd	s9, T_LABEL_S9(tp)
	sd	s10, T_LABEL_S10(tp)
	sd	s11, T_LABEL_S11(tp)

	ld	a1, T_CPU(tp)
	mv	a2, tp

	mv	s0, a0		// s0 <- new thread
	mv	s1, a1		// s1 <- CPU
	mv	s2, a2		// s2 <- curthread

	call	__dtrace_probe___sched_off__cpu

	// Disable FPU
	li	a0, SSR_FS_MASK
	csrc	sstatus, a0

	// Temporarily switch to the idle thread's stack
	ld	a0, CPU_IDLE_THREAD(s1)
	mv	tp, a0
	sd	a0, CPU_THREAD(s1)
	ld	sp, T_LABEL_SP(a0)

	// get hatp of new thread
	ld	a0, T_PROCP(s0)
	ld	a0, P_AS(a0)
	ld	a0, A_HAT(a0)
	call	hat_switch

	/*
	 * Put the zombie on death-row.
	 */
	mv	a0, s2
	call	reapq_add

	tail	_resume_from_idle
	SET_SIZE(resume_from_zombie)


	ENTRY(resume_from_intr)
	sd	ra, T_LABEL_PC(tp)
	sd	sp, T_LABEL_SP(tp)
	sd	s0, T_LABEL_S0(tp)
	sd	s1, T_LABEL_S1(tp)
	sd	s2, T_LABEL_S2(tp)
	sd	s3, T_LABEL_S3(tp)
	sd	s4, T_LABEL_S4(tp)
	sd	s5, T_LABEL_S5(tp)
	sd	s6, T_LABEL_S6(tp)
	sd	s7, T_LABEL_S7(tp)
	sd	s8, T_LABEL_S8(tp)
	sd	s9, T_LABEL_S9(tp)
	sd	s10, T_LABEL_S10(tp)
	sd	s11, T_LABEL_S11(tp)

	ld	a1, T_CPU(tp)
	mv	a2, tp

	mv	s0, a0		// s0 <- new thread
	mv	s1, a1		// s1 <- CPU
	mv	s2, a2		// s2 <- curthread

	call	__dtrace_probe___sched_off__cpu

	mv	tp, s0
	sd	s0, CPU_THREAD(s1)
	ld	sp, T_LABEL_SP(s0)

	addi	a0, s2, T_LOCK
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xff
	sllw	t1, t1, t0	// t1 = 0xff << (8 * ((uintptr_t)lp & 3));
	not	t1, t1
	andi	a0, a0, -4
	amoand.w.rl t1, t1, (a0)

	// store t_intr_start
	lh	a0, T_FLAG(s0)
	andi	a0, a0, T_INTR_THREAD
	beqz	a0, 1f
	rdtime	a0
	sd	a0, T_INTR_START(s0)
1:
	call	__dtrace_probe___sched_on__cpu

	ld	ra, T_LABEL_PC(tp)
	ld	s0, T_LABEL_S0(tp)
	ld	s1, T_LABEL_S1(tp)
	ld	s2, T_LABEL_S2(tp)
	ld	s3, T_LABEL_S3(tp)
	ld	s4, T_LABEL_S4(tp)
	ld	s5, T_LABEL_S5(tp)
	ld	s6, T_LABEL_S6(tp)
	ld	s7, T_LABEL_S7(tp)
	ld	s8, T_LABEL_S8(tp)
	ld	s9, T_LABEL_S9(tp)
	ld	s10, T_LABEL_S10(tp)
	ld	s11, T_LABEL_S11(tp)
	tail	spl0
	SET_SIZE(resume_from_intr)

