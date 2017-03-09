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

#define T_IALIGN	0x00
#define T_IACCESS	0x01
#define T_ILEXC		0x02
#define T_BRK		0x03
#define T_LACCESS	0x05
#define T_AALIGN	0x06
#define T_SACCESS	0x07
#define T_ECALL		0x08
#define T_IPGFLT	0x0c
#define T_LPGFLT	0x0d
#define T_SPGFLT	0x0f

#define	T_INTERRUPT		(1ul << 63)
#define	T_SSFTINT		1
#define	T_STIMINT		5
#define	T_SEXTINT		9

/*
 * Pseudo traps.
 */
#define	T_AST			0x100
#define	T_SYSCALL		0x101

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
