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

	ENTRY(memset)
	bis	a0, a0, v0
	beq	a2, 7f
	and	a1, 0xff, a1
	subq	zero, a0, t0
	and	t0, 0x7, t0
	beq	a1, 0f
	sll	a1, 8, t1
	or	a1, t1, a1
	sll	a1, 16, t1
	or	a1, t1, a1
	sll	a1, 32, t1
	or	a1, t1, a1

	.align	4
0:	bic	a2, 63, t3
	beq	t0, 1f
	cmpult	a2, t0, t1
	ldq_u	t2, 0(a0)
	insql	a1, a0, t4
	bne	t1, 8f
	subq	a2, t0, a2
	bic	a2, 63, t3
	mskql	t2, a0, t2
	or	t4, t2, t2
	stq_u	t2, 0(a0)
	addq	a0, t0, a0

	.align	4
1:	and	a2, 56, t4
	beq	t3, 3f
2:	stq	a1, 0*8(a0)
	subq	t3, 8*8, t3
	stq	a1, 1*8(a0)
	stq	a1, 2*8(a0)
	stq	a1, 3*8(a0)
	stq	a1, 4*8(a0)
	stq	a1, 5*8(a0)
	stq	a1, 6*8(a0)
	stq	a1, 7*8(a0)
	lda	a0, 8*8(a0)
	bne	t3, 2b

	.align	4
3:	and	a2, 0x7, t5
	beq	t4, 5f
4:	stq	a1, 0(a0)
	subq	t4, 8, t4
	lda	a0, 8(a0)
	bne	t4, 4b

	.align	4
5:	bne	t5, 6f
	ret

	.align	4
6:	ldq	t0, 0(a0)
	mskqh	t0, t5, t0
	mskql	a1, t5, a1
	or	a1, t0, t0
	stq	t0, 0(a0)
7:	ret

	.align	4
8:	lda	t0, -1
	mskql	t0, a2, t0
	insql	t0, a0, t0
	bic	t2, t0, t2
	and	a1, t0, a1
	or	a1, t2, t2
	stq_u	t2, 0(a0)
	ret
	SET_SIZE(memset)

	.weak _memset
_memset = memset

