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

	.file	"syscall.s"

#include "SYS.h"

	ANSI_PRAGMA_WEAK(syscall,function)

	ENTRY(syscall)
	mv	t0, a0
	mv	a0, a1
	mv	a1, a2
	mv	a2, a3
	mv	a3, a4
	mv	a4, a5
	mv	a5, a6
	mv	a6, a7
	ld	a7, 0(sp)
	ecall
	SYSCERROR
	RET
	SET_SIZE(syscall)

	ENTRY(_syscall6)
	mv	t0, a0
	mv	a0, a1
	mv	a1, a2
	mv	a2, a3
	mv	a3, a4
	mv	a4, a5
	mv	a5, a6
	ecall
	SYSCERROR
	RET
	SET_SIZE(_syscall6)

	ENTRY(__systemcall)
	addi	sp, sp, -16
	sd	a0, 0(sp)
	sd	a1, 8(sp)
	mv	t0, a1
	mv	a0, a2
	mv	a1, a3
	mv	a2, a4
	mv	a3, a5
	mv	a4, a6
	mv	a5, a7
	ld	a6, 16(sp)
	ld	a7, 24(sp)
	ecall
	ld	a2, 0(sp)
	ld	a3, 8(sp)
	addi	sp, sp, 16
	bnez	t0, 1f
	sd	a0, 0(a2)
	sd	a1, 8(a2)
	li	a0, 0
	RET
1:
	li	a3, -1
	sd	a3, 0(a2)
	sd	a3, 8(a2)
	mv	a0, t1
	RET
	SET_SIZE(__systemcall)

	ENTRY(__systemcall6)
	addi	sp, sp, -16
	sd	a0, 0(sp)
	sd	a1, 8(sp)
	mv	t0, a1
	mv	a0, a2
	mv	a1, a3
	mv	a2, a4
	mv	a3, a5
	mv	a4, a6
	mv	a5, a7
	ecall
	ld	a2, 0(sp)
	ld	a3, 8(sp)
	addi	sp, sp, 16
	bnez	t0, 1f
	sd	a0, 0(a2)
	sd	a1, 8(a2)
	li	a0, 0
	RET
1:
	li	a3, -1
	sd	a3, 0(a2)
	sd	a3, 8(a2)
	mv	a0, t1
	RET
	SET_SIZE(__systemcall6)
