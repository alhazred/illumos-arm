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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */

#include <sys/asm_linkage.h>

/*
 * _lock_try(lp)
 *	- returns non-zero on success.
 */
	ENTRY(_lock_try)
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 1
	sllw	t1, t1, t0	// t1 = 1 << (8 * ((uintptr_t)lp & 3));

	andi	a0, a0, -4
	amoor.w.aq t1, t1, (a0)

	srlw	t1, t1, t0
	andi	t1, t1, 1
	xori	a0, t1, 1
	ret
	SET_SIZE(_lock_try)

/*
 * _lock_clear(lp)
 *	- clear lock and force it to appear unlocked in memory.
 */
	ENTRY(_lock_clear)
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xff
	sllw	t1, t1, t0	// t1 = 0xFF << (8 * ((uintptr_t)lp & 3));
	not	t1, t1

	andi	a0, a0, -4
	amoand.w.rl t1, t1, (a0)

	ret
	SET_SIZE(_lock_clear)

