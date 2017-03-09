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

#ifndef	_SYS_IRQ_H
#define	_SYS_IRQ_H

#ifdef	__cplusplus
extern "C" {
#endif

enum {
	IRQ_ISA_PIT	= 0,
	IRQ_IPI_BASE	= 128,
	IRQ_IPI_XCALL	= IRQ_IPI_BASE + 0,	// PIL 15
	IRQ_IPI_CBE	= IRQ_IPI_BASE + 1,	// PIL 14
	IRQ_IPI_SYS	= IRQ_IPI_BASE + 2,	// PIL 13
	IRQ_IPI_DUMMY	= IRQ_IPI_BASE + 3,	// PIL 12
	IRQ_IPI_CPUPOKE	= IRQ_IPI_BASE + 4,	// PIL 11
	IRQ_MAX = 255
};

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_IRQ_H */
