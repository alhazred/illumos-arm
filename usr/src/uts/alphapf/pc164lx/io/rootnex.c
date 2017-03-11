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
 * Copyright (c) 1992, 2010, Oracle and/or its affiliates. All rights reserved.
 */
/*
 * Copyright 2011 Nexenta Systems, Inc.  All rights reserved.
 * Copyright (c) 2011 Bayard G. Bell.  All rights reserved.
 * Copyright 2012 Garrett D'Amore <garrett@damore.org>.  All rights reserved.
 */

/*
 * meson root nexus driver
 */

#include <sys/sysmacros.h>
#include <sys/conf.h>
#include <sys/autoconf.h>
#include <sys/sysmacros.h>
#include <sys/debug.h>
#include <sys/psw.h>
#include <sys/promif.h>
#include <sys/devops.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <vm/seg.h>
#include <vm/seg_kmem.h>
#include <vm/seg_dev.h>
#include <sys/vmem.h>
#include <sys/mman.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/avintr.h>
#include <sys/errno.h>
#include <sys/modctl.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/mach_intr.h>
#include <sys/ontrap.h>
#include <sys/atomic.h>
#include <sys/sdt.h>
#include <sys/rootnex.h>
#include <vm/hat_alpha.h>
#include <sys/ddifm.h>
#include <sys/ddi_isa.h>
#include <sys/spl.h>
#include <sys/machparam.h>

/* driver global state */
static rootnex_state_t *rootnex_state;

/* statically defined integer/boolean properties for the root node */
static rootnex_intprop_t rootnex_intprp[] = {
	{ "PAGESIZE",			PAGESIZE },
	{ "MMU_PAGESIZE",		MMU_PAGESIZE },
	{ "MMU_PAGEOFFSET",		MMU_PAGEOFFSET },
	{ DDI_RELATIVE_ADDRESSING,	1 },
};
#define	NROOT_INTPROPS	(sizeof (rootnex_intprp) / sizeof (rootnex_intprop_t))

static struct cb_ops rootnex_cb_ops = {
	nodev,		/* open */
	nodev,		/* close */
	nodev,		/* strategy */
	nodev,		/* print */
	nodev,		/* dump */
	nodev,		/* read */
	nodev,		/* write */
	nodev,		/* ioctl */
	nodev,		/* devmap */
	nodev,		/* mmap */
	nodev,		/* segmap */
	nochpoll,	/* chpoll */
	ddi_prop_op,	/* cb_prop_op */
	NULL,		/* struct streamtab */
	D_NEW | D_MP | D_HOTPLUG, /* compatibility flags */
	CB_REV,		/* Rev */
	nodev,		/* cb_aread */
	nodev		/* cb_awrite */
};

static int rootnex_map(dev_info_t *dip, dev_info_t *rdip, ddi_map_req_t *mp,
    off_t offset, off_t len, caddr_t *vaddrp);
static int rootnex_map_fault(dev_info_t *dip, dev_info_t *rdip,
    struct hat *hat, struct seg *seg, caddr_t addr,
    struct devpage *dp, pfn_t pfn, uint_t prot, uint_t lock);

static int rootnex_ctlops(dev_info_t *dip, dev_info_t *rdip,
    ddi_ctl_enum_t ctlop, void *arg, void *result);
static int rootnex_fm_init(dev_info_t *dip, dev_info_t *tdip, int tcap,
    ddi_iblock_cookie_t *ibc);
static int rootnex_intr_ops(dev_info_t *pdip, dev_info_t *rdip,
    ddi_intr_op_t intr_op, ddi_intr_handle_impl_t *hdlp, void *result);

static struct bus_ops rootnex_bus_ops = {
	BUSO_REV,
	rootnex_map,
	NULL,
	NULL,
	NULL,
	rootnex_map_fault,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	rootnex_ctlops,
	ddi_bus_prop_op,
	i_ddi_rootnex_get_eventcookie,
	i_ddi_rootnex_add_eventcall,
	i_ddi_rootnex_remove_eventcall,
	i_ddi_rootnex_post_event,
	0,			/* bus_intr_ctl */
	0,			/* bus_config */
	0,			/* bus_unconfig */
	rootnex_fm_init,	/* bus_fm_init */
	NULL,			/* bus_fm_fini */
	NULL,			/* bus_fm_access_enter */
	NULL,			/* bus_fm_access_exit */
	NULL,			/* bus_powr */
	rootnex_intr_ops	/* bus_intr_op */
};

