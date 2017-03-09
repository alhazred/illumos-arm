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

#include	<link.h>
#include	<sys/asm_linkage.h>

	.file	"boot_elf.s"

	.protected elf_rtbndr

	ENTRY(elf_rtbndr)
	lda	sp, -22*8(sp)
	stq	v0, 0*8(sp)
	stq	t0, 1*8(sp)
	stq	t1, 2*8(sp)
	stq	t2, 3*8(sp)
	stq	t3, 4*8(sp)
	stq	t4, 5*8(sp)
	stq	t5, 6*8(sp)
	stq	t6, 7*8(sp)
	stq	t7, 8*8(sp)
	stq	t8, 9*8(sp)
	stq	t9, 10*8(sp)
	stq	t10, 11*8(sp)
	stq	t11, 12*8(sp)
	stq	a0, 13*8(sp)
	stq	a1, 14*8(sp)
	stq	a2, 15*8(sp)
	stq	a3, 16*8(sp)
	stq	a4, 17*8(sp)
	stq	a5, 18*8(sp)
	stq	gp, 19*8(sp)
	stq	ra, 20*8(sp)

	br	t0, 1f
1:	LDGP(t0)

	mov	t11, a1
	mov	AT, a0
	CALL(elf_bndr)

	mov	v0, pv
	ldq	v0, 0*8(sp)
	ldq	t0, 1*8(sp)
	ldq	t1, 2*8(sp)
	ldq	t2, 3*8(sp)
	ldq	t3, 4*8(sp)
	ldq	t4, 5*8(sp)
	ldq	t5, 6*8(sp)
	ldq	t6, 7*8(sp)
	ldq	t7, 8*8(sp)
	ldq	t8, 9*8(sp)
	ldq	t9, 10*8(sp)
	ldq	t10, 11*8(sp)
	ldq	t11, 12*8(sp)
	ldq	a0, 13*8(sp)
	ldq	a1, 14*8(sp)
	ldq	a2, 15*8(sp)
	ldq	a3, 16*8(sp)
	ldq	a4, 17*8(sp)
	ldq	a5, 18*8(sp)
	ldq	gp, 19*8(sp)
	ldq	ra, 20*8(sp)
	imb
	lda	sp, 22*8(sp)
	jmp	zero, (pv)

	SET_SIZE(elf_rtbndr)
