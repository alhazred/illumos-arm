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
#include "assym.h"

	.text
/*
 * void _start(void)
 */
	ENTRY(_start)
.option push
.option norelax
	la	gp, __global_pointer$
.option pop

	csrw	sstatus, zero
	csrw	sie, zero

	li	s0, 0
	mv	tp, a0	// hart
	mv	s1, a1	// dtb

	la	t0, boot_hartmask
	li	t1, 1
	sll	t1, t1, tp
	amoor.d t0, t1, (t0)

	bnez	t0, _slave_start
	la	sp, _BootStackTop

	la	t0, boot_args
	sd	s1, 0 * 8(t0)

	call	main
1:	j	1b

_slave_start:
	la	t1, kernel_entrypoint
1:	ld	t0, 0(t1)
	beqz	t0, 1b

	fence
	la	t1, kernel_satp
	ld	t1, 0(t1)
	csrw	satp, t1
	fence.i
	sfence.vma

	la	a0, xboot_info
	la	t1, boot_hartmask
	ld	a1, 0(t1)
	jalr	t0

1:	j	1b
	SET_SIZE(_start)

