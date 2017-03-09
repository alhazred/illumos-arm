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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

	.file	"asm_subr.s"

#include <SYS.h>
#include "assym.h"

	/*
	 * This is where execution resumes when a thread created with
	 * thr_create() or pthread_create() returns (see setup_context()).
	 * We pass the (void *) return value to _thrp_terminate().
	 */
	ENTRY(_lwp_start)
	call	_thrp_terminate
	RET		/* actually, never returns */
	SET_SIZE(_lwp_start)

	/* All we need to do now is (carefully) call lwp_exit(). */
	ENTRY(_lwp_terminate)
	SYSTRAP_RVAL1(lwp_exit)
	RET		/* if we return, it is very bad */
	SET_SIZE(_lwp_terminate)

	ENTRY(set_curthread)
	mv	tp, a0
	ret
	SET_SIZE(set_curthread)

	ENTRY(__lwp_park)
	mv	a2, a1
	mv	a1, a0
	li	a0, 0
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_park)

	ENTRY(__lwp_unpark)
	mv	a1, a0
	li	a0, 1
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_unpark)

	ENTRY(__lwp_unpark_all)
	mv	a2, a1
	mv	a1, a0
	li	a0, 2
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_unpark_all)

/*
 * __sighndlr(int sig, siginfo_t *si, ucontext_t *uc, void (*hndlr)())
 *
 * This is called from sigacthandler() for the entire purpose of
 * communicating the ucontext to java's stack tracing functions.
 */
	ENTRY(__sighndlr)
	.globl	__sighndlrend
	jr	a3
__sighndlrend:
	SET_SIZE(__sighndlr)

/*
 * int _sigsetjmp(sigjmp_buf env, int savemask)
 *
 * This version is faster than the old non-threaded version because we
 * don't normally have to call __getcontext() to get the signal mask.
 * (We have a copy of it in the ulwp_t structure.)
 */

#undef	sigsetjmp

	ENTRY2(sigsetjmp,_sigsetjmp)
	// env		a0
	// savemask	a1
	mv	t0, sp
	addi	sp, sp, -(14 * 8)
	sd	s0, 0*8(sp)
	sd	s1, 1*8(sp)
	sd	s2, 2*8(sp)
	sd	s3, 3*8(sp)
	sd	s4, 4*8(sp)
	sd	s5, 5*8(sp)
	sd	s6, 6*8(sp)
	sd	s7, 7*8(sp)
	sd	s8, 8*8(sp)
	sd	s9, 9*8(sp)
	sd	s10, 10*8(sp)
	sd	s11, 11*8(sp)
	sd	t0, 12*8(sp)
	sd	ra, 13*8(sp)

	mv	a2, sp
	call	__csigsetjmp
	ld	ra,     (13 * 8)(sp)
	addi	sp, sp, (14 * 8)
	ret
	SET_SIZE(sigsetjmp)
	SET_SIZE(_sigsetjmp)
