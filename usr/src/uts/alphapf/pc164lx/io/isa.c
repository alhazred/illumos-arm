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
 * Copyright (c) 1992, 2011, Oracle and/or its affiliates. All rights reserved.
 */

/*
 *	ISA bus nexus driver
 */

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/autoconf.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/ddidmareq.h>
#include <sys/ddi_impldefs.h>
#include <sys/ddi_subrdefs.h>
#include <sys/dma_engine.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/mach_intr.h>
#include <sys/pci.h>
#include <sys/note.h>
#include <sys/bootinfo.h>
#include <sys/isabus.h>
#include <sys/promif.h>
#include <sys/pic.h>

static char USED_RESOURCES[] = "used-resources";
static void isa_enumerate(int);
static void free_isa_ppd(dev_info_t *);
static void
make_isa_ppd(dev_info_t *child, struct isa_parent_private_data **ppd);
static inline uint8_t _inb(int port)
{
	uint8_t value;
	uint8_t *ioport_addr = (uint8_t *)(0xFFFFFC8900000000UL + port);
	__asm__ __volatile__(
	    "ldbu %0, %1" : "=r" (value) : "m" (*ioport_addr) : "memory");
	__asm__ __volatile__("mb" ::: "memory");
	return value;
}

static inline void _outb(int port, uint8_t value)
{
	uint8_t *ioport_addr = (uint8_t *)(0xFFFFFC8900000000UL + port);
	__asm__ __volatile__(
	    "stb %1, %0" : "=m" (*ioport_addr) : "r" (value) : "memory");
	__asm__ __volatile__("mb" ::: "memory");
}
/*
 * The following typedef is used to represent an entry in the "ranges"
 * property of a pci-isa bridge device node.
 */
typedef struct {
	uint32_t child_high;
	uint32_t child_low;
	uint32_t parent_high;
	uint32_t parent_mid;
	uint32_t parent_low;
	uint32_t size;
} pib_ranges_t;

typedef struct {
	uint32_t base;
	uint32_t len;
} used_ranges_t;

#define	USED_CELL_SIZE	2	/* 1 byte addr, 1 byte size */
#define	ISA_ADDR_IO	1	/* IO address space */
#define	ISA_ADDR_MEM	0	/* memory adress space */
/*
 * #define ISA_DEBUG 1
 */

/*
 * For serial ports not enumerated by ACPI, and parallel ports with
 * illegal size. Typically, a system can have as many as 4 serial
 * ports and 3 parallel ports.
 */
#define	MAX_EXTRA_RESOURCE	7
static isa_regspec_t isa_extra_resource[MAX_EXTRA_RESOURCE];
static int isa_extra_count = 0;

/*
 *      Local data
 */
static ddi_dma_attr_t ISA_dma_attr = {
	DMA_ATTR_V0,
	(unsigned long long)0,
	(unsigned long long)0x00ffffff,
	0x0000ffff,
	1,
	1,
	1,
	(unsigned long long)0xffffffff,
	(unsigned long long)0x0000ffff,
	1,
	1,
	0
};


/*
 * Config information
 */

static int
isa_bus_map(dev_info_t *dip, dev_info_t *rdip, ddi_map_req_t *mp,
    off_t offset, off_t len, caddr_t *vaddrp);

static int
isa_dma_allochdl(dev_info_t *, dev_info_t *, ddi_dma_attr_t *,
    int (*waitfp)(caddr_t), caddr_t arg, ddi_dma_handle_t *);

static int
isa_dma_mctl(dev_info_t *, dev_info_t *, ddi_dma_handle_t, enum ddi_dma_ctlops,
    off_t *, size_t *, caddr_t *, uint_t);

static int
isa_ctlops(dev_info_t *, dev_info_t *, ddi_ctl_enum_t, void *, void *);

static int
isa_intr_ops(dev_info_t *pdip, dev_info_t *rdip, ddi_intr_op_t intr_op,
    ddi_intr_handle_impl_t *hdlp, void *result);
