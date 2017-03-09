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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */


/*
 * HAT interfaces used by the kernel debugger to interact with the VM system.
 * These interfaces are invoked when the world is stopped.  As such, no blocking
 * operations may be performed.
 */

#include <sys/cpuvar.h>
#include <sys/kdi_impl.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/mman.h>
#include <sys/bootconf.h>
#include <sys/cmn_err.h>
#include <vm/seg_kmem.h>
#include <vm/hat_alpha.h>
#include <sys/bootinfo.h>
#include <sys/machsystm.h>

/*
 * Get the address for remapping physical pages during boot
 */
void
hat_boot_kdi_init(void)
{
}

/*
 * Switch to using a page in the kernel's va range for physical memory access.
 * We need to allocate a virtual page, then permanently map in the page that
 * contains the PTE to it.
 */
void
hat_kdi_init(void)
{
}

/*ARGSUSED*/
static pte_t *
get_l1ptbl(void)
{
	pte_t *ptbl;
	uint64_t vptb_idx;

	vptb_idx = ((VPT_BASE >> LEVEL_SHIFT(mmu.max_level)) & (NPTEPERPT - 1));
	ptbl = (pte_t *)(VPT_BASE | (vptb_idx << PAGESHIFT) |
	    (vptb_idx << (PAGESHIFT + NPTESHIFT)) |
	    (vptb_idx << (PAGESHIFT + NPTESHIFT * 2)));
	return (pte_t *)(KSEG_BASE + mmu_ptob(PTE_TO_PFN(ptbl[vptb_idx])));
}
int
kdi_vtop(uintptr_t vaddr, uint64_t *pap)
{
	pte_t *pt = get_l1ptbl();
	int l = MMU_PAGE_LEVELS - 1;
	for (int l = MMU_PAGE_LEVELS - 1;; l--) {
		pte_t pte = pt[(vaddr >> LEVEL_SHIFT(l)) & (NPTEPERPT - 1)];
		if ((pte & PTE_VALID) == 0)
			return (ENOENT);
		if (l == 0) {
			*pap = (PTE_TO_PA(pte) | (vaddr & LEVEL_OFFSET(l)));
			return 0;
		}
		pt = (pte_t *)(PTE_TO_PA(pte) + SEGKPM_BASE);
	}
}

int
kdi_pread(caddr_t buf, size_t nbytes, uint64_t addr, size_t *ncopiedp)
{
	caddr_t va = (caddr_t)(addr + SEGKPM_BASE);
	bcopy(va, buf, nbytes);
	*ncopiedp = nbytes;
	return (0);
}

int
kdi_pwrite(caddr_t buf, size_t nbytes, uint64_t addr, size_t *ncopiedp)
{
	caddr_t va = (caddr_t)(addr + SEGKPM_BASE);
	bcopy(buf, va, nbytes);
	*ncopiedp = nbytes;
	return (0);
}


/*
 * Return the number of bytes, relative to the beginning of a given range, that
 * are non-toxic (can be read from and written to with relative impunity).
 */
/*ARGSUSED*/
size_t
kdi_range_is_nontoxic(uintptr_t va, size_t sz, int write)
{
	/*
	 * avoid any Virtual Address hole
	 */
	if (va + sz >= hole_start && va < hole_end)
		return (va < hole_start ? hole_start - va : 0);

	return (sz);
}
