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

	.file	"setjmp.s"

#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include "assym.h"

	ANSI_PRAGMA_WEAK(setjmp,function)
	ANSI_PRAGMA_WEAK(longjmp,function)

	ENTRY(setjmp)
	stq	s0, 0*8(a0)
	stq	s1, 1*8(a0)
	stq	s2, 2*8(a0)
	stq	s3, 3*8(a0)
	stq	s4, 4*8(a0)
	stq	s5, 5*8(a0)
	stq	s6, 6*8(a0)
	stq	ra, 7*8(a0)
	stq	sp, 8*8(a0)

	call_pal PAL_rdunique			// v0 = ulwp_t *
	ldq	a1, UL_SIGLINK(v0)
	stq	a1, 9*8(a0)
	clr	v0
	ret
	SET_SIZE(setjmp)


	ENTRY(longjmp)
	ldq	s0, 0*8(a0)
	ldq	s1, 1*8(a0)
	ldq	s2, 2*8(a0)
	ldq	s3, 3*8(a0)
	ldq	s4, 4*8(a0)
	ldq	s5, 5*8(a0)
	ldq	s6, 6*8(a0)
	ldq	ra, 7*8(a0)
	ldq	sp, 8*8(a0)

	ldq	t0, 9*8(a0)
	bne	t0, 1f
	call_pal PAL_rdunique			// v0 = ulwp_t *
	stq	zero, UL_SIGLINK(v0)
1:
	mov	a1, v0
	ret
	SET_SIZE(longjmp)
