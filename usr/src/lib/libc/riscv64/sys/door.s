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
	li	a5, code;			/* subcode */		\
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
	li	a5, DOOR_RETURN		/* subcode */
	SYSTRAP_RVAL1(door)
	bnez	t0, 2f			/* errno is set */

	lw	a2, DOOR_SERVERS(sp)
	bnez	a2, 1f

	/*
	 * this is the last server thread - call creation func for more
	 */
	ld	a0, DOOR_INFO_PTR(sp)
	call	door_depletion_cb

1:	/* Call the door server function now */
	ld	a0, DOOR_COOKIE(sp)
	ld	a1, DOOR_DATA_PTR(sp)
	ld	a2, DOOR_DATA_SIZE(sp)
	ld	a3, DOOR_DESC_PTR(sp)
	ld	a4, DOOR_DESC_SIZE(sp)
	ld	t0, DOOR_PC(sp)
	jalr	t0

	/* Exit the thread if we return here */
	li	a0, 0
	call	_thrp_terminate

	/* when error */
2:
	li	t0, ERESTART
	bne	t0, t1, 3f
	li	t1, EINTR
3:	li	t0, EINTR
	beq	t0, t1, 4f

	/* if error is not EINTR */
	mv	a0, t1
	j	__cerror

4:	/* if error is EINTR */

	addi	sp, sp, -16
	sd	ra, 0(sp)
	call	getpid
	ld	ra, 0(sp)
	addi	sp, sp, 16

	la	t1, door_create_pid
	lw	t1, 0(t1)
	beq	t1, a0, 5f
	li	a0, EINTR
	j	__cerror

5:	/* */
	li	a0, 0
	li	a1, 0
	li	a2, 0
	j	door_restart
	SET_SIZE(__door_return)
