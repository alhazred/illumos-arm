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

#ifndef	_SYS_PCI_DMA_H
#define	_SYS_PCI_DMA_H


#ifdef	__cplusplus
extern "C" {
#endif

typedef	pfn_t iopfn_t;
#define	MAKE_DMA_COOKIE(cp, address, size)	\
	{					\
		(cp)->dmac_notused = 0;		\
		(cp)->dmac_type = 0;		\
		(cp)->dmac_laddress = (address);	\
		(cp)->dmac_size = (size);	\
	}

#define	HAS_REDZONE(mp)	(((mp)->dmai_rflags & DDI_DMA_REDZONE) ? 1 : 0)

#define	PCI_DMA_HAT_NUM_CB_COOKIES	5

typedef struct pci_dma_hdl {
	ddi_dma_impl_t	pdh_ddi_hdl;
	ddi_dma_attr_t	pdh_attr_dev;
	uint64_t	pdh_sync_buf_pa;
	void		*pdh_cbcookie[PCI_DMA_HAT_NUM_CB_COOKIES];
} pci_dma_hdl_t;

/*
 * flags for overloading dmai_inuse field of the dma request
 * structure:
 */
#define	dmai_flags		dmai_inuse
#define	dmai_tte		dmai_nexus_private
#define	dmai_pfnlst		dmai_iopte
#define	dmai_winlst		dmai_minfo
#define	dmai_pfn0		dmai_sbi
#define	dmai_roffset		dmai_pool

#define	MP_PFN0(mp)		((iopfn_t)(mp)->dmai_pfn0)
#define	MP_HAT_CB_COOKIE(mp, i)	((i < PCI_DMA_HAT_NUM_CB_COOKIES)? \
	(((pci_dma_hdl_t *)(mp))->pdh_cbcookie[i]) : NULL)
#define	MP_HAT_CB_COOKIE_PTR(mp, i) \
	((i < PCI_DMA_HAT_NUM_CB_COOKIES)? \
	&(((pci_dma_hdl_t *)(mp))->pdh_cbcookie[i]) : NULL)
#define	WINLST(mp)		((pci_dma_win_t *)(mp)->dmai_winlst)
#define	DEV_ATTR(mp)		(&((pci_dma_hdl_t *)(mp))->pdh_attr_dev)
#define	SYNC_BUF_PA(mp)		(((pci_dma_hdl_t *)(mp))->pdh_sync_buf_pa)
#define	SET_DMAATTR(p, lo, hi, nocross, cntmax)	\
	(p)->dma_attr_addr_lo	= (lo); \
	(p)->dma_attr_addr_hi	= (hi); \
	(p)->dma_attr_seg	= (nocross); \
	(p)->dma_attr_count_max	= (cntmax);

#define	SET_DMAALIGN(p, align) \
	(p)->dma_attr_align = (align);

#define	DMAI_FLAGS_INUSE	0x1
#define	DMAI_FLAGS_PGPFN	0x800
#define	DMAI_FLAGS_NOSYNC	0x4000
#define	DMAI_FLAGS_MAPPED	0x10000

#define	PCI_DMA_ISPGPFN(mp)	((mp)->dmai_flags & DMAI_FLAGS_PGPFN)
#define	PCI_DMA_WINNPGS(mp)	IOMMU_BTOP((mp)->dmai_winsize)
#define	PCI_DMA_ISMAPPED(mp)	((mp)->dmai_flags & DMAI_FLAGS_MAPPED)

#define	PCI_SYNC_FLAG_SZSHIFT	6
#define	PCI_SYNC_FLAG_SIZE	(1 << PCI_SYNC_FLAG_SZSHIFT)
#define	PCI_SYNC_FLAG_FAILED	1
#define	PCI_SYNC_FLAG_LOCKED	2

#define	PCI_DMA_CURWIN(mp) \
	(((mp)->dmai_offset + (mp)->dmai_roffset) / (mp)->dmai_winsize)

