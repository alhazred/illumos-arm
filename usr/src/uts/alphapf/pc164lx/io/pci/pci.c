/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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

/*
 * PCI nexus driver interface
 */

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/ddifm.h>
#include <sys/ndifm.h>
#include <sys/ontrap.h>
#include <sys/ddi_impldefs.h>
#include <sys/ddi_subrdefs.h>
#include <sys/spl.h>
#include <sys/pci/pci_obj.h>
#include <sys/hotplug/pci/pcihp.h>
#include <sys/sysmacros.h>
#include <sys/pci/pci_tools_ext.h>
#include <sys/file.h>
#include <sys/mach_intr.h>
#include <sys/promif.h>
#include <sys/avintr.h>
#include <sys/pci/pci_pyxis.h>

/*
 * function prototypes for dev ops routines:
 */
static int pci_attach(dev_info_t *dip, ddi_attach_cmd_t cmd);
static int pci_detach(dev_info_t *dip, ddi_detach_cmd_t cmd);
static int pci_info(dev_info_t *dip, ddi_info_cmd_t infocmd,
	void *arg, void **result);
static void pci_bus_enter(dev_info_t *dip, ddi_acc_handle_t handle);
static void pci_bus_exit(dev_info_t *dip, ddi_acc_handle_t handle);
static int pci_open(dev_t *devp, int flags, int otyp, cred_t *credp);
static int pci_close(dev_t dev, int flags, int otyp, cred_t *credp);
static int pci_devctl_ioctl(dev_info_t *dip, int cmd, intptr_t arg, int mode,
						cred_t *credp, int *rvalp);
static int pci_ioctl(dev_t dev, int cmd, intptr_t arg, int mode,
						cred_t *credp, int *rvalp);
static int pci_prop_op(dev_t dev, dev_info_t *dip, ddi_prop_op_t prop_op,
    int flags, char *name, caddr_t valuep, int *lengthp);
static int
pci_intr_ops(dev_info_t *dip, dev_info_t *rdip, ddi_intr_op_t intr_op,
	ddi_intr_handle_impl_t *handle, void *result);
static int
pci_map(dev_info_t *dip, dev_info_t *rdip, ddi_map_req_t *mp,
	off_t offset, off_t len, caddr_t *addrp);
void
pci_fm_create(pci_t *pci_p);

static int pci_fm_init_child(dev_info_t *dip, dev_info_t *tdip, int cap,
    ddi_iblock_cookie_t *ibc);

static struct cb_ops pci_cb_ops = {
	pci_open,			/* open */
	pci_close,			/* close */
	nodev,				/* strategy */
	nodev,				/* print */
	nodev,				/* dump */
	nodev,				/* read */
	nodev,				/* write */
	pci_ioctl,			/* ioctl */
	nodev,				/* devmap */
	nodev,				/* mmap */
	nodev,				/* segmap */
	nochpoll,			/* poll */
	pci_prop_op,			/* cb_prop_op */
	NULL,				/* streamtab */
	D_NEW | D_MP | D_HOTPLUG,	/* Driver compatibility flag */
	CB_REV,				/* rev */
	nodev,				/* int (*cb_aread)() */
	nodev				/* int (*cb_awrite)() */
};

/*
 * bus ops and dev ops structures:
 */
static struct bus_ops pci_bus_ops = {
	BUSO_REV,
	pci_map,
	0,
	0,
	0,
	i_ddi_map_fault,
	pci_dma_setup,
	pci_dma_allochdl,
	pci_dma_freehdl,
	pci_dma_bindhdl,
	pci_dma_unbindhdl,
	pci_dma_sync,
	pci_dma_win,
	pci_dma_ctlops,
	pci_ctlops,
	ddi_bus_prop_op,
	ndi_busop_get_eventcookie,	/* (*bus_get_eventcookie)(); */
	ndi_busop_add_eventcall,	/* (*bus_add_eventcall)(); */
	ndi_busop_remove_eventcall,	/* (*bus_remove_eventcall)(); */
	ndi_post_event,			/* (*bus_post_event)(); */
	NULL,				/* (*bus_intr_ctl)(); */
	NULL,				/* (*bus_config)(); */
	NULL,				/* (*bus_unconfig)(); */
	pci_fm_init_child,		/* (*bus_fm_init)(); */
	NULL,				/* (*bus_fm_fini)(); */
	pci_bus_enter,			/* (*bus_fm_access_enter)(); */
	pci_bus_exit,			/* (*bus_fm_access_fini)(); */
	NULL,				/* (*bus_power)(); */
	pci_intr_ops			/* (*bus_intr_op)(); */
};

