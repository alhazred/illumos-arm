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
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include "assym.h"

	.set noat
/*
 * init_dbg()
 */
	ENTRY(init_dbg)
	LDGP(pv)
	lda	sp, -8*2(sp)
	stq	ra, 8*0(sp)

	lda	a0, XentIF
	ldiq	a1, 3
	call_pal PAL_wrent
	lda	a0, XentMM_DEBUG
	ldiq	a1, 2
	call_pal PAL_wrent
	CALL(INIT)

	ldq	ra, 8*0(sp)
	lda	sp, 8*2(sp)
	ret
	SET_SIZE(init_dbg)

	ENTRY(XentIF)
	br	a1, 1f
1:
	LDGP(a1)
	lda	a1, gdb_regs

	stq	v0, 8*0(a1)
	stq	t0, 8*1(a1)
	stq	t1, 8*2(a1)
	stq	t2, 8*3(a1)
	stq	t3, 8*4(a1)
	stq	t4, 8*5(a1)
	stq	t5, 8*6(a1)
	stq	t6, 8*7(a1)
	stq	t7, 8*8(a1)
	stq	s0, 8*9(a1)
	stq	s1, 8*10(a1)
	stq	s2, 8*11(a1)
	stq	s3, 8*12(a1)
	stq	s4, 8*13(a1)
	stq	s5, 8*14(a1)
	stq	s6, 8*15(a1)
	stq	a3, 8*19(a1)
	stq	a4, 8*20(a1)
	stq	a5, 8*21(a1)
	stq	t8, 8*22(a1)
	stq	t9, 8*23(a1)
	stq	t10, 8*24(a1)
	stq	t11, 8*25(a1)
	stq	ra, 8*26(a1)
	stq	pv, 8*27(a1)
	stq	AT, 8*28(a1)

	lda	a2, 8*6(sp)	// for SP
	stq	a2, 8*30(a1)

	ldq	a2, 8*0(sp)
	and	a2, 8, a2
	beq	a2, 1f
	call_pal PAL_rdusp
	stq	v0, 8*30(a1)
1:

	stq	zero, 8*31(a1)

	ldq	a2, 8*(1)(sp)	// PC
	subq	a2, 4, a2
	stq	a2, 8*(64)(a1)

	ldq	a2, 8*(2)(sp)	// GP
	stq	a2, 8*(29)(a1)

	ldq	a2, 8*(3)(sp)	// a0
	stq	a2, 8*(16)(a1)

	ldq	a2, 8*(4)(sp)	// a1
	stq	a2, 8*(17)(a1)

	ldq	a2, 8*(5)(sp)	// a2
	stq	a2, 8*(18)(a1)

	beq	a0, 0f
	mov	11, a0
	br	zero, 1f
0:	mov	5, a0
1:
	CALL(handle_exception)
	call_pal PAL_imb

	lda	a1, gdb_regs


	ldq	a2, 8*0(sp)
	and	a2, 8, a2
	beq	a2, 1f
	ldq	a0, 8*30(a1)
	call_pal PAL_wrusp
	br	2f
1:	ldq	a2, 8*30(a1)	// for SP
	lda	sp, -8*6(a2)
2:

	ldq	v0, 8*0(a1)
	ldq	t0, 8*1(a1)
	ldq	t1, 8*2(a1)
	ldq	t2, 8*3(a1)
	ldq	t3, 8*4(a1)
	ldq	t4, 8*5(a1)
	ldq	t5, 8*6(a1)
	ldq	t6, 8*7(a1)
	ldq	t7, 8*8(a1)
	ldq	s0, 8*9(a1)
	ldq	s1, 8*10(a1)
	ldq	s2, 8*11(a1)
	ldq	s3, 8*12(a1)
	ldq	s4, 8*13(a1)
	ldq	s5, 8*14(a1)
	ldq	s6, 8*15(a1)
	ldq	a3, 8*19(a1)
	ldq	a4, 8*20(a1)
	ldq	a5, 8*21(a1)
	ldq	t8, 8*22(a1)
	ldq	t9, 8*23(a1)
	ldq	t10, 8*24(a1)
	ldq	t11, 8*25(a1)
	ldq	ra, 8*26(a1)
	ldq	pv, 8*27(a1)
	ldq	AT, 8*28(a1)

	ldq	a2, 8*(64)(a1)
	stq	a2, 8*(1)(sp)	// PC

	ldq	a2, 8*(29)(a1)
	stq	a2, 8*(2)(sp)	// GP

	ldq	a2, 8*(16)(a1)
	stq	a2, 8*(3)(sp)	// a0

	ldq	a2, 8*(17)(a1)
	stq	a2, 8*(4)(sp)	// a1

	ldq	a2, 8*(18)(a1)
	stq	a2, 8*(5)(sp)	// a2

	call_pal PAL_rti
	SET_SIZE(XentIF)


	ENTRY(XentMM_DEBUG)
	br	a1, 1f
