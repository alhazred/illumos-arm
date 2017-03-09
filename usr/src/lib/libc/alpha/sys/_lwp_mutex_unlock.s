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

	.file	"_lwp_mutex_unlock.s"

#include "SYS.h"
#include "assym.h"

	ENTRY(_lwp_mutex_unlock)
	LDGP(pv)
	mb
	lda	a1, MUTEX_LOCK_WORD(a0)
1:	ldl_l	t0, 0(a1)
	clr	t1
	stl_c	t1, 0(a1)
	beq	t1, 1b

	lda	t1, WAITERMASK
	and	t0, t1, t1
	beq	t1, 3f

	clr	a1
	SYSTRAP_RVAL1(lwp_mutex_wakeup)	/* lwp_mutex_wakeup(mp, 0) */
	SYSLWPERR
	RET
3:
	RETC
	SET_SIZE(_lwp_mutex_unlock)
