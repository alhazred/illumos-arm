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
#include <sys/pal.h>
#include <sys/errno.h>
#include "assym.h"


	ENTRY(thread_start)
	ldq	a1, 2*8(sp)
	ldq	a0, 1*8(sp)
	ldq	pv, 0*8(sp)
	jsr	ra, (pv)
	LDGP(ra)
	CALL(thread_exit)
	SET_SIZE(thread_start)

/*
 * void
 * resume_from_intr(kthread_t *t)
 */
	ENTRY(resume_from_intr)
	br	pv, 0f
0:
	LDGP(pv)
	call_pal PAL_rdval			// v0 = CPU
	ldq	t0, CPU_THREAD(v0)		// t0 = curthread
	stq	s0, T_LABEL_S0(t0)
	stq	s1, T_LABEL_S1(t0)
	stq	s2, T_LABEL_S2(t0)
	stq	s3, T_LABEL_S3(t0)
	stq	s4, T_LABEL_S4(t0)
	stq	s5, T_LABEL_S5(t0)
	stq	s6, T_LABEL_S6(t0)
	stq	sp, T_LABEL_SP(t0)
	stq	ra, T_LABEL_PC(t0)

	mov	a0, s0				// s0 = new thread
	mov	v0, s1				// s1 = CPU
	mov	t0, s2				// s2 = old thread

	CALL(__dtrace_probe___sched_off__cpu)

	stq	s0, CPU_THREAD(s1)		// curthread = new thread
	ldq	sp, T_LABEL_SP(s0)		// switch sp

	/*
	 * Unlock outgoing thread's mutex dispatched by another processor.
	 */
	mb
	lda	a0, T_LOCK(s2)
	andnot	a0, 0x7, t0
.resume_from_intr_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1	// clear to 0
	stq_c	t1, 0(t0)
	beq	t1, .resume_from_intr_retry

	ldwu	a0, T_FLAG(s0)
	and	a0, T_INTR_THREAD, a0
	beq	a0, 1f
	CALL(tsc_read)
	stq	v0, T_INTR_START(s0)
1:
	CALL(__dtrace_probe___sched_on__cpu)

	// lock flagは必ずクリアされている。

	mov	s0, t0
	ldq	s0, T_LABEL_S0(t0)
	ldq	s1, T_LABEL_S1(t0)
	ldq	s2, T_LABEL_S2(t0)
	ldq	s3, T_LABEL_S3(t0)
	ldq	s4, T_LABEL_S4(t0)
	ldq	s5, T_LABEL_S5(t0)
	ldq	s6, T_LABEL_S6(t0)
	ldq	ra, T_LABEL_PC(t0)
	lda	pv, spl0
	jsr	zero, (pv)
	/* must never reach here! */
	SET_SIZE(resume_from_intr)


	ENTRY(resume_from_zombie)
	br	pv, 0f
0:
	LDGP(pv)
	call_pal PAL_rdval			// v0 = CPU
	ldq	t0, CPU_THREAD(v0)		// t0 = curthread
	stq	s0, T_LABEL_S0(t0)
	stq	s1, T_LABEL_S1(t0)
	stq	s2, T_LABEL_S2(t0)
	stq	s3, T_LABEL_S3(t0)
	stq	s4, T_LABEL_S4(t0)
	stq	s5, T_LABEL_S5(t0)
	stq	s6, T_LABEL_S6(t0)
	stq	sp, T_LABEL_SP(t0)
	stq	ra, T_LABEL_PC(t0)

	mov	a0, s0				// s0 = new thread
	mov	v0, s1				// s1 = CPU
	mov	t0, s2				// s2 = old thread

	CALL(__dtrace_probe___sched_off__cpu)

	ldq	t0, CPU_IDLE_THREAD(s1)		// t0 = idle thread
	stq	t0, CPU_THREAD(s1)		// curthread = idle thread
	ldq	sp, T_LABEL_SP(t0)		// switch sp

	mov	s0, a0
	CALL(hat_thread_switch)

	/* 
	 * Put the zombie on death-row.
	 */
	mov	s2, a0
	CALL(reapq_add)

	jsr	zero, _resume_from_idle		// finish job of resume
	SET_SIZE(resume_from_zombie)


	ENTRY(resume)
	br	pv, 0f