static int rootnex_attach(dev_info_t *dip, ddi_attach_cmd_t cmd);
static int rootnex_detach(dev_info_t *dip, ddi_detach_cmd_t cmd);
static int rootnex_quiesce(dev_info_t *dip);

static struct dev_ops rootnex_ops = {
	DEVO_REV,
	0,
	ddi_no_info,
	nulldev,
	nulldev,
	rootnex_attach,
	rootnex_detach,
	nulldev,
	&rootnex_cb_ops,
	&rootnex_bus_ops,
	NULL,
	rootnex_quiesce,		/* quiesce */
};

static struct modldrv rootnex_modldrv = {
	&mod_driverops,
	"alpha root nexus %I%",
	&rootnex_ops
};

static struct modlinkage rootnex_modlinkage = {
	MODREV_1,
	(void *)&rootnex_modldrv,
	NULL
};

/*
 *  extern hacks
 */
extern struct seg_ops segdev_ops;
#ifdef	DDI_MAP_DEBUG
extern int ddi_map_debug_flag;
#define	ddi_map_debug	if (ddi_map_debug_flag) prom_printf
#endif
extern int impl_ddi_sunbus_initchild(dev_info_t *dip);
extern void impl_ddi_sunbus_removechild(dev_info_t *dip);

/*
 *  Internal functions
 */
static void rootnex_add_props(dev_info_t *);
static int rootnex_ctl_reportdev(dev_info_t *dip);
static struct intrspec *rootnex_get_ispec(dev_info_t *rdip, int inum);
static int rootnex_map_regspec(ddi_map_req_t *mp, caddr_t *vaddrp);
static int rootnex_unmap_regspec(ddi_map_req_t *mp, caddr_t *vaddrp);
static int rootnex_map_handle(ddi_map_req_t *mp, off_t offset);

/*
 * _init()
 *
 */
int
_init(void)
{

	rootnex_state = NULL;
	return (mod_install(&rootnex_modlinkage));
}


/*
 * _info()
 *
 */
int
_info(struct modinfo *modinfop)
{
	return (mod_info(&rootnex_modlinkage, modinfop));
}


/*
 * _fini()
 *
 */
int
_fini(void)
{
	return (EBUSY);
}


/*
 * rootnex_attach()
 *
 */
static int
rootnex_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int fmcap;
	int e;

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	/*
	 * We should only have one instance of rootnex. Save it away since we
	 * don't have an easy way to get it back later.
	 */
	ASSERT(rootnex_state == NULL);
	rootnex_state = kmem_zalloc(sizeof (rootnex_state_t), KM_SLEEP);

	rootnex_state->r_dip = dip;
	rootnex_state->r_err_ibc = (ddi_iblock_cookie_t)ipltospl(15);

	ddi_system_fmcap = DDI_FM_EREPORT_CAPABLE | DDI_FM_ERRCB_CAPABLE |
	    DDI_FM_ACCCHK_CAPABLE | DDI_FM_DMACHK_CAPABLE;
	fmcap = ddi_system_fmcap;
	ddi_fm_init(dip, &fmcap, &rootnex_state->r_err_ibc);

	/* Add static root node properties */
	rootnex_add_props(dip);

	/* since we can't call ddi_report_dev() */
	cmn_err(CE_CONT, "?root nexus = %s\n", ddi_get_name(dip));

	/* Initialize rootnex event handle */
	i_ddi_rootnex_init_events(dip);

	return (DDI_SUCCESS);
}


/*
 * rootnex_detach()
 *
 */
/*ARGSUSED*/
static int
rootnex_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_SUSPEND:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}
	/*NOTREACHED*/

}


/*
 * rootnex_add_props()
 *
 */
static void
rootnex_add_props(dev_info_t *dip)
{
	rootnex_intprop_t *rpp;
	int i;

	/* Add static integer/boolean properties to the root node */
	rpp = rootnex_intprp;
	for (i = 0; i < NROOT_INTPROPS; i++) {
		(void) e_ddi_prop_update_int(DDI_DEV_T_NONE, dip,
		    rpp[i].prop_name, rpp[i].prop_value);
	}
}