static int isa_alloc_intr_fixed(dev_info_t *, ddi_intr_handle_impl_t *, void *);
static int isa_free_intr_fixed(dev_info_t *, ddi_intr_handle_impl_t *);

struct bus_ops isa_bus_ops = {
	BUSO_REV,
	isa_bus_map,
	NULL,
	NULL,
	NULL,
	i_ddi_map_fault,
	NULL,
	isa_dma_allochdl,
	ddi_dma_freehdl,
	ddi_dma_bindhdl,
	ddi_dma_unbindhdl,
	ddi_dma_flush,
	ddi_dma_win,
	isa_dma_mctl,
	isa_ctlops,
	ddi_bus_prop_op,
	NULL,		/* (*bus_get_eventcookie)();	*/
	NULL,		/* (*bus_add_eventcall)();	*/
	NULL,		/* (*bus_remove_eventcall)();	*/
	NULL,		/* (*bus_post_event)();		*/
	NULL,		/* (*bus_intr_ctl)(); */
	NULL,		/* (*bus_config)(); */
	NULL,		/* (*bus_unconfig)(); */
	NULL,		/* (*bus_fm_init)(); */
	NULL,		/* (*bus_fm_fini)(); */
	NULL,		/* (*bus_fm_access_enter)(); */
	NULL,		/* (*bus_fm_access_exit)(); */
	NULL,		/* (*bus_power)(); */
	isa_intr_ops	/* (*bus_intr_op)(); */
};


static int isa_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);

/*
 * Internal isa ctlops support routines
 */
static int isa_initchild(dev_info_t *child);

struct dev_ops isa_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	nulldev,		/* identify */
	nulldev,		/* probe */
	isa_attach,		/* attach */
	nulldev,		/* detach */
	nodev,			/* reset */
	(struct cb_ops *)0,	/* driver operations */
	&isa_bus_ops,		/* bus operations */
	NULL,			/* power */
	ddi_quiesce_not_needed,		/* quiesce */
};

/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
	&mod_driverops, /* Type of module.  This is ISA bus driver */
	"isa nexus driver for 'ISA'",
	&isa_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,
	&modldrv,
	NULL
};

int
_init(void)
{
	int	err;

	if ((err = mod_install(&modlinkage)) != 0)
		return (err);

	impl_bus_add_probe(isa_enumerate);
	return (0);
}

