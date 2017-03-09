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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_FASTTRAP_ISA_H
#define	_FASTTRAP_ISA_H

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* svc #(T_DTRACE_PID) */
#define	FASTTRAP_INSTR			0xd40000a1
/* svc #(T_DTRACE_RET) */
#define	FASTTRAP_RET_INSTR		0xd40000c1

#define	FASTTRAP_SUNWDTRACE_SIZE	128
typedef uint32_t	fasttrap_instr_t;

typedef struct fasttrap_machtp {
	uint32_t	ftmt_instr;	/* orig. instr. */
} fasttrap_machtp_t;

#define	ftt_instr	ftt_mtp.ftmt_instr

#define	FASTTRAP_AFRAMES		3
#define	FASTTRAP_RETURN_AFRAMES		4
#define	FASTTRAP_ENTRY_AFRAMES		3
#define	FASTTRAP_OFFSET_AFRAMES		3

#ifdef	__cplusplus
}
#endif

#endif	/* _FASTTRAP_ISA_H */