/*
 * *************************
 *  ctlops related routines
 * *************************
 */

/*
 * rootnex_ctlops()
 *
 */
/*ARGSUSED*/
static int
rootnex_ctlops(dev_info_t *dip, dev_info_t *rdip, ddi_ctl_enum_t ctlop,
    void *arg, void *result)
{
	int n, *ptr;
	struct ddi_parent_private_data *pdp;

	switch (ctlop) {
	case DDI_CTLOPS_DMAPMAPC:
		/*
		 * Return 'partial' to indicate that dma mapping
		 * has to be done in the main MMU.
		 */
		return (DDI_DMA_PARTIAL);

	case DDI_CTLOPS_BTOP:
		/*
		 * Convert byte count input to physical page units.
		 * (byte counts that are not a page-size multiple
		 * are rounded down)
		 */
		*(ulong_t *)result = btop(*(ulong_t *)arg);
		return (DDI_SUCCESS);

	case DDI_CTLOPS_PTOB:
		/*
		 * Convert size in physical pages to bytes
		 */
		*(ulong_t *)result = ptob(*(ulong_t *)arg);
		return (DDI_SUCCESS);

	case DDI_CTLOPS_BTOPR:
		/*
		 * Convert byte count input to physical page units
		 * (byte counts that are not a page-size multiple
		 * are rounded up)
		 */
		*(ulong_t *)result = btopr(*(ulong_t *)arg);
		return (DDI_SUCCESS);

	case DDI_CTLOPS_INITCHILD:
		return (impl_ddi_sunbus_initchild(arg));

	case DDI_CTLOPS_UNINITCHILD:
		impl_ddi_sunbus_removechild(arg);
		return (DDI_SUCCESS);

	case DDI_CTLOPS_REPORTDEV:
		return (rootnex_ctl_reportdev(rdip));

	case DDI_CTLOPS_IOMIN:
		/*
		 * Nothing to do here but reflect back..
		 */
		return (DDI_SUCCESS);

	case DDI_CTLOPS_REGSIZE:
	case DDI_CTLOPS_NREGS:
		break;

	case DDI_CTLOPS_SIDDEV:
		if (ndi_dev_is_prom_node(rdip))
			return (DDI_SUCCESS);
		if (ndi_dev_is_persistent_node(rdip))
			return (DDI_SUCCESS);
		return (DDI_FAILURE);

	case DDI_CTLOPS_POWER:
		return ((*pm_platform_power)((power_req_t *)arg));

	case DDI_CTLOPS_RESERVED0: /* Was DDI_CTLOPS_NINTRS, obsolete */
	case DDI_CTLOPS_RESERVED1: /* Was DDI_CTLOPS_POKE_INIT, obsolete */
	case DDI_CTLOPS_RESERVED2: /* Was DDI_CTLOPS_POKE_FLUSH, obsolete */
	case DDI_CTLOPS_RESERVED3: /* Was DDI_CTLOPS_POKE_FINI, obsolete */
	case DDI_CTLOPS_RESERVED4: /* Was DDI_CTLOPS_INTR_HILEVEL, obsolete */
	case DDI_CTLOPS_RESERVED5: /* Was DDI_CTLOPS_XLATE_INTRS, obsolete */
		return (DDI_FAILURE);

	default:
		return (DDI_FAILURE);
	}
	/*
	 * The rest are for "hardware" properties
	 */
	if ((pdp = ddi_get_parent_data(rdip)) == NULL)
		return (DDI_FAILURE);

	if (ctlop == DDI_CTLOPS_NREGS) {
		ptr = (int *)result;
		*ptr = pdp->par_nreg;
	} else {
		off_t *size = (off_t *)result;

		ptr = (int *)arg;
		n = *ptr;
		if (n >= pdp->par_nreg) {
			return (DDI_FAILURE);
		}
		uint64_t regspec_size = pdp->par_reg[n].regspec_size;
		regspec_size |= (((uint64_t)((pdp->par_reg[n].regspec_bustype >> 16) & 0xfff)) << 32);
		*size = (off_t)regspec_size;
	}
	return (DDI_SUCCESS);
}