static struct dev_ops pci_ops = {
	DEVO_REV,
	0,
	pci_info,
	nulldev,
	0,
	pci_attach,
	pci_detach,
	nodev,
	&pci_cb_ops,
	&pci_bus_ops,
	0,
	ddi_quiesce_not_supported,	/* devo_quiesce */
};

/*
 * module definitions:
 */
#include <sys/modctl.h>
extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops, 			/* Type of module - driver */
	"DEC Alpha Host to PCI nexus driver",	/* Name of module. */
	&pci_ops,				/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

/*
 * driver global data:
 */
void *per_pci_state;		/* per-pbm soft state pointer */
void *per_pci_common_state;	/* per-psycho soft state pointer */
kmutex_t pci_global_mutex;	/* attach/detach common struct lock */
int
_init(void)
{
	int e;

	/*
	 * Initialize per-pci bus soft state pointer.
	 */
	e = ddi_soft_state_init(&per_pci_state, sizeof(pci_t), 1);
	if (e != 0)
		return (e);

	/*
	 * Initialize per-psycho soft state pointer.
	 */
	e = ddi_soft_state_init(
	    &per_pci_common_state, sizeof(pci_common_t), 1);
	if (e != 0) {
		ddi_soft_state_fini(&per_pci_state);
		return (e);
	}

	/*
	 * Initialize global mutexes.
	 */
	mutex_init(&pci_global_mutex, NULL, MUTEX_DRIVER, NULL);

	/*
	 * Install the module.
	 */
	e = mod_install(&modlinkage);
	if (e != 0) {
		ddi_soft_state_fini(&per_pci_state);
		ddi_soft_state_fini(&per_pci_common_state);
		mutex_destroy(&pci_global_mutex);
	}
	return (e);
}

int
_fini(void)
{
	int e;

	/*
	 * Remove the module.
	 */
	e = mod_remove(&modlinkage);
	if (e != 0)
		return (e);

	/*
	 * Free the per-pci and per-psycho soft state info and destroy
	 * mutex for per-psycho soft state.
	 */
	ddi_soft_state_fini(&per_pci_state);
	ddi_soft_state_fini(&per_pci_common_state);
	mutex_destroy(&pci_global_mutex);
	return (e);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 * Lock accesses to the pci bus, to be able to protect against bus errors.
 */
static void
pci_bus_enter(dev_info_t *dip, ddi_acc_handle_t handle)
{
}

/*
 * Unlock access to bus and clear errors before exiting.
 */
static void
pci_bus_exit(dev_info_t *dip, ddi_acc_handle_t handle)
{
}

static int
pci_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	int	instance = PCIHP_AP_MINOR_NUM_TO_INSTANCE(getminor((dev_t)arg));
	pci_t	*pci_p = get_pci_soft_state(instance);

	/* non-hotplug or not attached */
	switch (infocmd) {
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)(uintptr_t)instance;
		return (DDI_SUCCESS);

	case DDI_INFO_DEVT2DEVINFO:
		if (pci_p == NULL)
			return (DDI_FAILURE);
		*result = (void *)pci_p->pci_dip;
		return (DDI_SUCCESS);

	default:
		return (DDI_FAILURE);
	}
}

static int
pci_fm_init_child(dev_info_t *dip, dev_info_t *tdip, int cap,
    ddi_iblock_cookie_t *ibc)
{
	pci_t *pci_p = get_pci_soft_state(ddi_get_instance(dip));

	ASSERT(ibc != NULL);
	*ibc = pci_p->pci_fm_ibc;

	return (pci_p->pci_fm_cap);
}

static int
pci_dma_alloc_constructor(void *buf, void *arg, int flag)
{
	memset(buf, 0, sizeof(pci_dma_hdl_t));
	return 0;
}

/* device driver entry points */
/*
 * attach entry point:
 */
