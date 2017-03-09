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

#include <sys/asm_linkage.h>
#include <sys/errno.h>
#include <sys/privregs.h>
#include <sys/trap.h>
#include "assym.h"

#define	SYSENT_SHIFT	5
#if (1 << SYSENT_SHIFT) != SYSENT_SIZE
#error invalid
#endif

	/*
	 * user mode:
	 *    sscratch: sp
	 * supervisor mode:
	 *    sscratch: 0
	 */

	ENTRY(trap_entry)
	csrrw	sp, sscratch, sp
	beqz	sp, 2f

	// user mode
	//	+----+
	//	| gp |
	//	+----+
	//	| tp |
	//	+----+ <- sp
	__SAVE_USER_REGS(t6)
	ld	tp, REG_FRAME + (0 * 8)(sp)
	ld	gp, REG_FRAME + (1 * 8)(sp)

	csrr	t6, scause
	srli	t5, t6, 63
	beqz	t5, 1f
	tail	irq_handler
1:	li	t5, 8
	beq	t5, t6, svc_handler
	la	ra, _user_rtt
	j	4f

	// supervisor mode
2:
	__SAVE_SV_REGS(t6)

	csrr	t6, scause
	srli	t5, t6, 63
	beqz	t5, 3f
	tail	irq_handler
3:	la	ra, _sys_rtt
4:	mv	a0, t6
	csrr	a1, stval
	mv	a2, sp
	mv	fp, zero
	csrsi	sstatus, SSR_SIE
	tail	trap
	SET_SIZE(trap_entry)

	/*
	 * System call Handler
	 */
	ENTRY(svc_handler)
	ld	t6, REGOFF_PC(sp)
	addi	t6, t6, 4
	sd	t6, REGOFF_PC(sp)
	mv	s1, t0	// s1 <- syscall number
	srli	t6, s1, 15
	bnez	t6, _fasttrap

	ld	t0, T_CPU(tp)

	// cpu_stats.sys.syscall++
	ld	t1, CPU_STATS_SYS_SYSCALL(t0)
	addi	t1, t1, 1
	sd	t1, CPU_STATS_SYS_SYSCALL(t0)

	ld	t0, T_LWP(tp)	// t1 <- lwp

	// lwp->lwp_state = LWP_SYS
	li	t1, LWP_SYS
	sb	t1, LWP_STATE(t0)

	// lwp->lwp_ru.sysc++
	ld	t1, LWP_RU_SYSC(t0)
	addi	t1, t1, 1
	sd	t1, LWP_RU_SYSC(t0)

	csrsi	sstatus, SSR_SIE

	// syscall_mstate(LMS_USER, LMS_SYSTEM);
	li	a0, LMS_USER
	li	a1, LMS_SYSTEM
	call	syscall_mstate

	sh	s1, T_SYSNUM(tp)
	lb	t0, T_PRE_SYS(tp)
	bnez	t0, _syscall_pre

_syscall_invoke:
	la	t0, sysent
	li	t1, NSYSCALL
	bgeu	s1, t1, _syscall_ill

	slli	s1, s1, SYSENT_SHIFT
	add	s1, t0, s1	// s1 <- sysent
	ld	t0, SY_CALLC(s1)
	ld	a0, REGOFF_A0(sp)
	ld	a1, REGOFF_A1(sp)
	ld	a2, REGOFF_A2(sp)
	ld	a3, REGOFF_A3(sp)
	ld	a4, REGOFF_A4(sp)
	ld	a5, REGOFF_A5(sp)
	ld	a6, REGOFF_A6(sp)
	ld	a7, REGOFF_A7(sp)

	jalr	t0
	li	a1, 0

	lh	t0, SY_FLAGS(s1)
	andi	t0, t0, SE_32RVAL2
	beqz	t0, 2f
	srai	a1, a0, 0x20
	sext.w	a0, a0
2:
	mv	s4, a0
	mv	s5, a1

	// syscall_mstate(LMS_SYSTEM, LMS_USER);
	li	a0, LMS_SYSTEM
	li	a1, LMS_USER
	call	syscall_mstate

	csrci	sstatus, SSR_SIE

	lw	t0, T_POST_SYS_AST(tp)
	bnez	t0, _syscall_post

	sd	zero, REGOFF_T0(sp)

	ld	t0, T_LWP(tp)
	li	t1, LWP_USER
	sb	t1, LWP_STATE(t0)
	sd	s4, REGOFF_A0(sp)
	sd	s5, REGOFF_A1(sp)
	sh	zero, T_SYSNUM(tp)
	__RESTORE_USER_REGS(t6)
	sret