int
_fini(void)
{
	int	err;

	impl_bus_delete_probe(isa_enumerate);

	if ((err = mod_remove(&modlinkage)) != 0)
		return (err);

	return (0);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*ARGSUSED*/
static int
isa_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{

	int rval;
	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	ddi_report_dev(devi);

	return DDI_SUCCESS;
}

static void
isa_create_cells_props(dev_info_t *dip)
{
	(void) ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#address-cells", 2);
	(void) ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#size-cells", 1);
}

static struct intrspec *
isa_get_ispec(dev_info_t *rdip, int inum)
{
	struct isa_parent_private_data *pdp = ddi_get_parent_data(rdip);

	/* Validate the interrupt number */
	if (inum >= pdp->par_nintr)
		return (NULL);

	/* Get the interrupt structure pointer and return that */
	return ((struct intrspec *)&pdp->par_intr[inum]);
}

static int
isa_bus_map(dev_info_t *dip, dev_info_t *rdip, ddi_map_req_t *mp,
    off_t offset, off_t len, caddr_t *vaddrp)
{
	isa_regspec_t *isa_rp, *isa_regs;
	ddi_map_req_t mr;
	uint_t reglen;
	int error;
	pci_regspec_t reg = {0};

	/*
	 * First, if given an rnumber, convert it to an isa_regspec...
	 */
	if (mp->map_type == DDI_MT_RNUMBER) {
		int rnumber = mp->map_obj.rnumber;
		int n;

		if (ddi_prop_lookup_int_array(DDI_DEV_T_ANY, rdip, DDI_PROP_DONTPASS, "reg", (int **)&isa_regs, &reglen) != DDI_SUCCESS)
			return (DDI_ME_RNUMBER_RANGE);

		n = (reglen * sizeof (int)) / sizeof (isa_regspec_t);

		if (rnumber < 0 || rnumber >= n) {
			ddi_prop_free(isa_regs);
			return (DDI_ME_RNUMBER_RANGE);
		}

		isa_rp = &isa_regs[rnumber];
	} else if (mp->map_type == DDI_MT_REGSPEC) {
		isa_rp = (isa_regspec_t *)mp->map_obj.rp;
	} else {
		return (DDI_ME_INVAL);
	}

	offset += isa_rp->phys_lo;
	reg.pci_phys_hi = PCI_ADDR_IO | PCI_RELOCAT_B;
	reg.pci_phys_low = 0;
	reg.pci_size_low = isa_rp->size;

	if (mp->map_type == DDI_MT_RNUMBER) {
		ddi_prop_free(isa_regs);
	}

	mr = *mp;
	mr.map_type = DDI_MT_REGSPEC;
	mr.map_obj.rp = (struct regspec *)&reg;
	mp = &mr;
	return  ddi_map(dip, mp, offset, 0, vaddrp);
}

static int
isa_dma_allochdl(dev_info_t *dip, dev_info_t *rdip, ddi_dma_attr_t *dma_attr,
    int (*waitfp)(caddr_t), caddr_t arg, ddi_dma_handle_t *handlep)
{
	ddi_dma_attr_merge(dma_attr, &ISA_dma_attr);
	return (ddi_dma_allochdl(dip, rdip, dma_attr, waitfp, arg, handlep));
}

static int
isa_dma_mctl(dev_info_t *dip, dev_info_t *rdip,
    ddi_dma_handle_t handle, enum ddi_dma_ctlops request,
    off_t *offp, size_t *lenp, caddr_t *objp, uint_t flags)
{
	return ddi_dma_mctl(dip, rdip, handle, request, offp, lenp, objp, flags);
}
/*
 * Name a child
 */
static int
isa_name_child(dev_info_t *child, char *name, int namelen)
{
	char vendor[8];
	int device;
	uint32_t serial;
	int func;
	int bustype;
	uint32_t base;
	int proplen;
	int pnpisa = 0;
	isa_regspec_t *isa_regs;

	/*
	 * Fill in parent-private data
	 */
	if (ddi_get_parent_data(child) == NULL) {
		struct isa_parent_private_data *pdptr;
		make_isa_ppd(child, &pdptr);
		ddi_set_parent_data(child, pdptr);
	}

	/*
	 * For hw nodes, look up "reg" property
	 */
	if (ddi_getlongprop(DDI_DEV_T_ANY, child, DDI_PROP_DONTPASS, "reg",
	    (caddr_t)&isa_regs, &proplen) != DDI_PROP_SUCCESS) {
		return (DDI_FAILURE);
	}

	/*
	 * extract the device identifications
	 */
	bustype = isa_regs[0].phys_hi;
	base = isa_regs[0].phys_lo;
	(void) sprintf(name, "%x,%x", bustype, base);

	/*
	 * free the memory allocated by ddi_getlongprop().
	 */
	kmem_free(isa_regs, proplen);

	return (DDI_SUCCESS);
}

static int
isa_initchild(dev_info_t *child)
{
	char name[80];

	if (isa_name_child(child, name, 80) != DDI_SUCCESS)
		return (DDI_FAILURE);
	ddi_set_name_addr(child, name);

	if (ndi_dev_is_persistent_node(child) != 0) {
		return (DDI_SUCCESS);
	}
	if (ndi_merge_node(child, isa_name_child) == DDI_SUCCESS) {
		impl_ddi_sunbus_removechild(child);
		return (DDI_FAILURE);
	}
	return (DDI_SUCCESS);
}

static int
isa_ctlops(dev_info_t *dip, dev_info_t *rdip,
	ddi_ctl_enum_t ctlop, void *arg, void *result)
{
	isa_regspec_t *regp;
	uint_t reglen;
	int nreg;

	switch (ctlop) {
	case DDI_CTLOPS_INITCHILD:
		return (isa_initchild((dev_info_t *)arg));

	case DDI_CTLOPS_UNINITCHILD:
		free_isa_ppd((dev_info_t *)arg);
		impl_ddi_sunbus_removechild((dev_info_t *)arg);
		return (DDI_SUCCESS);

	case DDI_CTLOPS_REPORTDEV:
		if (rdip == (dev_info_t *)0)
			return (DDI_FAILURE);
		cmn_err(CE_CONT, "?ISA-device: %s%d\n", ddi_driver_name(rdip), ddi_get_instance(rdip));
		return (DDI_SUCCESS);

	case DDI_CTLOPS_REGSIZE:
	case DDI_CTLOPS_NREGS:
		if (rdip == (dev_info_t *)0)
			return (DDI_FAILURE);

		*(int *)result = 0;
		if (ddi_prop_lookup_int_array(DDI_DEV_T_ANY, rdip, DDI_PROP_DONTPASS, "reg", (int **)&regp, &reglen) != DDI_PROP_SUCCESS) {
			return (DDI_FAILURE);
		}

		nreg = (reglen * sizeof (int)) / sizeof (isa_regspec_t);
		if (ctlop == DDI_CTLOPS_NREGS)
			*(int *)result = nreg;
		else if (ctlop == DDI_CTLOPS_REGSIZE) {
			int rn = *(int *)arg;
			if (rn >= nreg) {
				ddi_prop_free(regp);
				return (DDI_FAILURE);
			}
			*(off_t *)result = regp[rn].size;
		}
		ddi_prop_free(regp);
		return (DDI_SUCCESS);

	case DDI_CTLOPS_ATTACH:
	case DDI_CTLOPS_DETACH:
	case DDI_CTLOPS_PEEK:
	case DDI_CTLOPS_POKE:
		return (DDI_FAILURE);

	default:
		return (ddi_ctlops(dip, rdip, ctlop, arg, result));
	}
}

#define ISA_FIX_PRI	12
static void
make_isa_ppd(dev_info_t *child, struct isa_parent_private_data **ppd)
{
	struct isa_parent_private_data *pdptr;
	int *reg_prop, *rng_prop, *irupts_prop;
	uint_t reg_len, rng_len, irupts_len;

	*ppd = pdptr = kmem_zalloc(sizeof (*pdptr), KM_SLEEP);

	if ((ddi_prop_lookup_int_array(DDI_DEV_T_ANY, child, DDI_PROP_DONTPASS,
	    "reg", &reg_prop, &reg_len) == DDI_PROP_SUCCESS) &&
	    (reg_len != 0)) {
		pdptr->par_nreg = (reg_len * sizeof (int)) /
		    sizeof (isa_regspec_t);
		pdptr->par_reg = (isa_regspec_t *)reg_prop;
	}

	if (ddi_prop_lookup_int_array(DDI_DEV_T_ANY, child, DDI_PROP_DONTPASS,
	    "ranges", &rng_prop, &rng_len) == DDI_PROP_SUCCESS) {
		pdptr->par_nrng = (rng_len * sizeof (int)) /
		    sizeof (struct rangespec);
		pdptr->par_rng = (struct rangespec *)rng_prop;
	}

	if ((ddi_prop_lookup_int_array(DDI_DEV_T_ANY, child, DDI_PROP_DONTPASS,
	    "interrupts", &irupts_prop, &irupts_len) == DDI_PROP_SUCCESS) &&
	    (irupts_len != 0)) {
		int i;
		pdptr->par_nintr = irupts_len;
		pdptr->par_intr = kmem_zalloc(irupts_len * sizeof (struct intrspec), KM_SLEEP);
		for (i = 0; i < irupts_len; i++) {
			pdptr->par_intr[i].intrspec_pri = ISA_FIX_PRI;
			pdptr->par_intr[i].intrspec_vec = irupts_prop[i];
			pdptr->par_intr[i].intrspec_func = 0;
		}
		ddi_prop_free((void *)irupts_prop);
	}
	ddi_set_parent_data(child, pdptr);
}

static void
free_isa_ppd(dev_info_t *dip)
{
	struct isa_parent_private_data *pdptr;

	if ((pdptr = (struct isa_parent_private_data *)
		    ddi_get_parent_data(dip)) == NULL)
		return;

	if (pdptr->par_nintr != 0)
		kmem_free(pdptr->par_intr, pdptr->par_nintr * sizeof (struct intrspec));

	if (pdptr->par_nrng != 0)
		ddi_prop_free((void *)pdptr->par_rng);

	if (pdptr->par_nreg != 0)
		ddi_prop_free((void *)pdptr->par_reg);

	kmem_free(pdptr, sizeof (*pdptr));
	ddi_set_parent_data(dip, NULL);
}

static void
psm_set_imr(int vecno, int val)
{
	int imr_port = vecno > 7 ? SIMR_PORT : MIMR_PORT;
	int imr_bit = 1 << (vecno & 0x07);

	if (val) {
		_outb(imr_port, _inb(imr_port) | imr_bit);
	} else {
		_outb(imr_port, _inb(imr_port) & ~imr_bit);
	}
}

static int
isa_intr_ops(dev_info_t *pdip, dev_info_t *rdip, ddi_intr_op_t intr_op,
    ddi_intr_handle_impl_t *hdlp, void *result)
{
	struct intrspec *ispec;

	if ((ispec = isa_get_ispec(rdip, hdlp->ih_inum)) == NULL)
		return (DDI_FAILURE);

	switch (intr_op) {
	case DDI_INTROP_GETCAP:
		*(int *)result = DDI_INTR_FLAG_EDGE;
		break;

	case DDI_INTROP_SETCAP:
		return (DDI_FAILURE);

	case DDI_INTROP_ALLOC:
		hdlp->ih_pri = ispec->intrspec_pri;
		*(int *)result = hdlp->ih_scratch1;
		break;

	case DDI_INTROP_FREE:
		break;

	case DDI_INTROP_GETPRI:
		*(int *)result = ispec->intrspec_pri;
		break;
	case DDI_INTROP_SETPRI:
		if (*(int *)result > LOCK_LEVEL)
			return (DDI_FAILURE);
		ispec->intrspec_pri =  *(int *)result;
		break;
	case DDI_INTROP_ADDISR:
		ispec->intrspec_func = hdlp->ih_cb_func;
		break;
	case DDI_INTROP_REMISR:
		if (hdlp->ih_type != DDI_INTR_TYPE_FIXED)
			return (DDI_FAILURE);
		ispec->intrspec_func = (uint_t (*)()) 0;
		break;
	case DDI_INTROP_ENABLE:
		hdlp->ih_vector = ispec->intrspec_vec;
		((ihdl_plat_t *)hdlp->ih_private)->ip_ispecp = ispec;
		if (!add_avintr((void *)hdlp, ispec->intrspec_pri,
		    hdlp->ih_cb_func, DEVI(rdip)->devi_name, hdlp->ih_vector,
		    hdlp->ih_cb_arg1, hdlp->ih_cb_arg2, NULL, rdip))
			return (DDI_FAILURE);
		break;
	case DDI_INTROP_DISABLE:
		((ihdl_plat_t *)hdlp->ih_private)->ip_ispecp = ispec;
		hdlp->ih_vector = ispec->intrspec_vec;
		rem_avintr((void *)hdlp, ispec->intrspec_pri,
		    hdlp->ih_cb_func, hdlp->ih_vector);
		break;
	case DDI_INTROP_SETMASK:
	case DDI_INTROP_CLRMASK:
	case DDI_INTROP_GETPENDING:
		return (DDI_FAILURE);
	case DDI_INTROP_NAVAIL:
	case DDI_INTROP_NINTRS:
		*(int *)result = i_ddi_get_intx_nintrs(rdip);
		if (*(int *)result == 0) {
			return (DDI_FAILURE);
		}
		break;
	case DDI_INTROP_SUPPORTED_TYPES:
		*(int *)result = DDI_INTR_TYPE_FIXED;	/* Always ... */
		break;
	default:
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}

static void
add_known_used_resources(void)
{
	/* needs to be in increasing order */
	int intr[] = {0x1, 0x3, 0x4, 0x6, 0x7, 0xc};
	int dma[] = {0x2};
	int io[] = {0x60, 0x1, 0x64, 0x1, 0x2f8, 0x8, 0x378, 0x8, 0x3f0, 0x10, 0x778, 0x4};
	dev_info_t *usedrdip;

	usedrdip = ddi_find_devinfo(USED_RESOURCES, -1, 0);

	if (usedrdip == NULL) {
		(void) ndi_devi_alloc_sleep(ddi_root_node(), USED_RESOURCES, (pnode_t)DEVI_SID_NODEID, &usedrdip);
	}

	(void) ndi_prop_update_int_array(DDI_DEV_T_NONE, usedrdip, "interrupts", (int *)intr, (int)(sizeof (intr) / sizeof (int)));
	(void) ndi_prop_update_int_array(DDI_DEV_T_NONE, usedrdip, "io-space", (int *)io, (int)(sizeof (io) / sizeof (int)));
	(void) ndi_prop_update_int_array(DDI_DEV_T_NONE, usedrdip, "dma-channels", (int *)dma, (int)(sizeof (dma) / sizeof (int)));
	(void) ndi_devi_bind_driver(usedrdip, 0);
}

static void
isa_enumerate(int reprogram)
{
	int circ, i;
	dev_info_t *xdip;
	dev_info_t *isa_dip = ddi_find_devinfo("isa", -1, 0);

	/* hard coded isa stuff */
	isa_regspec_t asy_regs[] = {
		{1, 0x3f8, 0x8},
		//{1, 0x2f8, 0x8}
	};
	int asy_intrs[] = {
		0x4,
		//0x3
	};

	isa_regspec_t i8042_regs[] = {
		{1, 0x60, 0x1},
		{1, 0x64, 0x1}
	};
	int i8042_intrs[] = {0x1, 0xc};

	if (reprogram || !isa_dip)
		return;

	bzero(isa_extra_resource, MAX_EXTRA_RESOURCE * sizeof (isa_regspec_t));

	ndi_devi_enter(isa_dip, &circ);

	/* serial ports */
	for (i = 0; i < sizeof (asy_regs)/sizeof(asy_regs[0]); i++) {
		ndi_devi_alloc_sleep(isa_dip, "asy", (pnode_t)DEVI_SID_NODEID, &xdip);
		(void) ndi_prop_update_int_array(DDI_DEV_T_NONE, xdip, "reg", (int *)&asy_regs[i], 3);
		(void) ndi_prop_update_int(DDI_DEV_T_NONE, xdip, "interrupts", asy_intrs[i]);
		(void) ndi_devi_bind_driver(xdip, 0);
	}

	/* i8042 node */
	ndi_devi_alloc_sleep(isa_dip, "i8042", (pnode_t)DEVI_SID_NODEID, &xdip);
	(void) ndi_prop_update_int_array(DDI_DEV_T_NONE, xdip, "reg", (int *)i8042_regs, 6);
	(void) ndi_prop_update_int_array(DDI_DEV_T_NONE, xdip, "interrupts", (int *)i8042_intrs, 2);
	(void) ndi_prop_update_string(DDI_DEV_T_NONE, xdip, "unit-address", "1,60");
	(void) ndi_devi_bind_driver(xdip, 0);

	add_known_used_resources();

	ndi_devi_exit(isa_dip, circ);

	isa_create_cells_props(isa_dip);
}
