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
 * Copyright (c) 1992, 2010, Oracle and/or its affiliates. All rights reserved.
 */
/*
 * Copyright (c) 2010, Intel Corporation.
 * All rights reserved.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/*	All Rights Reserved   */

/*
 * Portions of this source code were derived from Berkeley 4.3 BSD
 * under license from the Regents of the University of California.
 */

/*
 * UNIX machine dependent virtual memory support.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <sys/vmem.h>
#include <sys/buf.h>
#include <sys/cpuvar.h>
#include <sys/lgrp.h>
#include <sys/disp.h>
#include <sys/vm.h>
#include <sys/mman.h>
#include <sys/vnode.h>
#include <sys/cred.h>
#include <sys/exec.h>
#include <sys/exechdr.h>
#include <sys/debug.h>
#include <sys/vmsystm.h>
#include <sys/swap.h>
#include <sys/dumphdr.h>
#include <sys/memnode.h>
#include <sys/random.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_kp.h>
#include <vm/seg_vn.h>
#include <vm/page.h>
#include <vm/seg_kmem.h>
#include <vm/seg_kpm.h>
#include <vm/vm_dep.h>

#include <sys/cpu.h>
#include <sys/vm_machparam.h>
#include <sys/memlist.h>
#include <sys/bootconf.h> /* XXX the memlist stuff belongs in memlist_plat.h */
#include <vm/hat_alpha.h>
#include <sys/cmn_err.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/secflags.h>

#include <sys/vtrace.h>
#include <sys/ddidmareq.h>
#include <sys/promif.h>
#include <sys/memnode.h>
#include <sys/stack.h>
#include <util/qsort.h>
#include <sys/taskq.h>
#include <sys/elf_alpha.h>

uint_t vac_colors = 1;

int largepagesupport = 0;
extern uint_t page_create_new;
extern uint_t page_create_exists;
extern uint_t page_create_putbacks;

/*
 * Only let one thread at a time try to coalesce large pages, to
 * prevent them from working against each other.
 */
static kmutex_t	contig_lock;
#define	CONTIG_LOCK()	mutex_enter(&contig_lock);
#define	CONTIG_UNLOCK()	mutex_exit(&contig_lock);

/*
 * combined memory ranges from mnode and memranges[] to manage single
 * mnode/mtype dimension in the page lists.
 */
typedef struct {
	pfn_t	mnr_pfnlo;
	pfn_t	mnr_pfnhi;
	int	mnr_mnode;
	int	mnr_memrange;		/* index into memranges[] */
	int	mnr_next;		/* next lower PA mnoderange */
	int	mnr_exists;
	/* maintain page list stats */
	pgcnt_t	mnr_mt_clpgcnt;		/* cache list cnt */
	pgcnt_t	mnr_mt_flpgcnt[MMU_PAGE_SIZES];	/* free list cnt per szc */
	pgcnt_t	mnr_mt_totcnt;		/* sum of cache and free lists */
#ifdef DEBUG
	struct mnr_mts {		/* mnode/mtype szc stats */
		pgcnt_t	mnr_mts_pgcnt;
		int	mnr_mts_colors;
		pgcnt_t *mnr_mtsc_pgcnt;
	} 	*mnr_mts;
#endif
} mnoderange_t;

#define	MEMRANGEHI(mtype)						\
	((mtype > 0) ? memranges[mtype - 1] - 1: physmax)
#define	MEMRANGELO(mtype)	(memranges[mtype])

#define	MTYPE_FREEMEM(mt)	(mnoderanges[mt].mnr_mt_totcnt)

/*
 * As the PC architecture evolved memory up was clumped into several
 * ranges for various historical I/O devices to do DMA.
 * < 16Meg - ISA bus
 * < 2Gig - ???
 * < 4Gig - PCI bus or drivers that don't understand PAE mode
 *
 * These are listed in reverse order, so that we can skip over unused
 * ranges on machines with small memories.
 *
 * For now under the Hypervisor, we'll only ever have one memrange.
 */
#define	PFN_4GIG	0x100000
/* Indices into the memory range (arch_memranges) array. */
#define	MRI_4G		0
#define	MRI_2G		1
#define	MRI_0		2
static pfn_t arch_memranges[NUM_MEM_RANGES] = {
    PFN_4GIG,	/* pfn range for 4G and above */
    0x80000,	/* pfn range for 2G-4G */
    0x00000,	/* pfn range for 0-2G */
};
pfn_t *memranges = &arch_memranges[0];
int nranges = NUM_MEM_RANGES;
mnoderange_t	*mnoderanges;
int		mnoderangecnt;
int		mtypetop;	/* index of highest pfn'ed mnoderange */

/*
 * This can be patched via /etc/system to allow old non-PAE aware device
 * drivers to use kmem_alloc'd memory on 32 bit systems with > 4Gig RAM.
 */
int restricted_kmemalloc = 0;

#ifdef VM_STATS
struct {
	ulong_t	pga_alloc;
	ulong_t	pga_notfullrange;
	ulong_t	pga_nulldmaattr;
	ulong_t	pga_allocok;
	ulong_t	pga_allocfailed;
	ulong_t	pgma_alloc;
	ulong_t	pgma_allocok;
	ulong_t	pgma_allocfailed;
	ulong_t	pgma_allocempty;
} pga_vmstats;
#endif

/*
 * Number of pages in 1 GB.  Don't enable automatic large pages if we have
 * fewer than this many pages.
 */
pgcnt_t shm_lpg_min_physmem = 1 << (30 - MMU_PAGESHIFT);
pgcnt_t privm_lpg_min_physmem = 1 << (30 - MMU_PAGESHIFT);

/*
 * Maximum and default segment size tunables for user private
 * and shared anon memory, and user text and initialized data.
 * These can be patched via /etc/system to allow large pages
 * to be used for mapping application private and shared anon memory.
 */
size_t max_uheap_lpsize = MMU_PAGESIZE;
size_t default_uheap_lpsize = MMU_PAGESIZE;
size_t max_ustack_lpsize = MMU_PAGESIZE;
size_t default_ustack_lpsize = MMU_PAGESIZE;
size_t max_privmap_lpsize = MMU_PAGESIZE;
size_t max_uidata_lpsize = MMU_PAGESIZE;
size_t max_utext_lpsize = MMU_PAGESIZE;
size_t max_shm_lpsize = MMU_PAGESIZE;


/*
 * initialized by page_coloring_init().
 */
uint_t	page_colors;
uint_t	page_colors_mask;
uint_t	page_coloring_shift;
int	cpu_page_colors;
static uint_t	l2_colors;

/*
 * Page freelists and cachelists are dynamically allocated once mnoderangecnt
 * and page_colors are calculated from the l2 cache n-way set size.  Within a
 * mnode range, the page freelist and cachelist are hashed into bins based on
 * color. This makes it easier to search for a page within a specific memory
 * range.
 */
#define	PAGE_COLORS_MIN	16

page_t ****page_freelists;
page_t ***page_cachelists;


/*
 * Used by page layer to know about page sizes
 */
hw_pagesize_t hw_page_array[MMU_PAGE_SIZES + 1] =
{
	{MMU_PAGESIZE, MMU_PAGESHIFT, 0, MMU_PAGESIZE >> MMU_PAGESHIFT},
	{0, 0, 0, 0}
};
kmutex_t	*fpc_mutex[NPC_MUTEX];
kmutex_t	*cpc_mutex[NPC_MUTEX];

/*
 * Return the optimum page size for a given mapping
 */
/*ARGSUSED*/
size_t
map_pgsz(int maptype, struct proc *p, caddr_t addr, size_t len, int memcntl)
{
	return (MMU_PAGESIZE);
}

/*
 * Return a bit vector of large page size codes that
 * can be used to map [addr, addr + len) region.
 */
/*ARGSUSED*/
uint_t
map_pgszcvec(caddr_t addr, size_t size, uintptr_t off, int flags, int type,
    int memcntl)
{
	return (0);
}

/*
 * Handle a pagefault.
 */