static int
pci_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	pci_t *pci_p;			/* per bus state pointer */
	int instance = ddi_get_instance(dip);
	char name[80];
	snprintf(name, sizeof(name), "pci_dma_cache_%d", instance);

	switch (cmd) {
	case DDI_ATTACH:
		if (alloc_pci_soft_state(instance) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "%s%d: can't allocate pci state",
			    ddi_driver_name(dip), instance);
			goto err_bad_pci_softstate;
		}
		pci_p = get_pci_soft_state(instance);
		pci_p->pci_dip = dip;
		mutex_init(&pci_p->pci_mutex, NULL, MUTEX_DRIVER, NULL);
		pci_p->pci_soft_state = PCI_SOFT_STATE_CLOSED;

		if (get_pci_properties(pci_p, dip) == DDI_FAILURE)
			goto err_bad_pci_prop;

		if (map_pci_registers(pci_p, dip) == DDI_FAILURE)
			goto err_bad_reg_prop;

		if ((PYXIS_PCI_WnBASE(PYXIS_PCI_DIRECT_MAP_INDEX) != (PYXIS_PCI_DIRECT_MAP_BASE | 1)) ||
			    (PYXIS_PCI_WnMASK(PYXIS_PCI_DIRECT_MAP_INDEX) != (PYXIS_PCI_DIRECT_MAP_SIZE - 0x00100000)) ||
			    (PYXIS_PCI_TnBASE(PYXIS_PCI_DIRECT_MAP_INDEX) != (PYXIS_PCI_DIRECT_MAP_TRANS_BASE >> 2))) {
			PYXIS_PCI_WnBASE(PYXIS_PCI_DIRECT_MAP_INDEX) = 0;
			PYXIS_PCI_WnBASE(PYXIS_PCI_DIRECT_MAP_INDEX);
			PYXIS_PCI_WnMASK(PYXIS_PCI_DIRECT_MAP_INDEX) = (PYXIS_PCI_DIRECT_MAP_SIZE - 0x00100000);
			PYXIS_PCI_TnBASE(PYXIS_PCI_DIRECT_MAP_INDEX) = (PYXIS_PCI_DIRECT_MAP_TRANS_BASE << 2);
			PYXIS_PCI_WnBASE(PYXIS_PCI_DIRECT_MAP_INDEX) = (PYXIS_PCI_DIRECT_MAP_BASE | 1);
		}

		pci_fm_create(pci_p);

		if (ddi_create_minor_node(dip, "devctl", S_IFCHR,
			    PCIHP_AP_MINOR_NUM(instance, PCIHP_DEVCTL_MINOR),
			    DDI_NT_NEXUS, 0) != DDI_SUCCESS)
			goto err_bad_devctl_node;

		if (pcitool_init(dip) != DDI_SUCCESS)
			goto err_bad_pcitool_nodes;

		ddi_report_dev(dip);
		pci_p->pci_state = PCI_ATTACHED;
		break;

err_bad_pcitool_nodes:
		ddi_remove_minor_node(dip, "devctl");
err_bad_devctl_node:
		unmap_pci_registers(pci_p);
err_bad_reg_prop:
		free_pci_properties(pci_p);
err_bad_pci_prop:
		mutex_destroy(&pci_p->pci_mutex);
		free_pci_soft_state(instance);
err_bad_pci_softstate:
		return (DDI_FAILURE);

	case DDI_RESUME:
		break;

	default:
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}

/*
 * detach entry point:
 */
static int
pci_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int instance = ddi_get_instance(dip);
	pci_t *pci_p = get_pci_soft_state(instance);

	if (pci_p->pci_state != PCI_ATTACHED) {
		return (DDI_FAILURE);
	}

	mutex_enter(&pci_p->pci_mutex);

	switch (cmd) {
	case DDI_DETACH:

		pcitool_uninit(dip);

		free_pci_properties(pci_p);
		unmap_pci_registers(pci_p);
		mutex_exit(&pci_p->pci_mutex);
		mutex_destroy(&pci_p->pci_mutex);
		free_pci_soft_state(instance);

		return (DDI_SUCCESS);

	case DDI_SUSPEND:
		pci_p->pci_state = PCI_SUSPENDED;

		mutex_exit(&pci_p->pci_mutex);
		return (DDI_SUCCESS);

	default:
		mutex_exit(&pci_p->pci_mutex);
		return (DDI_FAILURE);
	}
}

