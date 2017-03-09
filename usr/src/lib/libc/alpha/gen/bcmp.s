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

	ENTRY(bcmp)
	addl	a2, 0, a2
	xor	a0, a1, t0
	and	t0, 7, t0
	ble	a2, 13f
	ldq_u	t2, 0(a0)
	and	a1, 7, t1
	ldq_u	t3, 0(a1)
	bne	t0, 5f
	bne	t1, 4f

	.align 4
1:	subq	a2, 1, t5
	bic	t5, 7, t5
	subq	a2, t5, a2
	beq	t5, 3f

	.align 4
2:	xor	t2, t3, t0
	ldq_u	t2, 8(a0)
	lda	a0, 8(a0)
	ldq_u	t3, 8(a1)
	subq	t5, 8, t5
	bne	t0, 12f
	lda	a1, 8(a1)
	bne	t5, 2b

	.align 4
3:	xor	t2, t3, v0
	extqh	v0, a2, v0
	cmovne	v0, 1, v0
	ret

	.align	4
4:	mskqh	t2, a0, t2
	mskqh	t3, a0, t3
	addq	a2, t1, a2
	br	zero, 1b

	.align	4
5:	cmpule	a2, 8, t0
	addq	a0, a2, a3
	bne	t0, 11f
	bne	t1, 10f

	.align	4
6:	subq	a2, 1, t5
	bic	t5, 7, t5
	subq	a2, t5, a2
	beq	t5, 8f

	.align	4
7:	ldq_u	t4, 7(a0)
	lda	a0, 8(a0)
	extql	t2, a0, t0
	extqh	t4, a0, t1
	or	t0, t1, t0
	xor	t0, t3, t0
	ldq	t3, 8(a1)
	subq	t5, 8, t5
	bne	t0, 12f
	lda	a1, 8(a1)
	beq	t5, 9f
	ldq_u	t2, 7(a0)
	lda	a0, 8(a0)
	extql	t4, a0, t0
	extqh	t2, a0, t1
	or	t0, t1, t0
	xor	t0, t3, t0
	ldq	t3, 8(a1)
	subq	t5, 8, t5
	bne	t0, 12f
	lda	a1, 8(a1)
	bne	t5, 7b

	.align	4
8:	mov	t2, t4
9:	ldq_u	t2, -1(a3)
	extql	t4, a0, t4
	extqh	t2, a0, t2
	or	t2, t4, t2
	xor	t2, t3, v0
	extqh	v0, a2, v0
	cmovne	v0, 1, v0
	ret

	.align	4
10:	subq	zero, a1, t0
	and	t0, 7, t0
	ldq_u	t4, 7(a0)
	extql	t2, a0, t2
	extqh	t4, a0, t4
	or	t2, t4, t2
	insql	t2, a1, t2
	mskqh	t3, a1, t3
	xor	t2, t3, t2
	ldq_u	t3, 8(a1)
	addq	a0, t0, a0
	bne	t2, 12f
	addq	a1, t0, a1
	subq	a2, t0, a2
	ldq_u	t2, 0(a0)
	br	zero, 6b

	.align	4
11:	ldq_u	t4, -1(a3)
	extql	t2, a0, t2
	extqh	t4, a0, t4
	or	t2, t4, t2
	addq	a1, a2, a4
	ldq_u	t5, -1(a4)
	extql	t3, a1, t3
	extqh	t5, a1, t5
	or	t3, t5, t3
	xor	t2, t3, t2
	extqh	t2, a2, v0
	cmovne	v0, 1, v0
	ret

	.align	4
12:	lda	v0, 1
	ret

	.align	4
13:	lda	v0, 0
	ret

	SET_SIZE(bcmp)

	.weak _bcmp
_bcmp = bcmp
