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

	.file	"resumecontext.s"

#include "SYS.h"
#include "assym.h"

	.protected __resumecontext
	ENTRY(__resumecontext)
	br	pv, 1f
1:	LDGP(pv)
	lda	sp, (-UC_MCONTEXT - 8*2)(sp)
	stq	ra, UC_MCONTEXT(sp)
	mov	sp, a0
	CALL(getcontext)
	ldq	a0,	UC_MCONTEXT_UC_LINK(sp)
	CALL(setcontext)
	ldq	ra, UC_MCONTEXT(sp)
	lda	sp, (UC_MCONTEXT + 8*2)(sp)
	RET
	SET_SIZE(__resumecontext)
