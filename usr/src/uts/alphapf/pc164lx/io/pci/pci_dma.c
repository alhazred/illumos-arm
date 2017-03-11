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
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * PCI nexus DVMA and DMA core routines:
 *	dma_map/dma_bind_handle implementation
 *	bypass and peer-to-peer support
 *	fast track DVMA space allocation
 *	runtime DVMA debug
 */
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/sysmacros.h>
#include <sys/sunddi.h>
#include <sys/machsystm.h>	/* lddphys() */
#include <sys/ddi_impldefs.h>
#include <vm/hat.h>
#include <sys/pci/pci_obj.h>
#include <sys/promif.h>
#include <sys/pci/pci_pyxis.h>

/*LINTLIBRARY*/

int
pci_dma_sync(dev_info_t *dip, dev_info_t *rdip, ddi_dma_handle_t handle,
	off_t off, size_t len, uint32_t sync_flag)
{
	__asm __volatile__("mb":::"memory");
	return (DDI_SUCCESS);
}

/*
 * pci_dma_allocmp - Allocate a pci dma implementation structure
 *
 * An extra ddi_dma_attr structure is bundled with the usual ddi_dma_impl
 * to hold unmodified device limits. The ddi_dma_attr inside the
 * ddi_dma_impl structure is augumented with system limits to enhance
 * DVMA performance at runtime. The unaugumented device limits saved
 * right after (accessed through the DEV_ATTR macro) is used
 * strictly for peer-to-peer transfers which do not obey system limits.
 *
 * return: DDI_SUCCESS DDI_DMA_NORESOURCES
 */
ddi_dma_impl_t *
pci_dma_allocmp(dev_info_t *dip, dev_info_t *rdip, int (*waitfp)(caddr_t),
	caddr_t arg)
{
	ddi_dma_impl_t *mp;
	int sleep = (waitfp == DDI_DMA_SLEEP) ? KM_SLEEP : KM_NOSLEEP;

	/* Caution: we don't use zalloc to enhance performance! */
	if ((mp = kmem_alloc(sizeof (pci_dma_hdl_t), sleep)) == 0) {
		if (waitfp != DDI_DMA_DONTWAIT) {
			ddi_set_callback(waitfp, arg, &pci_kmem_clid);
		}
		return (mp);
	}

	mp->dmai_rdip = rdip;
	mp->dmai_flags = 0;
	mp->dmai_pfnlst = NULL;
	mp->dmai_winlst = NULL;
	mp->dmai_ncookies = 0;
	mp->dmai_curcookie = 0;

	/*
	 * kmem_alloc debug: the following fields are not zero-ed
	 * mp->dmai_size = 0;
	 * mp->dmai_offset = 0;
	 * mp->dmai_minxfer = 0;
	 * mp->dmai_burstsizes = 0;
	 * mp->dmai_ndvmapages = 0;
	 * mp->dmai_pool/roffset = 0;
	 * mp->dmai_rflags = 0;
	 * mp->dmai_inuse/flags
	 * mp->dmai_nwin = 0;
	 * mp->dmai_winsize = 0;
	 * mp->dmai_nexus_private/tte = 0;
	 * mp->dmai_iopte/pfnlst
	 * mp->dmai_sbi/pfn0 = 0;
	 * mp->dmai_minfo/winlst/fdvma
	 * mp->dmai_rdip
	 * bzero(&mp->dmai_object, sizeof (ddi_dma_obj_t));
	 * mp->dmai_cookie = 0;
	 */

	mp->dmai_attr.dma_attr_version = (uint_t)DMA_ATTR_VERSION;
	mp->dmai_attr.dma_attr_flags = (uint_t)0;
	mp->dmai_fault = 0;
	mp->dmai_fault_check = NULL;
	mp->dmai_fault_notify = NULL;

	mp->dmai_error.err_ena = 0;
	mp->dmai_error.err_status = DDI_FM_OK;
	mp->dmai_error.err_expected = DDI_FM_ERR_UNEXPECTED;
	mp->dmai_error.err_ontrap = NULL;
	mp->dmai_error.err_fep = NULL;
	mp->dmai_error.err_cf = NULL;
	ndi_fmc_insert(rdip, DMA_HANDLE, mp, NULL);

	SYNC_BUF_PA(mp) = 0ull;
	return (mp);
}