faultcode_t
pagefault(
	caddr_t addr,
	enum fault_type type,
	enum seg_rw rw,
	int iskernel)
{
	struct as *as;
	struct hat *hat;
	struct proc *p;
	kthread_t *t;
	faultcode_t res;
	caddr_t base;
	size_t len;
	int err;
	int mapped_red;
	uintptr_t ea;

	ASSERT_STACK_ALIGNED();

	if (INVALID_VADDR(addr))
		return (FC_NOMAP);

	mapped_red = segkp_map_red();

	if (iskernel) {
		as = &kas;
		hat = as->a_hat;
	} else {
		t = curthread;
		p = ttoproc(t);
		as = p->p_as;
		hat = as->a_hat;
	}

	/*
	 * Dispatch pagefault.
	 */
	res = as_fault(hat, as, addr, 1, type, rw);

	/*
	 * If this isn't a potential unmapped hole in the user's
	 * UNIX data or stack segments, just return status info.
	 */
	if (res != FC_NOMAP || iskernel)
		goto out;

	/*
	 * Check to see if we happened to faulted on a currently unmapped
	 * part of the UNIX data or stack segments.  If so, create a zfod
	 * mapping there and then try calling the fault routine again.
	 */
	base = p->p_brkbase;
	len = p->p_brksize;

	if (addr < base || addr >= base + len) {		/* data seg? */
		base = (caddr_t)p->p_usrstack - p->p_stksize;
		len = p->p_stksize;
		if (addr < base || addr >= p->p_usrstack) {	/* stack seg? */
			/* not in either UNIX data or stack segments */
			res = FC_NOMAP;
			goto out;
		}
	}

	/* the rest of this function implements a 3.X 4.X 5.X compatibility */
	/* This code is probably not needed anymore */

	/* expand the gap to the page boundaries on each side */
	len = (((uintptr_t)base + len + PAGEOFFSET) & PAGEMASK) -
	    ((uintptr_t)base & PAGEMASK);
	base = (caddr_t)((uintptr_t)base & PAGEMASK);

	as_rangelock(as);
	as_purge(as);
	if (as_gap(as, PAGESIZE, &base, &len, AH_CONTAIN, addr) == 0) {
		err = as_map(as, base, len, segvn_create, zfod_argsp);
		as_rangeunlock(as);
		if (err) {
			res = FC_MAKE_ERR(err);
			goto out;
		}
	} else {
		/*
		 * This page is already mapped by another thread after we
		 * returned from as_fault() above.  We just fallthrough
		 * as_fault() below.
		 */
		as_rangeunlock(as);
	}

	res = as_fault(as->a_hat, as, addr, 1, F_INVAL, rw);

out:
	if (mapped_red)
		segkp_unmap_red();

	return (res);
}

void
map_addr(caddr_t *addrp, size_t len, offset_t off, int vacalign, uint_t flags)
{
	struct proc *p = curproc;
	caddr_t userlimit =  p->p_as->a_userlimit;

	map_addr_proc(addrp, len, off, vacalign, userlimit, curproc, flags);
}

/*ARGSUSED*/
int
map_addr_vacalign_check(caddr_t addr, u_offset_t off)
{
	return (0);
}

/*
 * The maximum amount a randomized mapping will be slewed.  We should perhaps
 * arrange things so these tunables can be separate for mmap, mmapobj, and
 * ld.so
 */
size_t aslr_max_map_skew = 256 * 1024 * 1024; /* 256MB */

/*
 * map_addr_proc() is the routine called when the system is to
 * choose an address for the user.  We will pick an address
 * range which is the highest available below userlimit.
 *
 * Every mapping will have a redzone of a single page on either side of
 * the request. This is done to leave one page unmapped between segments.
 * This is not required, but it's useful for the user because if their
 * program strays across a segment boundary, it will catch a fault
 * immediately making debugging a little easier.  Currently the redzone
 * is mandatory.
 *
 * addrp is a value/result parameter.
 *	On input it is a hint from the user to be used in a completely
 *	machine dependent fashion.  We decide to completely ignore this hint.
 *	If MAP_ALIGN was specified, addrp contains the minimal alignment, which
 *	must be some "power of two" multiple of pagesize.
 *
 *	On output it is NULL if no address can be found in the current
 *	processes address space or else an address that is currently
 *	not mapped for len bytes with a page of red zone on either side.
 *
 *	vacalign is not needed on x86 (it's for viturally addressed caches)
 */
/*ARGSUSED*/
void
map_addr_proc(
	caddr_t *addrp,
	size_t len,
	offset_t off,
	int vacalign,
	caddr_t userlimit,
	struct proc *p,
	uint_t flags)
{
	struct as *as = p->p_as;
	caddr_t addr;
	caddr_t base;
	size_t slen;
	size_t align_amount;

	ASSERT32(userlimit == as->a_userlimit);

	base = p->p_brkbase;
	slen = userlimit - base;

	/* Make len be a multiple of PAGESIZE */
	len = (len + PAGEOFFSET) & PAGEMASK;

	/*
	 * figure out what the alignment should be
	 */
	align_amount = ELF_ALPHA_MAXPGSZ;
	if ((flags & MAP_ALIGN) && ((uintptr_t)*addrp > align_amount))
		align_amount = (uintptr_t)*addrp;

	ASSERT(ISP2(align_amount));
	ASSERT(align_amount == 0 || align_amount >= PAGESIZE);

	off = off & (align_amount - 1);
	/*
	 * Look for a large enough hole starting below userlimit.
	 * After finding it, use the upper part.
	 */
	if (as_gap_aligned(as, len, &base, &slen, AH_HI, NULL, align_amount,
	    PAGESIZE, off) == 0) {
		caddr_t as_addr;

		/*
		 * addr is the highest possible address to use since we have
		 * a PAGESIZE redzone at the beginning and end.
		 */
		addr = base + slen - (PAGESIZE + len);
		as_addr = addr;
		/*
		 * Round address DOWN to the alignment amount and
		 * add the offset in.
		 * If addr is greater than as_addr, len would not be large
		 * enough to include the redzone, so we must adjust down
		 * by the alignment amount.
		 */
		addr = (caddr_t)((uintptr_t)addr & (~(align_amount - 1)));
		addr += (uintptr_t)off;
		if (addr > as_addr) {
			addr -= align_amount;
		}

		/*
		 * If randomization is requested, slew the allocation
		 * backwards, within the same gap, by a random amount.
		 */
		if (flags & _MAP_RANDOMIZE) {
			uint32_t slew;

			(void) random_get_pseudo_bytes((uint8_t *)&slew,
			    sizeof (slew));

			slew = slew % MIN(aslr_max_map_skew, (addr - base));
			addr -= P2ALIGN(slew, align_amount);
		}

		ASSERT(addr > base);
		ASSERT(addr + len < base + slen);
		ASSERT(((uintptr_t)addr & (align_amount - 1)) ==
		    ((uintptr_t)(off)));
		*addrp = addr;
	} else {
		*addrp = NULL;	/* no more virtual space */
	}
}

int valid_va_range_aligned_wraparound;

/*
 * Determine whether [*basep, *basep + *lenp) contains a mappable range of
 * addresses at least "minlen" long, where the base of the range is at "off"
 * phase from an "align" boundary and there is space for a "redzone"-sized
 * redzone on either side of the range.  On success, 1 is returned and *basep
 * and *lenp are adjusted to describe the acceptable range (including
 * the redzone).  On failure, 0 is returned.
 */
