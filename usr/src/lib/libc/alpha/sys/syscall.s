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
	LDGP(pv)
	lda	sp, -8*2(sp)
	mov	a0, v0
	mov	a1, a0
	mov	a2, a1
	mov	a3, a2
	mov	a4, a3
	mov	a5, a4
	ldq	a5, 8*2(sp)
	ldq	t0, 8*3(sp)
	stq	t0, 8*0(sp)
	call_pal PAL_callsys
	lda	sp, 8*2(sp)
	SYSCERROR
	ret
	SET_SIZE(syscall)

	ENTRY(_syscall6)
	LDGP(pv)
	mov	a0, v0
	mov	a1, a0
	mov	a2, a1
	mov	a3, a2
	mov	a4, a3
	mov	a5, a4
	ldq	a5, 8*0(sp)
	call_pal PAL_callsys
	SYSCERROR
	ret
	SET_SIZE(_syscall6)

	ENTRY(__systemcall)
	LDGP(pv)
	lda	sp, -8*4(sp)
	stq	a0, 8*3(sp)
	mov	a1, v0
	mov	a2, a0
	mov	a3, a1
	mov	a4, a2
	mov	a5, a3
	ldq	a4, 8*4(sp)
	ldq	a5, 8*5(sp)
	ldq	t0, 8*6(sp)
	ldq	t1, 8*7(sp)
	stq	t0, 8*0(sp)
	stq	t1, 8*1(sp)

	call_pal PAL_callsys
	ldq	a0, 8*3(sp)
	lda	sp, 8*4(sp)
	bne	t1, 1f

	stq	v0, 8*0(a0)
	stq	t2, 8*1(a0)
	clr	v0
	ret
1:
	lda	t0, -1(zero)
	stq	t0, 8*0(a0)
	stq	t0, 8*1(a0)
	ret
	SET_SIZE(__systemcall)

	ENTRY(__systemcall6)
	LDGP(pv)
	lda	sp, -8*2(sp)
	stq	a0, 8*1(sp)
	mov	a1, v0
	mov	a2, a0
	mov	a3, a1
	mov	a4, a2
	mov	a5, a3
	ldq	a4, 8*2(sp)
	ldq	a5, 8*3(sp)
	call_pal PAL_callsys
	ldq	a0, 8*1(sp)
	lda	sp, 8*2(sp)
	bne	t1, 1f
	stq	v0, 8*0(a0)
	stq	t2, 8*1(a0)
	clr	v0
	ret
1:
	lda	t0, -1(zero)
	stq	t0, 8*0(a0)
	stq	t0, 8*1(a0)
	ret
	SET_SIZE(__systemcall6)