void
pci_dma_freemp(ddi_dma_impl_t *mp)
{
	ndi_fmc_remove(mp->dmai_rdip, DMA_HANDLE, mp);
	if (mp->dmai_ndvmapages > 1)
		pci_dma_freepfn(mp);
	if (mp->dmai_winlst)
		pci_dma_freewin(mp);
	kmem_free(mp, sizeof (pci_dma_hdl_t));
}

void
pci_dma_freepfn(ddi_dma_impl_t *mp)
{
	void *addr = mp->dmai_pfnlst;
	if (addr) {
		size_t npages = mp->dmai_ndvmapages;
		if (npages > 1)
			kmem_free(addr, npages * sizeof (iopfn_t));
		mp->dmai_pfnlst = NULL;
	}
	mp->dmai_ndvmapages = 0;
}

/*
 * pci_dma_lmts2hdl - alloate a ddi_dma_impl_t, validate practical limits
 *			and convert dmareq->dmar_limits to mp->dmai_attr
 *
 * ddi_dma_impl_t member modified     input
 * ------------------------------------------------------------------------
 * mp->dmai_minxfer		    - dev
 * mp->dmai_burstsizes		    - dev
 * mp->dmai_flags		    - no limit? peer-to-peer only?
 *
 * ddi_dma_attr member modified       input
 * ------------------------------------------------------------------------
 * mp->dmai_attr.dma_attr_addr_lo   - dev lo, sys lo
 * mp->dmai_attr.dma_attr_addr_hi   - dev hi, sys hi
 * mp->dmai_attr.dma_attr_count_max - dev count max, dev/sys lo/hi delta
 * mp->dmai_attr.dma_attr_seg       - 0         (no nocross   restriction)
 * mp->dmai_attr.dma_attr_align     - 1		(no alignment restriction)
 *
 * The dlim_dmaspeed member of dmareq->dmar_limits is ignored.
 */
ddi_dma_impl_t *
pci_dma_lmts2hdl(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_req_t *dmareq)
{
	ddi_dma_impl_t *mp;
	ddi_dma_attr_t *attr_p;
	ddi_dma_lim_t *lim_p	= dmareq->dmar_limits;
	uint32_t count_max	= lim_p->dlim_cntr_max;
	uint64_t lo		= lim_p->dlim_addr_lo;
	uint64_t hi		= lim_p->dlim_addr_hi;
	if (hi <= lo) {
		return ((ddi_dma_impl_t *)DDI_DMA_NOMAPPING);
	}
	if (!count_max)
		count_max--;

	if (!(mp = pci_dma_allocmp(dip, rdip, dmareq->dmar_fp,
	    dmareq->dmar_arg)))
		return (NULL);

	/* store original dev input at the 2nd ddi_dma_attr */
	attr_p = DEV_ATTR(mp);
	SET_DMAATTR(attr_p, lo, hi, -1, count_max);
	SET_DMAALIGN(attr_p, 1);

	count_max = MIN(count_max, hi - lo);

	/* store augumented dev input to mp->dmai_attr */
	mp->dmai_minxfer	= lim_p->dlim_minxfer;
	mp->dmai_burstsizes	= lim_p->dlim_burstsizes;
	attr_p = &mp->dmai_attr;
	SET_DMAATTR(attr_p, lo, hi, -1, count_max);
	SET_DMAALIGN(attr_p, 1);
	return (mp);
}

/*
 * pci_dma_attr2hdl
 *
 * This routine is called from the alloc handle entry point to sanity check the
 * dma attribute structure.
 *
 * use by: pci_dma_allochdl()
 *
 * return value:
 *
 *	DDI_SUCCESS		- on success
 *	DDI_DMA_BADATTR		- attribute has invalid version number
 *				  or address limits exclude dvma space
 */