/*ARGSUSED3*/
int
valid_va_range_aligned(caddr_t *basep, size_t *lenp, size_t minlen, int dir,
    size_t align, size_t redzone, size_t off)
{
	uintptr_t hi, lo;
	size_t tot_len;

	ASSERT(align == 0 ? off == 0 : off < align);
	ASSERT(ISP2(align));
	ASSERT(align == 0 || align >= PAGESIZE);

	lo = (uintptr_t)*basep;
	hi = lo + *lenp;
	tot_len = minlen + 2 * redzone; /* need at least this much space */

	/*
	 * If hi rolled over the top, try cutting back.
	 */
	if (hi < lo) {
		*lenp = 0UL - lo - 1UL;
		/* See if this really happens. If so, then we figure out why */
		valid_va_range_aligned_wraparound++;
		hi = lo + *lenp;
	}
	if (*lenp < tot_len) {
		return (0);
	}

	/*
	 * Deal with a possible hole in the address range between
	 * hole_start and hole_end that should never be mapped.
	 */
	if (lo < hole_start) {
		if (hi > hole_start) {
			if (hi < hole_end) {
				hi = hole_start;
			} else {
				/* lo < hole_start && hi >= hole_end */
				if (dir == AH_LO) {
					/*
					 * prefer lowest range
					 */
					if (hole_start - lo >= tot_len)
						hi = hole_start;
					else if (hi - hole_end >= tot_len)
						lo = hole_end;
					else
						return (0);
				} else {
					/*
					 * prefer highest range
					 */
					if (hi - hole_end >= tot_len)
						lo = hole_end;
					else if (hole_start - lo >= tot_len)
						hi = hole_start;
					else
						return (0);
				}
			}
		}
	} else {
		/* lo >= hole_start */
		if (hi < hole_end)
			return (0);
		if (lo < hole_end)
			lo = hole_end;
	}

	if (hi - lo < tot_len)
		return (0);

	if (align > 1) {
		uintptr_t tlo = lo + redzone;
		uintptr_t thi = hi - redzone;
		tlo = (uintptr_t)P2PHASEUP(tlo, align, off);
		if (tlo < lo + redzone) {
			return (0);
		}
		if (thi < tlo || thi - tlo < minlen) {
			return (0);
		}
	}

	*basep = (caddr_t)lo;
	*lenp = hi - lo;
	return (1);
}

/*
 * Determine whether [*basep, *basep + *lenp) contains a mappable range of
 * addresses at least "minlen" long.  On success, 1 is returned and *basep
 * and *lenp are adjusted to describe the acceptable range.  On failure, 0
 * is returned.
 */
int
valid_va_range(caddr_t *basep, size_t *lenp, size_t minlen, int dir)
{
	return (valid_va_range_aligned(basep, lenp, minlen, dir, 0, 0, 0));
}

/*
 * Default to forbidding the first 64k of address space.  This protects most
 * reasonably sized structures from dereferences through NULL:
 *     ((foo_t *)0)->bar
 */
uintptr_t forbidden_null_mapping_sz = 0x10000;

/*
 * Determine whether [addr, addr+len] are valid user addresses.
 */
/*ARGSUSED*/
int
valid_usr_range(caddr_t addr, size_t len, uint_t prot, struct as *as,
    caddr_t userlimit)
{
	caddr_t eaddr = addr + len;

	if (eaddr <= addr || addr >= userlimit || eaddr > userlimit)
		return (RANGE_BADADDR);

	if ((addr <= (caddr_t)forbidden_null_mapping_sz) &&
	    secflag_enabled(as->a_proc, PROC_SEC_FORBIDNULLMAP))
		return (RANGE_BADADDR);

	return (RANGE_OKAY);
}
/*
 * mnode_range_setup() initializes mnoderanges.
 */
void
mnode_range_setup(mnoderange_t *mnoderanges)
{
	mnoderange_t *mp = mnoderanges;
	int	mnode, mri;
	int	mindex = 0;	/* current index into mnoderanges array */
	int	i, j;
	pfn_t	hipfn;
	int	last, hi;

	for (mnode = 0; mnode < max_mem_nodes; mnode++) {
		if (mem_node_config[mnode].exists == 0)
			continue;

		mri = nranges - 1;

		while (MEMRANGEHI(mri) < mem_node_config[mnode].physbase)
			mri--;

		while (mri >= 0 && mem_node_config[mnode].physmax >=
		    MEMRANGELO(mri)) {
			mnoderanges->mnr_pfnlo = MAX(MEMRANGELO(mri),
			    mem_node_config[mnode].physbase);
			mnoderanges->mnr_pfnhi = MIN(MEMRANGEHI(mri),
			    mem_node_config[mnode].physmax);
			mnoderanges->mnr_mnode = mnode;
			mnoderanges->mnr_memrange = mri;
			mnoderanges->mnr_exists = 1;
			mnoderanges++;
			mindex++;
			if (mem_node_config[mnode].physmax > MEMRANGEHI(mri))
				mri--;
			else
				break;
		}
	}

	/*
	 * For now do a simple sort of the mnoderanges array to fill in
	 * the mnr_next fields.  Since mindex is expected to be relatively
	 * small, using a simple O(N^2) algorithm.
	 */
	pfn_t min_pfn = (pfn_t)-1;
	for (i = 0; i < mindex; i++) {
		if (mp[i].mnr_pfnlo < min_pfn)
			min_pfn = mp[i].mnr_pfnlo;
	}
	for (i = 0; i < mindex; i++) {
		if (mp[i].mnr_pfnlo == min_pfn)
			break;
	}

	ASSERT(i < mindex);
	last = i;
	mp[last].mnr_next = -1;
	for (i = 0; i < mindex - 1; i++) {
		hipfn = (pfn_t)(-1);
		hi = -1;
		/* find next highest mnode range */
		for (j = 0; j < mindex; j++) {
			if (mp[j].mnr_pfnlo > mp[last].mnr_pfnlo &&
			    mp[j].mnr_pfnlo < hipfn) {
				hipfn = mp[j].mnr_pfnlo;
				hi = j;
			}
		}
		mp[hi].mnr_next = last;
		last = hi;
	}
	mtypetop = last;
}

int
mtype_init(vnode_t *vp, caddr_t vaddr, uint_t *flags, size_t pgsz)
{
	int mtype = mtypetop;

	VM_STAT_ADD(vmm_vmstats.unrestrict16mcnt);
	VM_STAT_COND_ADD((*flags & PG_PANIC), vmm_vmstats.pgpanicalloc);
	*flags |= PGI_MT_RANGE0;

	return (mtype);
}


/* mtype init for page_get_replacement_page */
/*ARGSUSED*/
int
mtype_pgr_init(int *flags, page_t *pp, pgcnt_t pgcnt)
{
	int mtype = mtypetop;
	VM_STAT_ADD(vmm_vmstats.unrestrict16mcnt);
	*flags |= PGI_MT_RANGE0;
	return (mtype);
}

/*
 * Determine if the mnode range specified in mtype contains memory belonging
 * to memory node mnode.  If flags & PGI_MT_RANGE is set then mtype contains
 * the range from high pfn to 0, 16m or 4g.
 *
 * Return first mnode range type index found otherwise return -1 if none found.
 */
int
mtype_func(int mnode, int mtype, uint_t flags)
{
	if (flags & PGI_MT_RANGE) {
		int	mnr_lim = MRI_0;

		if (flags & PGI_MT_NEXT) {
			mtype = mnoderanges[mtype].mnr_next;
		}
		if (flags & PGI_MT_RANGE4G)
			mnr_lim = MRI_4G;	/* exclude 0-4g range */
		while (mtype != -1 &&
		    mnoderanges[mtype].mnr_memrange <= mnr_lim) {
			if (mnoderanges[mtype].mnr_mnode == mnode)
				return (mtype);
			mtype = mnoderanges[mtype].mnr_next;
		}
	} else if (mnoderanges[mtype].mnr_mnode == mnode) {
		return (mtype);
	}
	return (-1);
}

/*
 * Update the page list max counts with the pfn range specified by the
 * input parameters.
 */
void
mtype_modify_max(pfn_t startpfn, long cnt)
{
}

int
mtype_2_mrange(int mtype)
{
	return (mnoderanges[mtype].mnr_memrange);
}

void
mnodetype_2_pfn(int mnode, int mtype, pfn_t *pfnlo, pfn_t *pfnhi)
{
	_NOTE(ARGUNUSED(mnode));
	ASSERT(mnoderanges[mtype].mnr_mnode == mnode);
	*pfnlo = mnoderanges[mtype].mnr_pfnlo;
	*pfnhi = mnoderanges[mtype].mnr_pfnhi;
}