/*
 * rootnex_ctl_reportdev()
 *
 */
static int
rootnex_ctl_reportdev(dev_info_t *dev)
{
	int i, n, len, f_len = 0;
	char *buf;

	buf = kmem_alloc(REPORTDEV_BUFSIZE, KM_SLEEP);
	f_len += snprintf(buf, REPORTDEV_BUFSIZE,
	    "%s%d at root", ddi_driver_name(dev), ddi_get_instance(dev));
	len = strlen(buf);

	for (i = 0; i < sparc_pd_getnreg(dev); i++) {

		struct regspec *rp = sparc_pd_getreg(dev, i);

		if (i == 0)
			f_len += snprintf(buf + len, REPORTDEV_BUFSIZE - len,
			    ": ");
		else
			f_len += snprintf(buf + len, REPORTDEV_BUFSIZE - len,
			    " and ");
		len = strlen(buf);

		uint64_t regspec_addr = rp->regspec_addr;
		regspec_addr |= (((uint64_t)(rp->regspec_bustype & 0xffff)) << 32);

		switch (rp->regspec_bustype >> 28) {
		default:
			f_len += snprintf(buf + len, REPORTDEV_BUFSIZE - len,
			    "space %x offset %lx",
			    rp->regspec_bustype >> 28, regspec_addr);
			break;
		}
		len = strlen(buf);
	}
	for (i = 0, n = sparc_pd_getnintr(dev); i < n; i++) {
		int pri;

		if (i != 0) {
			f_len += snprintf(buf + len, REPORTDEV_BUFSIZE - len,
			    ",");
			len = strlen(buf);
		}
		pri = INT_IPL(sparc_pd_getintr(dev, i)->intrspec_pri);
		f_len += snprintf(buf + len, REPORTDEV_BUFSIZE - len,
		    " sparc ipl %d", pri);
		len = strlen(buf);
	}
#ifdef DEBUG
	if (f_len + 1 >= REPORTDEV_BUFSIZE) {
		cmn_err(CE_NOTE, "next message is truncated: "
		    "printed length 1024, real length %d", f_len);
	}
#endif /* DEBUG */
	cmn_err(CE_CONT, "?%s\n", buf);
	kmem_free(buf, REPORTDEV_BUFSIZE);
	return (DDI_SUCCESS);
}


/*
 * ******************
 *  map related code
 * ******************
 */

/*
 * rootnex_map()
 *
 */
