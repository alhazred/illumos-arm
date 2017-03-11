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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/stat.h>
#include <sys/sunddi.h>
#include <sys/param.h>

#include <sys/sysmacros.h>
#include <sys/machsystm.h>
#include <sys/promif.h>
#include <sys/cpuvar.h>

#include <sys/pci/pci_obj.h>
#include <sys/hotplug/pci/pcihp.h>

#include <sys/pci_tools.h>
#include <sys/pci/pci_tools_ext.h>
#include <sys/promif.h>

/*
 * Number of interrupts supported per PCI bus.
 */
#define	PCI_MAX_INO		0x3f

/*
 * PCI Space definitions.
 */
#define	PCI_CONFIG_RANGE_BANK	(PCI_REG_ADDR_G(PCI_ADDR_CONFIG))
#define	PCI_IO_RANGE_BANK	(PCI_REG_ADDR_G(PCI_ADDR_IO))
#define	PCI_MEM_RANGE_BANK	(PCI_REG_ADDR_G(PCI_ADDR_MEM32))
#define	PCI_MEM64_RANGE_BANK	(PCI_REG_ADDR_G(PCI_ADDR_MEM64))

/*
 * Extract 64 bit parent or size values from 32 bit cells of
 * pci_ranges_t property.
 *
 * Only bits 42-32 are relevant in parent_high.
 */
#define	PCI_GET_RANGE_PROP(ranges, bank) \
	((((uint64_t)(ranges[bank].parent_high & 0x7ff)) << 32) | \
	ranges[bank].parent_low)

#define	PCI_GET_RANGE_PROP_SIZE(ranges, bank) \
	((((uint64_t)(ranges[bank].size_high)) << 32) | \
	ranges[bank].size_low)

#define	PCI_BAR_OFFSET(x)	(pci_bars[x.barnum])

/* Big and little endian as boolean values. */
#define	BE B_TRUE
#define	LE B_FALSE

#define	SUCCESS	0

/* Mechanism for getting offsets of smaller datatypes aligned in 64 bit long */
typedef union {
	uint64_t u64;
	uint32_t u32;
	uint16_t u16;
	uint8_t u8;
} peek_poke_value_t;

/*
 * Offsets of BARS in config space.  First entry of 0 means config space.
 * Entries here correlate to pcitool_bars_t enumerated type.
 */
static uint8_t pci_bars[] = {
	0x0,
	PCI_CONF_BASE0,
	PCI_CONF_BASE1,
	PCI_CONF_BASE2,
	PCI_CONF_BASE3,
	PCI_CONF_BASE4,
	PCI_CONF_BASE5,
	PCI_CONF_ROM
};

/*LINTLIBRARY*/

static int pcitool_access(pci_t *pci_p, uint64_t phys_addr, uint64_t max_addr,
    uint64_t *data, uint8_t size, boolean_t write, boolean_t endian,
    uint32_t *pcitool_status);
static int pcitool_validate_barnum_bdf(pcitool_reg_t *prg);
static int pcitool_get_bar(pci_t *pci_p, pcitool_reg_t *prg,
    uint64_t config_base_addr, uint64_t config_max_addr, uint64_t *bar,
    boolean_t *is_io_space);
static int pcitool_config_request(pci_t *pci_p, pcitool_reg_t *prg,
    uint64_t base_addr, uint64_t max_addr, uint8_t size, boolean_t write_flag);
static int pcitool_intr_get_max_ino(uint32_t *arg, int mode);
static int pcitool_get_intr(dev_info_t *dip, void *arg, int mode, pci_t *pci_p);
static int pcitool_set_intr(dev_info_t *dip, void *arg, int mode, pci_t *pci_p);

