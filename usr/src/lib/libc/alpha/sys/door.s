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

	.file	"door.s"

#include "SYS.h"
#include <sys/door.h>

	/*
	 * weak aliases for public interfaces
	 */
	ANSI_PRAGMA_WEAK2(door_bind,__door_bind,function)
	ANSI_PRAGMA_WEAK2(door_getparam,__door_getparam,function)
	ANSI_PRAGMA_WEAK2(door_info,__door_info,function)
	ANSI_PRAGMA_WEAK2(door_revoke,__door_revoke,function)
	ANSI_PRAGMA_WEAK2(door_setparam,__door_setparam,function)

/*
 * Offsets within struct door_results
 */
#define	DOOR_COOKIE	(0 * CLONGSIZE)
#define	DOOR_DATA_PTR	(1 * CLONGSIZE)
#define	DOOR_DATA_SIZE	(2 * CLONGSIZE)
#define	DOOR_DESC_PTR	(3 * CLONGSIZE)
#define	DOOR_DESC_SIZE	(4 * CLONGSIZE)
#define	DOOR_PC		(5 * CLONGSIZE)
#define	DOOR_SERVERS	(6 * CLONGSIZE)
#define	DOOR_INFO_PTR	(7 * CLONGSIZE)

/*
 * All of the syscalls except door_return() follow the same pattern.  The
 * subcode goes in a5, after all of the other arguments.
 */
#define	DOOR_SYSCALL(name, code)					\
	ENTRY(name);							\
	LDGP(pv);							\
	mov	code, a5;			/* subcode */		\
	SYSTRAP_RVAL1(door);						\
	SYSCERROR;							\
	RET;								\
	SET_SIZE(name)

	DOOR_SYSCALL(__door_bind,	DOOR_BIND)
	DOOR_SYSCALL(__door_call,	DOOR_CALL)
	DOOR_SYSCALL(__door_create,	DOOR_CREATE)
	DOOR_SYSCALL(__door_getparam,	DOOR_GETPARAM)
	DOOR_SYSCALL(__door_info,	DOOR_INFO)
	DOOR_SYSCALL(__door_revoke,	DOOR_REVOKE)
	DOOR_SYSCALL(__door_setparam,	DOOR_SETPARAM)
	DOOR_SYSCALL(__door_ucred,	DOOR_UCRED)
	DOOR_SYSCALL(__door_unbind,	DOOR_UNBIND)
	DOOR_SYSCALL(__door_unref,	DOOR_UNREFSYS)

/*
 * int
 * __door_return(
 *	void 			*data_ptr,
 *	size_t			data_size,	(in bytes)
 *	door_return_desc_t	*door_ptr,	(holds returned desc info)
 *	caddr_t			stack_base,
 *	size_t			stack_size)
 */
	ENTRY(__door_return)
door_restart:
	LDGP(pv)
	mov	DOOR_RETURN, a5		/* subcode */
	SYSTRAP_RVAL1(door)
	bne	t1, 2f			/* errno is set */

	ldl	t2, DOOR_SERVERS(sp)
	bne	t2, 1f

	/*
	 * this is the last server thread - call creation func for more
	 */
	ldq	a0, DOOR_INFO_PTR(sp)
	CALL(door_depletion_cb)

1:	/* Call the door server function now */
	ldq	a0, DOOR_COOKIE(sp)
	ldq	a1, DOOR_DATA_PTR(sp)
	ldq	a2, DOOR_DATA_SIZE(sp)
	ldq	a3, DOOR_DESC_PTR(sp)
	ldq	a4, DOOR_DESC_SIZE(sp)
	ldq	pv, DOOR_PC(sp)
	CALL((pv))
	/* Exit the thread if we return here */
	clr	a0
	CALL(_thrp_terminate)

2:	/* エラー時 */
	br	pv, 3f
3:	LDGP	(pv)
	lda	t2, -ERESTART(v0)
	cmoveq	t2, EINTR, v0
	lda	t2, -EINTR(v0)
	beq	t2, 4f

	/* EINTR以外はエラー*/
	mov	v0, a0
	jsr	zero, __cerror

4:	/* EINTRの場合 */
	lda	sp, -8*2(sp)
	stq	ra, 8*0(sp)
	CALL(getpid)
	ldq	ra, 8*0(sp)
	lda	sp, 8*2(sp)
	lda	t0, door_create_pid
	ldl	t0, 0(t0)
	xor	t0, v0, t0
	beq	t0, 5f
	mov	EINTR, a0
	jsr	zero, __cerror

5:	/* */
	clr	a0
	clr	a1
	clr	a2
	lda	pv, door_restart
	jsr	zero, (pv)
	SET_SIZE(__door_return)
