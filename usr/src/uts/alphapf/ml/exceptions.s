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
#include <sys/pal.h>
#include <sys/machparam.h>
#include <sys/privregs.h>
#include "assym.h"

	.set noreorder
	.set volatile
	.set noat

	ENTRY(entSYS)
	blt	v0, fasttrap_entry
	__SAVE_SYSCALL_REGS
	br	pv, 0f
0:	LDGP(pv)

	mov	sp, a0
	/* call dosyscall(struct regs *regs) */
	CALL(dosyscall)

	mov	6, a0
	call_pal PAL_swpipl			// disable interrupt

	mov	sp, a0
	CALL(sys_rtt_common)
	ldq	t0, REGOFF_UPDATE(sp)
	bne	t0, 1f
	__RESTORE_SYSCALL_REGS
	call_pal PAL_retsys
1:
	__RESTORE_REGS
	call_pal PAL_rti
	SET_SIZE(entSYS)

	ENTRY(lwp_rtt_initial)
	br	pv, 0f
0:	LDGP(pv)
	call_pal PAL_rdval			// v0 = CPU
	ldq	a0, CPU_THREAD(v0)		// a0 = curthread
	ldq	sp, T_STACK(a0)			// switch stack
	CALL(__dtrace_probe___proc_start)
	br	zero, 2f

	ALTENTRY(lwp_rtt)
	br	pv, 1f
1:	LDGP(pv)
	call_pal PAL_rdval			// v0 = CPU
	ldq	a0, CPU_THREAD(v0)		// a0 = curthread
	ldq	sp, T_STACK(a0)			// switch stack
2:
	CALL(__dtrace_probe___proc_lwp__start)
	CALL(dtrace_systrace_rtt)
	ldq	a0, REGOFF_V0(sp)
	ldq	a1, REGOFF_T2(sp)
	CALL(post_syscall)

	mov	6, a0
	call_pal PAL_swpipl			// disable interrupt

	mov	sp, a0
	CALL(sys_rtt_common)

	__RESTORE_REGS
	call_pal PAL_rti
	SET_SIZE(lwp_rtt)
	SET_SIZE(lwp_rtt_initial)

	ENTRY(_sys_rtt)
	br	pv, 1f
1:	LDGP(pv)
	mov	6, a0
	call_pal PAL_swpipl			// disable interrupt

	mov	sp, a0
	CALL(sys_rtt_common)

	__RESTORE_REGS
	call_pal PAL_rti
	SET_SIZE(_sys_rtt)

	ENTRY(fakesoftint)
	mov	6, a0
	call_pal PAL_swpipl			// disable interrupt
	lda	sp, -6*8(sp)
	stq	v0, 0*8(sp)
	stq	ra, 1*8(sp)
	mov	T_INT_SFT, a0
	br	zero, entInt
	SET_SIZE(fakesoftint)

	ENTRY(entInt)
	__SAVE_REGS
	br	pv, 1f
1:	LDGP(pv)
	mov	sp, a3
	CALL(do_interrupt)		// disable interrupt in do_interrupt
	br	zero, _sys_rtt
	SET_SIZE(entInt)

	ENTRY(entArith)
	__SAVE_REGS
	br	pv, 1f
1:	LDGP(pv)
	mov	T_ARITH, a3
	mov	sp, a4
	CALL(trap)
	br	zero, _sys_rtt
	SET_SIZE(entArith)

	ENTRY(entIF)
	beq	a0, jmp_debugger
	__SAVE_REGS
	br	pv, 1f
1:	LDGP(pv)
	mov	T_FAULT, a3
	mov	sp, a4
	CALL(trap)
	br	zero, _sys_rtt
jmp_debugger:
	br	a1, 1f
1:	LDGP(a1)
	lda	a1, XentIF
	jmp	zero, (a1)
	SET_SIZE(entIF)

	ENTRY(entMM)
	__SAVE_REGS
	br	pv, 1f
1:	LDGP(pv)
	mov	sp, a3
	CALL(memory_management_fault)
	br	zero, _sys_rtt
	SET_SIZE(entMM)

	ENTRY(entUna)
	__SAVE_REGS
	br	pv, 1f
1:	LDGP(pv)
	mov	sp, a3
	CALL(unaligned_fault)
	br	zero, _sys_rtt
	SET_SIZE(entUna)

	ENTRY(install_exception)
	LDGP(pv)

	lda	a0, entInt
	ldiq	a1, 0
	call_pal PAL_wrent

	lda	a0, entArith
	ldiq	a1, 1
	call_pal PAL_wrent

	lda	a0, entMM
	ldiq	a1, 2
	call_pal PAL_wrent

	lda	a0, entIF
	ldiq	a1, 3
	call_pal PAL_wrent

	lda	a0, entUna
	ldiq	a1, 4
	call_pal PAL_wrent

	lda	a0, entSYS
	ldiq	a1, 5
	call_pal PAL_wrent
	ret
	SET_SIZE(install_exception)

	ENTRY(fasttrap_gethrestime)
	LDGP(pv)
	lda	sp, -(8*2 + TIMESPEC_SIZE)(sp)
	stq	ra, 0*8(sp)
	lda	a0, 8*2(sp)
	CALL(gethrestime)
	ldq	v0, (8*2 + TV_SEC)(sp)
	ldq	t0, (8*2 + TV_NSEC)(sp)
	ldq	ra, 0*8(sp)
	lda	sp, (8*2 + TIMESPEC_SIZE)(sp)
	ret
	SET_SIZE(fasttrap_gethrestime)

	ENTRY(getlgrp)
	LDGP(pv)
	call_pal PAL_rdval			// v0 = CPU
	ldq	t1, CPU_THREAD(v0)		// t1 = curthread

	ldq	t2, T_LPL(t1)
	ldl	t0, LPL_LGRPID(t2)		// t0 = t->t_lpl->lpl_lgrpid
	ldq	t2, T_CPU(t1)
	ldl	v0, CPU_ID(t2)			// v0 = t->t_cpu->cpu_id
	ret
	SET_SIZE(getlgrp)

	ENTRY(fasttrap_entry)
	br	pv, 0f
0:	LDGP(pv)
	negq	v0
	lda	sp, -2*8(sp)
	stq	ra, 0*8(sp)

	cmpule	v0,T_LASTFAST,t0
	bne	t0, 1f
	mov	0, v0
1:
	lda	t0, fasttrap_table
	s8addq	v0, t0, t1
	ldq	pv, 0(t1)
	jmp	ra, (pv)

	ldq	ra, 0*8(sp)
	lda	sp, 2*8(sp)
	call_pal PAL_retsys
	SET_SIZE(fasttrap_entry)
