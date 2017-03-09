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

#include <sys/types.h>
#include <sys/saio.h>
#include <sys/sysmacros.h>
#include <sys/promif.h>
#include <sys/bootconf.h>
#include <sys/salib.h>
#include <sys/memlist.h>
#include <sys/machparam.h>
#include <sys/cpu.h>
#include <sys/hwrpb.h>
#include <sys/pte.h>
#include <sys/pal.h>
#include <sys/memlist_impl.h>
#include <sys/boot.h>

#ifdef DEBUG
static int	debug = 1;
#else /* DEBUG */
extern int	debug;
#endif /* DEBUG */
#define	dprintf	if (debug) printf

extern struct memlist	*pfreelistp, *pinstalledp, *pscratchlistp;
extern pte_t *l1_ptbl;

static struct alpha_pcb bootpcb __attribute__((aligned (64)));
extern caddr_t		memlistpage;

extern char _BootScratch[];
extern char _RamdiskStart[];
extern char _RamdiskEnd[];
static caddr_t scratch_used_top;

extern uint64_t memlist_get(uint64_t size, int align, struct memlist **listp);


static void init_pt(void);
static void map_phys(uint32_t pte_attr, caddr_t vaddr, uint64_t paddr, size_t bytes);
static void remap_console(void);
static pte_t *vptb_l1;
extern void load_pcb(paddr_t newpcb_pa, struct alpha_pcb *oldpcb_va);
extern void update_hwrpb_cksum(void);

static inline caddr_t pa_to_kseg(paddr_t paddr) { return (caddr_t)(KSEG_BASE | paddr); }
static inline paddr_t kseg_to_pa(caddr_t vaddr) { return ((1ul << PA_BITS) - 1) & (paddr_t)vaddr; }
static inline paddr_t pfn_to_pa(pfn_t pfn) { return ((paddr_t)pfn) << PAGESHIFT; }
static inline pfn_t pa_to_pfn(paddr_t paddr) { return (pfn_t)(paddr >> PAGESHIFT); }
static inline pfn_t kseg_to_pfn(caddr_t vaddr) { return pa_to_pfn(kseg_to_pa(vaddr));}
static inline caddr_t pfn_to_kseg(pfn_t pfn) { return pa_to_kseg(pfn_to_pa(pfn)); }
static inline int l1_pteidx(caddr_t vaddr) { return ((((uintptr_t)vaddr) >> (PAGESHIFT+2*NPTESHIFT)) & ((1<<NPTESHIFT)-1));}
static inline int l2_pteidx(caddr_t vaddr) { return ((((uintptr_t)vaddr) >> (PAGESHIFT+NPTESHIFT)) & ((1<<NPTESHIFT)-1));}
static inline int l3_pteidx(caddr_t vaddr) { return ((((uintptr_t)vaddr) >> (PAGESHIFT)) & ((1<<NPTESHIFT)-1));}
static inline pte_t va_to_pte(caddr_t vaddr) { return ((pte_t *)hwrpb->vptb)[VPT_IDX(vaddr)]; }
static inline pfn_t va_to_pfn(caddr_t vaddr) { return va_to_pte(vaddr) >> 32; }
static inline paddr_t va_to_pa(caddr_t vaddr) { return pfn_to_pa(va_to_pfn(vaddr)) | ((paddr_t)vaddr & MMU_PAGEOFFSET); }


/*
 * OSF/1 PALcodeに切り替える
 * これ以降、KSEGのメモリにアクセスできるようになる。
 */