/*ARGSUSED*/
static int
pcitool_intr_info(dev_info_t *dip, void *arg, int mode)
{
	pcitool_intr_info_t intr_info;
	int rval = SUCCESS;

	/* If we need user_version, and to ret same user version as passed in */
	if (ddi_copyin(arg, &intr_info, sizeof (pcitool_intr_info_t), mode) !=
	    DDI_SUCCESS) {
		return (EFAULT);
	}

	if (intr_info.flags & PCITOOL_INTR_FLAG_GET_MSI)
		return (ENOTSUP);

	intr_info.ctlr_version = 0;	/* XXX how to get real version? */
	intr_info.ctlr_type = PCITOOL_CTLR_TYPE_RISC;
	intr_info.num_intr = PCI_MAX_INO;

	intr_info.drvr_version = PCITOOL_VERSION;
	if (ddi_copyout(&intr_info, arg, sizeof (pcitool_intr_info_t), mode) !=
	    DDI_SUCCESS) {
		rval = EFAULT;
	}

	return (rval);
}


/*
 * Get interrupt information for a given ino.
 * Returns info only for inos mapped to devices.
 *
 * Returned info is valid only when iget.num_devs is returned > 0.
 * If ino is not enabled or is not mapped to a device, num_devs will be = 0.
 */
/*ARGSUSED*/
static int
pcitool_get_intr(dev_info_t *dip, void *arg, int mode, pci_t *pci_p)
{
	/* Array part isn't used here, but oh well... */
	pcitool_intr_get_t partial_iget;
	pcitool_intr_get_t *iget = &partial_iget;
	size_t	iget_kmem_alloc_size = 0;
	ib_t *ib_p = pci_p->pci_ib_p;
	volatile uint64_t *imregp;
	uint64_t imregval;
	uint32_t ino;
	uint8_t num_devs_ret;
	int cpu_id;
	int copyout_rval;
	int rval = SUCCESS;

	/* Read in just the header part, no array section. */
	if (ddi_copyin(arg, &partial_iget, PCITOOL_IGET_SIZE(0), mode) !=
	    DDI_SUCCESS) {

		return (EFAULT);
	}

	if (partial_iget.flags & PCITOOL_INTR_FLAG_GET_MSI) {
		partial_iget.status = PCITOOL_IO_ERROR;
		partial_iget.num_devs_ret = 0;
		rval = ENOTSUP;
		goto done_get_intr;
	}

	ino = partial_iget.ino;
	num_devs_ret = partial_iget.num_devs_ret;

	/* Validate argument. */
	if (ino > PCI_MAX_INO) {
		partial_iget.status = PCITOOL_INVALID_INO;
		partial_iget.num_devs_ret = 0;
		rval = EINVAL;
		goto done_get_intr;
	}

	/* Caller wants device information returned. */
	if (num_devs_ret > 0) {

		/*
		 * Allocate room.
		 * Note if num_devs_ret == 0 iget remains pointing to
		 * partial_iget.
		 */
		iget_kmem_alloc_size = PCITOOL_IGET_SIZE(num_devs_ret);
		iget = kmem_alloc(iget_kmem_alloc_size, KM_SLEEP);

		/* Read in whole structure to verify there's room. */
		if (ddi_copyin(arg, iget, iget_kmem_alloc_size, mode) !=
		    SUCCESS) {

			/* Be consistent and just return EFAULT here. */
			kmem_free(iget, iget_kmem_alloc_size);

			return (EFAULT);
		}
	}

	bzero(iget, PCITOOL_IGET_SIZE(num_devs_ret));
	iget->ino = ino;
	iget->num_devs_ret = num_devs_ret;
	iget->ctlr = 0;
	iget->cpu_id = 0;

done_get_intr:
	iget->drvr_version = PCITOOL_VERSION;
	copyout_rval = ddi_copyout(iget, arg,
	    PCITOOL_IGET_SIZE(num_devs_ret), mode);

	if (iget_kmem_alloc_size > 0) {
		kmem_free(iget, iget_kmem_alloc_size);
	}

	if (copyout_rval != DDI_SUCCESS) {
		rval = EFAULT;
	}

	return (rval);
}

/*
 * Associate a new CPU with a given ino.
 *
 * Operate only on inos which are already mapped to devices.
 */
