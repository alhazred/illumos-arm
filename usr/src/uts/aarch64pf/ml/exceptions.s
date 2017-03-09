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

#define CPUP_THREADP(cp_reg, tp_reg)		\
	   mrs tp_reg, tpidr_el1;		\
	   ldr cp_reg, [tp_reg, #T_CPU]

#define CPUP(cp_reg)			\
	   mrs cp_reg, tpidr_el1;	\
	   ldr cp_reg, [cp_reg, #T_CPU]

#define THREADP(tp_reg)			\
	   mrs tp_reg, tpidr_el1


	.balign	2048
	ENTRY(exception_vector)
	/*
	 * From Current Exception level with SP_EL0
	 */
	.balign	0x80
from_current_el_sp0_sync:
0:	b	0b

	.balign	0x80
from_current_el_sp0_irq:
0:	b	0b

	.balign	0x80
from_current_el_sp0_fiq:
0:	b	0b

	.balign	0x80
from_current_el_sp0_error:
0:	b	0b


	/*
	 * From Current Exception level with SP_ELx
	 */
	.balign	0x80
from_current_el_sync:
	clrex
	__SAVE_REGS
	b	from_current_el_sync_handler

	.balign	0x80
from_current_el_irq:
	clrex
	__SAVE_REGS
	b	irq_handler

	.balign	0x80
from_current_el_fiq:
0:	b	0b

	.balign	0x80
from_current_el_error:
0:	b	0b


	/*
	 * From Lower Exception level using aarch64
	 */
	.balign	0x80
from_lower_el_aarch64_sync:
	clrex
	__SAVE_REGS
	mrs	x1, esr_el1
	lsr	w0, w1, #26
	and	w1, w1, #0x1ffffff
	cmp	w0, #T_SVC
	b.eq	svc_handler

	mrs	x2, far_el1
	mov	x3, sp

	adr	x30, user_rtt
	msr	DAIFClr, #2
	b	trap

	.balign	0x80
from_lower_el_aarch64_irq:
	clrex
	__SAVE_REGS
	b	irq_handler

	.balign	0x80
from_lower_el_aarch64_fiq:
0:	b	0b

	.balign	0x80
from_lower_el_aarch64_error:
0:	b	0b


	/*
	 * From Lower Exception level using aarch32
	 */
	.balign	0x80
from_lower_el_aarch32_sync:
0:	b	0b

	.balign	0x80
from_lower_el_aarch32_irq:
0:	b	0b

	.balign	0x80
from_lower_el_aarch32_fiq:
0:	b	0b

	.balign	0x80
from_lower_el_aarch32_error:
0:	b	0b

	.balign	0x80
	SET_SIZE(exception_vector)

	.globl dtrace_invop_callsite
	ENTRY(from_current_el_sync_handler)
	mrs	x1, esr_el1
	lsr	w0, w1, #26
	and	w1, w1, #0x1ffffff
	mrs	x2, far_el1

	adr	x30, _sys_rtt
	mov	x3, sp
	msr	DAIFClr, #2
	b	trap
	SET_SIZE(from_current_el_sync_handler)

	/*
	 * System call Handler From AArch64 EL0
	 */
	ENTRY(svc_handler)
	// w0: ESR_EL1.EC
	// w1: ESR_EL1.ISS
	mov	w20, w1			// w20 <- syscall number
	cbnz	w20, 1f
	mov	w20, w9

1:	tbnz	w20, #15, _fasttrap	// if (w20 & 0x8000) goto _fasttrap
	CPUP(x0)

	// cpu_stats.sys.syscall++
	ldr	x9, [x0, #CPU_STATS_SYS_SYSCALL]
	add	x9, x9, #1
	str	x9, [x0, #CPU_STATS_SYS_SYSCALL]

	THREADP(x21)	// x21 <- thread
	ldr	x1, [x21, #T_LWP]	// x1 <- lwp

	// lwp->lwp_state = LWP_SYS
	mov	w10, #LWP_SYS
	strb	w10, [x1, #LWP_STATE]

	// lwp->lwp_ru.sysc++
	ldr	x9, [x1, #LWP_RU_SYSC]
	add	x9, x9, #1
	str	x9, [x1, #LWP_RU_SYSC]

	msr	DAIFClr, #2

	// syscall_mstate(LMS_USER, LMS_SYSTEM);
	mov	w0, #LMS_USER
	mov	w1, #LMS_SYSTEM
	bl	syscall_mstate

	strh	w20, [x21, #T_SYSNUM]
	ldrb	w9, [x21, #T_PRE_SYS]
	cbnz	w9, _syscall_pre

_syscall_invoke:
	ldr	x1, =sysent
	cmp	w20, #NSYSCALL
	b.hs	_syscall_ill

	add	x20, x1, x20, lsl #SYSENT_SHIFT	// x20 <- sysent
	ldr	x16, [x20, #SY_CALLC]
	ldp	x0, x1, [sp, #REGOFF_X0]
	ldp	x2, x3, [sp, #REGOFF_X2]
	ldp	x4, x5, [sp, #REGOFF_X4]
	ldp	x6, x7, [sp, #REGOFF_X6]
	blr	x16
	mov	x1, xzr

	ldrh	w9, [x20, #SY_FLAGS]
	ands	w9, w9, #SE_32RVAL2
	b.eq	2f
	lsr	x1, x0, #32
	mov	w0, w0
	mov	w1, w1
	sxtw	x0, w0
	sxtw	x1, w1
2:
	mov	x22, x0
	mov	x23, x1

	// syscall_mstate(LMS_SYSTEM, LMS_USER);
	mov	w0, #LMS_SYSTEM
	mov	w1, #LMS_USER
	bl	syscall_mstate

	msr	DAIFSet, #2

	ldr	w9, [x21, #T_POST_SYS_AST]
	cbnz	w9, _syscall_post

	ldr	x2, [sp, #REGOFF_SPSR]
	bic	x2, x2, #PSR_C
	str	x2, [sp, #REGOFF_SPSR]

	ldr	x2, [x21, #T_LWP]	// x2 <- lwp
	mov	w10, #LWP_USER
	strb	w10, [x2, #LWP_STATE]
	stp	x22, x23, [sp, #REGOFF_X0]
	strh	wzr, [x21, #T_SYSNUM]

	__RESTORE_REGS
	eret

_user_rtt:
	msr	DAIFSet, #2

	ldrb	w9, [x21, #T_ASTFLAG]
	cbnz	w9, 3f

	__RESTORE_REGS
	eret

3:
	mov	x0, #T_AST
	mov	x1, #0
	mov	x2, #0
	mov	x3, sp

	adr	x30, _user_rtt
	msr	DAIFClr, #2
	b	trap

_syscall_pre:
	bl	pre_syscall
	mov	x22, x0
	mov	x23, xzr
	cbnz	w0, _syscall_post_call
	ldrh	w20, [x21, #T_SYSNUM]
	b	_syscall_invoke

_syscall_ill:
	bl	nosys
	mov	x22, x0
	mov	x23, xzr
	b	_syscall_post_call

_syscall_post:
	msr	DAIFClr, #2

	// syscall_mstate(LMS_USER, LMS_SYSTEM);
	mov	w0, #LMS_USER
	mov	w1, #LMS_SYSTEM
	bl	syscall_mstate
_syscall_post_call:
	mov	x0, x22
	mov	x1, x23
	bl	post_syscall

	// syscall_mstate(LMS_SYSTEM, LMS_USER);
	mov	w0, #LMS_SYSTEM
	mov	w1, #LMS_USER
	bl	syscall_mstate
	b	_sys_rtt

	ALTENTRY(user_rtt)
	THREADP(x21)
	b	_user_rtt
	SET_SIZE(_user_rtt)

_fasttrap:
	and	w20, w20, #0x7fff
	cmp	w20, #T_LASTFAST
	b.cc	1f
	mov	w20, #0
1:
	ldr	x8, =fasttrap_table
	lsl	w20, w20, #3
	ldr	x9, [x8, x20]

	ldp	x0, x1, [sp, #REGOFF_X0]
	ldp	x2, x3, [sp, #REGOFF_X2]

	msr	DAIFClr, #2
	blr	x9
	msr	DAIFSet, #2

	ldp	x17, x18, [sp, #REGOFF_PC]
	msr	elr_el1, x17
	msr	spsr_el1, x18
	ldp	x30, x16, [sp, #REGOFF_X30]
	msr	sp_el0, x16
	ldr	x20, [sp, #REGOFF_X20]
	add	sp, sp, #REG_FRAME
	eret

	SET_SIZE(svc_handler)

	ENTRY(getlgrp)
	CPUP_THREADP(x3, x2)

	ldr	x3, [x2, #T_LPL]
	ldr	w1, [x3, #LPL_LGRPID]	/* x1 = t->t_lpl->lpl_lgrpid */
	ldr	x3, [x2, #T_CPU]
	ldr	w0, [x3, #CPU_ID]	/* x0 = t->t_cpu->cpu_id */
	ret
	SET_SIZE(getlgrp)

	ENTRY(fasttrap_gethrestime)
	stp	x29, x30, [sp, #-(8*2)]!
	sub	sp, sp, #TIMESPEC_SIZE

	mov	x0, sp
	bl	gethrestime

	ldr	x0, [sp, #TV_SEC]
	ldr	x1, [sp, #TV_NSEC]

	add	sp, sp, #TIMESPEC_SIZE
	ldp	x29, x30, [sp], #(8*2)
	ret
	SET_SIZE(fasttrap_gethrestime)

	ENTRY(_sys_rtt)
	ldr	x2, [sp, #REGOFF_SPSR]
	and	x2, x2, #PSR_M_MASK
	cmp	x2, #PSR_M_EL0t
	b.eq	user_rtt

	msr	DAIFSet, #2

	CPUP(x21)
	ldrb	w0, [x21, #CPU_KPRUNRUN]
	cbnz	w0, _sys_rtt_preempt

_sys_rtt_preempt_ret:
	ldr	x0, [sp, #REGOFF_PC]

	ldr	x1, = mutex_owner_running_critical_start
	ldr	x2, = mutex_owner_running_critical_size
	ldr	x2, [x2]
	sub	x3, x0, x1
	cmp	x3, x2
	b.lo	2f
1:
	__RESTORE_REGS
	eret

2:	str	x1, [sp, #REGOFF_PC]
	b	1b

_sys_rtt_preempt:
	THREADP(x20)
	ldrb	w0, [x20, #T_PREEMPT_LK]
	cbnz	w0, _sys_rtt_preempt_ret
	mov	w0, #1
	strb	w0, [x20, #T_PREEMPT_LK]

	msr	DAIFClr, #2
	bl	kpreempt
	msr	DAIFSet, #2

	strb	wzr, [x20, #T_PREEMPT_LK]
	b	_sys_rtt_preempt_ret
	SET_SIZE(_sys_rtt)

	/*
	 * IRQ Handler
	 */
	ENTRY(irq_handler)
	CPUP(x19)				// x19 <- CPU
	ldr	w20, [x19, #CPU_PRI]		// x20 <- old pri

	ldr	x21, =gic_cpuif_base		// x21 <- gic_cpuif_base
	ldr	x21, [x21]
	ldr	w23, [x21, #GIC_CPUIF_IAR]	// x23 <- ack
	and	w22, w23, #0x3FF		// x22 <- irq
	cmp	w22, #0x3fc
	b.ge	_sys_rtt

	// irq mask (Clear Enable)
	mov	w0, w22
	bl	gic_mask_level_irq

	// irq ack
	str	w23, [x21, #GIC_CPUIF_EOIR]

	// set pri
	mov	w0, w22
	bl	setlvl
	cbnz	w0, 1f

	// ipl == 0
	mov	w0, w22
	bl	gic_unmask_level_irq
	b	check_softint

1:
	mov	w21, w0				// x21 <- new pri
	str	w21, [x19, #CPU_PRI]

	cmp	w21, #LOCK_LEVEL
	b.le	intr_thread

	//	x19: CPU
	//	x20: old pri
	//	x21: new pri
	//	x22: irq

	mov	x0, x19
	mov	w1, w21
	mov	w2, w20
	mov	x3, sp
	bl	hilevel_intr_prolog
	cbnz	x0, 2f

	mov	x23, sp				// x23 <- old sp
	ldr	x0, [x19, #CPU_INTR_STACK]
	mov	sp, x0
2:

	/* Dispatch interrupt handler. */
	msr	DAIFClr, #2
	mov	w0, w22
	bl	av_dispatch_autovect
	msr	DAIFSet, #2

	mov	x0, x19
	mov	w1, w21
	mov	w2, w20
	mov	w3, w22
	bl	hilevel_intr_epilog
	cbnz	x0, check_softint

	mov	sp, x23
	b	check_softint

intr_thread:
	//	x19: CPU
	//	x20: old pri
	//	x21: new pri
	//	x22: irq

	mov	x0, x19
	mov	x1, sp
	mov	w2, w21
	bl	intr_thread_prolog
	mov	x23, sp				// x23 <- old sp
	mov	sp, x0

	/* Dispatch interrupt handler. */
	msr	DAIFClr, #2
	mov	w0, w22
	bl	av_dispatch_autovect
	msr	DAIFSet, #2

	mov	x0, x19
	mov	x1, x22
	mov	x2, x20
	bl	intr_thread_epilog

	mov	sp, x23

check_softint:
	ldr	w22, [x19, #CPU_SOFTINFO]
	cbnz	w22, dosoftint
	b	_sys_rtt

dosoftint:
	//	x19: CPU
	//	x20: old pri
	//	x21: new pri
	//	x22: st_pending

	mov	x0, x19
	mov	x1, sp
	mov	w2, w22
	mov	w3, w20
	bl	dosoftint_prolog
	cbz	x0, _sys_rtt

	mov	x23, sp				// x23 <- old sp
	mov	sp, x0

	msr	DAIFClr, #2
	THREADP(x0)
	ldrb	w0, [x0, #T_PIL]
	bl	av_dispatch_softvect
	msr	DAIFSet, #2

	mov	x0, x19
	mov	w1, w20
	bl	dosoftint_epilog

	mov	sp, x23
	b	check_softint
	SET_SIZE(irq_handler)





	ENTRY(lwp_rtt_initial)
	THREADP(x0)
	ldr	x9, [x0, #T_STACK]			// switch stack
	mov	sp, x9
	bl	__dtrace_probe___proc_start
	b	1f

	ALTENTRY(lwp_rtt)
	THREADP(x0)
	ldr	x9, [x0, #T_STACK]			// switch stack
	mov	sp, x9
1:
	bl	__dtrace_probe___proc_lwp__start
	bl	dtrace_systrace_rtt

	ldr	x0, [sp, #REGOFF_X0]
	ldr	x1, [sp, #REGOFF_X1]
	adr	x30, user_rtt
	b	post_syscall
	SET_SIZE(lwp_rtt)
	SET_SIZE(lwp_rtt_initial)

	.data
	.globl t1stack
	.type t1stack, @object
	.size t1stack, DEFAULTSTKSZ
	.align MMU_PAGESHIFT
t1stack:
	.zero DEFAULTSTKSZ

	ENTRY(from_current_el_sync_debug)
	msr	tpidrro_el0, x3

	ldr	x3, =t1stack
	stp	x0, x1, [x3, #REGOFF_X0]
	str	x2,     [x3, #REGOFF_X2]
	stp	x4, x5, [x3, #REGOFF_X4]
	stp	x6, x7, [x3, #REGOFF_X6]
	stp	x8, x9, [x3, #REGOFF_X8]
	stp	x10, x11, [x3, #REGOFF_X10]
	stp	x12, x13, [x3, #REGOFF_X12]
	stp	x14, x15, [x3, #REGOFF_X14]
	stp	x16, x17, [x3, #REGOFF_X16]
	stp	x18, x19, [x3, #REGOFF_X18]
	stp	x20, x21, [x3, #REGOFF_X20]
	stp	x22, x23, [x3, #REGOFF_X22]
	stp	x24, x25, [x3, #REGOFF_X24]
	stp	x26, x27, [x3, #REGOFF_X26]
	stp	x28, x29, [x3, #REGOFF_X28]
	mov	x16, sp
	stp	x30, x16, [x3, #REGOFF_X30]
	mrs	x17, elr_el1
	mrs	x18, spsr_el1
	stp	x17, x18, [x3, #REGOFF_PC]
	mrs	x16, tpidrro_el0
	str	x16, [x3, #REGOFF_X3]

	mov	x20, x3
	mrs	x21, far_el1
	mrs	x22, esr_el1
	mrs	x23, elr_el1
	mrs	x24, spsr_el1
	mov	x25, sp

	mrs	x1, esr_el1
	lsr	w0, w1, #26
	and	w1, w1, #0x1ffffff
	mrs	x2, far_el1

	ldr	x4, =t1stack
	ldr	x5, =DEFAULTSTKSZ
	add	x4, x4, x5
	mov	sp, x4
	bl	dump_trap

	mov	x3, x20
	msr	far_el1, x21
	msr	esr_el1, x22
	msr	elr_el1, x23
	msr	spsr_el1, x24
	mov	sp, x25

	ldp	x0, x1, [x3, #REGOFF_X0]
	ldr	x2,     [x3, #REGOFF_X2]
	ldp	x4, x5, [x3, #REGOFF_X4]
	ldp	x6, x7, [x3, #REGOFF_X6]
	ldp	x8, x9, [x3, #REGOFF_X8]
	ldp	x10, x11, [x3, #REGOFF_X10]
	ldp	x12, x13, [x3, #REGOFF_X12]
	ldp	x14, x15, [x3, #REGOFF_X14]
	ldp	x16, x17, [x3, #REGOFF_X16]
	ldp	x18, x19, [x3, #REGOFF_X18]
	ldp	x20, x21, [x3, #REGOFF_X20]
	ldp	x22, x23, [x3, #REGOFF_X22]
	ldp	x24, x25, [x3, #REGOFF_X24]
	ldp	x26, x27, [x3, #REGOFF_X26]
	ldp	x28, x29, [x3, #REGOFF_X28]
	ldr	x30,      [x3, #REGOFF_X30]
	mrs	x3, tpidrro_el0

	__SAVE_REGS
	mrs	x1, esr_el1
	lsr	w0, w1, #26
	and	w1, w1, #0x1ffffff
	mrs	x2, far_el1
	msr	DAIFClr, #2
	mov	x3, sp
	adr	x30, _sys_rtt
	b	trap
	SET_SIZE(from_current_el_sync_debug)