static int
rootnex_map(dev_info_t *dip, dev_info_t *rdip, ddi_map_req_t *mp, off_t offset,
    off_t len, caddr_t *vaddrp)
{
	struct regspec *rp, tmp_reg;
	ddi_map_req_t mr = *mp;
	int error;
	ddi_acc_hdl_t *hp = NULL;

	mp = &mr;

	switch (mp->map_op)  {
	case DDI_MO_MAP_LOCKED:
	case DDI_MO_UNMAP:
	case DDI_MO_MAP_HANDLE:
		break;
	default:
		cmn_err(CE_WARN, "rootnex_map: unimplemented map "
		    "op %d.", mp->map_op);
		return (DDI_ME_UNIMPLEMENTED);
	}

	if (mp->map_flags & DDI_MF_USER_MAPPING)  {
		cmn_err(CE_WARN, "rootnex_map: unimplemented map "
		    "type: user.");
		return (DDI_ME_UNIMPLEMENTED);
	}

	if (mp->map_type == DDI_MT_RNUMBER)  {
		int rnumber = mp->map_obj.rnumber;

		rp = i_ddi_rnumber_to_regspec(rdip, rnumber);
		if (rp == (struct regspec *)0)  {
			cmn_err(CE_WARN, "rootnex_map: Out of "
			    "range rnumber <%d>, device <%s>", rnumber,
			    ddi_get_name(rdip));
			return (DDI_ME_RNUMBER_RANGE);
		}

		mp->map_type = DDI_MT_REGSPEC;
		mp->map_obj.rp = rp;
	}

	tmp_reg = *(mp->map_obj.rp);
	rp = mp->map_obj.rp = &tmp_reg;

	if (len != 0)
		rp->regspec_size = (uint_t)len;

	if ((error = i_ddi_apply_range(dip, rdip, mp->map_obj.rp)) != 0)
		return (error);

	switch (mp->map_op)  {
	case DDI_MO_MAP_LOCKED:
		if (mp->map_handlep) {
			hp = mp->map_handlep;
		}
		if (rp->regspec_bustype >> 28) {
			if (mp->map_flags & DDI_MF_DEVICE_MAPPING)
				return (DDI_ME_INVAL);
			error = rootnex_map_regspec(mp, vaddrp);
			if (error == 0) {
				if (hp) {
					ddi_acc_impl_t *ap = (ddi_acc_impl_t *)hp->ah_platform_private;
					ap->ahi_acc_attr |= DDI_ACCATTR_IO_SPACE;
					hp->ah_addr = (caddr_t)offset;
					ap->ahi_io_port_base = (ulong_t)*vaddrp;
					*vaddrp = (caddr_t)offset;
					hp->ah_hat_flags = 0;
					impl_acc_hdl_init(hp);
				}
				*vaddrp = (caddr_t)offset;
			}
		} else {
			uint64_t regspec_addr = rp->regspec_addr;
			regspec_addr |= (((uint64_t)(rp->regspec_bustype & 0xffff)) << 32);
			regspec_addr += offset;

			rp->regspec_addr = regspec_addr & 0xffffffff;
			rp->regspec_bustype &= ~0xffff;
			rp->regspec_bustype |= ((regspec_addr >> 32) & 0xffff);

			error = rootnex_map_regspec(mp, vaddrp);
			if (error == 0) {
				if (hp) {
					ddi_acc_impl_t *ap = (ddi_acc_impl_t *)hp->ah_platform_private;
					ap->ahi_acc_attr |= DDI_ACCATTR_CPU_VADDR;
					hp->ah_addr = *vaddrp;
					hp->ah_hat_flags = 0;
					impl_acc_hdl_init(hp);
				}
			}
		}
		return (error);

	case DDI_MO_UNMAP:
		return (rootnex_unmap_regspec(mp, vaddrp));

	case DDI_MO_MAP_HANDLE:
		return (rootnex_map_handle(mp, offset));
	}

	return (DDI_ME_UNIMPLEMENTED);
}


/*
 * rootnex_map_fault()
 *
 *	fault in mappings for requestors
 */
/*ARGSUSED*/
static int
rootnex_map_fault(dev_info_t *dip, dev_info_t *rdip, struct hat *hat,
    struct seg *seg, caddr_t addr, struct devpage *dp, pfn_t pfn, uint_t prot,
    uint_t lock)
{

#ifdef	DDI_MAP_DEBUG
	ddi_map_debug("rootnex_map_fault: address <%x> pfn <%x>", addr, pfn);
	ddi_map_debug(" Seg <%s>\n",
	    seg->s_ops == &segdev_ops ? "segdev" :
	    seg == &kvseg ? "segkmem" : "NONE!");
#endif	/* DDI_MAP_DEBUG */

	/*
	 * This is all terribly broken, but it is a start
	 *
	 * XXX	Note that this test means that segdev_ops
	 *	must be exported from seg_dev.c.
	 * XXX	What about devices with their own segment drivers?
	 */
	if (seg->s_ops == &segdev_ops) {
		struct segdev_data *sdp = (struct segdev_data *)seg->s_data;

		if (hat == NULL) {
			/*
			 * This is one plausible interpretation of
			 * a null hat i.e. use the first hat on the
			 * address space hat list which by convention is
			 * the hat of the system MMU.  At alternative
			 * would be to panic .. this might well be better ..
			 */
			ASSERT(AS_READ_HELD(seg->s_as));
			hat = seg->s_as->a_hat;
			cmn_err(CE_NOTE, "rootnex_map_fault: nil hat");
		}
		hat_devload(hat, addr, MMU_PAGESIZE, pfn, prot | sdp->hat_attr,
		    (lock ? HAT_LOAD_LOCK : HAT_LOAD));
	} else if (seg == &kvseg && dp == NULL) {
		hat_devload(kas.a_hat, addr, MMU_PAGESIZE, pfn, prot,
		    HAT_LOAD_LOCK);
	} else
		return (DDI_FAILURE);
	return (DDI_SUCCESS);
}


/*
 * rootnex_map_regspec()
 *     we don't support mapping of I/O cards above 4Gb
 */