_user_rtt:
	csrci	sstatus, SSR_SIE

	lb	t6, T_ASTFLAG(tp)
	bnez	t6, 3f
	__RESTORE_USER_REGS(t6)
	sret

3:	li	a0, T_AST
	li	a1, 0
	mv	a2, sp
	la	ra, _user_rtt
	csrsi	sstatus, SSR_SIE
	tail	trap

_syscall_pre:
	call	pre_syscall
	mv	s4, a0
	mv	s5, zero
	bnez	a0, _syscall_post_call
	lh	s1, T_SYSNUM(tp)
	j	_syscall_invoke

_syscall_ill:
	call	nosys
	mv	s4, a0
	mv	s5, zero
	j	_syscall_post_call

_syscall_post:
	csrsi	sstatus, SSR_SIE

	// syscall_mstate(LMS_USER, LMS_SYSTEM);
	li	a0, LMS_USER
	li	a1, LMS_SYSTEM
	call	syscall_mstate
_syscall_post_call:
	mv	a0, s4
	mv	a1, s5
	call	post_syscall

	// syscall_mstate(LMS_SYSTEM, LMS_USER);
	li	a0, LMS_SYSTEM
	li	a1, LMS_USER
	call	syscall_mstate
	tail	_sys_rtt
	SET_SIZE(_user_rtt)

_fasttrap:
	slli	s1, s1, 49
	srli	s1, s1, 49
	li	t0, T_LASTFAST
	bltu	s1, t0, 1f
	li	s1, 0
1:	la	t0, fasttrap_table
	slli	s1, s1, 3
	add	t0, t0, s1
	ld	t0, 0(t0)

	ld	a0, REGOFF_A0(sp)
	ld	a1, REGOFF_A1(sp)
	ld	a2, REGOFF_A2(sp)
	ld	a3, REGOFF_A3(sp)

	csrsi	sstatus, SSR_SIE
	jalr	t0
	csrci	sstatus, SSR_SIE

	//	sepc, sstatus, sscratch
	ld	t0, REGOFF_PC(sp)
	csrw	sepc, t0
	ld	t0, REGOFF_SSR(sp)
	csrw	sstatus, t0
	ld	t0, REGOFF_SP(sp)
	csrw	sscratch, t0
	ld	gp, REGOFF_GP(sp)
	ld	tp, REGOFF_TP(sp)
	ld	s1, REGOFF_S1(sp)
	ld	ra, REGOFF_RA(sp)
	addi	sp, sp, REG_FRAME
	csrrw	sp, sscratch, sp
	sret
	SET_SIZE(svc_handler)

	ENTRY(getlgrp)
	ld	a3, T_LPL(tp)
	lw	a1, LPL_LGRPID(a3)	/* a1 = t->t_lpl->lpl_lgrpid */
	ld	a3, T_CPU(tp)
	lw	a0, CPU_ID(a3)		/* a0 = t->t_cpu->cpu_id */
	ret
	SET_SIZE(getlgrp)

	ENTRY(fasttrap_gethrestime)
	addi	sp, sp, -(16 + TIMESPEC_SIZE)
	sd	ra, TIMESPEC_SIZE(sp)

	mv	a0, sp
	call	gethrestime
	ld	a0, TV_SEC(sp)
	ld	a1, TV_NSEC(sp)

	ld	ra, TIMESPEC_SIZE(sp)
	addi	sp, sp, (16 + TIMESPEC_SIZE)
	ret
	SET_SIZE(fasttrap_gethrestime)

	ENTRY(_sys_rtt)
	ld	t0, REGOFF_SSR(sp)
	andi	t0, t0, SSR_SPP
	beqz	t0, _user_rtt

	csrci	sstatus, SSR_SIE

	ld	s3, T_CPU(tp)

	lb	a0, CPU_KPRUNRUN(s3)
	bnez	a0, _sys_rtt_preempt

_sys_rtt_preempt_ret:
	ld	a0, REGOFF_PC(sp)

	la	a1, mutex_owner_running_critical_start
	la	a2, mutex_owner_running_critical_size
	ld	a2, 0(a2)
	sub	a3, a0, a1
	bltu	a3, a2, 2f
1:
	__RESTORE_SV_REGS(t0)
	sret

2:	sd	a1, REGOFF_PC(sp)
	j	1b

