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
	addi	sp, sp, -32
	sd	ra, 0*8(sp)
	sd	a0, 1*8(sp)
	sd	a1, 2*8(sp)
	call	__getcontext
	ld	a1, 1*8(sp)
	ld	a2, 2*8(sp)
	ld	ra, 0*8(sp)
	addi	sp, sp, 32
	bnez	a0, 1f

	sd	ra, (UC_MCONTEXT_GREGS + 8*REG_PC)(a1)
	sd	ra, (UC_MCONTEXT_GREGS + 8*REG_RA)(a1)
	sd	sp, (UC_MCONTEXT_GREGS + 8*REG_SP)(a1)
	sd	zero,(UC_MCONTEXT_GREGS + 8*REG_A0)(a1)
	ret

1:	li	a0, -1
	ret
	SET_SIZE(getcontext)


	ENTRY(swapcontext)
	addi	sp, sp, -32
	sd	ra, 0*8(sp)
	sd	a0, 1*8(sp)
	sd	a1, 2*8(sp)
	call	__getcontext
	ld	a1, 1*8(sp)
	ld	a2, 2*8(sp)
	ld	ra, 0*8(sp)
	addi	sp, sp, 32
	bnez	a0, 1f

	sd	ra, (UC_MCONTEXT_GREGS + 8*REG_PC)(a1)
	sd	ra, (UC_MCONTEXT_GREGS + 8*REG_RA)(a1)
	sd	sp, (UC_MCONTEXT_GREGS + 8*REG_SP)(a1)
	sd	zero,(UC_MCONTEXT_GREGS + 8*REG_A0)(a1)
	mv	a0, a2
	call	setcontext

1:	li	a0, -1
	ret
	SET_SIZE(swapcontext)
