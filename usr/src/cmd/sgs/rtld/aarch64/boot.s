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
	mov	x28, sp
	stp	xzr, xzr, [sp, #(-8 * 2)]!
	mov	x29, sp
	sub	sp, sp, #EB_MAX_SIZE64
	mov	x0, sp

	mov	x9, #EB_ARGV
	add	x10, x28, #8
	stp	x9, x10, [x0, #(8 * 0)]

	mov	x9, #EB_ENVP
	ldr	x11, [x28]	// x11 <- argc
	lsl	x11, x11, #3
	add	x10, x11, x10
	add	x10, x10, #8
	stp	x9, x10, [x0, #(8 * 2)]

	mov	x9, #EB_AUXV
1:	ldr	x11, [x10]
	add	x10, x10, #8
	cbnz	x11, 1b
	stp	x9, x10, [x0, #(8 * 4)]

	mov	x9, #EB_LDSO_BASE
	adrp	x11, :got:_GLOBAL_OFFSET_TABLE_
	ldr	x11, [x11, #:got_lo12:_GLOBAL_OFFSET_TABLE_]
	bl	2f
1:	.long _GLOBAL_OFFSET_TABLE_ - 1b
2:
	ldr	w10, [x30]
	adr	x12, 1b
	add	x10, x10, x12
	sub	x10, x10, x11

	stp	x9, x10, [x0, #(8 * 6)]

	mov	x9, #EB_NULL
	str	x9, [x0, #(8 * 8)]

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

	bl	2f
1:	.long _DYNAMIC - 1b
2:	ldr	w1, [x30]
	adr	x10, 1b
	add	x1, x1, x10

	ldr	x9, [x0, #(8 * 7)]

	stp	x0, x1, [sp, #-16]!
	mov	x0, x9
	bl	_setup_reloc
	ldp	x0, x1, [sp], #16

	bl	_setup

	add	sp, sp, #EB_MAX_SIZE64
	add	sp, sp, #(8 * 2)

	adrp	x2, :got:atexit_fini
	ldr	x2, [x2, #:got_lo12:atexit_fini]

	br	x0
	SET_SIZE(_rt_boot)

	.protected strerror
