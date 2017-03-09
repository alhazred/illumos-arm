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
#include <sys/types.h>
#include <asm/cpu.h>
#include <sys/hwrpb.h>
#ifdef __cplusplus
extern "C" {
#endif

struct alpha_pcb {
	uint64_t ksp;	/* 0x00: Kernel Stack Pointer */
	uint64_t usp;	/* 0x08: User Stack Pointer */
	uint64_t ptbr;	/* 0x10: Page Pable Base Register */
	uint32_t cc;	/* 0x18: Cycle Counter */
	uint32_t asn;	/* 0x1c: Address Space Number */
	uint64_t uniq;	/* 0x20: Process Unique Value */
	uint64_t flags;	/* 0x28: Floating point enable */
	uint64_t resv0;	/* 0x30 */
	uint64_t resv1;	/* 0x38 */
};

#if defined(_KERNEL) && !defined(_ASM)

#define	SMT_PAUSE()	\
    __asm__ __volatile__("nop; nop; nop; nop;":::"memory")

#endif

#ifdef	__cplusplus
}
#endif
