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
 */
#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include <sys/hwrpb.h>
#include "assym.h"

	.text
	.set    noreorder
	.global dbg_start

//#define GDB_DEBUG

/*
 * void _start(void)
 */
	ENTRY(_start)
	br	pv, 1f
1:
	LDGP(pv)

	/* clear _edata - _ebss */
	lda	a0, _edata
	lda	a1, _ebss
	subq	a1, a0, a1
	CALL(bzero)

	CALL(init_pal)
	lda	sp, _BootStackTop

#ifdef GDB_DEBUG
	CALL(init_dbg)
dbg_start:
	call_pal PAL_bpt
#else
dbg_start:
#endif

	CALL(main)
	call_pal PAL_halt
	SET_SIZE(_start)

	ENTRY(swap_pal)
	lda	sp, -8*8(sp)
	stq	s0, 8*0(sp)
	stq	s1, 8*1(sp)
	stq	s2, 8*2(sp)
	stq	s3, 8*3(sp)
	stq	s4, 8*4(sp)
	stq	s5, 8*5(sp)
	stq	s6, 8*6(sp)
	stq	ra, 8*7(sp)

	stq	sp, 0(a4)
	br	a1, 1f
	br	2f
1:	call_pal PAL_swppal
2:	ldq	s0, 8*0(sp)
	ldq	s1, 8*1(sp)
	ldq	s2, 8*2(sp)
	ldq	s3, 8*3(sp)
	ldq	s4, 8*4(sp)
	ldq	s5, 8*5(sp)
	ldq	s6, 8*6(sp)
	ldq	ra, 8*7(sp)
	lda	sp, 8*8(sp)
	ret
	SET_SIZE(swap_pal)

	ENTRY_NP(__tbia)
	call_pal PAL_tbi
	ret
	SET_SIZE(__tbia)

	ENTRY_NP(pal_wrvptptr)
	call_pal PAL_wrvptptr
	ret
	SET_SIZE(pal_wrvptptr)