0:
	LDGP(pv)
	call_pal PAL_rdval			// v0 = CPU
	ldq	t0, CPU_THREAD(v0)		// t0 = curthread
	stq	s0, T_LABEL_S0(t0)
	stq	s1, T_LABEL_S1(t0)
	stq	s2, T_LABEL_S2(t0)
	stq	s3, T_LABEL_S3(t0)
	stq	s4, T_LABEL_S4(t0)
	stq	s5, T_LABEL_S5(t0)
	stq	s6, T_LABEL_S6(t0)
	stq	sp, T_LABEL_SP(t0)
	stq	ra, T_LABEL_PC(t0)

	mov	a0, s0				// s0 = new thread
	mov	v0, s1				// s1 = CPU
	mov	t0, s2				// s2 = old thread

	CALL(__dtrace_probe___sched_off__cpu)

	/*
	 * Call savectx if thread has installed context ops.
	 *
	 * Note that if we have floating point context, the save op
	 * (either fpsave_begin or fpxsave_begin) will issue the
	 * async save instruction (fnsave or fxsave respectively)
	 * that we fwait for below.
	 */
	ldq	t0, T_CTX(s2)
	beq	t0, .nosavectx
	mov	s2, a0
	CALL(savectx)
.nosavectx:

	ldq	s3, T_PROCP(s2)
	ldq	t0, P_PCTX(s3)
	beq	t0, .nosavepctx
	mov	s3, a0
	CALL(savepctx)
.nosavepctx:

	ldq	t0, CPU_IDLE_THREAD(s1)		// t0 = idle thread
	stq	t0, CPU_THREAD(s1)		// curthread = idle thread
	ldq	sp, T_LABEL_SP(t0)		// switch sp

	mov	s0, a0
	CALL(hat_thread_switch)

	mb
	lda	a0, T_LOCK(s2)
	andnot	a0, 0x7, t0
.resume_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1	// clear to 0
	stq_c	t1, 0(t0)
	beq	t1, .resume_retry

	ALTENTRY(_resume_from_idle)
	// s0 = new thread
	// s1 = CPU

	lda	a0, T_LOCK(s0)
	mov	LOCK_HELD_VALUE, a1
	insbl	a1, a0, a1
	andnot	a0, 0x7, t0
.lock_set_retry:
	ldq_l	t2, 0(t0)
	extbl	t2, a0, t3
	bne	t3, .lock_set_retry
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, .lock_set_retry
	mb

	/*
	 * Fix CPU structure to indicate new running thread.
	 * Set pointer in new thread to the CPU structure.
	 */
	ldq	a0, T_CPU(s0)
	cmpeq	a0, s1, t0
	bne	t0, .setup_cpu
	ldq	t0, CPU_STATS_SYS_CPUMIGRATE(s1)
	addq	t0, 1, t0
	stq	t0, CPU_STATS_SYS_CPUMIGRATE(s1)
	stq	s1, T_CPU(s0)
.setup_cpu:

	stq	s0, CPU_THREAD(s1)		// set CPU's thread pointer
	ldq	t0, T_LWP(s0)
	stq	t0, CPU_LWP(s1)			// set CPU's lwp ptr
	ldq	sp, T_LABEL_SP(s0)		// switch sp

	ldq	t0, T_CTX(s0)
	beq	t0, .norestorectx
	mov	s0, a0
	CALL(restorectx)
.norestorectx:

	ldq	s3, T_PROCP(s0)
	ldq	t0, P_PCTX(s3)
	beq	t0, .norestorepctx
	mov	s3, a0
	CALL(restorepctx)
.norestorepctx:

	ldwu	a0, T_FLAG(s0)
	and	a0, T_INTR_THREAD, a0
	beq	a0, 1f
	CALL(tsc_read)
	stq	v0, T_INTR_START(s0)
1:
	CALL(__dtrace_probe___sched_on__cpu)

	// lock flagは必ずクリアされている。

	mov	s0, t0
	ldq	s0, T_LABEL_S0(t0)
	ldq	s1, T_LABEL_S1(t0)
	ldq	s2, T_LABEL_S2(t0)
	ldq	s3, T_LABEL_S3(t0)
	ldq	s4, T_LABEL_S4(t0)
	ldq	s5, T_LABEL_S5(t0)
	ldq	s6, T_LABEL_S6(t0)
	ldq	ra, T_LABEL_PC(t0)
	lda	pv, spl0
	jsr	zero, (pv)
	SET_SIZE(resume)
