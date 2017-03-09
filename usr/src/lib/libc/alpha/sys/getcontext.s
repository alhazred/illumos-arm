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

	.file	"getcontext.s"

#include "SYS.h"
#include "assym.h"

	ENTRY(getcontext)
	LDGP(pv)
	lda	sp, -8*4(sp)
	stq	ra, 8*0(sp)
	stq	a0, 8*1(sp)
	stq	a1, 8*2(sp)

	CALL(__getcontext)

	ldq	ra, 8*0(sp)
	ldq	a0, 8*1(sp)
	ldq	a1, 8*2(sp)
	lda	sp, 8*4(sp)

	bne	v0, 1f

	lda	a0, UC_MCONTEXT_GREGS(a0)
	stq	ra, 8*REG_PC(a0)
	stq	ra, 8*REG_RA(a0)
	stq	sp, 8*REG_SP(a0)
	stq	zero, 8*REG_V0(a0)
	ret

	.align 4
1:	lda	v0, -1(zero)
	ret
	SET_SIZE(getcontext)


	ENTRY(swapcontext)
	LDGP(pv)
	lda	sp, -8*4(sp)
	stq	ra, 8*0(sp)
	stq	a0, 8*1(sp)
	stq	a1, 8*2(sp)

	CALL(__getcontext)

	ldq	ra, 8*0(sp)
	ldq	a0, 8*1(sp)
	ldq	a1, 8*2(sp)
	lda	sp, 8*4(sp)

	bne	v0, 1f

	lda	a0, UC_MCONTEXT_GREGS(a0)
	stq	ra, 8*REG_PC(a0)
	stq	ra, 8*REG_RA(a0)
	stq	sp, 8*REG_SP(a0)
	stq	zero, 8*REG_V0(a0)
	mov	a1, a0
	CALL(setcontext)

	.align 4
1:	lda	v0, -1(zero)
	ret
	SET_SIZE(swapcontext)
