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
	mv	a5, a0
	j	1f
	ENTRY_NP(vfork)
	li	a5, 0
1:
	li	a0, SIG_SETMASK
	li	a1, MASKSET0
	li	a2, MASKSET1
	li	a3, MASKSET2
	li	a4, MASKSET3
	SYSTRAP_RVAL1(lwp_sigmask)

	mv	a1, a5
	li	a0, 2
	SYSTRAP_RVAL2(forksys)
	beqz	t0, 3f

	mv	a5, t1		/* save the vfork() error number */

	li	a0, SIG_SETMASK
	lw	a1, (UL_SIGMASK + 0 * 4)(tp)
	lw	a2, (UL_SIGMASK + 1 * 4)(tp)
	lw	a3, (UL_SIGMASK + 2 * 4)(tp)
	lw	a4, (UL_SIGMASK + 3 * 4)(tp)
	SYSTRAP_RVAL1(lwp_sigmask)
	mv	a0, a5
	j	__cerror

3:
	/*
	 * To determine if we are (still) a child of vfork(), the child
	 * increments curthread->ul_vfork by one and the parent decrements
	 * it by one.  If the result is zero, then we are not a child of
	 * vfork(), else we are.  We do this to deal with the case of
	 * a vfork() child calling vfork().
	 */
	mv	a5, a0		/* save the vfork() return value */
	bnez	a1, child

	/* parent process */
	lw	a2, UL_VFORK(tp)
	beqz	a2, update_ul_vfork
	addi	a2, a2, -1
	j	update_ul_vfork
child:
	li	a5, 0
	lw	a2, UL_VFORK(tp)
	addi	a2, a2, 1

update_ul_vfork:
	sw	a2, UL_VFORK(tp)

	/*
	 * Clear the schedctl interface in both parent and child.
	 * (The child might have modified the parent.)
	 */
	sd	zero, UL_SCHEDCTL(tp)
	sd	zero, UL_SCHEDCTL_CALLED(tp)

	li	a0, SIG_SETMASK
	lw	a1, (UL_SIGMASK + 0 * 4)(tp)
	lw	a2, (UL_SIGMASK + 1 * 4)(tp)
	lw	a3, (UL_SIGMASK + 2 * 4)(tp)
	lw	a4, (UL_SIGMASK + 3 * 4)(tp)
	SYSTRAP_RVAL1(lwp_sigmask)

	mv	a0, a5
	ret
	SET_SIZE(vfork)
	SET_SIZE(vforkx)