size_t
plcnt_sz(size_t ctrs_sz)
{
#ifdef DEBUG
	int	szc, colors;

	ctrs_sz += mnoderangecnt * sizeof (struct mnr_mts) * mmu_page_sizes;
	for (szc = 0; szc < mmu_page_sizes; szc++) {
		colors = page_get_pagecolors(szc);
		ctrs_sz += mnoderangecnt * sizeof (pgcnt_t) * colors;
	}
#endif
	return (ctrs_sz);
}

caddr_t
plcnt_init(caddr_t addr)
{
#ifdef DEBUG
	int	mt, szc, colors;

	for (mt = 0; mt < mnoderangecnt; mt++) {
		mnoderanges[mt].mnr_mts = (struct mnr_mts *)addr;
		addr += (sizeof (struct mnr_mts) * mmu_page_sizes);
		for (szc = 0; szc < mmu_page_sizes; szc++) {
			colors = page_get_pagecolors(szc);
			mnoderanges[mt].mnr_mts[szc].mnr_mts_colors = colors;
			mnoderanges[mt].mnr_mts[szc].mnr_mtsc_pgcnt =
			    (pgcnt_t *)addr;
			addr += (sizeof (pgcnt_t) * colors);
		}
	}
#endif
	return (addr);
}

void
plcnt_inc_dec(page_t *pp, int mtype, int szc, long cnt, int flags)
{
	_NOTE(ARGUNUSED(pp));
#ifdef DEBUG
	int	bin = PP_2_BIN(pp);

	atomic_add_long(&mnoderanges[mtype].mnr_mts[szc].mnr_mts_pgcnt, cnt);
	atomic_add_long(&mnoderanges[mtype].mnr_mts[szc].mnr_mtsc_pgcnt[bin],
	    cnt);
#endif
	ASSERT(mtype == PP_2_MTYPE(pp));
	if (flags & PG_CACHE_LIST)
		atomic_add_long(&mnoderanges[mtype].mnr_mt_clpgcnt, cnt);
	else
		atomic_add_long(&mnoderanges[mtype].mnr_mt_flpgcnt[szc], cnt);
	atomic_add_long(&mnoderanges[mtype].mnr_mt_totcnt, cnt);
}

/*
 * Returns the free page count for mnode
 */
int
mnode_pgcnt(int mnode)
{
	int	mtype = mtypetop;
	int	flags = PGI_MT_RANGE0;
	pgcnt_t	pgcnt = 0;

	mtype = mtype_func(mnode, mtype, flags);

	while (mtype != -1) {
		pgcnt += MTYPE_FREEMEM(mtype);
		mtype = mtype_func(mnode, mtype, flags | PGI_MT_NEXT);
	}
	return (pgcnt);
}

/*
 * Initialize page coloring variables based on the l2 cache parameters.
 * Calculate and return memory needed for page coloring data structures.
 */
size_t
page_coloring_init(uint_t l2_sz, int l2_linesz, int l2_assoc)
{
	_NOTE(ARGUNUSED(l2_linesz));
	size_t	colorsz = 0;
	int	i;
	int	colors;

	i = memrange_num(physmax);
	memranges += i;
	nranges -= i;

	ASSERT(mmu_page_sizes <= MMU_PAGE_SIZES);

	ASSERT(ISP2(l2_linesz));
	ASSERT(l2_sz > MMU_PAGESIZE);

	/* l2_assoc is 0 for fully associative l2 cache */
	if (l2_assoc)
		l2_colors = MAX(1, l2_sz / (l2_assoc * MMU_PAGESIZE));
	else
		l2_colors = 1;

	ASSERT(ISP2(l2_colors));

	/* for scalability, configure at least PAGE_COLORS_MIN color bins */
	page_colors = MAX(l2_colors, PAGE_COLORS_MIN);

	/*
	 * cpu_page_colors is non-zero when a page color may be spread across
	 * multiple bins.
	 */
	if (l2_colors < page_colors)
		cpu_page_colors = l2_colors;

	ASSERT(ISP2(page_colors));

	page_colors_mask = page_colors - 1;

	ASSERT(ISP2(CPUSETSIZE()));
	page_coloring_shift = lowbit(CPUSETSIZE());

	/* initialize number of colors per page size */
	for (i = 0; i < MMU_PAGE_SIZES; i++) {
		hw_page_array[i].hp_colors = (page_colors_mask >> (hw_page_array[i].hp_shift - hw_page_array[0].hp_shift)) + 1;
		colorequivszc[i] = 0;
	}

	/*
	 * The value of cpu_page_colors determines if additional color bins
	 * need to be checked for a particular color in the page_get routines.
	 */
	if (cpu_page_colors != 0) {

		int a = lowbit(page_colors) - lowbit(cpu_page_colors);
		ASSERT(a > 0);
		ASSERT(a < 16);

		for (i = 0; i < MMU_PAGE_SIZES; i++) {
			if ((colors = hw_page_array[i].hp_colors) <= 1) {
				colorequivszc[i] = 0;
				continue;
			}
			while ((colors >> a) == 0)
				a--;
			ASSERT(a >= 0);

			/* higher 4 bits encodes color equiv mask */
			colorequivszc[i] = (a << 4);
		}
	}

	/* factor in colorequiv to check additional 'equivalent' bins. */
	if (colorequiv > 1) {

		int a = lowbit(colorequiv) - 1;
		if (a > 15)
			a = 15;

		for (i = 0; i < MMU_PAGE_SIZES; i++) {
			if ((colors = hw_page_array[i].hp_colors) <= 1) {
				continue;
			}
			while ((colors >> a) == 0)
				a--;
			if ((a << 4) > colorequivszc[i]) {
				colorequivszc[i] = (a << 4);
			}
		}
	}

	/* size for mnoderanges */
	for (mnoderangecnt = 0, i = 0; i < max_mem_nodes; i++)
		mnoderangecnt += mnode_range_cnt(i);
	colorsz = mnoderangecnt * sizeof (mnoderange_t);

	/* size for fpc_mutex and cpc_mutex */
	colorsz += (2 * max_mem_nodes * sizeof (kmutex_t) * NPC_MUTEX);

	/* size of page_freelists */
	colorsz += mnoderangecnt * sizeof (page_t ***);
	colorsz += mnoderangecnt * mmu_page_sizes * sizeof (page_t **);

	for (i = 0; i < mmu_page_sizes; i++) {
		colors = page_get_pagecolors(i);
		colorsz += mnoderangecnt * colors * sizeof (page_t *);
	}

	/* size of page_cachelists */
	colorsz += mnoderangecnt * sizeof (page_t **);
	colorsz += mnoderangecnt * page_colors * sizeof (page_t *);

	return (colorsz);
}

/*
 * Called once at startup to configure page_coloring data structures and
 * does the 1st page_free()/page_freelist_add().
 */
void
page_coloring_setup(caddr_t pcmemaddr)
{
	int	i;
	int	j;
	int	k;
	caddr_t	addr;
	int	colors;

	/*
	 * do page coloring setup
	 */
	addr = pcmemaddr;

	mnoderanges = (mnoderange_t *)addr;
	addr += (mnoderangecnt * sizeof (mnoderange_t));

	mnode_range_setup(mnoderanges);

	for (k = 0; k < NPC_MUTEX; k++) {
		fpc_mutex[k] = (kmutex_t *)addr;
		addr += (max_mem_nodes * sizeof (kmutex_t));
	}
	for (k = 0; k < NPC_MUTEX; k++) {
		cpc_mutex[k] = (kmutex_t *)addr;
		addr += (max_mem_nodes * sizeof (kmutex_t));
	}
	page_freelists = (page_t ****)addr;
	addr += (mnoderangecnt * sizeof (page_t ***));

	page_cachelists = (page_t ***)addr;
	addr += (mnoderangecnt * sizeof (page_t **));

	for (i = 0; i < mnoderangecnt; i++) {
		page_freelists[i] = (page_t ***)addr;
		addr += (mmu_page_sizes * sizeof (page_t **));

		for (j = 0; j < mmu_page_sizes; j++) {
			colors = page_get_pagecolors(j);
			page_freelists[i][j] = (page_t **)addr;
			addr += (colors * sizeof (page_t *));
		}
		page_cachelists[i] = (page_t **)addr;
		addr += (page_colors * sizeof (page_t *));
	}
}
/*ARGSUSED*/
int
bp_color(struct buf *bp)
{
	return (0);
}

