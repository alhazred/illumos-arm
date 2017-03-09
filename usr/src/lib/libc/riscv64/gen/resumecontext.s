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
	addi	sp, sp, -16
	sd	ra, 0(sp)

	addi	sp, sp, -UC_MCONTEXT
	mv	a0, sp
	call	getcontext
	ld	a0, UC_MCONTEXT_UC_LINK(sp)
	call	setcontext
	addi	sp, sp, UC_MCONTEXT

	ld	ra, 0(sp)
	addi	sp, sp, 16
	ret
	SET_SIZE(__resumecontext)
