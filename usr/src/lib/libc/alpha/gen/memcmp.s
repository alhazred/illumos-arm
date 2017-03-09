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

	ENTRY(memcmp)
	xor	a0, a1, t0
	beq	a2, 13f
	and	t0, 7, t0
	ldq_u	t2, 0(a0)
	and	a1, 7, t1
	ldq_u	t3, 0(a1)
	bne	t0, 4f
	bne	t1, 3f

	.align	4
0:	subq	a2, 1, t5
	bic	t5, 7, t5
	subq	a2, t5, a2
	beq	t5, 2f

	.align	4
1:	xor	t2, t3, v0
	bne	v0, 12f
	ldq_u	t2, 8(a0)
	lda	a0, 8(a0)
	ldq_u	t3, 8(a1)
	subq	t5, 8, t5
	lda	a1, 8(a1)
	bne	t5, 1b

	.align	4
2:	extqh	t2, a2, t2
	extqh	t3, a2, t3
	xor	t2, t3, v0
	bne	v0, 12f
	ret

	.align	4
3:	mskqh	t2, a0, t2
	mskqh	t3, a0, t3
	addq	a2, t1, a2
	br	zero, 0b

	.align	4
4:	cmpule	a2, 8, t0
	addq	a0, a2, a3
	bne	t0, 10f
	bne	t1, 9f

	.align	4
5:	subq	a2, 1, t5
	bic	t5, 7, t5
	subq	a2, t5, a2
	beq	t5, 7f

	.align	4
6:	ldq_u	t4, 7(a0)
	lda	a0, 8(a0)
	extql	t2, a0, t0
	extqh	t4, a0, t1
	or	t0, t1, t0
	xor	t0, t3, v0
	bne	v0, 11f
	ldq	t3, 8(a1)
	subq	t5, 8, t5
	lda	a1, 8(a1)
	beq	t5, 8f
	ldq_u	t2, 7(a0)
	lda	a0, 8(a0)
	extql	t4, a0, t0
	extqh	t2, a0, t1
	or	t0, t1, t0
	xor	t0, t3, v0
	bne	v0, 11f
	ldq	t3, 8(a1)
	subq	t5, 8, t5
	lda	a1, 8(a1)
	bne	t5, 6b

	.align	4
7:	mov	t2, t4
8:	ldq_u	t2, -1(a3)
	extql	t4, a0, t4
	extqh	t2, a0, t2
	or	t2, t4, t2
	extqh	t2, a2, t2
	extqh	t3, a2, t3
	xor	t2, t3, v0
	bne	v0, 12f
	ret

	.align	4
9:	subq	zero, a1, t0
	and	t0, 7, t0
	ldq_u	t4, 7(a0)
	extql	t2, a0, t2
	extqh	t4, a0, t4
	or	t2, t4, t2
	insql	t2, a1, t2
	mskqh	t3, a1, t3
	xor	t2, t3, v0
	bne	v0, 12f
	ldq_u	t3, 8(a1)
	addq	a0, t0, a0
	addq	a1, t0, a1
	subq	a2, t0, a2
	ldq_u	t2, 0(a0)
	br	zero, 5b

	.align	4
10:	ldq_u	t4, -1(a3)
	extql	t2, a0, t2
	extqh	t4, a0, t4
	or	t2, t4, t2
	addq	a1, a2, a4
	ldq_u	t5, -1(a4)
	extql	t3, a1, t3
	extqh	t5, a1, t5
	or	t3, t5, t3
	extqh	t2, a2, t2
	extqh	t3, a2, t3
	xor	t2, t3, v0
	bne	v0, 12f
	ret

	.align	4
11:	mov	t0, t2
12:	cmpbge	zero, v0, t0
	cmpbge	t2, t3, t1
	subq	t0, 0xff, t4
	andnot	t1, t0, t2
	and	t2, t4, v0
	cmoveq	v0, t4, v0
	ret

	.align	4
13:	lda	v0, 0
	ret
	SET_SIZE(memcmp)

	.weak _memcmp
_memcmp = memcmp

