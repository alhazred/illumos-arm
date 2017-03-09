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
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xFF
	sllw	t1, t1, t0	// t1 = LOCK_HELD_VALUE << (8 * ((uintptr_t)lp & 3));

	andi	a0, a0, -4
	amoor.w.aq t1, t1, (a0)

	srlw	a0, t1, t0
	andi	a0, a0, 0xFF
	ret
	SET_SIZE(tnfw_b_get_lock)

/*
 * void tnfw_b_clear_lock(tnf_byte_lock_t *);
 */
	ENTRY(tnfw_b_clear_lock)
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xff
	sllw	t1, t1, t0	// t1 = 0xff << (8 * ((uintptr_t)lp & 3));
	not	t1, t1

	andi	a0, a0, -4
	amoand.w.rl t1, t1, (a0)

	ret
	SET_SIZE(tnfw_b_clear_lock)

/*
 * u_long tnfw_b_atomic_swap(uint *, u_long);
 */
	ENTRY(tnfw_b_atomic_swap)
	amoswap.w a0, a1, (a0)
	ret
	SET_SIZE(tnfw_b_atomic_swap)
