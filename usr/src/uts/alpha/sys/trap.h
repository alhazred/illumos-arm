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
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Copyright 2017 Hayashi Naoyuki
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_TRAP_H
#define	_SYS_TRAP_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Trap type values
 */
#define	T_ARITH_IOV	(1<<6)	/* Integer overflow (IOV)		*/
#define	T_ARITH_INE	(1<<5)	/* Inexact result (INE)			*/
#define	T_ARITH_UNF	(1<<4)	/* Underflow (UNF)			*/
#define	T_ARITH_OVF	(1<<3)	/* Overflow (OVF)			*/
#define	T_ARITH_DZE	(1<<2)	/* Division by zero (DZE)		*/
#define	T_ARITH_INV	(1<<1)	/* Invalid operation (INV)		*/
#define	T_ARITH_SWC	(1<<0)	/* Software completion (SWC		*/

#define	T_IF_BPT	0
#define	T_IF_BUGCHK	1
#define	T_IF_GENTRAP	2
#define	T_IF_FEN	3
#define	T_IF_OPDEC	4

#define	T_INT_IPI	0
#define	T_INT_CLK	1
#define	T_INT_MCE	2
#define	T_INT_IO	3
#define	T_INT_PMC	4
#define	T_INT_SFT	5 // fake

/*
 * Pseudo traps.
 */
#define	T_ARITH		0
#define	T_FAULT		1
#define	T_INTERRUPT	2
#define	T_MM		3
#define	T_SYSCALL	4
#define	T_UNA		5
#define	T_AST		6

/*
 *  Values of error code on stack in case of page fault
 */

/*
 *  Definitions for fast system call subfunctions
 */
/*
 *  Definitions for fast system call subfunctions
 */
#define	T_GETHRTIME	1	/* Get high resolution time		*/
#define	T_GETHRVTIME	2	/* Get high resolution virtual time	*/
#define	T_GETHRESTIME	3	/* Get high resolution time		*/
#define	T_GETLGRP	4	/* Get home lgrpid			*/

#define	T_LASTFAST	4	/* Last valid subfunction		*/
#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_TRAP_H */
