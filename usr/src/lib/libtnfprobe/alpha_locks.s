/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
#pragma ident	"%Z%%M%	%I%	%E% SMI"
/*
 *	Copyright (c) 1994, by Sun Microsytems, Inc.
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */

#include <sys/asm_linkage.h>

	.file		__FILE__
/*
 * int tnfw_b_get_lock(tnf_byte_lock_t *);
 */
	ENTRY(tnfw_b_get_lock)
	mov	0xFF, a1
	insbl	a1, a0, a1
	andnot	a0, 0x7, t0
.tnfw_b_get_lock_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	extbl	t2, a0, v0
	mb
	ret
1:	br	zero, .tnfw_b_get_lock_retry
	SET_SIZE(tnfw_b_get_lock)

/*
 * void tnfw_b_clear_lock(tnf_byte_lock_t *);
 */
	ENTRY(tnfw_b_clear_lock)
	mb
	stb	zero, 0(a0)
	ret
	SET_SIZE(tnfw_b_clear_lock)

/*
 * u_long tnfw_b_atomic_swap(uint *, u_long);
 */
	ENTRY(tnfw_b_atomic_swap)
.tnfw_b_atomic_swap_retry:
	ldl_l	v0, 0(a0)
	mov	a1, t0
	stl_c	t0, 0(a0)
	beq	t0, 1f
	ret
1:	br	zero, .tnfw_b_atomic_swap_retry
	SET_SIZE(tnfw_b_atomic_swap)