static int
pcitool_set_intr(dev_info_t *dip, void *arg, int mode, pci_t *pci_p)
{
	int rval = SUCCESS;
	return (rval);
}


/* Main function for handling interrupt CPU binding requests and queries. */
int
pcitool_intr_admn(dev_t dev, void *arg, int cmd, int mode)
{
	pci_t		*pci_p = DEV_TO_SOFTSTATE(dev);
	dev_info_t	*dip = pci_p->pci_dip;
	int		rval = SUCCESS;

	switch (cmd) {

	/* Get system interrupt information. */
	case PCITOOL_SYSTEM_INTR_INFO:
		rval = pcitool_intr_info(dip, arg, mode);
		break;

	/* Get interrupt information for a given ino. */
	case PCITOOL_DEVICE_GET_INTR:
		rval = pcitool_get_intr(dip, arg, mode, pci_p);
		break;

	/* Associate a new CPU with a given ino. */
	case PCITOOL_DEVICE_SET_INTR:
		rval = pcitool_set_intr(dip, arg, mode, pci_p);
		break;

	default:
		rval = ENOTTY;
	}

	return (rval);
}


/*
 * Wrapper around pcitool_phys_peek/poke.
 *
 * Validates arguments and calls pcitool_phys_peek/poke appropriately.
 *
 * Dip is of the nexus,
 * phys_addr is the address to write in physical space,
 * max_addr is the upper bound on the physical space used for bounds checking,
 * pcitool_status returns more detailed status in addition to a more generic
 * errno-style function return value.
 * other args are self-explanatory.
 */
static int
pcitool_access(pci_t *pci_p, uint64_t phys_addr, uint64_t max_addr,
	uint64_t *data, uint8_t size, boolean_t write, boolean_t endian,
	uint32_t *pcitool_status)
{

	int rval = SUCCESS;
	dev_info_t *dip = pci_p->pci_dip;

	/* Upper bounds checking. */
	if (phys_addr > max_addr) {
		prom_printf(
		    "%s(): Phys addr 0x%llx out of range (max 0x%llx).\n",
		    __FUNCTION__, phys_addr, max_addr);
		*pcitool_status = PCITOOL_INVALID_ADDRESS;

		rval = EINVAL;

	/* Alignment checking. */
	} else if (!IS_P2ALIGNED(phys_addr, size)) {
		prom_printf("%s(): not aligned.\n", __FUNCTION__);
		*pcitool_status = PCITOOL_NOT_ALIGNED;

		rval = EINVAL;

	/* Made it through checks.  Do the access. */
	} else if (write) {
		prom_printf(
		    "%s(): %d byte %s write at addr 0x%llx\n",
		    __FUNCTION__, size, (endian ? "BE" : "LE"), phys_addr);

		if (endian) {
			switch (size) {
			case 8: *(uint64_t *)phys_addr = BSWAP_64(*data); break;
			case 4: *(uint32_t *)phys_addr = BSWAP_32(*data); break;
			case 2: *(uint16_t *)phys_addr = BSWAP_16(*data); break;
			case 1: *(uint8_t *)phys_addr = *data; break;
			}
		} else {
			switch (size) {
			case 8: *(uint64_t *)phys_addr = *data; break;
			case 4: *(uint32_t *)phys_addr = *data; break;
			case 2: *(uint16_t *)phys_addr = *data; break;
			case 1: *(uint8_t *)phys_addr = *data; break;
			}
		}
	} else {	/* Read */
		prom_printf(
		    "%s(): %d byte %s read at addr 0x%llx\n",
		    __FUNCTION__, size, (endian ? "BE" : "LE"), phys_addr);
		if (endian) {
			switch (size) {
			case 8: *data = BSWAP_64(*(uint64_t *)phys_addr); break;
			case 4: *data = BSWAP_32(*(uint32_t *)phys_addr); break;
			case 2: *data = BSWAP_16(*(uint16_t *)phys_addr); break;
			case 1: *data = *(uint8_t *)phys_addr; break;
			}
		} else {
			switch (size) {
			case 8: *data = *(uint64_t *)phys_addr; break;
			case 4: *data = *(uint32_t *)phys_addr; break;
			case 2: *data = *(uint16_t *)phys_addr; break;
			case 1: *data = *(uint8_t *)phys_addr; break;
			}
		}
	}
	return (rval);
}

