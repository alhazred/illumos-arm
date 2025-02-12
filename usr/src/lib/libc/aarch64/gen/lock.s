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
	mov	w2, #0x1
1:	ldaxrb	w3, [x0]
	stlxrb	w4, w2, [x0]
	cbnz	w4, 1b
	eor	w0, w3, w2
	ret
	SET_SIZE(_lock_try)

/*
 * _lock_clear(lp)
 *	- clear lock and force it to appear unlocked in memory.
 */
	ENTRY(_lock_clear)
	stlrb	wzr, [x0]
	ret
	SET_SIZE(_lock_clear)

