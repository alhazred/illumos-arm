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
	LDGP(ra)
	mov	v0, a0
	CALL(_thrp_terminate)
	RET		/* actually, never returns */
	SET_SIZE(_lwp_start)

	/* All we need to do now is (carefully) call lwp_exit(). */
	ENTRY(_lwp_terminate)
	LDGP(pv)
	SYSTRAP_RVAL1(lwp_exit)
	RET		/* if we return, it is very bad */
	SET_SIZE(_lwp_terminate)

	ENTRY(set_curthread)
	call_pal PAL_wrunique
	ret
	SET_SIZE(set_curthread)

	ENTRY(__lwp_park)
	LDGP(pv)
	mov	a1, a2
	mov	a0, a1
	mov	0, a0
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_park)

	ENTRY(__lwp_unpark)
	LDGP(pv)
	mov	a0, a1
	mov	1, a0
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_unpark)

	ENTRY(__lwp_unpark_all)
	LDGP(pv)
	mov	a1, a2
	mov	a0, a1
	mov	2, a0
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_unpark_all)


	ENTRY(_getfcr)
	excb
	mf_fpcr $f0
	excb
	stt	$f0, 0(a0)
	RET
	SET_SIZE(_getfcr)

/*
 * __sighndlr(int sig, siginfo_t *si, ucontext_t *uc, void (*hndlr)())
 *
 * This is called from sigacthandler() for the entire purpose of
 * communicating the ucontext to java's stack tracing functions.
 */
	ENTRY(__sighndlr)
	.globl	__sighndlrend
	mov	a3, pv
	jmp	zero, (pv)
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
	LDGP(pv)
	mov	sp, a2
	mov	s0, a3
	mov	s1, a4
	mov	s2, a5
	lda	sp, -8*6(sp)
	stq	s3, 8*0(sp)
	stq	s4, 8*1(sp)
	stq	s5, 8*2(sp)
	stq	s6, 8*3(sp)
	stq	ra, 8*4(sp)

	CALL(__csigsetjmp)
	ldq	ra, 8*4(sp)
	lda	sp, 8*6(sp)
	ret
	SET_SIZE(sigsetjmp)
	SET_SIZE(_sigsetjmp)

	ENTRY_NP(pal_rdunique)
	call_pal PAL_rdunique
	ret
	SET_SIZE(pal_rdunique)