/*
 * Perform register accesses on the nexus device itself.
 */
int
pcitool_bus_reg_ops(dev_t dev, void *arg, int cmd, int mode)
{

	pci_t			*pci_p = DEV_TO_SOFTSTATE(dev);
	dev_info_t		*dip = pci_p->pci_dip;
	pci_nexus_regspec_t	*pci_rp = NULL;
	boolean_t		write_flag = B_FALSE;
	pcitool_reg_t		prg;
	uint64_t		base_addr;
	uint64_t		max_addr;
	uint32_t		reglen;
	uint8_t			size;
	uint32_t		rval = 0;

	if (cmd == PCITOOL_NEXUS_SET_REG)
		write_flag = B_TRUE;

	prom_printf(
	    "%s(): nexus set/get reg\n", __FUNCTION__);

	/* Read data from userland. */
	if (ddi_copyin(arg, &prg, sizeof (pcitool_reg_t), mode) !=
	    DDI_SUCCESS) {
		prom_printf(
		    "%s(): Error reading arguments\n", __FUNCTION__);
		return (EFAULT);
	}

	/* Read reg property which contains starting addr and size of banks. */
	if (ddi_prop_lookup_int_array(
	    DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
	    "reg", (int **)&pci_rp, &reglen) == DDI_SUCCESS) {
		if (((reglen * sizeof (int)) %
		    sizeof (pci_nexus_regspec_t)) != 0) {
			prom_printf(
			    "%s(): reg prop not well-formed\n", __FUNCTION__);
			prg.status = PCITOOL_REGPROP_NOTWELLFORMED;
			rval = EIO;
			goto done;
		}
	}

	/* Bounds check the bank number. */
	if (prg.barnum >=
	    (reglen * sizeof (int)) / sizeof (pci_nexus_regspec_t)) {
		prg.status = PCITOOL_OUT_OF_RANGE;
		rval = EINVAL;
		goto done;
	}

	size = PCITOOL_ACC_ATTR_SIZE(prg.acc_attr);
	base_addr = pci_rp[prg.barnum].phys_addr;
	max_addr = base_addr + pci_rp[prg.barnum].size;
	prg.phys_addr = base_addr + prg.offset;

	prom_printf(
	    "%s(): "
	    "pcitool_bus_reg_ops: nexus: base:0x%llx, offset:0x%llx, "
	    "addr:0x%llx, max_addr:0x%llx\n",
	    __FUNCTION__, base_addr, prg.offset, prg.phys_addr, max_addr);

	/* Access device.  prg.status is modified. */
	rval = pcitool_access(pci_p,
	    prg.phys_addr, max_addr, &prg.data, size, write_flag,
	    PCITOOL_ACC_IS_BIG_ENDIAN(prg.acc_attr), &prg.status);

done:
	if (pci_rp != NULL)
		ddi_prop_free(pci_rp);

	prg.drvr_version = PCITOOL_VERSION;
	if (ddi_copyout(&prg, arg, sizeof (pcitool_reg_t), mode) !=
	    DDI_SUCCESS) {
		prom_printf("%s(): Copyout failed.\n", __FUNCTION__);
		return (EFAULT);
	}

	return (rval);
}