static int
pci_open(dev_t *devp, int flags, int otyp, cred_t *credp)
{
	pci_t *pci_p;
	int rval;
	uint_t orig_pci_soft_state;

	/*
	 * Make sure the open is for the right file type.
	 */
	if (otyp != OTYP_CHR)
		return (EINVAL);

	/*
	 * Get the soft state structure for the device.
	 */
	pci_p = DEV_TO_SOFTSTATE(*devp);
	if (pci_p == NULL)
		return (ENXIO);

	/*
	 * Handle the open by tracking the device state.
	 */
	mutex_enter(&pci_p->pci_mutex);
	orig_pci_soft_state = pci_p->pci_soft_state;
	if (flags & FEXCL) {
		if (pci_p->pci_soft_state != PCI_SOFT_STATE_CLOSED) {
			mutex_exit(&pci_p->pci_mutex);
			return (EBUSY);
		}
		pci_p->pci_soft_state = PCI_SOFT_STATE_OPEN_EXCL;
	} else {
		if (pci_p->pci_soft_state == PCI_SOFT_STATE_OPEN_EXCL) {
			mutex_exit(&pci_p->pci_mutex);
			return (EBUSY);
		}
		pci_p->pci_soft_state = PCI_SOFT_STATE_OPEN;
	}

	mutex_exit(&pci_p->pci_mutex);

	return (0);
}

static int
pci_close(dev_t dev, int flags, int otyp, cred_t *credp)
{
	pci_t *pci_p;
	int rval;

	if (otyp != OTYP_CHR)
		return (EINVAL);

	pci_p = DEV_TO_SOFTSTATE(dev);
	if (pci_p == NULL)
		return (ENXIO);

	mutex_enter(&pci_p->pci_mutex);
	pci_p->pci_soft_state = PCI_SOFT_STATE_CLOSED;
	mutex_exit(&pci_p->pci_mutex);
	return (0);
}

static int
pci_devctl_ioctl(dev_info_t *dip, int cmd, intptr_t arg, int mode,
    cred_t *credp, int *rvalp)
{
	int rv = 0;
	struct devctl_iocdata *dcp;
	uint_t bus_state;

	switch (cmd) {
	case DEVCTL_DEVICE_GETSTATE:
	case DEVCTL_DEVICE_ONLINE:
	case DEVCTL_DEVICE_OFFLINE:
	case DEVCTL_BUS_GETSTATE:
		return (ndi_devctl_ioctl(dip, cmd, arg, mode, 0));
	}

	if (ndi_dc_allochdl((void *)arg, &dcp) != NDI_SUCCESS)
		return (EFAULT);

	switch (cmd) {
	case DEVCTL_DEVICE_RESET:
		rv = ENOTSUP;
		break;

	case DEVCTL_BUS_QUIESCE:
		if (ndi_get_bus_state(dip, &bus_state) == NDI_SUCCESS)
			if (bus_state == BUS_QUIESCED)
				break;
		ndi_set_bus_state(dip, BUS_QUIESCED);
		break;

	case DEVCTL_BUS_UNQUIESCE:
		if (ndi_get_bus_state(dip, &bus_state) == NDI_SUCCESS)
			if (bus_state == BUS_ACTIVE)
				break;
		ndi_set_bus_state(dip, BUS_ACTIVE);
		break;

	case DEVCTL_BUS_RESET:
		rv = ENOTSUP;
		break;

	case DEVCTL_BUS_RESETALL:
		rv = ENOTSUP;
		break;

	default:
		rv = ENOTTY;
	}

	ndi_dc_freehdl(dcp);
	return (rv);
}

static int
pci_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *credp, int *rvalp)
{
	pci_t *pci_p;
	dev_info_t *dip;
	minor_t minor = getminor(dev);
	int rv = ENOTTY;

	pci_p = DEV_TO_SOFTSTATE(dev);
	if (pci_p == NULL)
		return (ENXIO);

	dip = pci_p->pci_dip;

	switch (PCIHP_AP_MINOR_NUM_TO_PCI_DEVNUM(minor)) {
	case PCI_TOOL_REG_MINOR_NUM:
		switch (cmd) {
		case PCITOOL_DEVICE_SET_REG:
		case PCITOOL_DEVICE_GET_REG:
			rv = pcitool_dev_reg_ops(dev,
			    (void *)arg, cmd, mode);
			break;

		case PCITOOL_NEXUS_SET_REG:
		case PCITOOL_NEXUS_GET_REG:
			rv = pcitool_bus_reg_ops(dev,
			    (void *)arg, cmd, mode);
			break;
		}
		break;

	case PCI_TOOL_INTR_MINOR_NUM:
		switch (cmd) {
		case PCITOOL_DEVICE_SET_INTR:
		case PCITOOL_DEVICE_GET_INTR:
		case PCITOOL_SYSTEM_INTR_INFO:
			rv = pcitool_intr_admn(dev, (void *)arg, cmd, mode);
			break;
		}
		break;

	default:
		rv = pci_devctl_ioctl(dip, cmd, arg, mode, credp, rvalp);
		break;
	}

	return (rv);
}