_sys_rtt_preempt:
	lb	t0, T_PREEMPT_LK(tp)
	bnez	t0, _sys_rtt_preempt_ret
	li	t0, 1
	sb	t0, T_PREEMPT_LK(tp)

	csrsi	sstatus, SSR_SIE
	call	kpreempt
	csrci	sstatus, SSR_SIE

	sb	zero, T_PREEMPT_LK(tp)
	tail	_sys_rtt_preempt_ret
	SET_SIZE(_sys_rtt)

	/*
	 * IRQ Handler
	 */
	// t6 <- cause
	ENTRY(irq_handler)
	ld	s0, T_CPU(tp)			// s0 <- CPU
	lw	s1, CPU_PRI(s0)			// s1 <- old pri
	mv	a0, t6
	call	get_irq
	blt	a0, zero, _sys_rtt
	mv	s2, a0				// s2 <- irq

	// irq mask and eoi
	call	mask_irq

	// set pri
	mv	a0, s2
	call	setlvl
	bnez	a0, 1f

	// ipl == 0
	mv	a0, s2
	call	unmask_irq
	j	check_softint

1:
	mv	s3, a0				// s3 <- new pri
	sw	s3, CPU_PRI(s0)

	li	t0, LOCK_LEVEL
	ble	s3, t0, intr_thread

	//	s0: CPU
	//	s1: old pri
	//	s3: new pri
	//	s2: irq

	mv	a0, s0
	mv	a1, s3
	mv	a2, s1
	mv	a3, sp
	mv	a4, s2
	call	hilevel_intr_prolog
	bnez	a0, 2f

	mv	s4, sp				// s4 <- old sp
	ld	sp, CPU_INTR_STACK(s0)

	/* Dispatch interrupt handler. */
	csrsi	sstatus, SSR_SIE
	mv	a0, s2
	call	av_dispatch_autovect
	csrci	sstatus, SSR_SIE
	mv	sp, s4
	j	3f
2:
	/* Dispatch interrupt handler. */
	csrsi	sstatus, SSR_SIE
	mv	a0, s2
	call	av_dispatch_autovect
	csrci	sstatus, SSR_SIE

3:
	mv	a0, s0
	mv	a1, s3
	mv	a2, s1
	mv	a3, s2
	call	hilevel_intr_epilog
	bnez	a0, check_softint

	j	check_softint

intr_thread:
	//	s0: CPU
	//	s1: old pri
	//	s3: new pri
	//	s2: irq

	mv	a0, s0
	mv	a1, sp
	mv	a2, s3
	call	intr_thread_prolog
	//	tp is changed in intr_thread_prolog
	mv	s4, sp				// s4 <- old sp
	mv	sp, a0

	/* Dispatch interrupt handler. */
	csrsi	sstatus, SSR_SIE
	mv	a0, s2
	call	av_dispatch_autovect
	csrci	sstatus, SSR_SIE

	mv	a0, s0
	mv	a1, s2
	mv	a2, s1
	call	intr_thread_epilog

	mv	sp, s4

check_softint:
	lw	s2, CPU_SOFTINFO(s0)
	bnez	s2, dosoftint
	tail	_sys_rtt

dosoftint:
	//	s0: CPU
	//	s1: old pri
	//	s3: new pri
	//	s2: st_pending

	mv	a0, s0
	mv	a1, sp
	mv	a2, s2
	mv	a3, s1
	call	dosoftint_prolog
	beqz	a0, _sys_rtt

	mv	s4, sp				// s4 <- old sp
	mv	sp, a0

	csrsi	sstatus, SSR_SIE
	lb	a0, T_PIL(tp)
	call	av_dispatch_softvect
	csrci	sstatus, SSR_SIE

	mv	a0, s0
	mv	a1, s1
	call	dosoftint_epilog

	mv	sp, s4
	j	check_softint
	SET_SIZE(irq_handler)


	ENTRY(lwp_rtt_initial)
	ld	sp, T_STACK(tp)			// switch stack
	call	__dtrace_probe___proc_start
	j	1f

	ALTENTRY(lwp_rtt)
	ld	sp, T_STACK(tp)			// switch stack
1:
	call	__dtrace_probe___proc_lwp__start
	call	dtrace_systrace_rtt

	sd	tp, REG_FRAME + (0 * 8)(sp)
	sd	gp, REG_FRAME + (1 * 8)(sp)

	ld	a0, REGOFF_A0(sp)
	ld	a1, REGOFF_A1(sp)
	la	ra, _user_rtt
	tail	post_syscall
	SET_SIZE(lwp_rtt)
	SET_SIZE(lwp_rtt_initial)

	.data
	.globl t1stack
	.type t1stack, @object
	.size t1stack, DEFAULTSTKSZ
	.align MMU_PAGESHIFT
t1stack:
	.zero DEFAULTSTKSZ