static int
pcitool_validate_barnum_bdf(pcitool_reg_t *prg)
{
	int rval = SUCCESS;

	if (prg->barnum >= (sizeof (pci_bars) / sizeof (pci_bars[0]))) {
		prg->status = PCITOOL_OUT_OF_RANGE;
		rval = EINVAL;

	/* Validate address arguments of bus / dev / func */
	} else if (((prg->bus_no &
	    (PCI_REG_BUS_M >> PCI_REG_BUS_SHIFT)) != prg->bus_no) ||
	    ((prg->dev_no &
	    (PCI_REG_DEV_M >> PCI_REG_DEV_SHIFT)) != prg->dev_no) ||
	    ((prg->func_no &
	    (PCI_REG_FUNC_M >> PCI_REG_FUNC_SHIFT)) != prg->func_no)) {
		prg->status = PCITOOL_INVALID_ADDRESS;
		rval = EINVAL;
	}

	return (rval);
}

static int
pcitool_get_bar(pci_t *pci_p, pcitool_reg_t *prg, uint64_t config_base_addr,
	uint64_t config_max_addr, uint64_t *bar, boolean_t *is_io_space)
{

	uint8_t bar_offset;
	int rval;
	dev_info_t *dip = pci_p->pci_dip;

	*bar = 0;
	*is_io_space = B_FALSE;

	/*
	 * Translate BAR number into offset of the BAR in
	 * the device's config space.
	 */
	bar_offset = PCI_BAR_OFFSET((*prg));

	prom_printf("%s(): barnum:%d, bar_offset:0x%x\n",
	    __FUNCTION__, prg->barnum, bar_offset);

	/*
	 * Get Bus Address Register (BAR) from config space.
	 * bar_offset is the offset into config space of the BAR desired.
	 * prg->status is modified on error.
	 */
	rval = pcitool_access(pci_p, config_base_addr + bar_offset,
	    config_max_addr, bar,
	    4,		/* 4 bytes. */
	    B_FALSE,	/* Read */
	    B_FALSE, 	/* Little endian. */
	    &prg->status);
	if (rval != SUCCESS)
		return (rval);

	prom_printf("%s(): bar returned is 0x%llx\n", __FUNCTION__, *bar);
	if (!(*bar)) {
		prg->status = PCITOOL_INVALID_ADDRESS;
		return (EINVAL);
	}

	/*
	 * BAR has bits saying this space is IO space, unless
	 * this is the ROM address register.
	 */
	if (((PCI_BASE_SPACE_M & *bar) == PCI_BASE_SPACE_IO) &&
	    (bar_offset != PCI_CONF_ROM)) {
		*is_io_space = B_TRUE;
		*bar &= PCI_BASE_IO_ADDR_M;

	/*
	 * BAR has bits saying this space is 64 bit memory
	 * space, unless this is the ROM address register.
	 *
	 * The 64 bit address stored in two BAR cells is not necessarily
	 * aligned on an 8-byte boundary.  Need to keep the first 4
	 * bytes read, and do a separate read of the high 4 bytes.
	 */

	} else if ((PCI_BASE_TYPE_ALL & *bar) && (bar_offset != PCI_CONF_ROM)) {

		uint32_t low_bytes = (uint32_t)(*bar & ~PCI_BASE_TYPE_ALL);

		/* Don't try to read past the end of BARs. */
		if (bar_offset >= PCI_CONF_BASE5) {
			prg->status = PCITOOL_OUT_OF_RANGE;
			return (EIO);
		}

		/* Access device.  prg->status is modified on error. */
		rval = pcitool_access(pci_p,
		    config_base_addr + bar_offset + 4, config_max_addr, bar,
		    4,		/* 4 bytes. */
		    B_FALSE,	/* Read */
		    B_FALSE, 	/* Little endian. */
		    &prg->status);
		if (rval != SUCCESS)
			return (rval);

		*bar = (*bar << 32) + low_bytes;
	}

	return (SUCCESS);
}


