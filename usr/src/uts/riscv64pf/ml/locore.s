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
/*
 * Copyright 2017 Hayashi Naoyuki
 */

#include <sys/asm_linkage.h>
#include "assym.h"

	.data
	.globl t0stack
	.hidden t0stack
	.type t0stack, @object
	.size t0stack, DEFAULTSTKSZ
	.align MMU_PAGESHIFT
t0stack:
	.zero DEFAULTSTKSZ

	.globl t0
	.type t0, @object
	.size t0, MMU_PAGESIZE
	.align MMU_PAGESHIFT
t0:
	.zero MMU_PAGESIZE

	// a0: &xboot_info
	// a1: enable_hartmask
	// tp: hartid
	ENTRY(_start)
.option push
.option norelax
	la	gp, __global_pointer$
.option pop

	jal	2f
.Lfailvec:
	csrr	t4, scause
	csrr	t5, sepc
	csrr	t6, satp

	j	.Lfailvec
2:
	csrw	stvec, ra

	li	t1, 1
	sll	t0, t1, tp
	sub	t1, t0, t1
	and	t1, t1, a1
	bnez	t1, .Lslave

	la	t0, t0stack
	li	t1, DEFAULTSTKSZ
	add	t0, t0, t1
	li	s0, 0
	mv	sp, t0
	mv	s1, tp
	mv	s2, a1

	call	kobj_start
	mv	a0, sp
	mv	a1, s1
	mv	a2, s2
	call	mlsetup
	call	main
	call	halt
.Lslave:
	j	secondary_vec_start
	SET_SIZE(_start)

	ENTRY(halt)
1:	j	1b
	SET_SIZE(halt)

	ENTRY(secondary_vec_start)
	li	t1, 1
	sll	t0, t1, tp
	la	t1, wakeup_hartmask
1:	ld	t2, 0(t1)
	and	t2, t2, t0
	beqz	t2, 1b

	// find cpu_id
	//   t0: hartmask
	li	t1, 0

1:	slli	t2, t1, 3
	la	t3, cpu
	add	t2, t3, t2
	ld	t2, 0(t2)
	beqz	t2, 2f
	ld	t3, CPU_HARTMASK(t2)
	beq	t3, t0, .Lfind_cpuid
2:	addi	t1, t1, 1
	sltiu	t2, t1, NCPU
	bnez	t2, 1b
3:	j	3b

.Lfind_cpuid:
	// t2: cp
	ld	tp, CPU_THREAD(t2)
	ld	sp, T_LABEL_SP(tp)
	ld	t0, T_LABEL_PC(tp)
	li	s0, 0
	jalr	t0
4:	j	4b
	SET_SIZE(secondary_vec_start)