int
pci_dma_attr2hdl(pci_t *pci_p, ddi_dma_impl_t *mp)
{
	ddi_dma_attr_t *attrp		= DEV_ATTR(mp);
	uint64_t hi		= attrp->dma_attr_addr_hi;
	uint64_t lo		= attrp->dma_attr_addr_lo;
	uint64_t align		= attrp->dma_attr_align;
	uint64_t nocross	= attrp->dma_attr_seg;
	uint64_t count_max	= attrp->dma_attr_count_max;

	if (!nocross)
		nocross--;
	if (attrp->dma_attr_flags & DDI_DMA_FORCE_PHYSICAL)
		return (DDI_DMA_BADATTR);

	if (attrp->dma_attr_granular > IOMMU_PAGE_SIZE) {
		dev_info_t *rdip = mp->dmai_rdip;
		cmn_err(CE_WARN, "%s%d dma_attr_granular > %x",
		    NAMEINST(rdip), IOMMU_PAGE_SIZE);
		return (DDI_DMA_BADATTR);
	}

	if ((attrp->dma_attr_granular & (attrp->dma_attr_granular - 1)) != 0) {
		dev_info_t *rdip = mp->dmai_rdip;
		cmn_err(CE_WARN, "%s%d dma_attr_granular is not aligned",
		    NAMEINST(rdip));
		return (DDI_DMA_BADATTR);
	}

	align = MAX(align, IOMMU_PAGE_SIZE);
	if (((align - 1) & nocross) != (align - 1)) {
		dev_info_t *rdip = mp->dmai_rdip;
		cmn_err(CE_WARN, "%s%d dma_attr_seg not aligned",
		    NAMEINST(rdip));
		return (DDI_DMA_BADATTR);
	}
	if (hi <= lo) {
		dev_info_t *rdip = mp->dmai_rdip;
		cmn_err(CE_WARN, "%s%d limits out of range", NAMEINST(rdip));
		return (DDI_DMA_BADATTR);
	}
	if (!count_max)
		count_max--;

	count_max = MIN(count_max, hi - lo);

	mp->dmai_minxfer	= attrp->dma_attr_minxfer;
	mp->dmai_burstsizes	= attrp->dma_attr_burstsizes;
	attrp = &mp->dmai_attr;
	SET_DMAATTR(attrp, lo, hi, nocross, count_max);
	return (DDI_SUCCESS);
}

/*
 * pci_dma_type - determine which of the three types DMA (peer-to-peer,
 *		iommu bypass, or iommu translate) we are asked to do.
 *		Also checks pfn0 and rejects any non-peer-to-peer
 *		requests for peer-only devices.
 *
 *	return values:
 *		DDI_DMA_NOMAPPING - can't get valid pfn0, or bad dma type
 *		DDI_SUCCESS
 *
 *	dma handle members affected (set on exit):
 *	mp->dmai_object		- dmareq->dmar_object
 *	mp->dmai_rflags		- consistent?, nosync?, dmareq->dmar_flags
 *	mp->dmai_flags   	- DMA type
 *	mp->dmai_pfn0   	- 1st page pfn (if va/size pair and not shadow)
 *	mp->dmai_roffset 	- initialized to starting IOMMU page offset
 *	mp->dmai_ndvmapages	- # of total IOMMU pages of entire object
 *	mp->pdh_sync_buf_pa	- dma sync buffer PA is DMA flow is supported
 */
int
pci_dma_type(pci_t *pci_p, ddi_dma_req_t *dmareq, ddi_dma_impl_t *mp)
{
	dev_info_t *dip = pci_p->pci_dip;
	ddi_dma_obj_t *dobj_p = &dmareq->dmar_object;
	page_t **pplist;
	struct as *as_p;
	uint32_t offset;
	caddr_t vaddr;
	pfn_t pfn0;

	mp->dmai_rflags = dmareq->dmar_flags;
	mp->dmai_flags |= mp->dmai_rflags & DMP_NOSYNC ? DMAI_FLAGS_NOSYNC : 0;

	switch (dobj_p->dmao_type) {
	case DMA_OTYP_BUFVADDR:
	case DMA_OTYP_VADDR:
		vaddr = dobj_p->dmao_obj.virt_obj.v_addr;
		pplist = dobj_p->dmao_obj.virt_obj.v_priv;
		as_p = dobj_p->dmao_obj.virt_obj.v_as;
		if (as_p == NULL)
			as_p = &kas;

		offset = (ulong_t)vaddr & IOMMU_PAGE_OFFSET;

		if (pplist) {				/* shadow list */
			mp->dmai_flags |= DMAI_FLAGS_PGPFN;
			ASSERT(PAGE_LOCKED(*pplist));
			pfn0 = page_pptonum(*pplist);
		} else {
			pfn0 = hat_getpfnum(as_p->a_hat, vaddr);
		}
		break;

	case DMA_OTYP_PAGES:
		offset = dobj_p->dmao_obj.pp_obj.pp_offset;
		mp->dmai_flags |= DMAI_FLAGS_PGPFN;
		pfn0 = page_pptonum(dobj_p->dmao_obj.pp_obj.pp_pp);
		ASSERT(PAGE_LOCKED(dobj_p->dmao_obj.pp_obj.pp_pp));
		break;

	case DMA_OTYP_PADDR:
	default:
		cmn_err(CE_WARN, "%s%d requested unsupported dma type %x",
		    NAMEINST(mp->dmai_rdip), dobj_p->dmao_type);
		return (DDI_DMA_NOMAPPING);
	}
	if (pfn0 == PFN_INVALID) {
		cmn_err(CE_WARN, "%s%d: invalid pfn0 for DMA object %p",
		    NAMEINST(dip), dobj_p);
		return (DDI_DMA_NOMAPPING);
	}
	mp->dmai_object	 = *dobj_p;			/* whole object    */
	mp->dmai_pfn0	 = (void *)pfn0;		/* cache pfn0	   */
	mp->dmai_roffset = offset;			/* win0 pg0 offset */
	mp->dmai_ndvmapages = IOMMU_BTOPR(offset + mp->dmai_object.dmao_size);

	return (DDI_SUCCESS);
}

