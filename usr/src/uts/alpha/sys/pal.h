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
 */

#ifndef _SYS_PAL_H
#define _SYS_PAL_H

/* Tru64 UNIX Unprivileged PALcode Instructions */
#define PAL_bpt		0x0080
#define PAL_bugchk	0x0081
#define PAL_callsys	0x0083
#define PAL_clrfen	0x00ae
#define PAL_gentrap	0x00aa
#define PAL_imb		0x0086
#define PAL_rdunique	0x009e
#define PAL_urti	0x0092
#define PAL_wrunique	0x009f

/* Tru64 UNIX Privileged PALcode Instructions */
#define PAL_cflush	0x0001
#define PAL_cserve	0x0009
#define PAL_draina	0x0002
#define PAL_halt	0x0000
#define PAL_rdmces	0x0010
#define PAL_rdps	0x0036
#define PAL_rdusp	0x003a
#define PAL_rdval	0x0032
#define PAL_retsys	0x003d
#define PAL_rti		0x003f
#define PAL_swpctx	0x0030
#define PAL_swppal	0x000a
#define PAL_swpipl	0x0035
#define PAL_tbi		0x0033
#define PAL_whami	0x003c
#define PAL_wrasn	0x002e
#define PAL_wrent	0x0034
#define PAL_wrfen	0x002b
#define PAL_ipir	0x000d
#define PAL_wrkgp	0x0037
#define PAL_wrmces	0x0011
#define PAL_wrperfmon	0x0039
#define PAL_wrusp	0x0038
#define PAL_wrval	0x0031
#define PAL_wrsysptb	0x0014
#define PAL_wrvirbnd	0x0013
#define PAL_wrvptptr	0x002d
#define PAL_wtint	0x003e

#define PAL_VMS		0x1
#define PAL_OSF		0x2

#define PAL_ENTRY_INT	0
#define PAL_ENTRY_ARITH 1
#define PAL_ENTRY_MM	2
#define PAL_ENTRY_IF	3
#define PAL_ENTRY_UNA	4
#define PAL_ENTRY_SYS	5

#if !defined(_ASM)
#include <asm/pal.h>
#endif
#endif /* _SYS_PAL_H */