1:
	LDGP(a1)
	lda	a1, gdb_regs

	stq	v0, 8*0(a1)
	stq	t0, 8*1(a1)
	stq	t1, 8*2(a1)
	stq	t2, 8*3(a1)
	stq	t3, 8*4(a1)
	stq	t4, 8*5(a1)
	stq	t5, 8*6(a1)
	stq	t6, 8*7(a1)
	stq	t7, 8*8(a1)
	stq	s0, 8*9(a1)
	stq	s1, 8*10(a1)
	stq	s2, 8*11(a1)
	stq	s3, 8*12(a1)
	stq	s4, 8*13(a1)
	stq	s5, 8*14(a1)
	stq	s6, 8*15(a1)
	stq	a3, 8*19(a1)
	stq	a4, 8*20(a1)
	stq	a5, 8*21(a1)
	stq	t8, 8*22(a1)
	stq	t9, 8*23(a1)
	stq	t10, 8*24(a1)
	stq	t11, 8*25(a1)
	stq	ra, 8*26(a1)
	stq	pv, 8*27(a1)
	stq	AT, 8*28(a1)

	lda	a2, 8*6(sp)	// for SP
	stq	a2, 8*30(a1)

	stq	zero, 8*31(a1)

	ldq	a2, 8*(1)(sp)	// PC
	stq	a2, 8*(64)(a1)

	ldq	a2, 8*(2)(sp)	// GP
	stq	a2, 8*(29)(a1)

	ldq	a2, 8*(3)(sp)	// a0
	stq	a2, 8*(16)(a1)

	ldq	a2, 8*(4)(sp)	// a1
	stq	a2, 8*(17)(a1)

	ldq	a2, 8*(5)(sp)	// a2
	stq	a2, 8*(18)(a1)

	mov	11, a0		// page fault
	CALL(handle_exception)
	call_pal PAL_imb

	lda	a1, gdb_regs

	ldq	v0, 8*0(a1)
	ldq	t0, 8*1(a1)
	ldq	t1, 8*2(a1)
	ldq	t2, 8*3(a1)
	ldq	t3, 8*4(a1)
	ldq	t4, 8*5(a1)
	ldq	t5, 8*6(a1)
	ldq	t6, 8*7(a1)
	ldq	t7, 8*8(a1)
	ldq	s0, 8*9(a1)
	ldq	s1, 8*10(a1)
	ldq	s2, 8*11(a1)
	ldq	s3, 8*12(a1)
	ldq	s4, 8*13(a1)
	ldq	s5, 8*14(a1)
	ldq	s6, 8*15(a1)
	ldq	a3, 8*19(a1)
	ldq	a4, 8*20(a1)
	ldq	a5, 8*21(a1)
	ldq	t8, 8*22(a1)
	ldq	t9, 8*23(a1)
	ldq	t10, 8*24(a1)
	ldq	t11, 8*25(a1)
	ldq	ra, 8*26(a1)
	ldq	pv, 8*27(a1)
	ldq	AT, 8*28(a1)

	ldq	a2, 8*30(a1)	// for SP
	lda	sp, -8*6(a2)

	ldq	a2, 8*(64)(a1)
	stq	a2, 8*(1)(sp)	// PC

	ldq	a2, 8*(29)(a1)
	stq	a2, 8*(2)(sp)	// GP

	ldq	a2, 8*(16)(a1)
	stq	a2, 8*(3)(sp)	// a0

	ldq	a2, 8*(17)(a1)
	stq	a2, 8*(4)(sp)	// a1

	ldq	a2, 8*(18)(a1)
	stq	a2, 8*(5)(sp)	// a2

	call_pal PAL_rti
	SET_SIZE(XentMM_DEBUG)