/*
 * pci_dma_pgpfn - set up pfnlst array according to pages
 *	VA/size pair: <shadow IO, bypass, peer-to-peer>, or OTYP_PAGES
 */
/*ARGSUSED*/
static int
pci_dma_pgpfn(pci_t *pci_p, ddi_dma_impl_t *mp, uint_t npages)
{
	int i;
	switch (mp->dmai_object.dmao_type) {
	case DMA_OTYP_BUFVADDR:
	case DMA_OTYP_VADDR: {
		page_t **pplist = mp->dmai_object.dmao_obj.virt_obj.v_priv;
		for (i = 1; i < npages; i++) {
			iopfn_t pfn = page_pptonum(pplist[i]);
			ASSERT(PAGE_LOCKED(pplist[i]));
			PCI_SET_MP_PFN1(mp, i, pfn);
		}
		}
		break;

	case DMA_OTYP_PAGES: {
		page_t *pp = mp->dmai_object.dmao_obj.pp_obj.pp_pp->p_next;
		for (i = 1; i < npages; i++, pp = pp->p_next) {
			iopfn_t pfn = page_pptonum(pp);
			ASSERT(PAGE_LOCKED(pp));
			PCI_SET_MP_PFN1(mp, i, pfn);
		}
		}
		break;

	default:	/* check is already done by pci_dma_type */
		ASSERT(0);
		break;
	}
	return (DDI_SUCCESS);
}

/*
 * pci_dma_vapfn - set up pfnlst array according to VA
 *	VA/size pair: <normal, bypass, peer-to-peer>
 *	pfn0 is skipped as it is already done.
 *	In this case, the cached pfn0 is used to fill pfnlst[0]
 */
static int
pci_dma_vapfn(pci_t *pci_p, ddi_dma_req_t *dmareq, ddi_dma_impl_t *mp,
	uint_t npages)
{
	dev_info_t *dip = pci_p->pci_dip;
	int i;
	caddr_t vaddr = (caddr_t)mp->dmai_object.dmao_obj.virt_obj.v_as;
	struct hat *hat_p = vaddr ? ((struct as *)vaddr)->a_hat : kas.a_hat;
	caddr_t sva;

	sva = (caddr_t)(((uintptr_t)mp->dmai_object.dmao_obj.virt_obj.v_addr +
	    IOMMU_PAGE_SIZE) & IOMMU_PAGE_MASK);

	for (vaddr = sva, i = 1; i < npages; i++, vaddr += IOMMU_PAGE_SIZE) {
		pfn_t pfn;
		pfn = hat_getpfnum(hat_p, vaddr);
		if (pfn == PFN_INVALID)
			goto err_badpfn;
		PCI_SET_MP_PFN1(mp, i, (iopfn_t)pfn);
	}
	return (DDI_SUCCESS);
err_badpfn:
	cmn_err(CE_WARN, "%s%d: bad page frame vaddr=%p", NAMEINST(dip), vaddr);
	return (DDI_DMA_NOMAPPING);
}

