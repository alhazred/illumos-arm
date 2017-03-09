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
#include <sys/errno.h>
#include "assym.h"

/*
 * void _fpu_restore(pcb_t *pcb)
 */
	ENTRY(_fpu_restore)
	fld	f0,   ((0 * 8) + PCB_FPU_REGS)(a0)
	fld	f1,   ((1 * 8) + PCB_FPU_REGS)(a0)
	fld	f2,   ((2 * 8) + PCB_FPU_REGS)(a0)
	fld	f3,   ((3 * 8) + PCB_FPU_REGS)(a0)
	fld	f4,   ((4 * 8) + PCB_FPU_REGS)(a0)
	fld	f5,   ((5 * 8) + PCB_FPU_REGS)(a0)
	fld	f6,   ((6 * 8) + PCB_FPU_REGS)(a0)
	fld	f7,   ((7 * 8) + PCB_FPU_REGS)(a0)
	fld	f8,   ((8 * 8) + PCB_FPU_REGS)(a0)
	fld	f9,   ((9 * 8) + PCB_FPU_REGS)(a0)
	fld	f10,  ((10 * 8) + PCB_FPU_REGS)(a0)
	fld	f11,  ((11 * 8) + PCB_FPU_REGS)(a0)
	fld	f12,  ((12 * 8) + PCB_FPU_REGS)(a0)
	fld	f13,  ((13 * 8) + PCB_FPU_REGS)(a0)
	fld	f14,  ((14 * 8) + PCB_FPU_REGS)(a0)
	fld	f15,  ((15 * 8) + PCB_FPU_REGS)(a0)
	fld	f16,  ((16 * 8) + PCB_FPU_REGS)(a0)
	fld	f17,  ((17 * 8) + PCB_FPU_REGS)(a0)
	fld	f18,  ((18 * 8) + PCB_FPU_REGS)(a0)
	fld	f19,  ((19 * 8) + PCB_FPU_REGS)(a0)
	fld	f20,  ((20 * 8) + PCB_FPU_REGS)(a0)
	fld	f21,  ((21 * 8) + PCB_FPU_REGS)(a0)
	fld	f22,  ((22 * 8) + PCB_FPU_REGS)(a0)
	fld	f23,  ((23 * 8) + PCB_FPU_REGS)(a0)
	fld	f24,  ((24 * 8) + PCB_FPU_REGS)(a0)
	fld	f25,  ((25 * 8) + PCB_FPU_REGS)(a0)
	fld	f26,  ((26 * 8) + PCB_FPU_REGS)(a0)
	fld	f27,  ((27 * 8) + PCB_FPU_REGS)(a0)
	fld	f28,  ((28 * 8) + PCB_FPU_REGS)(a0)
	fld	f29,  ((29 * 8) + PCB_FPU_REGS)(a0)
	fld	f30,  ((30 * 8) + PCB_FPU_REGS)(a0)
	fld	f31,  ((31 * 8) + PCB_FPU_REGS)(a0)
	lw	t1,  PCB_FPU_CSR(a0)
	fscsr	t1
	ret
	SET_SIZE(_fpu_restore)


/*
 * void _fpu_save(pcb_t *pcb)
 */
	ENTRY(_fpu_save)
	fsd	f0,   ((0 * 8) + PCB_FPU_REGS)(a0)
	fsd	f1,   ((1 * 8) + PCB_FPU_REGS)(a0)
	fsd	f2,   ((2 * 8) + PCB_FPU_REGS)(a0)
	fsd	f3,   ((3 * 8) + PCB_FPU_REGS)(a0)
	fsd	f4,   ((4 * 8) + PCB_FPU_REGS)(a0)
	fsd	f5,   ((5 * 8) + PCB_FPU_REGS)(a0)
	fsd	f6,   ((6 * 8) + PCB_FPU_REGS)(a0)
	fsd	f7,   ((7 * 8) + PCB_FPU_REGS)(a0)
	fsd	f8,   ((8 * 8) + PCB_FPU_REGS)(a0)
	fsd	f9,   ((9 * 8) + PCB_FPU_REGS)(a0)
	fsd	f10,  ((10 * 8) + PCB_FPU_REGS)(a0)
	fsd	f11,  ((11 * 8) + PCB_FPU_REGS)(a0)
	fsd	f12,  ((12 * 8) + PCB_FPU_REGS)(a0)
	fsd	f13,  ((13 * 8) + PCB_FPU_REGS)(a0)
	fsd	f14,  ((14 * 8) + PCB_FPU_REGS)(a0)
	fsd	f15,  ((15 * 8) + PCB_FPU_REGS)(a0)
	fsd	f16,  ((16 * 8) + PCB_FPU_REGS)(a0)
	fsd	f17,  ((17 * 8) + PCB_FPU_REGS)(a0)
	fsd	f18,  ((18 * 8) + PCB_FPU_REGS)(a0)
	fsd	f19,  ((19 * 8) + PCB_FPU_REGS)(a0)
	fsd	f20,  ((20 * 8) + PCB_FPU_REGS)(a0)
	fsd	f21,  ((21 * 8) + PCB_FPU_REGS)(a0)
	fsd	f22,  ((22 * 8) + PCB_FPU_REGS)(a0)
	fsd	f23,  ((23 * 8) + PCB_FPU_REGS)(a0)
	fsd	f24,  ((24 * 8) + PCB_FPU_REGS)(a0)
	fsd	f25,  ((25 * 8) + PCB_FPU_REGS)(a0)
	fsd	f26,  ((26 * 8) + PCB_FPU_REGS)(a0)
	fsd	f27,  ((27 * 8) + PCB_FPU_REGS)(a0)
	fsd	f28,  ((28 * 8) + PCB_FPU_REGS)(a0)
	fsd	f29,  ((29 * 8) + PCB_FPU_REGS)(a0)
	fsd	f30,  ((30 * 8) + PCB_FPU_REGS)(a0)
	fsd	f31,  ((31 * 8) + PCB_FPU_REGS)(a0)
	frcsr	t1
	sw	t1,  PCB_FPU_CSR(a0)
	ret
	SET_SIZE(_fpu_save)