static int
rootnex_map_regspec(ddi_map_req_t *mp, caddr_t *vaddrp)
{
	caddr_t kaddr;
	pfn_t	pfn;
	struct regspec *rp = mp->map_obj.rp;
	ddi_acc_hdl_t *hp;
	uint_t	pgoffset;

	uint64_t regspec_size = rp->regspec_size;
	regspec_size |= (((uint64_t)((rp->regspec_bustype >> 16) & 0xfff)) << 32);

	if (regspec_size == 0) {
		cmn_err(CE_NOTE, "rootnex_map_regspec: zero regspec_size\n");
		return (DDI_ME_INVAL);
	}

	uint64_t regspec_addr = rp->regspec_addr;
	regspec_addr |= (((uint64_t)(rp->regspec_bustype & 0xffff)) << 32);
	pfn = mmu_btop(regspec_addr);
	if (mp->map_flags & DDI_MF_DEVICE_MAPPING)
		*vaddrp = (caddr_t)pfn;
	else {
		pgoffset = (ulong_t)regspec_addr & MMU_PAGEOFFSET;
		paddr_t pa = mmu_ptob(pfn) | pgoffset;
		*vaddrp = (caddr_t)(SEGKPM_BASE | pa);
		cmn_err(CE_CONT, "rootnex_map_regspec: %p -> %p\n", (void *)pa, *vaddrp);
		hp = mp->map_handlep;
		if (hp) {
			hp->ah_pfn = pfn;
			hp->ah_pnum = mmu_btopr(regspec_size + pgoffset);
		}
	}

	return (0);
}


/*
 * rootnex_unmap_regspec()
 *
 */
static int
rootnex_unmap_regspec(ddi_map_req_t *mp, caddr_t *vaddrp)
{
	struct regspec *rp;

	if (mp->map_flags & DDI_MF_DEVICE_MAPPING)
		return (0);

	rp = mp->map_obj.rp;

	uint64_t regspec_size = rp->regspec_size;
	regspec_size |= (((uint64_t)((rp->regspec_bustype >> 16) & 0xfff)) << 32);

	if (regspec_size == 0) {
		cmn_err(CE_WARN, "rootnex_unmap_regspec: zero regspec_size\n");
		return (DDI_ME_INVAL);
	}

	*vaddrp = (caddr_t)0;

	return (0);
}


/*
 * rootnex_map_handle()
 *
 */
static int
rootnex_map_handle(ddi_map_req_t *mp, off_t offset)
{
	ddi_acc_hdl_t *hp;
	uint_t hat_flags;
	register struct regspec *rp;

	hp = mp->map_handlep;
	rp = mp->map_obj.rp;

	uint64_t regspec_addr = rp->regspec_addr;
	regspec_addr |= (((uint64_t)(rp->regspec_bustype & 0xffff)) << 32);

	uint64_t regspec_size = rp->regspec_size;
	regspec_size |= (((uint64_t)((rp->regspec_bustype >> 16) & 0xfff)) << 32);

	if (regspec_size == 0)
		return (DDI_ME_INVAL);

	hp->ah_hat_flags = 0;
	hp->ah_pfn = mmu_btop(regspec_addr + offset);
	hp->ah_pnum = mmu_btopr(regspec_addr + offset + regspec_size) - hp->ah_pfn;
	return (DDI_SUCCESS);
}



/*
 * ************************
 *  interrupt related code
 * ************************
 */

/*
 * rootnex_intr_ops()
 *	bus_intr_op() function for interrupt support
 */