/*
 * pci_dma_pfn - Fills pfn list for all pages being DMA-ed.
 *
 * dependencies:
 *	mp->dmai_ndvmapages	- set to total # of dma pages
 *
 * return value:
 *	DDI_SUCCESS
 *	DDI_DMA_NOMAPPING
 */
int
pci_dma_pfn(pci_t *pci_p, ddi_dma_req_t *dmareq, ddi_dma_impl_t *mp)
{
	uint32_t npages = mp->dmai_ndvmapages;
	int (*waitfp)(caddr_t) = dmareq->dmar_fp;
	int i, ret;

	/* 1 page: no array alloc/fill, no mixed mode check */
	if (npages == 1) {
		PCI_SET_MP_PFN(mp, 0, MP_PFN0(mp));
		return (DDI_SUCCESS);
	}
	/* allocate pfn array */
	if (!(mp->dmai_pfnlst = kmem_alloc(npages * sizeof (iopfn_t),
	    waitfp == DDI_DMA_SLEEP ? KM_SLEEP : KM_NOSLEEP))) {
		if (waitfp != DDI_DMA_DONTWAIT)
			ddi_set_callback(waitfp, dmareq->dmar_arg,
			    &pci_kmem_clid);
		return (DDI_DMA_NORESOURCES);
	}
	/* fill pfn array */
	PCI_SET_MP_PFN(mp, 0, MP_PFN0(mp));	/* pfnlst[0] */
	if ((ret = PCI_DMA_ISPGPFN(mp) ? pci_dma_pgpfn(pci_p, mp, npages) :
	    pci_dma_vapfn(pci_p, dmareq, mp, npages)) != DDI_SUCCESS)
		goto err;

	return (DDI_SUCCESS);
err:
	pci_dma_freepfn(mp);
	return (ret);
}

void
pci_dma_freewin(ddi_dma_impl_t *mp)
{
	pci_dma_win_t *win_p = mp->dmai_winlst, *win2_p;
	for (win2_p = win_p; win_p; win2_p = win_p) {
		win_p = win2_p->win_next;
		kmem_free(win2_p, sizeof (pci_dma_win_t) +
		    sizeof (ddi_dma_cookie_t) * win2_p->win_ncookies);
	}
	mp->dmai_nwin = 0;
	mp->dmai_winlst = NULL;
}

/*
 * pci_dma_newwin - create a dma window object and cookies
 *
 *	After the initial scan in pci_dma_physwin(), which identifies
 *	a portion of the pfn array that belongs to a dma window,
 *	we are called to allocate and initialize representing memory
 *	resources. We know from the 1st scan the number of cookies
 *	or dma segment in this window so we can allocate a contiguous
 *	memory array for the dma cookies (The implementation of
 *	ddi_dma_nextcookie(9f) dictates dma cookies be contiguous).
 *
 *	A second round scan is done on the pfn array to identify
 *	each dma segment and initialize its corresponding dma cookie.
 *	We don't need to do all the safety checking and we know they
 *	all belong to the same dma window.
 *
 *	Input:	cookie_no - # of cookies identified by the 1st scan
 *		start_idx - subscript of the pfn array for the starting pfn
 *		end_idx   - subscript of the last pfn in dma window
 *		win_pp    - pointer to win_next member of previous window
 *	Return:	DDI_SUCCESS - with **win_pp as newly created window object
 *		DDI_DMA_NORESROUCE - caller frees all previous window objs
 *	Note:	Each cookie and window size are all initialized on page
 *		boundary. This is not true for the 1st cookie of the 1st
 *		window and the last cookie of the last window.
 *		We fix that later in upper layer which has access to size
 *		and offset info.
 *
 */
