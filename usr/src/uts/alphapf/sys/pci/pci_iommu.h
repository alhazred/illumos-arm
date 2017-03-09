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

#ifndef _SYS_PCI_IOMMU_H
#define	_SYS_PCI_IOMMU_H


#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/vmem.h>

/*
 * The following macros define the iommu page size and related operations.
 */
#define	IOMMU_PAGE_SHIFT	13
#define	IOMMU_PAGE_SIZE		(1 << IOMMU_PAGE_SHIFT)
#define	IOMMU_PAGE_MASK		~(IOMMU_PAGE_SIZE - 1)
#define	IOMMU_PAGE_OFFSET	(IOMMU_PAGE_SIZE - 1)
#define	IOMMU_PTOB(x)		(((uint64_t)(x)) << IOMMU_PAGE_SHIFT)
#define	IOMMU_BTOP(x)		((x) >> IOMMU_PAGE_SHIFT)
#define	IOMMU_BTOPR(x)		IOMMU_BTOP((x) + IOMMU_PAGE_OFFSET)

/*
 * iommu block soft state structure:
 *
 * Each pci node may share an iommu block structure with its peer
 * node of have its own private iommu block structure.
 */
typedef struct {
	uint64_t window_base;
	uint64_t window_size;
	uint64_t *trans_base;
	vmem_t *dvma_map;
} window_t;

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_PCI_IOMMU_H */