static int
pci_prop_op(dev_t dev, dev_info_t *dip, ddi_prop_op_t prop_op,
    int flags, char *name, caddr_t valuep, int *lengthp)
{
	return (ddi_prop_op(dev, dip, prop_op, flags, name, valuep, lengthp));
}

static int
pci_intr_ops(dev_info_t *dip, dev_info_t *rdip, ddi_intr_op_t intr_op,
    ddi_intr_handle_impl_t *hdlp, void *result)
{
	pci_t		*pci_p = get_pci_soft_state(ddi_get_instance(dip));
	int		ret = DDI_SUCCESS;
	ddi_intrspec_t		isp;
	struct intrspec		*ispec;
	int			pci_status = 0;
	int			irq;

	switch (intr_op) {
	case DDI_INTROP_GETCAP:
		pci_intx_get_cap(rdip, (int *)result);
		break;
	case DDI_INTROP_SETCAP:
		ret = DDI_ENOTSUP;
		break;
	case DDI_INTROP_ALLOC:
		*(int *)result = hdlp->ih_scratch1;
		break;
	case DDI_INTROP_FREE:
		break;
	case DDI_INTROP_GETPRI:
		*(int *)result = hdlp->ih_pri ?
		    hdlp->ih_pri : pci_class_to_pil(rdip);
		break;
	case DDI_INTROP_SETPRI:
		break;
	case DDI_INTROP_ADDISR:
		isp = pci_intx_get_ispec(dip, rdip, (int)hdlp->ih_inum);
		ispec = (struct intrspec *)isp;
		if (ispec) {
			ispec->intrspec_func = hdlp->ih_cb_func;
		} else {
			ret = DDI_FAILURE;
		}
		break;
	case DDI_INTROP_REMISR:
		isp = pci_intx_get_ispec(dip, rdip, (int)hdlp->ih_inum);
		ispec = (struct intrspec *)isp;
		if (ispec) {
			ispec->intrspec_func = (uint_t (*)()) 0;
		} else {
			ret = DDI_FAILURE;
		}
		break;
	case DDI_INTROP_ENABLE:
		isp = pci_intx_get_ispec(dip, rdip, (int)hdlp->ih_inum);
		ispec = (struct intrspec *)isp;
		if (ispec) {
			irq = ispec->intrspec_vec + 16;
			hdlp->ih_vector = irq;
			if (!add_avintr((void *)hdlp, hdlp->ih_pri, hdlp->ih_cb_func,
				    DEVI(rdip)->devi_name, irq, hdlp->ih_cb_arg1,
				    hdlp->ih_cb_arg2, NULL, rdip))
				return (DDI_FAILURE);
		} else {
			ret = DDI_FAILURE;
		}
		break;
	case DDI_INTROP_DISABLE:
		isp = pci_intx_get_ispec(dip, rdip, (int)hdlp->ih_inum);
		ispec = (struct intrspec *)isp;
		if (ispec) {
			irq = ispec->intrspec_vec + 16;
			rem_avintr((void *)hdlp, hdlp->ih_pri, hdlp->ih_cb_func, irq);
		} else {
			ret = DDI_FAILURE;
		}
		break;
	case DDI_INTROP_GETPENDING:
		ret = pci_intx_get_pending(rdip, &pci_status);
		if (ret == DDI_SUCCESS) {
			*(int *)result = pci_status;
		}
		break;
	case DDI_INTROP_GETTARGET:
		*(int *)result = 0;
		break;
	case DDI_INTROP_SETTARGET:
		ret = DDI_ENOTSUP;
		break;
	case DDI_INTROP_SETMASK:
		ret = pci_intx_set_mask(rdip);
		break;
	case DDI_INTROP_CLRMASK:
		ret = pci_intx_clr_mask(rdip);
		break;
	case DDI_INTROP_NINTRS:
	case DDI_INTROP_NAVAIL:
		*(int *)result = i_ddi_get_intx_nintrs(rdip);
		break;
	case DDI_INTROP_SUPPORTED_TYPES:
		/* PCI nexus driver supports only fixed interrupts */
		*(int *)result = i_ddi_get_intx_nintrs(rdip) ?
		    DDI_INTR_TYPE_FIXED : 0;
		break;
	default:
		ret = DDI_ENOTSUP;
		break;
	}

	return ret;
}

