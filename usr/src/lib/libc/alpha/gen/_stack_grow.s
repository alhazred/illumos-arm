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

	.file	"_stack_grow.s"
#include "SYS.h"
#include "assym.h"

	ENTRY(_stack_grow)
	LDGP(pv)
	call_pal PAL_rdunique			// v0 = ulwp_t *
	ldq	t0, (UL_USTACK + SS_SP)(v0)
	ldq	t1, (UL_USTACK + SS_SIZE)(v0)

	subq	a0, t0, t2
	cmpult	t2, t1, t3
	bne	t3, 1f
	bne	t1, 2f
1:
	mov	a0, v0
	ret

2:
	subq	sp, t0, t2
	cmpult	t2, t1, t3
	beq	t3, 3f
	lda	sp, -STACK_ALIGN(t0)
3:
	stq	zero, -4(t0)

	SYSTRAP_RVAL1(lwp_self)
	mov	v0, a0
	mov	SIGSEGV, a1
	SYSTRAP_RVAL1(lwp_kill)
	ret
	SET_SIZE(_stack_grow)