extern void swap_pal(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
void
init_pal()
{
	struct pcs *pcs0 =
	    (struct pcs *)((caddr_t)hwrpb + hwrpb->pcs_off + hwrpb->pcs_size * hwrpb->cpu_id);
	paddr_t pcbb = va_to_pa((caddr_t)&bootpcb);

	bootpcb.usp = 0;
	bootpcb.ptbr = pcs0->hwpcb.ptbr;
	bootpcb.asn = 0;
	bootpcb.uniq = 0;
	bootpcb.flags = 0;

	swap_pal(PAL_OSF, 0, pcbb, hwrpb->vptb, (uint64_t)&bootpcb.ksp);
	pal_tbia();

	pcs0->pal_rev = pcs0->pal_rev_avl[PAL_OSF];
	pcs0->stat_flags &= ~PCS_STAT_BIP;	// BIP clear
}

void
init_memory(void)
{
	init_pt();
	remap_console();
	kmem_init();
}

/*
 * memlistの初期化
 *  Memory Data Descriptor Table (mddt) から、
 *  − システムのメモリ
 *  − 空きメモリ(コンソールが使用していないメモリ)
 *  - ブートローダが使用するメモリ
 *  の情報を取得する。
 *
 *  ブートローダ自体のメモリは、mddtで freeとなっていることに注意。
 *  →物理メモリの最初の方が使用される(_BootScratch[] を調整済み)
 */
void
init_memlists(void)
{
	int i;
	struct mddt *mddt;
	uintptr_t pmem_min = kseg_to_pa(_RamdiskEnd);
	uintptr_t pmem_top = 0;

	scratch_used_top = _BootScratch;
	memlistpage = scratch_used_top;
	scratch_used_top += MMU_PAGESIZE;

	mddt = (struct mddt *)((caddr_t)hwrpb + hwrpb->mddt_off);
	for (i = 0; i < mddt->cluster_num; i++) {
		struct mddt_cluster *mddtc = &mddt->cluster[i];
		uintptr_t pa_start = pfn_to_pa(mddtc->pfn);
		size_t size = mddtc->pfncount * MMU_PAGESIZE;

		if (pmem_top < pa_start + size)
			pmem_top = pa_start + size;

		if (mddtc->usage == 0) {
			if (pa_start <= pmem_min) {
				if (pmem_min <= (pa_start + size)) {
					memlist_add_span(pa_start,  pmem_min - pa_start, &pscratchlistp);
					memlist_add_span(pmem_min, (pa_start + size) - pmem_min, &pfreelistp);
				} else {
					memlist_add_span(pa_start, size, &pscratchlistp);
				}
			} else {
				memlist_add_span(pa_start, size, &pfreelistp);
			}
		}
	}

	/*
	 * 一旦、コンソールが使用している領域も追加してから、
	 * 使用中のものを削除する。
	 */
	memlist_add_span(0, pmem_top, &pinstalledp);
}


/*
 * ページテーブルの初期化
 */
static void
init_pt(void)
{
	int i;
	uint64_t vptb_idx;
	uintptr_t paddr;

	vptb_idx = l1_pteidx((caddr_t)hwrpb->vptb);
	vptb_l1 = (pte_t *)(hwrpb->vptb | (vptb_idx << PAGESHIFT) |
	    (vptb_idx << (PAGESHIFT + NPTESHIFT)) |
	    (vptb_idx << (PAGESHIFT + NPTESHIFT * 2)));

	paddr = memlist_get(PAGESIZE, PAGESIZE, &pfreelistp);
	if (paddr == 0)
		prom_panic("phy alloc error for L1 PT\n");

	l1_ptbl = (uint64_t *)pa_to_kseg(paddr);

	/* コンソールがマッピングしている領域を消さないようにコピーする。 */
	memcpy(l1_ptbl, (void *)vptb_l1, PAGESIZE);

	/* SEG1は L2 page tableまでマッピングしておく */
	for (i = l1_pteidx((caddr_t)SEG1_BASE); i < NPTEPERPT; i++) {
		if (i == l1_pteidx((caddr_t)VPT_BASE)) {
			paddr = kseg_to_pa((caddr_t)l1_ptbl);
		} else {
			paddr = memlist_get(PAGESIZE, PAGESIZE, &pfreelistp);
			if (paddr == 0)
				prom_panic("phy alloc error for L2 PT\n");
			bzero((void *)pa_to_kseg(paddr), PAGESIZE);
		}
		l1_ptbl[i] = (pa_to_pfn(paddr)<<32) | PTE_KRE | PTE_VALID;
	}

	/* 新しいページテーブルの有効化 */
	bootpcb.usp = 0;
	bootpcb.ptbr = kseg_to_pfn((caddr_t)l1_ptbl);
	bootpcb.cc = 0;
	bootpcb.asn = 0;
	bootpcb.uniq = 0;
	bootpcb.flags = 0;
	load_pcb(va_to_pa((caddr_t)&bootpcb), &bootpcb);
	pal_wrvptptr(VPT_BASE);
	hwrpb->vptb = VPT_BASE;
	update_hwrpb_cksum();
	pal_tbia();
}

static void
remap_console(void)
{
	int i;

	// remap HWRPB
	map_phys(PTE_KWE | PTE_KRE, (caddr_t)CONSOLE_BASE, hwrpb->phys_addr, hwrpb->size);
	hwrpb = (struct rpb *)CONSOLE_BASE;

	// fixup
	uintptr_t va = roundup(CONSOLE_BASE + hwrpb->size, MMU_PAGESIZE);
	struct crblk *crb = (struct crblk *)pa_to_kseg(hwrpb->phys_addr + hwrpb->crb_off);

	// remap CRB
	prom_fixup((uintptr_t)va, (uintptr_t)CONSOLE_BASE);
	for (i = 0; i < crb->num; i++) {
		crb->map[i].va = va;
		map_phys(PTE_KWE | PTE_KRE, (caddr_t)va, crb->map[i].pa, crb->map[i].num * MMU_PAGESIZE);
		va += crb->map[i].num * MMU_PAGESIZE;
	}

	/* Update DISPATCH and FIXUP */
	crb->va_disp = crb->map[0].va + crb->pa_disp - crb->map[0].pa;
	crb->va_fixup = crb->map[0].va + crb->pa_fixup - crb->map[0].pa;

}

static void
map_phys(uint32_t pte_attr, caddr_t vaddr, uint64_t paddr, size_t bytes)
{
	pfn_t pfn = pa_to_pfn(paddr);
	size_t count = roundup(bytes, PAGESIZE) / PAGESIZE;
	size_t idx;
	int l1_idx = l1_pteidx(vaddr);
	int l2_idx = l2_pteidx(vaddr);
	int l3_idx = l3_pteidx(vaddr);

	for (idx = 0; idx < count; idx++) {
		pte_t *l2_ptbl;
		pte_t *l3_ptbl;

		if ((l1_ptbl[l1_idx] & PTE_VALID) == 0) {
			paddr_t pa = memlist_get(PAGESIZE, PAGESIZE, &pfreelistp);
			if (pa == 0)
				prom_panic("phy alloc error for L1 PT\n");
			pte_t *ptbl = (pte_t *)pa_to_kseg(pa);
			bzero(ptbl, PAGESIZE);
			l1_ptbl[l1_idx] = (kseg_to_pfn((caddr_t)ptbl)<<32) | PTE_KRE | PTE_VALID;
		}

		l2_ptbl = (pte_t *)pfn_to_kseg(l1_ptbl[l1_idx]>>32);

		if ((l2_ptbl[l2_idx] & PTE_VALID) == 0) {
			paddr_t pa = memlist_get(PAGESIZE, PAGESIZE, &pfreelistp);
			if (pa == 0)
				prom_panic("phy alloc error for L2 PT\n");
			pte_t *ptbl = (pte_t *)pa_to_kseg(pa);
			bzero(ptbl, PAGESIZE);
			l2_ptbl[l2_idx] = (kseg_to_pfn((caddr_t)ptbl)<<32) | PTE_KRE | PTE_VALID;
		}

		l3_ptbl = (pte_t *)pfn_to_kseg(l2_ptbl[l2_idx]>>32);

		l3_ptbl[l3_idx] = (pfn<<32) | pte_attr | PTE_VALID | PTE_ASM | PTE_BOOTMAP;

		l3_idx = (l3_idx + 1) % NPTEPERPT;
		if (l3_idx == 0) {
			l2_idx = (l2_idx + 1) % NPTEPERPT;
			if (l2_idx == 0) {
				l1_idx = (l1_idx + 1) % NPTEPERPT;
			}
		}
		pfn++;
	}
	pal_tbia();
}


static caddr_t
get_low_vpage(size_t bytes)
{
	caddr_t v;

	if ((scratch_used_top + bytes) <= _RamdiskStart) {
		v = scratch_used_top;
		scratch_used_top += bytes;
		return (v);
	}

	return (NULL);
}

caddr_t
resalloc(enum RESOURCES type, size_t bytes, caddr_t virthint, int align)
{
	caddr_t	vaddr = 0;
	uintptr_t paddr = 0;

	if (bytes != 0) {
		/* extend request to fill a page */
		bytes = roundup(bytes, MMU_PAGESIZE);
		dprintf("resalloc:  bytes = %lu\n", bytes);
		switch (type) {
		case RES_BOOTSCRATCH:
			vaddr = get_low_vpage(bytes);
			break;
		case RES_CHILDVIRT:
			vaddr = virthint;
			while (bytes) {
				uintptr_t va = (uintptr_t)virthint;
				size_t maxalign = va & (-va);
				uint64_t gh;
				size_t mapsz;
				if (maxalign >= MMU_PAGESIZE4M && bytes >= MMU_PAGESIZE4M) {
					mapsz = MMU_PAGESIZE4M;
					gh = PTE_GH_4M;
				} else if (maxalign >= MMU_PAGESIZE512K && bytes >= MMU_PAGESIZE512K) {
					mapsz = MMU_PAGESIZE512K;
					gh = PTE_GH_512K;
				} else if (maxalign >= MMU_PAGESIZE64K && bytes >= MMU_PAGESIZE64K) {
					mapsz = MMU_PAGESIZE64K;
					gh = PTE_GH_64K;
				} else {
					mapsz = MMU_PAGESIZE;
					gh = 0;
				}
				paddr = memlist_get(mapsz, mapsz, &pfreelistp);
				if (paddr == 0) {
					prom_panic("phys mem allocate error\n");
				}
				map_phys(PTE_KWE | PTE_KRE | gh, virthint, paddr, mapsz);
				bytes -= mapsz;
				virthint += mapsz;
			}
			break;
		default:
			dprintf("Bad resurce type\n");
			break;
		}
	}

	dprintf("resalloc:  vaddr = %p\n", vaddr);
	return vaddr;
}

void
reset_alloc(void)
{}

void
resfree(enum RESOURCES type, caddr_t virtaddr, size_t size)
{}