/*
 * Copy the data from the physical page represented by "frompp" to
 * that represented by "topp". ppcopy uses CPU->cpu_caddr1 and
 * CPU->cpu_caddr2.  It assumes that no one uses either map at interrupt
 * level and no one sleeps with an active mapping there.
 *
 * Note that the ref/mod bits in the page_t's are not affected by
 * this operation, hence it is up to the caller to update them appropriately.
 */
int
ppcopy(page_t *frompp, page_t *topp)
{
	caddr_t		pp_addr1;
	caddr_t		pp_addr2;
	int		ret = 1;

	ASSERT_STACK_ALIGNED();
	ASSERT(PAGE_LOCKED(frompp));
	ASSERT(PAGE_LOCKED(topp));

	pp_addr1 = hat_kpm_page2va(frompp, 0);
	pp_addr2 = hat_kpm_page2va(topp, 0);
	bcopy(pp_addr1, pp_addr2, PAGESIZE);

	return (ret);
}

void
pagezero(page_t *pp, uint_t off, uint_t len)
{
	ASSERT(PAGE_LOCKED(pp));
	pfnzero(page_pptonum(pp), off, len);
}

/*
 * Zero the physical page from off to off + len given by pfn
 * without changing the reference and modified bits of page.
 *
 * We use this using CPU private page address #2, see ppcopy() for more info.
 * pfnzero() must not be called at interrupt level.
 */
void
pfnzero(pfn_t pfn, uint_t off, uint_t len)
{
	caddr_t		pp_addr2;

	ASSERT_STACK_ALIGNED();
	ASSERT(len <= MMU_PAGESIZE);
	ASSERT(off <= MMU_PAGESIZE);
	ASSERT(off + len <= MMU_PAGESIZE);

	pp_addr2 = hat_kpm_pfn2va(pfn);

	bzero(pp_addr2 + off, len);
}

/*
 * Platform-dependent page scrub call.
 */
void
pagescrub(page_t *pp, uint_t off, uint_t len)
{
	/*
	 * For now, we rely on the fact that pagezero() will
	 * always clear UEs.
	 */
	pagezero(pp, off, len);
}

/*
 * Function for flushing D-cache when performing module relocations
 * to an alternate mapping.  Unnecessary on Intel / AMD platforms.
 */
void
dcache_flushall()
{}

/*
 * Allocate a memory page.  The argument 'seed' can be any pseudo-random
 * number to vary where the pages come from.  This is quite a hacked up
 * method -- it works for now, but really needs to be fixed up a bit.
 *
 * We currently use page_create_va() on the kvp with fake offsets,
 * segments and virt address.  This is pretty bogus, but was copied from the
 * old hat_i86.c code.  A better approach would be to specify either mnode
 * random or mnode local and takes a page from whatever color has the MOST
 * available - this would have a minimal impact on page coloring.
 */
page_t *
page_get_physical(uintptr_t seed)
{
	page_t *pp;
	u_offset_t offset;
	static struct seg tmpseg;
	static uintptr_t ctr = 0;

	/*
	 * This code is gross, we really need a simpler page allocator.
	 *
	 * We need to assign an offset for the page to call page_create_va()
	 * To avoid conflicts with other pages, we get creative with the offset.
	 * For 32 bits, we need an offset > 4Gig
	 * For 64 bits, need an offset somewhere in the VA hole.
	 */
	offset = seed;
	offset &= (HOLE_START - 1);
	offset <<= MMU_PAGESHIFT;
	offset += HOLE_START;	/* something in VA hole */

	if (page_resv(1, KM_NOSLEEP) == 0)
		return (NULL);

#ifdef	DEBUG
	pp = page_exists(&kvp, offset);
	if (pp != NULL)
		panic("page already exists %p", (void *)pp);
#endif

	pp = page_create_va(&kvp, offset, MMU_PAGESIZE, PG_EXCL,
	    &tmpseg, (caddr_t)(ctr += MMU_PAGESIZE));	/* changing VA usage */
	if (pp != NULL) {
		page_io_unlock(pp);
		page_downgrade(pp);
	}
	return (pp);
}

#ifndef DEBUG
#define	check_dma(a, b, c) (void)(0)
#else
static void
check_dma(ddi_dma_attr_t *dma_attr, page_t *pp, int cnt)
{
	if (dma_attr == NULL)
		return;

	while (cnt-- > 0) {
		if (pfn_to_pa(pp->p_pagenum) <
		    dma_attr->dma_attr_addr_lo)
			panic("PFN (pp=%p) below dma_attr_addr_lo", (void *)pp);
		if (pfn_to_pa(pp->p_pagenum) >=
		    dma_attr->dma_attr_addr_hi)
			panic("PFN (pp=%p) above dma_attr_addr_hi", (void *)pp);
		pp = pp->p_next;
	}
}
#endif

/*
 * Return 1 if the page frame is onboard memory, else 0.
 */
int
pf_is_memory(pfn_t pf)
{
	return (address_in_memlist(phys_install, pfn_to_pa(pf), 1));
}

/*
 * return the memrange containing pfn
 */
int
memrange_num(pfn_t pfn)
{
	int n;

	for (n = 0; n < nranges - 1; ++n) {
		if (pfn >= memranges[n])
			break;
	}
	return (n);
}

int
pfn_2_mtype(pfn_t pfn)
{
	int	n;

	/* Always start from highest pfn and work our way down */
	for (n = mtypetop; n != -1; n = mnoderanges[n].mnr_next) {
		if (pfn >= mnoderanges[n].mnr_pfnlo) {
			break;
		}
	}
	return (n);
}

/*
 * mnode_range_cnt() calculates the number of memory ranges for mnode and
 * memranges[]. Used to determine the size of page lists and mnoderanges.
 */
int
mnode_range_cnt(int mnode)
{
	int	mri;
	int	mnrcnt = 0;

	if (mem_node_config[mnode].exists != 0) {
		mri = nranges - 1;

		/* find the memranges index below contained in mnode range */

		while (MEMRANGEHI(mri) < mem_node_config[mnode].physbase)
			mri--;

		/*
		 * increment mnode range counter when memranges or mnode
		 * boundary is reached.
		 */
		while (mri >= 0 &&
		    mem_node_config[mnode].physmax >= MEMRANGELO(mri)) {
			mnrcnt++;
			if (mem_node_config[mnode].physmax > MEMRANGEHI(mri))
				mri--;
			else
				break;
		}
	}
	ASSERT(mnrcnt <= MAX_MNODE_MRANGES);
	return (mnrcnt);
}

/*
 * is_contigpage_free:
 *	returns a page list of contiguous pages. It minimally has to return
 *	minctg pages. Caller determines minctg based on the scatter-gather
 *	list length.
 *
 *	pfnp is set to the next page frame to search on return.
 */
