/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 */

#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include <sys/machparam.h>
#include "assym.h"

//#define GDB_DEBUG

#undef zero
#undef t0

	.set noreorder
	.set volatile
	.set noat

	.data
	.globl t0stack
	.type t0stack, @object
	.size t0stack, DEFAULTSTKSZ
	.align MMU_PAGESHIFT
t0stack:
	.zero DEFAULTSTKSZ

	.globl t0
	.type t0, @object
	.size t0, MMU_PAGESIZE
	.align MMU_PAGESHIFT
t0:
	.zero MMU_PAGESIZE

	.globl dbg_start

	ENTRY(_start)
	br	pv, 1f
1:	LDGP(pv)

	mov	a0, s0
	mov	gp, a0
	call_pal PAL_wrkgp
	mov	s0, a0

	mov	a0, s0
#ifdef GDB_DEBUG
	CALL(init_dbg)
	bpt
#endif

	mov	s0, a0
	CALL(kobj_start)

	lda	t1, t0stack
	ldiq	t2, DEFAULTSTKSZ
	addq	t1, t2, sp
	mov	sp, a0

	CALL(mlsetup)

	lda	t1, t0
	ldq	t1, T_LWP(t1)
	stq	sp, LWP_PCB_KSP(t1)
	ldq	a0, LWP_PCB_SELF(t1)
	call_pal PAL_swpctx

	mov	6, a0
	call_pal PAL_swpipl

	CALL(main)

	call_pal PAL_halt
	SET_SIZE(_start)

