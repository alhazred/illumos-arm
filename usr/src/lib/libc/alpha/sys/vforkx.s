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
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

	.file	"vforkx.s"

#include "SYS.h"
#include "assym.h"

/*
 * The child of vfork() will execute in the parent's address space,
 * thereby changing the stack before the parent runs again.
 * Therefore we have to be careful how we return from vfork().
 * Pity the poor debugger developer who has to deal with this kludge.
 *
 * We block all blockable signals while performing the vfork() system call
 * trap.  This enables us to set curthread->ul_vfork safely, so that we
 * don't end up in a signal handler with curthread->ul_vfork set wrong.
 */

	ENTRY_NP(vforkx)
	LDGP(pv)
	mov	a0, a5
	br	zero, 0f
	ENTRY_NP(vfork)
	LDGP(pv)
	clr	a5
0:
	lda	a0, SIG_SETMASK
	lda	a1, MASKSET0
	lda	a2, MASKSET1
	lda	a3, MASKSET2
	lda	a4, MASKSET3
	SYSTRAP_RVAL1(lwp_sigmask)

	mov	a5, a1
	mov	2, a0
	SYSTRAP_RVAL2(forksys)
	beq	t1, 3f

	mov	v0, a5		/* save the vfork() error number */

	call_pal PAL_rdunique

	lda	a0, SIG_SETMASK
	ldl	a1, UL_SIGMASK(v0)
	ldl	a2, UL_SIGMASK + 4(v0)
	ldl	a3, UL_SIGMASK + 8(v0)
	ldl	a4, UL_SIGMASK + 12(v0)
	SYSTRAP_RVAL1(lwp_sigmask)

	mov	a5, a0
	br	pv, 1f
1:	LDGP(pv)
	jsr	zero, __cerror

3:
	/*
	 * To determine if we are (still) a child of vfork(), the child
	 * increments curthread->ul_vfork by one and the parent decrements
	 * it by one.  If the result is zero, then we are not a child of
	 * vfork(), else we are.  We do this to deal with the case of
	 * a vfork() child calling vfork().
	 */
	mov	v0, a5	/* save the vfork() return value */
	call_pal PAL_rdunique

	bne	t2, child

	/* parent process */
	ldl	t1, UL_VFORK(v0)
	beq	t1, update_ul_vfork
	subq	t1, 1, t1
	br	zero, update_ul_vfork
child:
	clr	a5
	ldl	t1, UL_VFORK(v0)
	addq	t1, 1, t1

update_ul_vfork:
	stl	t1, UL_VFORK(v0)

	/*
	 * Clear the schedctl interface in both parent and child.
	 * (The child might have modified the parent.)
	 */
	stq	zero, UL_SCHEDCTL(v0)
	stq	zero, UL_SCHEDCTL_CALLED(v0)

	call_pal PAL_rdunique
	lda	a0, SIG_SETMASK
	ldl	a1, UL_SIGMASK(v0)
	ldl	a2, UL_SIGMASK + 4(v0)
	ldl	a3, UL_SIGMASK + 8(v0)
	ldl	a4, UL_SIGMASK + 12(v0)
	SYSTRAP_RVAL1(lwp_sigmask)

	mov	a5, v0

	ret
	SET_SIZE(vfork)
	SET_SIZE(vforkx)
