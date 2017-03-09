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

/*
 * int
 * lock_spin_try(lock_t *lp)
 *	Same as lock_try(), but it has no patch point for dtrace.
 */
	ENTRY(_lock_try)
	mov	1, a1
	insql	a1, a0, a1
	andnot	a0, 0x7, t0
.ulock_try_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	br	zero, 2f
1:	br	zero, .ulock_try_retry
2:	extbl	t2, a0, v0
	xor	v0, 1, v0
	mb
	ret
	SET_SIZE(_lock_try)

/*
 * void
 * ulock_clear(ulock_t *lp)
 *	Release lock without changing interrupt priority level.
 */
	ENTRY(_lock_clear)
	mb
	stb	zero, 0(a0)
	ret
	SET_SIZE(_lock_clear)

