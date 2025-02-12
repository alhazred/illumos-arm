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
 * Copyright (c) 2001 by Sun Microsystems, Inc.
 * All rights reserved.
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */

	.file	"crti.s"

#include <sys/asm_linkage.h>

	.hidden _init
	.hidden _fini

/*
 * _init function prologue
 */
	.section .init,"ax",@progbits
	.globl	_init
	.type	_init,@function
_init:
	addi	sp, sp, -16
	sd	ra, 0(sp)

/*
 * _fini function prologue
 */
	.section .fini,"ax",@progbits
	.globl	_fini
	.type	_fini,@function
_fini:
	addi	sp, sp, -16
	sd	ra, 0(sp)

