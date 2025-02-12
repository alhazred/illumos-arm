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

	.file	"cerror.s"

#include <SYS.h>

	ENTRY(__cerror)
	addi	sp, sp, -16
	sd	ra, 0(sp)
	sd	s1, 8(sp)

	li	t0, ERESTART
	bne	a0, t0, 1f
	li	a0, EINTR
1:	mv	s1, a0
	call	___errno
	sw	s1, 0(a0)
	li	a0, -1

	ld	s1, 8(sp)
	ld	ra, 0(sp)
	addi	sp, sp, 16
	ret
	SET_SIZE(__cerror)
