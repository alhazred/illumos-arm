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

#include	<link.h>
#include	<sys/asm_linkage.h>

	.file	"boot_elf.s"

	.protected elf_rtbndr

	ENTRY(elf_rtbndr)
	addi	sp, sp, -(18 * 8)
	sd	a0, 0*8(sp)
	sd	a1, 1*8(sp)
	sd	a2, 2*8(sp)
	sd	a3, 3*8(sp)
	sd	a4, 4*8(sp)
	sd	a5, 5*8(sp)
	sd	a6, 6*8(sp)
	sd	a7, 7*8(sp)
	sd	ra, 8*8(sp)
	fsd	fa0, 9*8(sp)
	fsd	fa1, 10*8(sp)
	fsd	fa2, 11*8(sp)
	fsd	fa3, 12*8(sp)
	fsd	fa4, 13*8(sp)
	fsd	fa5, 14*8(sp)
	fsd	fa6, 15*8(sp)
	fsd	fa7, 16*8(sp)

	mv	a0, t0
	slli	a1, t1, 1
	add	a1, a1, t1

	jal	elf_bndr
	mv	t0, a0

	ld	a0, 0*8(sp)
	ld	a1, 1*8(sp)
	ld	a2, 2*8(sp)
	ld	a3, 3*8(sp)
	ld	a4, 4*8(sp)
	ld	a5, 5*8(sp)
	ld	a6, 6*8(sp)
	ld	a7, 7*8(sp)
	ld	ra, 8*8(sp)
	fld	fa0, 9*8(sp)
	fld	fa1, 10*8(sp)
	fld	fa2, 11*8(sp)
	fld	fa3, 12*8(sp)
	fld	fa4, 13*8(sp)
	fld	fa5, 14*8(sp)
	fld	fa6, 15*8(sp)
	fld	fa7, 16*8(sp)
	addi	sp, sp, (18 * 8)

	jr	t0
	SET_SIZE(elf_rtbndr)