static page_t *
is_contigpage_free(
	pfn_t *pfnp,
	pgcnt_t *pgcnt,
	pgcnt_t minctg,
	uint64_t pfnseg,
	int iolock)
{
	int	i = 0;
	pfn_t	pfn = *pfnp;
	page_t	*pp;
	page_t	*plist = NULL;

	/*
	 * fail if pfn + minctg crosses a segment boundary.
	 * Adjust for next starting pfn to begin at segment boundary.
	 */

	if (((*pfnp + minctg - 1) & pfnseg) < (*pfnp & pfnseg)) {
		*pfnp = roundup(*pfnp, pfnseg + 1);
		return (NULL);
	}

	do {
retry:
		pp = page_numtopp_nolock(pfn + i);
		if ((pp == NULL) || IS_DUMP_PAGE(pp) ||
		    (page_trylock(pp, SE_EXCL) == 0)) {
			(*pfnp)++;
			break;
		}
		if (page_pptonum(pp) != pfn + i) {
			page_unlock(pp);
			goto retry;
		}

		if (!(PP_ISFREE(pp))) {
			page_unlock(pp);
			(*pfnp)++;
			break;
		}

		if (!PP_ISAGED(pp)) {
			page_list_sub(pp, PG_CACHE_LIST);
			page_hashout(pp, (kmutex_t *)NULL);
		} else {
			page_list_sub(pp, PG_FREE_LIST);
		}

		if (iolock)
			page_io_lock(pp);
		page_list_concat(&plist, &pp);

		/*
		 * exit loop when pgcnt satisfied or segment boundary reached.
		 */

	} while ((++i < *pgcnt) && ((pfn + i) & pfnseg));

	*pfnp += i;		/* set to next pfn to search */

	if (i >= minctg) {
		*pgcnt -= i;
		return (plist);
	}

	/*
	 * failure: minctg not satisfied.
	 *
	 * if next request crosses segment boundary, set next pfn
	 * to search from the segment boundary.
	 */
	if (((*pfnp + minctg - 1) & pfnseg) < (*pfnp & pfnseg))
		*pfnp = roundup(*pfnp, pfnseg + 1);

	/* clean up any pages already allocated */

	while (plist) {
		pp = plist;
		page_sub(&plist, pp);
		page_list_add(pp, PG_FREE_LIST | PG_LIST_TAIL);
		if (iolock)
			page_io_unlock(pp);
		page_unlock(pp);
	}

	return (NULL);
}

static page_t *
page_get_contigpage(pgcnt_t *pgcnt, ddi_dma_attr_t *mattr, int iolock)
{
	pfn_t		pfn;
	int		sgllen;
	uint64_t	pfnseg;
	pgcnt_t		minctg;
	page_t		*pplist = NULL, *plist;
	uint64_t	lo, hi;
	pgcnt_t		pfnalign = 0;
	static pfn_t	startpfn;
	static pgcnt_t	lastctgcnt;
	uintptr_t	align;

	CONTIG_LOCK();

	if (mattr) {
		lo = mmu_btop((mattr->dma_attr_addr_lo + MMU_PAGEOFFSET));
		hi = mmu_btop(mattr->dma_attr_addr_hi);
		if (hi >= physmax)
			hi = physmax - 1;
		sgllen = mattr->dma_attr_sgllen;
		pfnseg = mmu_btop(mattr->dma_attr_seg);

		align = maxbit(mattr->dma_attr_align, mattr->dma_attr_minxfer);
		if (align > MMU_PAGESIZE)
			pfnalign = mmu_btop(align);

		/*
		 * in order to satisfy the request, must minimally
		 * acquire minctg contiguous pages
		 */
		minctg = howmany(*pgcnt, sgllen);

		ASSERT(hi >= lo);

		/*
		 * start from where last searched if the minctg >= lastctgcnt
		 */
		if (minctg < lastctgcnt || startpfn < lo || startpfn > hi)
			startpfn = lo;
	} else {
		hi = physmax - 1;
		lo = 0;
		sgllen = 1;
		pfnseg = mmu_btop(1ul << hwrpb->pa_size);
		minctg = *pgcnt;

		if (minctg < lastctgcnt)
			startpfn = lo;
	}
	lastctgcnt = minctg;

	ASSERT(pfnseg + 1 >= (uint64_t)minctg);

	pfn = startpfn;
	if (pfnalign)
		pfn = P2ROUNDUP(pfn, pfnalign);

	while (pfn + minctg - 1 <= hi) {

		plist = is_contigpage_free(&pfn, pgcnt, minctg, pfnseg, iolock);
		if (plist) {
			page_list_concat(&pplist, &plist);
			sgllen--;
			/*
			 * return when contig pages no longer needed
			 */
			if (!*pgcnt || ((*pgcnt <= sgllen) && !pfnalign)) {
				startpfn = pfn;
				CONTIG_UNLOCK();
				check_dma(mattr, pplist, *pgcnt);
				return (pplist);
			}
			minctg = howmany(*pgcnt, sgllen);
		}
		if (pfnalign)
			pfn = P2ROUNDUP(pfn, pfnalign);
	}

	/* cannot find contig pages in specified range */
	if (startpfn == lo) {
		CONTIG_UNLOCK();
		return (NULL);
	}

	/* did not start with lo previously */
	pfn = lo;
	if (pfnalign)
		pfn = P2ROUNDUP(pfn, pfnalign);

	/* allow search to go above startpfn */
	while (pfn < startpfn) {

		plist = is_contigpage_free(&pfn, pgcnt, minctg, pfnseg, iolock);
		if (plist != NULL) {

			page_list_concat(&pplist, &plist);
			sgllen--;

			/*
			 * return when contig pages no longer needed
			 */
			if (!*pgcnt || ((*pgcnt <= sgllen) && !pfnalign)) {
				startpfn = pfn;
				CONTIG_UNLOCK();
				check_dma(mattr, pplist, *pgcnt);
				return (pplist);
			}
			minctg = howmany(*pgcnt, sgllen);
		}
		if (pfnalign)
			pfn = P2ROUNDUP(pfn, pfnalign);
	}
	CONTIG_UNLOCK();
	return (NULL);
}

/*
 * get a page from any list with the given mnode
 */