static int
pci_map(dev_info_t *dip, dev_info_t *rdip, ddi_map_req_t *mp,
	off_t off, off_t len, caddr_t *addrp)
{
	pci_t *pci_p = get_pci_soft_state(ddi_get_instance(dip));
	struct regspec p_regspec;
	ddi_map_req_t p_mapreq;
	int reglen, rval, r_no;
	pci_regspec_t reloc_reg, *rp = &reloc_reg;

	if (mp->map_flags & DDI_MF_USER_MAPPING)
		return (DDI_ME_UNIMPLEMENTED);

	switch (mp->map_type) {
	case DDI_MT_REGSPEC:
		reloc_reg = *(pci_regspec_t *)mp->map_obj.rp;	/* dup whole */
		break;

	case DDI_MT_RNUMBER:
		r_no = mp->map_obj.rnumber;
		if (ddi_getlongprop(DDI_DEV_T_ANY, rdip, DDI_PROP_DONTPASS,
		    "reg", (caddr_t)&rp, &reglen) != DDI_SUCCESS)
				return (DDI_ME_RNUMBER_RANGE);

		if (r_no < 0 || r_no >= reglen / sizeof (pci_regspec_t)) {
			kmem_free(rp, reglen);
			return (DDI_ME_RNUMBER_RANGE);
		}
		rp += r_no;
		break;

	default:
		return (DDI_ME_INVAL);
	}

	if (rval = pci_reloc_reg(dip, rdip, pci_p, rp))
		goto done;

	if (len)
		rp->pci_size_low = len;

	if (rval = pci_xlate_reg(pci_p, rp, &p_regspec))
		goto done;

	p_mapreq = *mp;
	p_mapreq.map_type = DDI_MT_REGSPEC;
	p_mapreq.map_obj.rp = &p_regspec;

	rval = ddi_map(dip, &p_mapreq, off, 0, addrp);

	if (rval == DDI_SUCCESS) {
		ddi_acc_impl_t *hdlp;
		if (DDI_FM_ACC_ERR_CAP(pci_p->pci_fm_cap) &&
		    DDI_FM_ACC_ERR_CAP(ddi_fm_capable(rdip)) &&
		    mp->map_handlep->ah_acc.devacc_attr_access != DDI_DEFAULT_ACC)
			pci_fm_acc_setup(mp, rdip);
		hdlp = (ddi_acc_impl_t *)(mp->map_handlep)->ah_platform_private;
		hdlp->ahi_peekpoke_mutexp = &pci_p->pci_peek_poke_mutex;
	}

done:
	if (mp->map_type == DDI_MT_RNUMBER)
		kmem_free(rp - r_no, reglen);

	return (rval);
}

/*
 * bus dma map entry point
 * return value:
 *	DDI_DMA_PARTIAL_MAP	 1
 *	DDI_DMA_MAPOK		 0
 *	DDI_DMA_MAPPED		 0
 *	DDI_DMA_NORESOURCES	-1
 *	DDI_DMA_NOMAPPING	-2
 *	DDI_DMA_TOOBIG		-3
 */
int
pci_dma_setup(dev_info_t *dip, dev_info_t *rdip, ddi_dma_req_t *dmareq,
	ddi_dma_handle_t *handlep)
{
	pci_t *pci_p = get_pci_soft_state(ddi_get_instance(dip));
	ddi_dma_impl_t *mp;
	int ret;

	if (!(mp = pci_dma_lmts2hdl(dip, rdip, dmareq)))
		return (DDI_DMA_NORESOURCES);
	if (mp == (ddi_dma_impl_t *)DDI_DMA_NOMAPPING)
		return (DDI_DMA_NOMAPPING);
	if (ret = pci_dma_type(pci_p, dmareq, mp))
		goto freehandle;
	if (ret = pci_dma_pfn(pci_p, dmareq, mp))
		goto freehandle;

	if ((ret = pci_dma_physwin(pci_p, dmareq, mp)) || !handlep)
		goto freehandle;

	*handlep = (ddi_dma_handle_t)mp;
	mp->dmai_flags |= (DMAI_FLAGS_INUSE | DMAI_FLAGS_MAPPED);

	return ((mp->dmai_nwin == 1) ? DDI_DMA_MAPPED : DDI_DMA_PARTIAL_MAP);
freehandle:
	if (ret == DDI_DMA_NORESOURCES)
		pci_dma_freemp(mp); /* don't run_callback() */
	else
		(void) pci_dma_freehdl(dip, rdip, (ddi_dma_handle_t)mp);
	return (ret);
}