typedef struct pci_dma_win {
	struct pci_dma_win *win_next;
	uint32_t win_ncookies;
	uint32_t win_curseg;
	uint64_t win_size;
	uint64_t win_offset;
	/* cookie table: sizeof (ddi_dma_cookie_t) * win_ncookies */
} pci_dma_win_t;

extern int pci_dma_sync(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_handle_t handle, off_t off, size_t len, uint32_t sync_flags);

extern int pci_dma_win(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_handle_t handle, uint_t win, off_t *offp,
	size_t *lenp, ddi_dma_cookie_t *cookiep, uint_t *ccountp);

extern ddi_dma_impl_t *pci_dma_allocmp(dev_info_t *dip, dev_info_t *rdip,
	int (*waitfp)(caddr_t), caddr_t arg);
extern void pci_dma_freemp(ddi_dma_impl_t *mp);
extern void pci_dma_freepfn(ddi_dma_impl_t *mp);
extern ddi_dma_impl_t *pci_dma_lmts2hdl(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_req_t *dmareq);
extern int pci_dma_attr2hdl(pci_t *pci_p, ddi_dma_impl_t *mp);
extern int pci_dma_type(pci_t *pci_p, ddi_dma_req_t *req, ddi_dma_impl_t *mp);
extern int pci_dma_pfn(pci_t *pci_p, ddi_dma_req_t *req, ddi_dma_impl_t *mp);
extern void pci_dma_freewin(ddi_dma_impl_t *mp);
extern void pci_dma_sync_unmap(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_impl_t *mp);
extern int pci_dma_physwin(pci_t *pci_p, ddi_dma_req_t *dmareq,
	ddi_dma_impl_t *mp);
extern int pci_dma_ctl(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_impl_t *mp, enum ddi_dma_ctlops cmd, off_t *offp,
	size_t *lenp, caddr_t *objp, uint_t cache_flags);

#define	PCI_GET_MP_NCOOKIES(mp)		((mp)->dmai_ncookies)
#define	PCI_SET_MP_NCOOKIES(mp, nc)	((mp)->dmai_ncookies = (nc))
#define	PCI_GET_MP_PFN1_ADDR(mp)	(((iopfn_t *)(mp)->dmai_pfnlst) + 1)

#define	PCI_GET_MP_TTE(tte) \
	(((uint64_t)(uintptr_t)(tte) >> 5) << (32 + 5) | \
	    ((uint32_t)(uintptr_t)(tte)) & 0x12)
#define	PCI_SAVE_MP_TTE(mp, tte)	\
	(mp)->dmai_tte = (caddr_t)(HI32(tte) | ((tte) & 0x12))

#define	PCI_GET_MP_PFN1(mp, page_no) (((iopfn_t *)(mp)->dmai_pfnlst)[page_no])
#define	PCI_GET_MP_PFN(mp, page_no)	((mp)->dmai_ndvmapages == 1 ? \
	(iopfn_t)(mp)->dmai_pfnlst : PCI_GET_MP_PFN1(mp, page_no))

#define	PCI_SET_MP_PFN(mp, page_no, pfn) { \
	if ((mp)->dmai_ndvmapages == 1) { \
		ASSERT(!((page_no) || (mp)->dmai_pfnlst)); \
		(mp)->dmai_pfnlst = (void *)(pfn); \
	} else \
		((iopfn_t *)(mp)->dmai_pfnlst)[page_no] = (iopfn_t)(pfn); \
}
#define	PCI_SET_MP_PFN1(mp, page_no, pfn) { \
	((iopfn_t *)(mp)->dmai_pfnlst)[page_no] = (pfn); \
}

#define	GET_TTE_TEMPLATE(mp) MAKE_TTE_TEMPLATE(PCI_GET_MP_PFN((mp), 0), (mp))

extern int pci_dma_freehdl(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_handle_t handle);

int pci_dma_handle_clean(dev_info_t *rdip, ddi_dma_handle_t handle);

#if defined(DEBUG)
extern void dump_dma_handle(uint64_t flag, dev_info_t *dip, ddi_dma_impl_t *hp);
#else
#define	dump_dma_handle(flag, dip, hp)
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_PCI_DMA_H */
