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
#include "assym.h"

	ANSI_PRAGMA_WEAK(setjmp,function)
	ANSI_PRAGMA_WEAK(longjmp,function)


	ENTRY(setjmp)
	sd	s0, 0*8(a0)
	sd	s1, 1*8(a0)
	sd	s2, 2*8(a0)
	sd	s3, 3*8(a0)
	sd	s4, 4*8(a0)
	sd	s5, 5*8(a0)
	sd	s6, 6*8(a0)
	sd	s7, 7*8(a0)
	sd	s8, 8*8(a0)
	sd	s9, 9*8(a0)
	sd	s10, 10*8(a0)
	sd	s11, 11*8(a0)
	sd	sp, 12*8(a0)
	sd	ra, 13*8(a0)
	fsd	fs0, 14*8(a0)
	fsd	fs1, 15*8(a0)
	fsd	fs2, 16*8(a0)
	fsd	fs3, 17*8(a0)
	fsd	fs4, 18*8(a0)
	fsd	fs5, 19*8(a0)
	fsd	fs6, 20*8(a0)
	fsd	fs7, 21*8(a0)
	fsd	fs8, 22*8(a0)
	fsd	fs9, 23*8(a0)
	fsd	fs10, 24*8(a0)
	fsd	fs11, 25*8(a0)
	ld	t0, UL_SIGLINK(tp)
	sd	t0, 26*8(a0)
	li	a0, 0
	ret
	SET_SIZE(setjmp)

	ENTRY(longjmp)
	ld	s0, 0*8(a0)
	ld	s1, 1*8(a0)
	ld	s2, 2*8(a0)
	ld	s3, 3*8(a0)
	ld	s4, 4*8(a0)
	ld	s5, 5*8(a0)
	ld	s6, 6*8(a0)
	ld	s7, 7*8(a0)
	ld	s8, 8*8(a0)
	ld	s9, 9*8(a0)
	ld	s10, 10*8(a0)
	ld	s11, 11*8(a0)
	ld	sp, 12*8(a0)
	ld	ra, 13*8(a0)
	fld	fs0, 14*8(a0)
	fld	fs1, 15*8(a0)
	fld	fs2, 16*8(a0)
	fld	fs3, 17*8(a0)
	fld	fs4, 18*8(a0)
	fld	fs5, 19*8(a0)
	fld	fs6, 20*8(a0)
	fld	fs7, 21*8(a0)
	fld	fs8, 22*8(a0)
	fld	fs9, 23*8(a0)
	fld	fs10, 24*8(a0)
	fld	fs11, 25*8(a0)
	ld	t0, 26*8(a0)
	bnez	t0, 1f
	sd	zero, UL_SIGLINK(tp)
1:	bnez	a1, 2f
	li	a1, 1
2:	mv	a0, a1
	ret
	SET_SIZE(longjmp)
