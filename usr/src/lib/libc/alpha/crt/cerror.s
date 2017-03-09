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

	.file	"cerror.s"

#include <SYS.h>

	ENTRY(__cerror)
	LDGP(pv)
	lda	sp, -8*2(sp)
	stq	ra, 8*0(sp)

	lda	a1, -ERESTART(a0)
	cmoveq	a1, EINTR, a0
	stq	a0, 8*1(sp)

	jsr	ra, ___errno

	ldq	a0, 8*1(sp)
	stl	a0, 0(v0)

	lda	v0, -1
	ldq	ra, 8*0(sp)
	lda	sp, 8*2(sp)
	ret
	SET_SIZE(__cerror)
