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
	ld	a1, (UL_USTACK + SS_SP)(tp)
	ld	a2, (UL_USTACK + SS_SIZE)(tp)
	sub	a3, a0, a1
	bleu	a3, a2, 1f
	bnez	a2, 2f
1:	ret

2:	sub	a3, sp, a1
	bgtu	a3, a2, 3f
	addi	sp, a1, -STACK_ALIGN
3:	sb	zero, -1(a1)
	SYSTRAP_RVAL1(lwp_self)
	li	a1, SIGSEGV
	SYSTRAP_RVAL1(lwp_kill)
	ret
	SET_SIZE(_stack_grow)