static page_t *
page_get_mnode_anylist(ulong_t origbin, uchar_t szc, uint_t flags,
    int mnode, int mtype, ddi_dma_attr_t *dma_attr)
{
	kmutex_t		*pcm;
	int			i;
	page_t			*pp;
	page_t			*first_pp;
	uint64_t		pgaddr;
	ulong_t			bin;
	int			mtypestart;
	int			plw_initialized;
	page_list_walker_t	plw;

	VM_STAT_ADD(pga_vmstats.pgma_alloc);

	ASSERT((flags & PG_MATCH_COLOR) == 0);
	ASSERT(szc == 0);
	ASSERT(dma_attr != NULL);

	MTYPE_START(mnode, mtype, flags);
	if (mtype < 0) {
		VM_STAT_ADD(pga_vmstats.pgma_allocempty);
		return (NULL);
	}

	mtypestart = mtype;

	bin = origbin;

	/*
	 * check up to page_colors + 1 bins - origbin may be checked twice
	 * because of BIN_STEP skip
	 */
	do {
		plw_initialized = 0;

		for (plw.plw_count = 0;
		    plw.plw_count < page_colors; plw.plw_count++) {

			if (PAGE_FREELISTS(mnode, szc, bin, mtype) == NULL)
				goto nextfreebin;

			pcm = PC_BIN_MUTEX(mnode, bin, PG_FREE_LIST);
			mutex_enter(pcm);
			pp = PAGE_FREELISTS(mnode, szc, bin, mtype);
			first_pp = pp;
			while (pp != NULL) {
				if (IS_DUMP_PAGE(pp) || page_trylock(pp,
				    SE_EXCL) == 0) {
					pp = pp->p_next;
					if (pp == first_pp) {
						pp = NULL;
					}
					continue;
				}

				ASSERT(PP_ISFREE(pp));
				ASSERT(PP_ISAGED(pp));
				ASSERT(pp->p_vnode == NULL);
				ASSERT(pp->p_hash == NULL);
				ASSERT(pp->p_offset == (u_offset_t)-1);
				ASSERT(pp->p_szc == szc);
				ASSERT(PFN_2_MEM_NODE(pp->p_pagenum) == mnode);
				/* check if page within DMA attributes */
				pgaddr = pfn_to_pa(pp->p_pagenum);
				if ((pgaddr >= dma_attr->dma_attr_addr_lo) &&
				    (pgaddr + MMU_PAGESIZE - 1 <=
				    dma_attr->dma_attr_addr_hi)) {
					break;
				}

				/* continue looking */
				page_unlock(pp);
				pp = pp->p_next;
				if (pp == first_pp)
					pp = NULL;

			}
			if (pp != NULL) {
				ASSERT(mtype == PP_2_MTYPE(pp));
				ASSERT(pp->p_szc == 0);

				/* found a page with specified DMA attributes */
				page_sub(&PAGE_FREELISTS(mnode, szc, bin,
				    mtype), pp);
				page_ctr_sub(mnode, mtype, pp, PG_FREE_LIST);

				if ((PP_ISFREE(pp) == 0) ||
				    (PP_ISAGED(pp) == 0)) {
					cmn_err(CE_PANIC, "page %p is not free",
					    (void *)pp);
				}

				mutex_exit(pcm);
				check_dma(dma_attr, pp, 1);
				VM_STAT_ADD(pga_vmstats.pgma_allocok);
				return (pp);
			}
			mutex_exit(pcm);
nextfreebin:
			if (plw_initialized == 0) {
				page_list_walk_init(szc, 0, bin, 1, 0, &plw);
				ASSERT(plw.plw_ceq_dif == page_colors);
				plw_initialized = 1;
			}

			if (plw.plw_do_split) {
				pp = page_freelist_split(szc, bin, mnode,
				    mtype,
				    mmu_btop(dma_attr->dma_attr_addr_lo),
				    mmu_btop(dma_attr->dma_attr_addr_hi + 1),
				    &plw);
				if (pp != NULL) {
					check_dma(dma_attr, pp, 1);
					return (pp);
				}
			}

			bin = page_list_walk_next_bin(szc, bin, &plw);
		}

		MTYPE_NEXT(mnode, mtype, flags);
	} while (mtype >= 0);

	/* failed to find a page in the freelist; try it in the cachelist */

	/* reset mtype start for cachelist search */
	mtype = mtypestart;
	ASSERT(mtype >= 0);

	/* start with the bin of matching color */
	bin = origbin;

	do {
		for (i = 0; i <= page_colors; i++) {
			if (PAGE_CACHELISTS(mnode, bin, mtype) == NULL)
				goto nextcachebin;
			pcm = PC_BIN_MUTEX(mnode, bin, PG_CACHE_LIST);
			mutex_enter(pcm);
			pp = PAGE_CACHELISTS(mnode, bin, mtype);
			first_pp = pp;
			while (pp != NULL) {
				if (IS_DUMP_PAGE(pp) || page_trylock(pp,
				    SE_EXCL) == 0) {
					pp = pp->p_next;
					if (pp == first_pp)
						pp = NULL;
					continue;
				}
				ASSERT(pp->p_vnode);
				ASSERT(PP_ISAGED(pp) == 0);
				ASSERT(pp->p_szc == 0);
				ASSERT(PFN_2_MEM_NODE(pp->p_pagenum) == mnode);

				/* check if page within DMA attributes */

				pgaddr = pfn_to_pa(pp->p_pagenum);
				if ((pgaddr >= dma_attr->dma_attr_addr_lo) &&
				    (pgaddr + MMU_PAGESIZE - 1 <=
				    dma_attr->dma_attr_addr_hi)) {
					break;
				}

				/* continue looking */
				page_unlock(pp);
				pp = pp->p_next;
				if (pp == first_pp)
					pp = NULL;
			}

			if (pp != NULL) {
				ASSERT(mtype == PP_2_MTYPE(pp));
				ASSERT(pp->p_szc == 0);

				/* found a page with specified DMA attributes */
				page_sub(&PAGE_CACHELISTS(mnode, bin,
				    mtype), pp);
				page_ctr_sub(mnode, mtype, pp, PG_CACHE_LIST);

				mutex_exit(pcm);
				ASSERT(pp->p_vnode);
				ASSERT(PP_ISAGED(pp) == 0);
				check_dma(dma_attr, pp, 1);
				VM_STAT_ADD(pga_vmstats.pgma_allocok);
				return (pp);
			}
			mutex_exit(pcm);
nextcachebin:
			bin += (i == 0) ? BIN_STEP : 1;
			bin &= page_colors_mask;
		}
		MTYPE_NEXT(mnode, mtype, flags);
	} while (mtype >= 0);

	VM_STAT_ADD(pga_vmstats.pgma_allocfailed);
	return (NULL);
}

/*
 * This function is similar to page_get_freelist()/page_get_cachelist()
 * but it searches both the lists to find a page with the specified
 * color (or no color) and DMA attributes. The search is done in the
 * freelist first and then in the cache list within the highest memory
 * range (based on DMA attributes) before searching in the lower
 * memory ranges.
 *
 * Note: This function is called only by page_create_io().
 */
/*ARGSUSED*/
static page_t *
page_get_anylist(struct vnode *vp, u_offset_t off, struct as *as, caddr_t vaddr,
    size_t size, uint_t flags, ddi_dma_attr_t *dma_attr, lgrp_t	*lgrp)
{
	uint_t		bin;
	int		mtype;
	page_t		*pp;
	int		n;
	int		m;
	int		szc;
	int		fullrange;

	VM_STAT_ADD(pga_vmstats.pga_alloc);

	/* only base pagesize currently supported */
	if (size != MMU_PAGESIZE)
		return (NULL);

	/* LINTED */
	AS_2_BIN(as, seg, vp, vaddr, bin, 0);

	/*
	 * Only hold one freelist or cachelist lock at a time, that way we
	 * can start anywhere and not have to worry about lock
	 * ordering.
	 */
	if (dma_attr == NULL) {
		n = 0;
		m = mtypetop;
		fullrange = 1;
		VM_STAT_ADD(pga_vmstats.pga_nulldmaattr);
	} else {
		pfn_t pfnlo = mmu_btop(dma_attr->dma_attr_addr_lo);
		pfn_t pfnhi = mmu_btop(dma_attr->dma_attr_addr_hi);

		/*
		 * We can guarantee alignment only for page boundary.
		 */
		if (dma_attr->dma_attr_align > MMU_PAGESIZE)
			return (NULL);

		/* Sanity check the dma_attr */
		if (pfnlo > pfnhi)
			return (NULL);

		n = pfn_2_mtype(pfnlo);
		m = pfn_2_mtype(pfnhi);

		fullrange = ((pfnlo == mnoderanges[n].mnr_pfnlo) &&
		    (pfnhi >= mnoderanges[m].mnr_pfnhi));
	}
	VM_STAT_COND_ADD(fullrange == 0, pga_vmstats.pga_notfullrange);

	szc = 0;

	/*
	 * Try local memory node first, but try remote if we can't
	 * get a page of the right color.
	 */
	/*
	 * allocate pages from high pfn to low.
	 */
	mtype = m;
	do {
		if (fullrange != 0) {
			pp = page_get_mnode_freelist(0, bin, mtype, szc, flags);
			if (pp == NULL) {
				pp = page_get_mnode_cachelist(bin, flags, 0, mtype);
			}
		} else {
			pp = page_get_mnode_anylist(bin, szc, flags, 0, mtype, dma_attr);
		}
		if (pp != NULL) {
			VM_STAT_ADD(pga_vmstats.pga_allocok);
			check_dma(dma_attr, pp, 1);
			return (pp);
		}
	} while (mtype != n && (mtype = mnoderanges[mtype].mnr_next) != -1);
	VM_STAT_ADD(pga_vmstats.pga_allocfailed);

	return (NULL);
}

/*
 * page_create_io()
 *
 * This function is a copy of page_create_va() with an additional
 * argument 'mattr' that specifies DMA memory requirements to
 * the page list functions. This function is used by the segkmem
 * allocator so it is only to create new pages (i.e PG_EXCL is
 * set).
 *
 * Note: This interface is currently used by x86 PSM only and is
 *	 not fully specified so the commitment level is only for
 *	 private interface specific to x86. This interface uses PSM
 *	 specific page_get_anylist() interface.
 */

#define	PAGE_HASH_SEARCH(index, pp, vp, off) { \
	for ((pp) = page_hash[(index)]; (pp); (pp) = (pp)->p_hash) { \
		if ((pp)->p_vnode == (vp) && (pp)->p_offset == (off)) \
			break; \
	} \
}


