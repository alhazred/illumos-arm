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
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _KERNEL

#define FCSR_RM_MASK	(7u<<5)
#define FCSR_RM_RNE	(0u<<5)	// Round to Nearest
#define FCSR_RM_RTZ	(1u<<5)	// Round towards Zero
#define FCSR_RM_RDN	(2u<<5)	// Round towards Minus Infinity
#define FCSR_RM_RUP	(3u<<5)	// Round towards Plus Infinity
#define FCSR_RM_RMM	(4u<<5)	// Round to Nearest, ties to Max Magnitude
#define FCSR_RM_DYN	(7u<<5)	// dynamic rounding mode (instruction’s rm field select)

#define FCSR_FLAGS_MASK	(0x1F)
#define FCSR_FLAGS_NX	(0x01)	// Inexact exception
#define FCSR_FLAGS_UF	(0x02)	// Underflow exception
#define FCSR_FLAGS_OF	(0x04)	// Overflow exception
#define FCSR_FLAGS_DZ	(0x08)	// Divide by Zero exception
#define FCSR_FLAGS_NV	(0x10)	// Invalid Operation exception



#define FCSR_INIT (FCSR_RM_RNE)

extern void fp_save(pcb_t *pcb);
extern void fp_restore(pcb_t *pcb);
extern void fp_init(void);
extern void fp_free(pcb_t *pcb, int isexec);
extern int fp_fenflt(void);

#endif	/* _KERNEL */

#ifdef __cplusplus
}
#endif