/*
 * bus dma alloc handle entry point:
 */
int
pci_dma_allochdl(dev_info_t *dip, dev_info_t *rdip, ddi_dma_attr_t *attrp,
	int (*waitfp)(caddr_t), caddr_t arg, ddi_dma_handle_t *handlep)
{
	pci_t *pci_p = get_pci_soft_state(ddi_get_instance(dip));
	ddi_dma_impl_t *mp;
	int rval;

	if (attrp->dma_attr_version != DMA_ATTR_V0)
		return (DDI_DMA_BADATTR);

	if (!(mp = pci_dma_allocmp(dip, rdip, waitfp, arg)))
		return (DDI_DMA_NORESOURCES);

	/*
	 * Save requestor's information
	 */
	mp->dmai_attr	= *attrp; /* whole object - augmented later  */
	*DEV_ATTR(mp)	= *attrp; /* whole object - device orig attr */

	/* check and convert dma attributes to handle parameters */
	if (rval = pci_dma_attr2hdl(pci_p, mp)) {
		pci_dma_freehdl(dip, rdip, (ddi_dma_handle_t)mp);
		*handlep = NULL;
		return (rval);
	}
	*handlep = (ddi_dma_handle_t)mp;
	return (DDI_SUCCESS);
}


/*
 * bus dma free handle entry point:
 */
/*ARGSUSED*/
int
pci_dma_freehdl(dev_info_t *dip, dev_info_t *rdip, ddi_dma_handle_t handle)
{
	pci_dma_freemp((ddi_dma_impl_t *)handle);

	if (pci_kmem_clid) {
		ddi_run_callback(&pci_kmem_clid);
	}
	return (DDI_SUCCESS);
}


/*
 * bus dma bind handle entry point:
 */
int
pci_dma_bindhdl(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_handle_t handle, ddi_dma_req_t *dmareq,
	ddi_dma_cookie_t *cookiep, uint_t *ccountp)
{
	pci_t *pci_p = get_pci_soft_state(ddi_get_instance(dip));
	ddi_dma_impl_t *mp = (ddi_dma_impl_t *)handle;
	int ret;

	if (mp->dmai_flags & DMAI_FLAGS_INUSE)
		return (DDI_DMA_INUSE);

	mp->dmai_flags |= DMAI_FLAGS_INUSE;

	if (ret = pci_dma_type(pci_p, dmareq, mp))
		goto err;

	if (ret = pci_dma_pfn(pci_p, dmareq, mp))
		goto err;

	if (ret = pci_dma_physwin(pci_p, dmareq, mp))
		goto map_err;

	*ccountp = WINLST(mp)->win_ncookies;
	*cookiep = *(ddi_dma_cookie_t *)(WINLST(mp) + 1); /* wholeobj */

	if (mp->dmai_attr.dma_attr_flags & DDI_DMA_FLAGERR)
		mp->dmai_error.err_cf = impl_dma_check;

	mp->dmai_flags |= DMAI_FLAGS_MAPPED;
	return (mp->dmai_nwin == 1 ? DDI_DMA_MAPPED : DDI_DMA_PARTIAL_MAP);
map_err:
	pci_dma_freepfn(mp);
err:
	return (ret);
}

/*
 * bus dma unbind handle entry point:
 */
/*ARGSUSED*/
int
pci_dma_unbindhdl(dev_info_t *dip, dev_info_t *rdip, ddi_dma_handle_t handle)
{
	ddi_dma_impl_t *mp = (ddi_dma_impl_t *)handle;
	pci_t *pci_p = get_pci_soft_state(ddi_get_instance(dip));

	if ((mp->dmai_flags & DMAI_FLAGS_INUSE) == 0) {
		return (DDI_FAILURE);
	}

	mp->dmai_flags &= ~DMAI_FLAGS_MAPPED;

	pci_dma_freewin(mp);
	if (pci_kmem_clid) {
		ddi_run_callback(&pci_kmem_clid);
	}
	SYNC_BUF_PA(mp) = 0;

	mp->dmai_error.err_cf = NULL;
	mp->dmai_ncookies = 0;
	mp->dmai_curcookie = 0;

	return (DDI_SUCCESS);
}