page_t *
page_create_io(
	struct vnode	*vp,
	u_offset_t	off,
	uint_t		bytes,
	uint_t		flags,
	struct as	*as,
	caddr_t		vaddr,
	ddi_dma_attr_t	*mattr)	/* DMA memory attributes if any */
{
	page_t		*plist = NULL;
	uint_t		plist_len = 0;
	pgcnt_t		npages;
	page_t		*npp = NULL;
	uint_t		pages_req;
	page_t		*pp;
	kmutex_t	*phm = NULL;
	uint_t		index;

	ASSERT((flags & ~(PG_EXCL | PG_WAIT | PG_PHYSCONTIG)) == 0);

	pages_req = npages = mmu_btopr(bytes);

	/*
	 * Do the freemem and pcf accounting.
	 */
	if (!page_create_wait(npages, flags)) {
		return (NULL);
	}

	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back.  The
	 * first clause of the test prevents waking up the pageout
	 * daemon in situations where it would decide that there's
	 * nothing to do.
	 */
	if (nscan < desscan && freemem < minfree) {
		cv_signal(&proc_pageout->p_cv);
	}

	if (flags & PG_PHYSCONTIG) {

		plist = page_get_contigpage(&npages, mattr, 1);
		if (plist == NULL) {
			page_create_putback(npages);
			return (NULL);
		}

		pp = plist;

		do {
			if (!page_hashin(pp, vp, off, NULL)) {
				panic("pg_creat_io: hashin failed %p %p %llx",
				    (void *)pp, (void *)vp, off);
			}
			VM_STAT_ADD(page_create_new);
			off += MMU_PAGESIZE;
			PP_CLRFREE(pp);
			PP_CLRAGED(pp);
			page_set_props(pp, P_REF);
			pp = pp->p_next;
		} while (pp != plist);

		if (!npages) {
			check_dma(mattr, plist, pages_req);
			return (plist);
		} else {
			vaddr += (pages_req - npages) << MMU_PAGESHIFT;
		}

		/*
		 * fall-thru:
		 *
		 * page_get_contigpage returns when npages <= sgllen.
		 * Grab the rest of the non-contig pages below from anylist.
		 */
	}

	/*
	 * Loop around collecting the requested number of pages.
	 * Most of the time, we have to `create' a new page. With
	 * this in mind, pull the page off the free list before
	 * getting the hash lock.  This will minimize the hash
	 * lock hold time, nesting, and the like.  If it turns
	 * out we don't need the page, we put it back at the end.
	 */
	while (npages--) {
		phm = NULL;

		index = PAGE_HASH_FUNC(vp, off);
top:
		ASSERT(phm == NULL);
		ASSERT(index == PAGE_HASH_FUNC(vp, off));
		ASSERT(MUTEX_NOT_HELD(page_vnode_mutex(vp)));

		if (npp == NULL) {
			/*
			 * Try to get the page of any color either from
			 * the freelist or from the cache list.
			 */
			npp = page_get_anylist(vp, off, as, vaddr, MMU_PAGESIZE,
			    flags & ~PG_MATCH_COLOR, mattr, NULL);
			if (npp == NULL) {
				if (mattr == NULL) {
					/*
					 * Not looking for a special page;
					 * panic!
					 */
					panic("no page found %d", (int)npages);
				}
				/*
				 * No page found! This can happen
				 * if we are looking for a page
				 * within a specific memory range
				 * for DMA purposes. If PG_WAIT is
				 * specified then we wait for a
				 * while and then try again. The
				 * wait could be forever if we
				 * don't get the page(s) we need.
				 *
				 * Note: XXX We really need a mechanism
				 * to wait for pages in the desired
				 * range. For now, we wait for any
				 * pages and see if we can use it.
				 */

				if ((mattr != NULL) && (flags & PG_WAIT)) {
					delay(10);
					goto top;
				}
				goto fail; /* undo accounting stuff */
			}

			if (PP_ISAGED(npp) == 0) {
				/*
				 * Since this page came from the
				 * cachelist, we must destroy the
				 * old vnode association.
				 */
				page_hashout(npp, (kmutex_t *)NULL);
			}
		}

		/*
		 * We own this page!
		 */
		ASSERT(PAGE_EXCL(npp));
		ASSERT(npp->p_vnode == NULL);
		ASSERT(!hat_page_is_mapped(npp));
		PP_CLRFREE(npp);
		PP_CLRAGED(npp);

		/*
		 * Here we have a page in our hot little mits and are
		 * just waiting to stuff it on the appropriate lists.
		 * Get the mutex and check to see if it really does
		 * not exist.
		 */
		phm = PAGE_HASH_MUTEX(index);
		mutex_enter(phm);
		PAGE_HASH_SEARCH(index, pp, vp, off);
		if (pp == NULL) {
			VM_STAT_ADD(page_create_new);
			pp = npp;
			npp = NULL;
			if (!page_hashin(pp, vp, off, phm)) {
				/*
				 * Since we hold the page hash mutex and
				 * just searched for this page, page_hashin
				 * had better not fail.  If it does, that
				 * means somethread did not follow the
				 * page hash mutex rules.  Panic now and
				 * get it over with.  As usual, go down
				 * holding all the locks.
				 */
				ASSERT(MUTEX_HELD(phm));
				panic("page_create: hashin fail %p %p %llx %p",
				    (void *)pp, (void *)vp, off, (void *)phm);

			}
			ASSERT(MUTEX_HELD(phm));
			mutex_exit(phm);
			phm = NULL;

			/*
			 * Hat layer locking need not be done to set
			 * the following bits since the page is not hashed
			 * and was on the free list (i.e., had no mappings).
			 *
			 * Set the reference bit to protect
			 * against immediate pageout
			 *
			 * XXXmh modify freelist code to set reference
			 * bit so we don't have to do it here.
			 */
			page_set_props(pp, P_REF);
		} else {
			ASSERT(MUTEX_HELD(phm));
			mutex_exit(phm);
			phm = NULL;
			/*
			 * NOTE: This should not happen for pages associated
			 *	 with kernel vnode 'kvp'.
			 */
			/* XX64 - to debug why this happens! */
			ASSERT(!VN_ISKAS(vp));
			if (VN_ISKAS(vp))
				cmn_err(CE_NOTE,
				    "page_create: page not expected "
				    "in hash list for kernel vnode - pp 0x%p",
				    (void *)pp);
			VM_STAT_ADD(page_create_exists);
			goto fail;
		}

		/*
		 * Got a page!  It is locked.  Acquire the i/o
		 * lock since we are going to use the p_next and
		 * p_prev fields to link the requested pages together.
		 */
		page_io_lock(pp);
		page_add(&plist, pp);
		plist = plist->p_next;
		off += MMU_PAGESIZE;
		vaddr += MMU_PAGESIZE;
	}

	check_dma(mattr, plist, pages_req);
	return (plist);

fail:
	if (npp != NULL) {
		/*
		 * Did not need this page after all.
		 * Put it back on the free list.
		 */
		VM_STAT_ADD(page_create_putbacks);
		PP_SETFREE(npp);
		PP_SETAGED(npp);
		npp->p_offset = (u_offset_t)-1;
		page_list_add(npp, PG_FREE_LIST | PG_LIST_TAIL);
		page_unlock(npp);
	}

	/*
	 * Give up the pages we already got.
	 */
	while (plist != NULL) {
		pp = plist;
		page_sub(&plist, pp);
		page_io_unlock(pp);
		plist_len++;
		/*LINTED: constant in conditional ctx*/
		VN_DISPOSE(pp, B_INVAL, 0, kcred);
	}

	/*
	 * VN_DISPOSE does freemem accounting for the pages in plist
	 * by calling page_free. So, we need to undo the pcf accounting
	 * for only the remaining pages.
	 */
	VM_STAT_ADD(page_create_putbacks);
	page_create_putback(pages_req - plist_len);

	return (NULL);
}
