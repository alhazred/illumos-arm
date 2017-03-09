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
 * Copyright 2017 Hayashi Naoyuki
 * Copyright 2015 Nexenta Systems, Inc.  All rights reserved.
 *
 * Copyright (c) 1989, 2010, Oracle and/or its affiliates. All rights reserved.
 */
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc. */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T		*/
/*	All Rights Reserved	*/

#ifndef	_SYS_REGSET_H
#define	_SYS_REGSET_H

#include <sys/feature_tests.h>

#if !defined(_ASM)
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
#define	REG_V0	0
#define	REG_T0	1
#define	REG_T1	2
#define	REG_T2	3
#define	REG_T3	4
#define	REG_T4	5
#define	REG_T5	6
#define	REG_T6	7
#define	REG_T7	8
#define	REG_S0	9
#define	REG_S1	10
#define	REG_S2	11
#define	REG_S3	12
#define	REG_S4	13
#define	REG_S5	14
#define	REG_S6	15
#define	REG_A0	16
#define	REG_A1	17
#define	REG_A2	18
#define	REG_A3	19
#define	REG_A4	20
#define	REG_A5	21
#define	REG_T8	22
#define	REG_T9	23
#define	REG_T10	24
#define	REG_T11	25
#define	REG_RA	26
#define	REG_T12	27
#define	REG_AT	28
#define	REG_GP	29
#define	REG_SP	30
#define	REG_PC	31
#define	REG_UQ	32
#define	REG_PV	REG_T12
#endif	/* !defined(_XPG4_2) || defined(__EXTENSIONS__) */

/*
 * A gregset_t is defined as an array type for compatibility with the reference
 * source. This is important due to differences in the way the C language
 * treats arrays and structures as parameters.
 */
#define	_NGREG	34

#if !defined(_ASM)

typedef long	greg_t;
typedef greg_t	gregset_t[_NGREG];

/*
 * Floating point definitions.
 */
typedef struct fpu {
#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
	uint64_t	d_fpregs[31];
	uint64_t	fp_cr;
#else
	uint64_t	__d_fpregs[31];
	uint64_t	__fp_cr;
#endif
} fpregset_t;

/*
 * Structure mcontext defines the complete hardware machine state.
 */
typedef struct {
#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
	gregset_t	gregs;	/* general register set */
	fpregset_t	fpregs;	/* floating point register set */
#else
	gregset_t	__gregs;	/* general register set */
	fpregset_t	__fpregs;	/* floating point register set */
#endif
} mcontext_t;

#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
#if defined(_SYSCALL32)
#define	_NGREG32	_NGREG
#define	_NGREG64	_NGREG
typedef int32_t	greg32_t;
typedef int64_t	greg64_t;
typedef greg32_t gregset32_t[_NGREG32];
typedef greg64_t gregset64_t[_NGREG64];
typedef struct fpu32 {
	uint64_t	__d_fpregs[31];
	uint64_t	__fp_cr;
} fpregset32_t;
typedef struct {
	gregset32_t	__gregs;	/* general register set */
	fpregset32_t	__fpregs;	/* floating point register set */
} mcontext32_t;
#endif	/* _SYSCALL32 */

/*
 * Kernel's FPU save area
 */
typedef struct {
	uint64_t	kfpu_regs[31];
	uint64_t	kfpu_cr;
} kfpu_t;
#endif	/* !defined(_XPG4_2) || defined(__EXTENSIONS__) */

#endif	/* _ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_REGSET_H */
