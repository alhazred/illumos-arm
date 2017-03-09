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

#ifndef	_SYS_PTE_H
#define	_SYS_PTE_H

#ifndef _ASM
#include <sys/types.h>
#endif /* _ASM */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _ASM
typedef uint64_t pte_t;

#define NPTESHIFT	10

#define NPTEPERPT	(MMU_PAGESIZE / sizeof(pte_t))

#define PTE_VALID		0x0001ul
#define PTE_FOR			0x0002ul
#define PTE_FOW			0x0004ul
#define PTE_FOE			0x0008ul
#define PTE_ASM			0x0010ul
#define PTE_GH			0x0060ul
#define PTE_NOMB		0x0080ul
#define PTE_KRE			0x0100ul
#define PTE_URE			0x0200ul
#define PTE_KWE			0x1000ul
#define PTE_UWE			0x2000ul
#define PTE_SOFTWARE		0xffff0000ul

#define PTE_GH_8K		0x0000ul
#define PTE_GH_64K		0x0020ul
#define PTE_GH_512K		0x0040ul
#define PTE_GH_4M		0x0060ul
#define PTE_GH_MASK		0x0060ul

#define PTE_FLT_MASK		0xeul
#define PTE_PROT_MASK		0xff00ul
#define PTE_PFN_MASK		0xffffffff00000000ul

#define PA_TO_PFN(pa)		((pa) >> MMU_PAGESHIFT)
#define PA_FROM_PFN(pfn)	((pfn) << MMU_PAGESHIFT)
#define PTE_TO_PFN(pte)		(((pte) >> 32) & 0xffffffff)
#define PTE_FROM_PFN(pfn)	((pte_t)(pfn) << 32)
#define PTE_TO_PA(pte)		PA_FROM_PFN(PTE_TO_PFN(pte))
#define PTE_FROM_PA(pa)		PTE_FROM_PFN(PA_TO_PFN(pa))

#define	PTE_BOOTMAP		(0x04000000ul)

#endif /* !_ASM */

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_PTE_H */
