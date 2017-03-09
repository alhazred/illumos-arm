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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2011 Joyent, Inc. All rights reserved.
 */

#pragma once

#ifndef	_ASM

#include <sys/types.h>
#include <sys/avintr.h>
#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct	machcpu {
	struct hat	*mcpu_current_hat;
	uint32_t	mcpu_asid;
	uint64_t	mcpu_asid_gen;
	int		mcpu_pri;

	volatile int	xc_pend;
	volatile int	xc_wait;
	volatile int	xc_ack;
	volatile int	xc_state;
	volatile int	xc_retval;

	struct softint	mcpu_softinfo;
	uint64_t	pil_high_start[HIGH_LEVELS];
	uint64_t	intrstat[PIL_MAX + 1][2];

	uint64_t	mcpu_hartmask;
	bool		mcpu_timer_mask;
	bool		mcpu_ipi_mask;
	char		*mcpu_mmu;
	char		*mcpu_isa;
	char		*mcpu_uarch;
};

#ifndef NINTR_THREADS
#define	NINTR_THREADS	(LOCK_LEVEL)	/* number of interrupt threads */
#endif

#define	cpu_asid cpu_m.mcpu_asid
#define	cpu_asid_gen cpu_m.mcpu_asid_gen
#define	cpu_current_hat cpu_m.mcpu_current_hat
#define	cpu_softinfo cpu_m.mcpu_softinfo
#define	cpu_pri cpu_m.mcpu_pri
#define	cpu_hartmask cpu_m.mcpu_hartmask
#define	cpu_timer_mask cpu_m.mcpu_timer_mask
#define	cpu_ipi_mask cpu_m.mcpu_ipi_mask
#define	cpu_mmu cpu_m.mcpu_mmu
#define	cpu_isa cpu_m.mcpu_isa
#define	cpu_uarch cpu_m.mcpu_uarch

struct	cpu_startup_data {
	uint64_t	satp;
};

#ifdef	__cplusplus
}
#endif

#endif	/* _ASM */
