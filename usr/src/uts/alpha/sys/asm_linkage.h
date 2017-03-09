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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma once

#include <sys/stack.h>
#include <sys/trap.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef _ASM	/* The remainder of this file is only for assembly files */

#define	v0	$0	/* (T) */
#define	t0	$1	/* (T) */
#define	t1	$2
#define	t2	$3
#define	t3	$4
#define	t4	$5
#define	t5	$6
#define	t6	$7
#define	t7	$8
#define	s0	$9	/* (S) */
#define	s1	$10
#define	s2	$11
#define	s3	$12
#define	s4	$13
#define	s5	$14
#define	s6	$15
#define	a0	$16	/* (T) */
#define	a1	$17
#define	a2	$18
#define	a3	$19
#define	a4	$20
#define	a5	$21
#define	t8	$22	/* (T) */
#define	t9	$23
#define	t10	$24
#define	t11	$25
#define	ra	$26	/* (S) */
#define	t12	$27	/* (T) */
#define	AT	$28	/* (T) */
#define	gp	$29	/* (T) */
#define	sp	$30	/* (S) */
#define	zero	$31

#define	fp	$15
#define	pv	$27

#define	fv0	$f0	/* (T) */
#define	fv1	$f1	/* (T) */
#define	ft0	fv1	/* (T) */
#define	fs0	$f2	/* (S) */
#define	fs1	$f3
#define	fs2	$f4
#define	fs3	$f5
#define	fs4	$f6
#define	fs5	$f7
#define	fs6	$f8
#define	fs7	$f9
#define	ft1	$f10	/* (T) */
#define	ft2	$f11
#define	ft3	$f12
#define	ft4	$f13
#define	ft5	$f14
#define	ft6	$f15
#define	fa0	$f16	/* (T) */
#define	fa1	$f17
#define	fa2	$f18
#define	fa3	$f19
#define	fa4	$f20
#define	fa5	$f21
#define	ft7	$f22	/* (T) */
#define	ft8	$f23
#define	ft9	$f24
#define	ft10	$f25
#define	ft11	$f26
#define	ft12	$f27
#define	ft13	$f28
#define	ft14	$f29
#define	ft15	$f30
#define	fzero	$f31


/*
 * These constants can be used to compute offsets into pointer arrays.
 */
#define	CPTRSHIFT	3
#define	CLONGSHIFT	3
#define	CPTRSIZE	(1<<CPTRSHIFT)
#define	CLONGSIZE	(1<<CLONGSHIFT)
#define	CPTRMASK	(CPTRSIZE - 1)
#define	CLONGMASK	(CLONGSIZE - 1)

/*
 * Symbolic section definitions.
 */
#define	RODATA	".rodata"

/*
 * profiling causes defintions of the MCOUNT and RTMCOUNT
 * particular to the type
 */
#ifdef GPROF

#define	MCOUNT_SIZE	(4*1)	/* 1 instructions */
#define	MCOUNT(x) \
	jsr AT, _mcount
#endif /* GPROF */

/*
 * if we are not profiling, MCOUNT should be defined to nothing
 */
#if !defined(GPROF)
#define	MCOUNT_SIZE	0	/* no instructions inserted */
#define	MCOUNT(x)
#endif /* !defined(PROF) && !defined(GPROF) */

#define	RTMCOUNT(x)	MCOUNT(x)

/*
 * Macro to define weak symbol aliases. These are similar to the ANSI-C
 *	#pragma weak _name = name
 * except a compiler can determine type. The assembler must be told. Hence,
 * the second parameter must be the type of the symbol (i.e.: function,...)
 */
#define	ANSI_PRAGMA_WEAK(sym, stype)	\
	.weak	_##sym; \
	.type	_##sym, @stype; \
/* CSTYLED */ \
_##sym	= sym

/*
 * Like ANSI_PRAGMA_WEAK(), but for unrelated names, as in:
 *	#pragma weak sym1 = sym2
 */
#define	ANSI_PRAGMA_WEAK2(sym1, sym2, stype)	\
	.weak	sym1; \
	.type sym1, @stype; \
sym1	= sym2

/*
 * ENTRY provides the standard procedure entry code and an easy way to
 * insert the calls to mcount for profiling. ENTRY_NP is identical, but
 * never calls mcount.
 */
#define	ENTRY(x) \
	.text; \
	.set noat; \
	.arch ev56; \
	.align	4; \
	.globl	x; \
	.type	x, @function; \
x:	MCOUNT(x)

#define	ENTRY_SIZE	MCOUNT_SIZE

#define	ENTRY_NP(x) \
	.text; \
	.set noat; \
	.arch ev56; \
	.align	4; \
	.globl	x; \
	.type	x, @function; \
x:

#define	RTENTRY(x) \
	.text; \
	.set noat; \
	.arch ev56; \
	.align	4; \
	.globl	x; \
	.type	x, @function; \
x:	RTMCOUNT(x)

/*
 * ENTRY2 is identical to ENTRY but provides two labels for the entry point.
 */
#define	ENTRY2(x, y) \
	.text; \
	.set noat; \
	.arch ev56; \
	.align	4; \
	.globl	x, y; \
	.type	x, @function; \
	.type	y, @function; \
/* CSTYLED */ \
x:	; \
y:	MCOUNT(x)

#define	ENTRY_NP2(x, y) \
	.text; \
	.set noat; \
	.arch ev56; \
	.align	4; \
	.globl	x, y; \
	.type	x, @function; \
	.type	y, @function; \
/* CSTYLED */ \
x:	; \
y:

/*
 * Load the global pointer.
 */
#define	LDGP(x) \
	ldgp	gp, 0(x)

#define	CALL(x) \
	jsr	ra, x; \
	LDGP(ra)

/*
 * ALTENTRY provides for additional entry points.
 */
#define	ALTENTRY(x) \
	.globl x; \
	.type	x, @function; \
x:

/*
 * DGDEF and DGDEF2 provide global data declarations.
 *
 * DGDEF provides a word aligned word of storage.
 *
 * DGDEF2 allocates "sz" bytes of storage with **NO** alignment.  This
 * implies this macro is best used for byte arrays.
 *
 * DGDEF3 allocates "sz" bytes of storage with "algn" alignment.
 */
#define	DGDEF2(name, sz) \
	.data; \
	.global name; \
	.type	name, @object; \
	.size	name, sz; \
name:

#define	DGDEF3(name, sz, algn) \
	.data; \
	.balign	algn; \
	.global name; \
	.type	name, @object; \
	.size	name, sz; \
name:

#define	DGDEF(name)	DGDEF3(name, 4, 4)

/*
 * SET_SIZE trails a function and set the size for the ELF symbol table.
 */
#define	SET_SIZE(x) \
	.size	x, [.-x]

#endif /* _ASM */

#ifdef	__cplusplus
}
#endif

