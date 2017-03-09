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
	br	pv, 1f
1:	LDGP(pv)

	mov	sp, fp
	lda	sp, -EB_MAX_SIZE64(sp)
	mov	sp, a0

	mov	EB_ARGV, t0
	addq	fp, 8, t1
	stq	t0, 8*0(a0)
	stq	t1, 8*1(a0)

	mov	EB_ENVP, t0
	ldq	t1, 8*0(fp)
	addq	fp, 8*2, t2
	s8addq	t1, t2, t1
	stq	t0, 8*2(a0)
	stq	t1, 8*3(a0)

	mov	EB_AUXV, t0
1:	ldq	t2, 0(t1)
	addq	t1, 8, t1
	bne	t2, 1b
	stq	t0, 8*4(a0)
	stq	t1, 8*5(a0)

	mov	EB_LDSO_BASE, t0
	br	t2, 1f
1:	ldiq	t3, 1b
	subq	t2, t3, t1
	stq	t0, 8*6(a0)
	stq	t1, 8*7(a0)

	mov	EB_NULL, t0
	stq	t0, 8*8(a0)

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

	ldq	t1, 8*7(a0)
	lda	t5, _DYNAMIC
	addq	t1, t5, a1	/* &_DYNAMIC */

	lda	sp, -8*2(sp)
	stq	a0, 8*0(sp)
	stq	a1, 8*1(sp)
	ldq	a0, 8*7(a0)	// EB_LDSO_BASE
	br	ra, _setup_reloc
	LDGP(ra)
	ldq	a0, 8*0(sp)
	ldq	a1, 8*1(sp)
	lda	sp, 8*2(sp)

	CALL(_setup)

	lda	sp, EB_MAX_SIZE64(sp)

	lda	a2, atexit_fini
	mov	v0, pv
	jmp	zero, (pv)
	SET_SIZE(_rt_boot)

	.protected strerror