static int
pci_dma_newwin(ddi_dma_req_t *dmareq, ddi_dma_impl_t *mp, uint32_t cookie_no,
	uint32_t start_idx, uint32_t end_idx, pci_dma_win_t **win_pp,
	uint64_t count_max)
{
	int (*waitfp)(caddr_t) = dmareq->dmar_fp;
	ddi_dma_cookie_t *cookie_p;
	uint32_t pfn_no = 1;
	iopfn_t pfn = PCI_GET_MP_PFN(mp, start_idx);
	iopfn_t prev_pfn = pfn;
	uint64_t seg_pfn0 = pfn;
	size_t sz = cookie_no * sizeof (ddi_dma_cookie_t);
	pci_dma_win_t *win_p = kmem_alloc(sizeof (pci_dma_win_t) + sz,
	    waitfp == DDI_DMA_SLEEP ? KM_SLEEP : KM_NOSLEEP);
	if (!win_p)
		goto noresource;

	win_p->win_next = NULL;
	win_p->win_ncookies = cookie_no;
	win_p->win_curseg = 0;	/* start from segment 0 */
	win_p->win_size = IOMMU_PTOB(end_idx - start_idx + 1);
	/* win_p->win_offset is left uninitialized */

	cookie_p = (ddi_dma_cookie_t *)(win_p + 1);
	start_idx++;
	for (; start_idx <= end_idx; start_idx++, prev_pfn = pfn, pfn_no++) {
		pfn = PCI_GET_MP_PFN1(mp, start_idx);
		if ((pfn == prev_pfn + 1) &&
		    (IOMMU_PTOB(pfn_no + 1) - 1 <= count_max))
			continue;

		/* close up the cookie up to (including) prev_pfn */
		MAKE_DMA_COOKIE(cookie_p, IOMMU_PTOB(seg_pfn0) + PYXIS_PCI_DIRECT_MAP_BASE,
		    IOMMU_PTOB(pfn_no));

		cookie_p++;	/* advance to next available cookie cell */
		pfn_no = 0;
		seg_pfn0 = pfn;	/* start a new segment from current pfn */
	}
	MAKE_DMA_COOKIE(cookie_p, IOMMU_PTOB(seg_pfn0) + PYXIS_PCI_DIRECT_MAP_BASE,
	    IOMMU_PTOB(pfn_no));
	*win_pp = win_p;
	return (DDI_SUCCESS);
noresource:
	if (waitfp != DDI_DMA_DONTWAIT)
		ddi_set_callback(waitfp, dmareq->dmar_arg, &pci_kmem_clid);
	return (DDI_DMA_NORESOURCES);
}

/*
 * pci_dma_adjust - adjust 1st and last cookie and window sizes
 *	remove initial dma page offset from 1st cookie and window size
 *	remove last dma page remainder from last cookie and window size
 *	fill win_offset of each dma window according to just fixed up
 *		each window sizes
 *	pci_dma_win_t members modified:
 *	win_p->win_offset - this window's offset within entire DMA object
 *	win_p->win_size	  - xferrable size (in bytes) for this window
 *
 *	ddi_dma_impl_t members modified:
 *	mp->dmai_size	  - 1st window xferrable size
 *	mp->dmai_offset   - 0, which is the dma offset of the 1st window
 *
 *	ddi_dma_cookie_t members modified:
 *	cookie_p->dmac_size - 1st and last cookie remove offset or remainder
 *	cookie_p->dmac_laddress - 1st cookie add page offset
 */
static void
pci_dma_adjust(ddi_dma_req_t *dmareq, ddi_dma_impl_t *mp, pci_dma_win_t *win_p)
{
	pci_dma_win_t *top = win_p;
	ddi_dma_cookie_t *cookie_p = (ddi_dma_cookie_t *)(win_p + 1);
	size_t pg_offset = mp->dmai_roffset;
	size_t win_offset = 0;
	uint32_t granular = mp->dmai_attr.dma_attr_granular;
	if (granular == 0)
		granular = 1;
	boolean_t truncate = (pg_offset % granular);

	if (pg_offset) {
		cookie_p->dmac_size -= pg_offset;
		cookie_p->dmac_laddress |= pg_offset;
		win_p->win_size -= pg_offset;
	}

	while (win_p->win_next) {
		win_p->win_offset = win_offset;

		if (truncate) {
			cookie_p[win_p->win_ncookies - 1].dmac_size -= (IOMMU_PAGE_SIZE - pg_offset);
			win_p->win_size -= (IOMMU_PAGE_SIZE - pg_offset);
		}

		ASSERT((win_p->win_size % granular) == 0);
		win_offset += win_p->win_size;
		win_p = win_p->win_next;

		if (truncate) {
			cookie_p = (ddi_dma_cookie_t *)(win_p + 1);
			cookie_p->dmac_size -= pg_offset;
			cookie_p->dmac_laddress |= pg_offset;
			win_p->win_size -= pg_offset;
		}
	}

	pg_offset += mp->dmai_object.dmao_size;
	pg_offset &= IOMMU_PAGE_OFFSET;
	if (pg_offset)
		pg_offset = IOMMU_PAGE_SIZE - pg_offset;

	/* last window */
	win_p->win_offset = win_offset;
	cookie_p = (ddi_dma_cookie_t *)(win_p + 1);
	cookie_p[win_p->win_ncookies - 1].dmac_size -= pg_offset;
	win_p->win_size -= pg_offset;
	ASSERT((win_offset + win_p->win_size) == mp->dmai_object.dmao_size);

	mp->dmai_size = top->win_size;
	mp->dmai_offset = 0;
}

