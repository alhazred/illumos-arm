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

	ENTRY(strlen)
	ldq_u	t0, 0(a0)
	lda	t1, -1
	insqh	t1, a0, t1
	bic	a0, 7, v0
	or	t0, t1, t0
	cmpbge	zero, t0, t1
	bne	t1, 2f

	.align 4
1:	ldq	t0, 8(v0)
	lda	v0, 8(v0)
	cmpbge	zero, t0, t1
	beq	t1, 1b

	.align 4
2:	subq	zero, t1, t2
	blbs	t1, 3f
	and	t1, t2, t1
	and	t1, 0x0f, t2
	addq	v0, 4, t0
	cmoveq	t2, t0, v0
	and	t1, 0x33, t3
	addq	v0, 2, t0
	cmoveq	t3, t0, v0
	and	t1, 0x55, t4
	addq	v0, 1, t0
	cmoveq	t4, t0, v0
3:	subq	v0, a0, v0
	ret
	SET_SIZE(strlen)
