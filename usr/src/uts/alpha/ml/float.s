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
#include <sys/pal.h>
#include "assym.h"

/*
 * void fp_restore(pcb_t *pcb)
 */
	ENTRY(fp_restore)
	mov	a0, t1

	ldt	$f0, (KFPU_CR + PCB_FPU)(t1)
	mt_fpcr	$f0

	ldt	$f0,  ( 0*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f1,  ( 1*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f2,  ( 2*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f3,  ( 3*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f4,  ( 4*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f5,  ( 5*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f6,  ( 6*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f7,  ( 7*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f8,  ( 8*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f9,  ( 9*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f10, (10*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f11, (11*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f12, (12*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f13, (13*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f14, (14*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f15, (15*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f16, (16*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f17, (17*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f18, (18*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f19, (19*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f20, (20*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f21, (21*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f22, (22*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f23, (23*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f24, (24*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f25, (25*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f26, (26*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f27, (27*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f28, (28*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f29, (29*8 + KFPU_REGS + PCB_FPU)(t1)
	ldt	$f30, (30*8 + KFPU_REGS + PCB_FPU)(t1)

	ret
	SET_SIZE(fp_restore)


/*
 * void fp_save(pcb_t *pcb)
 */
	ENTRY(fp_save)
	mov	a0, t1

	stt	$f0,  ( 0*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f1,  ( 1*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f2,  ( 2*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f3,  ( 3*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f4,  ( 4*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f5,  ( 5*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f6,  ( 6*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f7,  ( 7*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f8,  ( 8*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f9,  ( 9*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f10, (10*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f11, (11*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f12, (12*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f13, (13*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f14, (14*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f15, (15*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f16, (16*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f17, (17*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f18, (18*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f19, (19*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f20, (20*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f21, (21*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f22, (22*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f23, (23*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f24, (24*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f25, (25*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f26, (26*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f27, (27*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f28, (28*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f29, (29*8 + KFPU_REGS + PCB_FPU)(t1)
	stt	$f30, (30*8 + KFPU_REGS + PCB_FPU)(t1)
	mf_fpcr	$f0
	stt	$f0, (KFPU_CR + PCB_FPU)(t1)

	ret
	SET_SIZE(fp_save)