/* ARGSUSED */
static int
rootnex_intr_ops(dev_info_t *pdip, dev_info_t *rdip, ddi_intr_op_t intr_op,
    ddi_intr_handle_impl_t *hdlp, void *result)
{
	struct intrspec			*ispec;
	struct ddi_parent_private_data	*pdp;
	int ret;

	DDI_INTR_NEXDBG((CE_CONT,
	    "rootnex_intr_ops: pdip = %p, rdip = %p, intr_op = %x, hdlp = %p\n",
	    (void *)pdip, (void *)rdip, intr_op, (void *)hdlp));

	switch (intr_op) {
	case DDI_INTROP_GETCAP:
		break;
	case DDI_INTROP_SETCAP:
		break;
	case DDI_INTROP_ALLOC:
		hdlp->ih_pri = 0;
		*(int *)result = hdlp->ih_scratch1;
		break;
	case DDI_INTROP_FREE:
		break;
	case DDI_INTROP_GETPRI:
		*(int *)result = 0;
		break;
	case DDI_INTROP_SETPRI:
		break;
	case DDI_INTROP_ADDISR:
		hdlp->ih_vector = hdlp->ih_inum;
		break;
	case DDI_INTROP_REMISR:
		break;
	case DDI_INTROP_ENABLE:
		ret = add_avintr((void *)hdlp,
				 hdlp->ih_pri,
				 hdlp->ih_cb_func,
				 DEVI(rdip)->devi_name,
				 hdlp->ih_vector,
				 hdlp->ih_cb_arg1,
				 hdlp->ih_cb_arg2,
				 NULL,
				 rdip);
		if (ret == 0) {
			return (DDI_FAILURE);
		}
		break;
	case DDI_INTROP_DISABLE:
		rem_avintr((void *)hdlp,
			   hdlp->ih_pri,
			   hdlp->ih_cb_func,
			   hdlp->ih_vector);
		break;
	case DDI_INTROP_SETMASK:
		break;
	case DDI_INTROP_CLRMASK:
		break;
	case DDI_INTROP_GETPENDING:
		break;
	case DDI_INTROP_NAVAIL:
	case DDI_INTROP_NINTRS:
		*(int *)result = i_ddi_get_intx_nintrs(rdip);
		break;
	case DDI_INTROP_SUPPORTED_TYPES:
		*(int *)result = DDI_INTR_TYPE_FIXED;
		break;
	default:
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}


/*
 * rootnex_get_ispec()
 *	convert an interrupt number to an interrupt specification.
 *	The interrupt number determines which interrupt spec will be
 *	returned if more than one exists.
 *
 *	Look into the parent private data area of the 'rdip' to find out
 *	the interrupt specification.  First check to make sure there is
 *	one that matchs "inumber" and then return a pointer to it.
 *
 *	Return NULL if one could not be found.
 *
 *	NOTE: This is needed for rootnex_intr_ops()
 */
static struct intrspec *
rootnex_get_ispec(dev_info_t *rdip, int inum)
{
	struct ddi_parent_private_data *pdp = ddi_get_parent_data(rdip);

	/*
	 * Special case handling for drivers that provide their own
	 * intrspec structures instead of relying on the DDI framework.
	 *
	 * A broken hardware driver in ON could potentially provide its
	 * own intrspec structure, instead of relying on the hardware.
	 * If these drivers are children of 'rootnex' then we need to
	 * continue to provide backward compatibility to them here.
	 *
	 * Following check is a special case for 'pcic' driver which
	 * was found to have broken hardwre andby provides its own intrspec.
	 *
	 * Verbatim comments from this driver are shown here:
	 * "Don't use the ddi_add_intr since we don't have a
	 * default intrspec in all cases."
	 *
	 * Since an 'ispec' may not be always created for it,
	 * check for that and create one if so.
	 *
	 * NOTE: Currently 'pcic' is the only driver found to do this.
	 */
	if (!pdp->par_intr && strcmp(ddi_get_name(rdip), "pcic") == 0) {
		pdp->par_nintr = 1;
		pdp->par_intr = kmem_zalloc(sizeof (struct intrspec) *
		    pdp->par_nintr, KM_SLEEP);
	}

	/* Validate the interrupt number */
	if (inum >= pdp->par_nintr)
		return (NULL);

	/* Get the interrupt structure pointer and return that */
	return ((struct intrspec *)&pdp->par_intr[inum]);
}

/*
 * *********
 *  FMA Code
 * *********
 */

/*
 * rootnex_fm_init()
 *    FMA init busop
 */
/* ARGSUSED */
static int
rootnex_fm_init(dev_info_t *dip, dev_info_t *tdip, int tcap,
    ddi_iblock_cookie_t *ibc)
{
	*ibc = rootnex_state->r_err_ibc;

	return (ddi_system_fmcap);
}

/*ARGSUSED*/
static int
rootnex_quiesce(dev_info_t *dip)
{
	return (DDI_SUCCESS);
}