/*
 * bus dma win entry point:
 */
int
pci_dma_win(dev_info_t *dip, dev_info_t *rdip,
	ddi_dma_handle_t handle, uint_t win, off_t *offp,
	size_t *lenp, ddi_dma_cookie_t *cookiep, uint_t *ccountp)
{
	ddi_dma_impl_t *mp = (ddi_dma_impl_t *)handle;
	if (win >= mp->dmai_nwin) {
		return (DDI_FAILURE);
	}

	ddi_dma_cookie_t *ck_p;
	pci_dma_win_t *win_p = mp->dmai_winlst;

	for (int i = 0; i < win; win_p = win_p->win_next, i++)
		;
	ck_p = (ddi_dma_cookie_t *)(win_p + 1);
	*cookiep = *ck_p;
	mp->dmai_offset = win_p->win_offset;
	mp->dmai_size   = win_p->win_size;
	mp->dmai_cookie = ck_p + 1;
	win_p->win_curseg = 0;
	if (ccountp)
		*ccountp = win_p->win_ncookies;

	if (offp)
		*offp = (off_t)mp->dmai_offset;
	if (lenp)
		*lenp = mp->dmai_size;

	mp->dmai_ncookies = win_p->win_ncookies;
	mp->dmai_curcookie = 1;

	return (DDI_SUCCESS);
}

/*
 * bus dma control entry point:
 */
int
pci_dma_ctlops(dev_info_t *dip, dev_info_t *rdip, ddi_dma_handle_t handle,
	enum ddi_dma_ctlops cmd, off_t *offp, size_t *lenp, caddr_t *objp,
	uint_t cache_flags)
{
	ddi_dma_impl_t *mp = (ddi_dma_impl_t *)handle;

	switch (cmd) {
	case DDI_DMA_FREE:
		(void) pci_dma_unbindhdl(dip, rdip, handle);
		(void) pci_dma_freehdl(dip, rdip, handle);
		return (DDI_SUCCESS);
	default:
		break;
	}

	return (pci_dma_ctl(dip, rdip, mp, cmd, offp, lenp, objp, cache_flags));
}

int
pci_ctlops(dev_info_t *dip, dev_info_t *rdip,
	ddi_ctl_enum_t op, void *arg, void *result)
{
	pci_t *pci_p = get_pci_soft_state(ddi_get_instance(dip));
	pci_regspec_t *rp;
	int reglen;

	switch (op) {
	case DDI_CTLOPS_REPORTDEV:
		return (report_dev(rdip));

	case DDI_CTLOPS_DVMAPAGESIZE:
		*((ulong_t *)result) = IOMMU_PAGE_SIZE;
		return (DDI_SUCCESS);

	case DDI_CTLOPS_POWER:
		return (DDI_SUCCESS);

	case DDI_CTLOPS_NREGS:
		if (ddi_getlongprop(DDI_DEV_T_ANY, rdip, DDI_PROP_DONTPASS, "reg", (caddr_t)&rp, &reglen) != DDI_SUCCESS)
			return (DDI_FAILURE);

		*(int *)result = reglen / sizeof(pci_regspec_t);
		kmem_free(rp, reglen);
		return DDI_SUCCESS;

	case DDI_CTLOPS_REGSIZE:
		if (ddi_getlongprop(DDI_DEV_T_ANY, rdip, DDI_PROP_DONTPASS, "reg", (caddr_t)&rp, &reglen) != DDI_SUCCESS)
			return (DDI_FAILURE);
		if (*(int *)arg >= reglen / sizeof(pci_regspec_t)) {
			kmem_free(rp, reglen);
			return (DDI_FAILURE);
		}
		*(off_t *)result = ((uint64_t)(rp[*(int *)arg].pci_size_hi) << 32) | rp[*(int *)arg].pci_size_low;
		kmem_free(rp, reglen);
		return DDI_SUCCESS;
	default:
		break;
	}

	/*
	 * Now pass the request up to our parent.
	 */
	return (ddi_ctlops(dip, rdip, op, arg, result));
}