/*
 * pci_dma_physwin() - carve up dma windows using physical addresses.
 *	Called to handle iommu bypass and pci peer-to-peer transfers.
 *	Calls pci_dma_newwin() to allocate window objects.
 *
 * Dependency: mp->dmai_pfnlst points to an array of pfns
 *
 * 1. Each dma window is represented by a pci_dma_win_t object.
 *	The object will be casted to ddi_dma_win_t and returned
 *	to leaf driver through the DDI interface.
 * 2. Each dma window can have several dma segments with each
 *	segment representing a physically contiguous either memory
 *	space (if we are doing an iommu bypass transfer) or pci address
 *	space (if we are doing a peer-to-peer transfer).
 * 3. Each segment has a DMA cookie to program the DMA engine.
 *	The cookies within each DMA window must be located in a
 *	contiguous array per ddi_dma_nextcookie(9f).
 * 4. The number of DMA segments within each DMA window cannot exceed
 *	mp->dmai_attr.dma_attr_sgllen. If the transfer size is
 *	too large to fit in the sgllen, the rest needs to be
 *	relocated to the next dma window.
 * 5. Peer-to-peer DMA segment follows device hi, lo, count_max,
 *	and nocross restrictions while bypass DMA follows the set of
 *	restrictions with system limits factored in.
 *
 * Return:
 *	mp->dmai_winlst	 - points to a link list of pci_dma_win_t objects.
 *		Each pci_dma_win_t object on the link list contains
 *		infomation such as its window size (# of pages),
 *		starting offset (also see Restriction), an array of
 *		DMA cookies, and # of cookies in the array.
 *	mp->dmai_pfnlst	 - NULL, the pfn list is freed to conserve memory.
 *	mp->dmai_nwin	 - # of total DMA windows on mp->dmai_winlst.
 *	mp->dmai_rflags	 - consistent, nosync, no redzone
 *	mp->dmai_cookie	 - start of cookie table of the 1st DMA window
 *
 * Restriction:
 *	Each pci_dma_win_t object can theoratically start from any offset
 *	since the iommu is not involved. However, this implementation
 *	always make windows start from page aligned offset (except
 *	the 1st window, which follows the requested offset) due to the
 *	fact that we are handed a pfn list. This does require device's
 *	count_max and attr_seg to be at least IOMMU_PAGE_SIZE aligned.
 */
