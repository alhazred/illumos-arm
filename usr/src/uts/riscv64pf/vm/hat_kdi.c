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

#include <sys/cpuvar.h>
#include <sys/kdi_impl.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/mman.h>
#include <sys/bootconf.h>
#include <sys/cmn_err.h>
#include <vm/seg_kmem.h>
#include <vm/hat_riscv64.h>
#include <sys/bootinfo.h>
#include <sys/machsystm.h>

void
hat_boot_kdi_init(void)
{
}

void
hat_kdi_init(void)
{
}

int
kdi_vtop(uintptr_t vaddr, uint64_t *pap)
{
	uint64_t satp = csr_read_satp();
	ASSERT((satp & SATP_MODE_MASK) == SATP_MODE_SV39);

	pte_t *pt = (pte_t *)pa_to_kseg((satp & SATP_PPN_MASK) << MMU_PAGESHIFT);
	int l = MMU_PAGE_LEVELS - 1;
	for (int l = MMU_PAGE_LEVELS - 1;; l--) {
		pte_t pte = pt[(vaddr >> LEVEL_SHIFT(l)) & (NPTEPERPT - 1)];
		if ((pte & PTE_V) == 0)
			return (ENOENT);
		if (l == 0 || !IS_TABLE(pte)) {
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
