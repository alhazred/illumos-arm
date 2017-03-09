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

#ifndef _ASM
#include <sys/types.h>
#endif /* _ASM */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _ASM
typedef uint64_t pte_t;

#define NPTESHIFT	(MMU_PAGESHIFT - PTE_BITS)

#define NPTEPERPT	(MMU_PAGESIZE / sizeof(pte_t))

#define PTE_PFN_MASK		(((1ull << 44) - 1) << 10)

#define PTE_SFW_SHIFT		8
#define PTE_SFW_MASK		(0x3ull << PTE_SFW_SHIFT)
#define PTE_D			(1ull << 7)	// Dirty
#define PTE_A			(1ull << 6)	// Accessed
#define PTE_G			(1ull << 5)	// Global
#define PTE_U			(1ull << 4)	// User
#define PTE_X			(1ull << 3)	// execute
#define PTE_W			(1ull << 2)	// write
#define PTE_R			(1ull << 1)	// read
#define PTE_V			(1ull << 0)	// valid

#define PA_TO_PFN(pa)		((pa) >> MMU_PAGESHIFT)
#define PA_FROM_PFN(pfn)	((pfn) << MMU_PAGESHIFT)
#define PTE_TO_PFN(pte)		(((pte) >> 10) & ((1ull << 44) - 1))
#define PTE_FROM_PFN(pfn)	((pte_t)(pfn) << 10)
#define PTE_TO_PA(pte)		PA_FROM_PFN(PTE_TO_PFN(pte))
#define PTE_FROM_PA(pa)		PTE_FROM_PFN(PA_TO_PFN(pa))

#define IS_TABLE(pte)	(((pte) & (PTE_X | PTE_W | PTE_R | PTE_V)) == PTE_V)

#endif /* !_ASM */