int
pci_dma_physwin(pci_t *pci_p, ddi_dma_req_t *dmareq, ddi_dma_impl_t *mp)
{
	uint_t npages = mp->dmai_ndvmapages;
	int ret, sgllen = mp->dmai_attr.dma_attr_sgllen;
	iopfn_t pfn_lo, pfn_hi, prev_pfn;
	iopfn_t pfn = PCI_GET_MP_PFN(mp, 0);
	uint32_t i, win_no = 0, pfn_no = 1, win_pfn0_index = 0, cookie_no = 0;
	uint64_t count_max;
	pci_dma_win_t **win_pp = (pci_dma_win_t **)&mp->dmai_winlst;
	ddi_dma_cookie_t *cookie0_p;

	count_max = mp->dmai_attr.dma_attr_count_max;
	uint32_t granular = mp->dmai_attr.dma_attr_granular;
	if (granular == 0)
		granular = 1;

	boolean_t truncate = (mp->dmai_roffset % granular);
	for (prev_pfn = pfn, i = 1; i < npages;
	    i++, prev_pfn = pfn, pfn_no++) {
		pfn = PCI_GET_MP_PFN1(mp, i);
		if ((pfn == prev_pfn + 1) &&
		    (IOMMU_PTOB(pfn_no + 1) - 1 <= count_max))
			continue;
		cookie_no++;
		pfn_no = 0;
		if (cookie_no < sgllen)
			continue;

		if (ret = pci_dma_newwin(dmareq, mp, cookie_no,
		    win_pfn0_index, i - 1, win_pp, count_max))
			goto err;

		win_pp = &(*win_pp)->win_next;	/* win_pp = *(win_pp) */
		win_no++;
		cookie_no = 0;
		if (truncate) {
			i--;
			if (win_pfn0_index == i) {
				dev_info_t *rdip = mp->dmai_rdip;
				cmn_err(CE_WARN,
				    "%s%d: sgllen %d offset %d granular %d",
				    NAMEINST(rdip), sgllen, mp->dmai_roffset,
				    granular);
				goto err;
			}
			pfn = PCI_GET_MP_PFN1(mp, i);
		}
		win_pfn0_index = i;
	}

	cookie_no++;
	if (ret = pci_dma_newwin(dmareq, mp, cookie_no, win_pfn0_index,
	    i - 1, win_pp, count_max))
		goto err;
	win_no++;
	pci_dma_adjust(dmareq, mp, mp->dmai_winlst);
	mp->dmai_nwin = win_no;
	mp->dmai_rflags |= DDI_DMA_CONSISTENT;
	mp->dmai_rflags &= ~DDI_DMA_REDZONE;
	cookie0_p = (ddi_dma_cookie_t *)(WINLST(mp) + 1);
	mp->dmai_cookie = cookie0_p + 1;
	mp->dmai_ncookies = WINLST(mp)->win_ncookies;
	mp->dmai_curcookie = 1;
	pci_dma_freepfn(mp);
	return (DDI_DMA_MAPPED);
err:
	pci_dma_freewin(mp);
	return (ret);
}

/*ARGSUSED*/
int
pci_dma_ctl(dev_info_t *dip, dev_info_t *rdip, ddi_dma_impl_t *mp,
	enum ddi_dma_ctlops cmd, off_t *offp, size_t *lenp, caddr_t *objp,
	uint_t cache_flags)
{
	switch (cmd) {

	case DDI_DMA_HTOC: {
		off_t off = *offp;
		ddi_dma_cookie_t *loop_cp, *cp;
		pci_dma_win_t *win_p = mp->dmai_winlst;

		if (off >= mp->dmai_object.dmao_size)
			return (DDI_FAILURE);

		/* locate window */
		while (win_p->win_offset + win_p->win_size <= off)
			win_p = win_p->win_next;

		loop_cp = cp = (ddi_dma_cookie_t *)(win_p + 1);
		mp->dmai_offset = win_p->win_offset;
		mp->dmai_size   = win_p->win_size;

		/* adjust cookie addr/len if we are not on cookie boundary */
		off -= win_p->win_offset;	   /* offset within window */
		for (; off >= loop_cp->dmac_size; loop_cp++)
			off -= loop_cp->dmac_size; /* offset within cookie */

		mp->dmai_cookie = loop_cp + 1;
		win_p->win_curseg = loop_cp - cp;
		cp = (ddi_dma_cookie_t *)objp;
		MAKE_DMA_COOKIE(cp, loop_cp->dmac_laddress + off,
		    loop_cp->dmac_size - off);
		}
		return (DDI_SUCCESS);

	case DDI_DMA_COFF: {
		pci_dma_win_t *win_p;
		ddi_dma_cookie_t *cp;
		uint64_t addr, key = ((ddi_dma_cookie_t *)offp)->dmac_laddress;
		size_t win_off;

		for (win_p = mp->dmai_winlst; win_p; win_p = win_p->win_next) {
			int i;
			win_off = 0;
			cp = (ddi_dma_cookie_t *)(win_p + 1);
			for (i = 0; i < win_p->win_ncookies; i++, cp++) {
				size_t sz = cp->dmac_size;

				addr = cp->dmac_laddress;
				if ((addr <= key) && (addr + sz >= key))
					goto found;
				win_off += sz;
			}
		}
		return (DDI_FAILURE);
found:
		*objp = (caddr_t)(win_p->win_offset + win_off + (key - addr));
		return (DDI_SUCCESS);
		}

	case DDI_DMA_REMAP:
		return (DDI_FAILURE);

	default:
		break;
	}
	return (DDI_FAILURE);
}

