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

	ENTRY(memchr)
	and	a1, 0xff, a1
	beq	a2, 7f
	ldq_u	t0, 0(a0)
	sll	a1, 8, t2
	negq	a0, t1
	or	a1, t2, a1
	sll	a1, 16, t2
	and	t1, 7, t1
	or	a1, t2, a1
	sll	a1, 32, t2
	or	a1, t2, a1
	bne	t1, 5f

	.align	4
1:	subq	a2, 1, t3
	bic	t3, 7, t3
	subq	a2, t3, a2
	beq	t3, 3f
2:	xor	a1, t0, t0
	cmpbge	zero, t0, t0
	subq	t3, 8, t3
	bne	t0, 4f
	ldq	t0, 8(a0)
	lda	a0, 8(a0)
	bne	t3, 2b

	.align	4
3:	xor	a1, t0, t0
	cmpbge	zero, t0, t0
	lda	t5, 0xff
	sll	t5, a2, t5
	bic	t0, t5, t0
	beq	t0, 7f

	.align	4
4:	negq	t0, t1
	and	t0, t1, t0
	and	t0, 0x0f, t1
	addq	a0, 4, v0
	cmovne	t1, a0, v0
	and	t0, 0x33, t3
	addq	v0, 2, t2
	cmoveq	t3, t2, v0
	and	t0, 0x55, t4
	addq	v0, 1, t2
	cmoveq	t4, t2, v0
	ret

	.align	4
5:	and	a0, 7, t6
	cmpult	a2, t1, t3
	xor	a1, t0, t0
	cmpbge	zero, t0, t0
	bic	a0, 7, a0
	lda	t5, 0xff
	sll	t5, t6, t5
	and	t0, t5, t0
	bne	t3, 6f
	subq	a2, t1, a2
	bne	t0, 4b
	beq	a2, 7f
	ldq	t0, 8(a0)
	lda	a0, 8(a0)
	br	zero, 1b

	.align	4
6:	sll	t5, a2, t5
	andnot	t0, t5, t0
	bne	t0, 4b

	.align	4
7:	lda	v0, 0
	ret

	SET_SIZE(memchr)

	.weak _memchr
_memchr = memchr