static int
pcitool_config_request(pci_t *pci_p, pcitool_reg_t *prg, uint64_t base_addr,
	uint64_t max_addr, uint8_t size, boolean_t write_flag)
{
	int rval;
	dev_info_t *dip = pci_p->pci_dip;

	/* Access config space and we're done. */
	prg->phys_addr = base_addr + prg->offset;

	prom_printf(
	    "%s(): ",
	    "config access: base:0x%llx, "
	    "offset:0x%llx, phys_addr:0x%llx, end:%s\n",
	    __FUNCTION__, base_addr, prg->offset, prg->phys_addr,
	    (PCITOOL_ACC_IS_BIG_ENDIAN(prg->acc_attr)? "big" : "ltl"));

	/* Access device.  pr.status is modified. */
	rval = pcitool_access(pci_p, prg->phys_addr, max_addr, &prg->data, size,
	    write_flag, PCITOOL_ACC_IS_BIG_ENDIAN(prg->acc_attr), &prg->status);

	prom_printf("%s(): config access: data:0x%llx\n", __FUNCTION__, prg->data);

	return (rval);
}

/* Perform register accesses on PCI leaf devices. */
int
pcitool_dev_reg_ops(dev_t dev, void *arg, int cmd, int mode)
{
	pci_t		*pci_p = DEV_TO_SOFTSTATE(dev);
	dev_info_t	*dip = pci_p->pci_dip;
	pci_ranges_t	*rp = pci_p->pci_ranges;
	pcitool_reg_t	prg;
	uint64_t	max_addr;
	uint64_t	base_addr;
	uint64_t	range_prop;
	uint64_t	range_prop_size;
	uint64_t	bar = 0;
	int		rval = 0;
	boolean_t	write_flag = B_FALSE;
	boolean_t	is_io_space = B_FALSE;
	uint8_t		size;

	if (cmd == PCITOOL_DEVICE_SET_REG)
		write_flag = B_TRUE;

	prom_printf("%s(): nexus set/get reg\n", __FUNCTION__);
	if (ddi_copyin(arg, &prg, sizeof (pcitool_reg_t), mode) !=
	    DDI_SUCCESS) {
		prom_printf("%s(): Error reading arguments\n", __FUNCTION__);
		return (EFAULT);
	}

	prom_printf("%s(): raw bus:0x%x, dev:0x%x, func:0x%x\n",
	    __FUNCTION__, prg.bus_no, prg.dev_no, prg.func_no);

	if ((rval = pcitool_validate_barnum_bdf(&prg)) != SUCCESS)
		goto done_reg;

	size = PCITOOL_ACC_ATTR_SIZE(prg.acc_attr);

	/* Get config space first. */
	range_prop = PCI_GET_RANGE_PROP(rp, PCI_CONFIG_RANGE_BANK);
	range_prop_size = PCI_GET_RANGE_PROP_SIZE(rp, PCI_CONFIG_RANGE_BANK);
	max_addr = range_prop + range_prop_size;

	/*
	 * Build device address based on base addr from range prop, and bus,
	 * dev and func values passed in.  This address is where config space
	 * begins.
	 */
	base_addr = range_prop +
	    (prg.bus_no << PCI_REG_BUS_SHIFT) +
	    (prg.dev_no << PCI_REG_DEV_SHIFT) +
	    (prg.func_no << PCI_REG_FUNC_SHIFT);

	if ((base_addr < range_prop) || (base_addr >= max_addr)) {
		prg.status = PCITOOL_OUT_OF_RANGE;
		rval = EINVAL;
		goto done_reg;
	}

	prom_printf("%s(): "
	    "range_prop:0x%llx, shifted: bus:0x%x, dev:0x%x "
	    "func:0x%x, addr:0x%x\n",
	    __FUNCTION__, range_prop,
	    prg.bus_no << PCI_REG_BUS_SHIFT, prg.dev_no << PCI_REG_DEV_SHIFT,
	    prg.func_no << PCI_REG_FUNC_SHIFT, base_addr);

	/* Proper config space desired. */
	if (prg.barnum == 0) {

		rval = pcitool_config_request(pci_p, &prg, base_addr, max_addr,
		    size, write_flag);

	} else {	/* IO / MEM / MEM64 space. */

		if (pcitool_get_bar(pci_p, &prg, base_addr, max_addr, &bar,
		    &is_io_space) != SUCCESS)
			goto done_reg;

		/* IO space. */
		if (is_io_space) {
			prom_printf("%s(): IO space\n", __FUNCTION__);

			/* Reposition to focus on IO space. */
			range_prop = PCI_GET_RANGE_PROP(rp, PCI_IO_RANGE_BANK);
			range_prop_size = PCI_GET_RANGE_PROP_SIZE(rp,
			    PCI_IO_RANGE_BANK);

		/* 64 bit memory space. */
		} else if ((bar >> 32) != 0) {
			prom_printf(
			    "%s(): 64 bit mem space.  64-bit bar is 0x%llx\n",
			    __FUNCTION__, bar);

			/* Reposition to MEM64 range space. */
			range_prop = PCI_GET_RANGE_PROP(rp,
			    PCI_MEM64_RANGE_BANK);
			range_prop_size = PCI_GET_RANGE_PROP_SIZE(rp,
			    PCI_MEM64_RANGE_BANK);

		} else {	/* Mem32 space, including ROM */
			prom_printf(
			    "%s(): 32 bit mem space\n", __FUNCTION__);

			if (PCI_BAR_OFFSET(prg) == PCI_CONF_ROM) {
				prom_printf(
				    "%s(): Additional ROM checking\n",
				    __FUNCTION__);

				/* Can't write to ROM */
				if (write_flag) {
					prg.status = PCITOOL_ROM_WRITE;
					rval = EIO;
					goto done_reg;

				/* ROM disabled for reading */
				} else if (!(bar & 0x00000001)) {
					prg.status = PCITOOL_ROM_DISABLED;
					rval = EIO;
					goto done_reg;
				}
			}

			range_prop = PCI_GET_RANGE_PROP(rp, PCI_MEM_RANGE_BANK);
			range_prop_size = PCI_GET_RANGE_PROP_SIZE(rp,
			    PCI_MEM_RANGE_BANK);
		}

		/* Common code for all IO/MEM range spaces. */
		max_addr = range_prop + range_prop_size;
		base_addr = range_prop + bar;

		prom_printf(
		    "%s(): addr portion of bar is 0x%llx, base=0x%llx, "
		    "offset:0x%lx\n",
		    __FUNCTION__, bar, base_addr, prg.offset);

		/*
		 * Use offset provided by caller to index into
		 * desired space, then access.
		 * Note that prg.status is modified on error.
		 */
		prg.phys_addr = base_addr + prg.offset;
		rval = pcitool_access(pci_p, prg.phys_addr,
		    max_addr, &prg.data, size, write_flag,
		    PCITOOL_ACC_IS_BIG_ENDIAN(prg.acc_attr), &prg.status);
	}

done_reg:
	prg.drvr_version = PCITOOL_VERSION;
	if (ddi_copyout(&prg, arg, sizeof (pcitool_reg_t), mode) !=
	    DDI_SUCCESS) {
		prom_printf(
		    "%s(): Error returning arguments.\n", __FUNCTION__);
		rval = EFAULT;
	}
	return (rval);
}

int
pcitool_init(dev_info_t *dip)
{
	int instance = ddi_get_instance(dip);

	if (ddi_create_minor_node(dip, PCI_MINOR_REG, S_IFCHR,
	    PCIHP_AP_MINOR_NUM(instance, PCI_TOOL_REG_MINOR_NUM),
	    DDI_NT_REGACC, 0) != DDI_SUCCESS)
		return (DDI_FAILURE);

	if (ddi_create_minor_node(dip, PCI_MINOR_INTR, S_IFCHR,
	    PCIHP_AP_MINOR_NUM(instance, PCI_TOOL_INTR_MINOR_NUM),
	    DDI_NT_INTRCTL, 0) != DDI_SUCCESS) {
		ddi_remove_minor_node(dip, PCI_MINOR_REG);
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}

void
pcitool_uninit(dev_info_t *dip)
{
	ddi_remove_minor_node(dip, PCI_MINOR_REG);
	ddi_remove_minor_node(dip, PCI_MINOR_INTR);
}
