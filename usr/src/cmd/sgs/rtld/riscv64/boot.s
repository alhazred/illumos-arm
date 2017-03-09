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
 *	Copyright (c) 1988 AT&T
 *	  All Rights Reserved
 *
 *
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */

/*
 * Bootstrap routine for run-time linker.
 * We get control from exec which has loaded our text and
 * data into the process' address space and created the process
 * stack.
 *
 * On entry, the process stack looks like this:
 *
 *	#_______________________#  high addresses
 *	#	strings		#
 *	#_______________________#
 *	#	0 word		#
 *	#_______________________#
 *	#	Auxiliary	#
 *	#	entries		#
 *	#	...		#
 *	#	(size varies)	#
 *	#_______________________#
 *	#	0 word		#
 *	#_______________________#
 *	#	Environment	#
 *	#	pointers	#
 *	#	...		#
 *	#	(one word each)	#
 *	#_______________________#
 *	#	0 word		#
 *	#_______________________#
 *	#	Argument	# low addresses
 *	#	pointers	#
 *	#	Argc words	#
 *	#_______________________#
 *	#	argc		#
 *	#_______________________# <- sp
 *
 *
 * We must calculate the address at which ld.so was loaded,
 * find the addr of the dynamic section of ld.so, of argv[0], and  of
 * the process' environment pointers - and pass the thing to _setup
 * to handle.  We then call _rtld - on return we jump to the entry
 * point for the a.out.
 */
#include <sys/asm_linkage.h>
#include <link.h>
	.protected _rt_boot
	ENTRY(_rt_boot)
	li	s0, 0
	mv	s1, sp
	addi	sp, sp, -EB_MAX_SIZE64

	li	t1, EB_ARGV
	sd	t1, 0*8(sp)
	addi	t0, s1, 8
	sd	t0, 1*8(sp)

	li	t1, EB_ENVP
	sd	t1, 2*8(sp)
	lw	t1, 0*8(s1)	// t1 <- argc
	slli	t1, t1, 3
	add	t0, t0, t1
	addi	t0, t0, 8
	sd	t0, 3*8(sp)

	li	t1, EB_AUXV
	sd	t1, 4*8(sp)
1:	ld	t1, 0(t0)
	add	t0, t0, 8
	bnez	t1, 1b
	sd	t0, 5*8(sp)

	li	t1, EB_LDSO_BASE
	sd	t1, 6*8(sp)
	la	t0, .Lgot_offset
	lw	t1, 0(t0)
	add	t1, t0, t1	// t1 <- GOT
	ld	t2, 0(t1)	// <- offset of _DYNAMIC
	la	t0, .Ldyn_offset
	lw	t1, 0(t0)
	add	t1, t0, t1	// t1 <- _DYNAMIC
	sub	t0, t1, t2	// t0 <- base
	sd	t0, 7*8(sp)

	li	t2, EB_NULL
	sd	t2, 8*8(sp)
	sd	zero, 9*8(sp)

/*
 * Now bootstrap structure has been constructed.
 * The process stack looks like this:
 *
 *	#	...		#
 *	#_______________________#
 *	#	Argument	# high addresses
 *	#	pointers	#
 *	#	Argc words	#
 *	#_______________________#
 *	#	argc		#
 *	#_______________________# <- fp (= sp on entry)
 *	#   reserved area of    #
 *	#  bootstrap structure  #
 *	#  (currently not used) #
 *	#	...		#
 *	#_______________________#
 *	#  garbage (not used)   #
 *	#_ _ _ _ _ _ _ _ _ _ _ _#
 *	#	EB_NULL		#
 *	#_______________________# <- sp + 64 (= &eb[4])
 *	#	relocbase	#
 *	#_ _ _ _ _ _ _ _ _ _ _ _#
 *	#	EB_LDSO_BASE	#
 *	#_______________________# <- sp + 48 (= &eb[3])
 *	#	&auxv[0]	#
 *	#_ _ _ _ _ _ _ _ _ _ _ _#
 *	#	EB_AUXV		#
 *	#_______________________# <- sp + 32 (= &eb[2])
 *	#	&envp[0]	#
 *	#_ _ _ _ _ _ _ _ _ _ _ _#
 *	#	EB_ENVP		#
 *	#_______________________# <- sp + 16 (= &eb[1])
 *	#	&argv[0]	#
 *	#_ _ _ _ _ _ _ _ _ _ _ _# low addresses
 *	#	EB_ARGV		#
 *	#_______________________# <- sp (= fp - EB_MAX_SIZE64) = a0 (= &eb[0])
 */

	mv	s1, t1	// <- _DYNAMIC

	mv	a0, t0
	mv	a1, t1
	call	_setup_reloc

	mv	a0, sp
	mv	a1, s1
	call	_setup

	la	a2, atexit_fini
	addi	sp, sp, EB_MAX_SIZE64

	li	ra, 0
	jr	a0

.Lgot_offset:
	.long _GLOBAL_OFFSET_TABLE_ - .Lgot_offset
.Ldyn_offset:
	.long _DYNAMIC - .Ldyn_offset
	SET_SIZE(_rt_boot)

	.protected strerror
