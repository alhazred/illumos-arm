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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * PCI nexus utility routines:
 *	property and config routines for attach()
 *	reg/intr/range/assigned-address property routines for bus_map()
 *	init_child()
 *	fault handling
 */

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/sysmacros.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/fm/protocol.h>
#include <sys/fm/io/pci.h>
#include <sys/fm/util.h>
#include <sys/ddi_impldefs.h>
#include <sys/pci/pci_obj.h>
#include <sys/promif.h>

#define DEBUG_PRINTF(...)	// prom_printf(__VA_ARGS__)
static int pci_get_hose(dev_info_t *dip);

/*
 * get_pci_properties
 *
 * This function is called from the attach routine to get the key
 * properties of the pci nodes.
 *
 * used by: pci_attach()
 *
 * return value: DDI_FAILURE on failure
 */
int
get_pci_properties(pci_t *pci_p, dev_info_t *dip)
{
	int i;

	/*
	 * Get the device's PCI HOSE.
	 */
	if ((pci_p->pci_id = (uint32_t)pci_get_hose(dip)) == -1u) {
		cmn_err(CE_WARN, "%s%d: no Hose property\n",
			ddi_driver_name(dip), ddi_get_instance(dip));
		return (DDI_FAILURE);
	}
	DEBUG_PRINTF("%s() pci_id=%u\n", __FUNCTION__, pci_p->pci_id);

	/*
	 * Get the bus-ranges property.
	 */
	i = sizeof (pci_p->pci_bus_range);
	if (ddi_getlongprop_buf(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
	    "bus-range", (caddr_t)&pci_p->pci_bus_range, &i) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "%s%d: no bus-range property\n",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		return (DDI_FAILURE);
	}
	DEBUG_PRINTF("get_pci_properties: bus-range (%x,%x)\n",
		pci_p->pci_bus_range.lo, pci_p->pci_bus_range.hi);

	/*
	 * Get the ranges property.
	 */
	if (ddi_getlongprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS, "ranges",
		(caddr_t)&pci_p->pci_ranges, &pci_p->pci_ranges_length) !=
		DDI_SUCCESS) {

		cmn_err(CE_WARN, "%s%d: no ranges property\n",
			ddi_driver_name(dip), ddi_get_instance(dip));
		return (DDI_FAILURE);
	}

	for (i=0; i < pci_p->pci_ranges_length / sizeof (pci_ranges_t); i++) {
		DEBUG_PRINTF("%s: %08x %08x %08x %08x %08x %08x %08x\n",
		    __FUNCTION__,
		    pci_p->pci_ranges[i].child_high,
		    pci_p->pci_ranges[i].child_mid,
		    pci_p->pci_ranges[i].child_low,
		    pci_p->pci_ranges[i].parent_high,
		    pci_p->pci_ranges[i].parent_low,
		    pci_p->pci_ranges[i].size_high,
		    pci_p->pci_ranges[i].size_low
		    );
	}
	return (DDI_SUCCESS);
}

/*
 * free_pci_properties:
 *
 * This routine frees the memory used to cache the
 * "ranges" properties of the pci bus device node.
 *
 * used by: pci_detach()
 *
 * return value: none
 */
void
free_pci_properties(pci_t *pci_p)
{
	kmem_free(pci_p->pci_ranges, pci_p->pci_ranges_length);
}


/*
 * report_dev
 *
 * This function is called from our control ops routine on a
 * DDI_CTLOPS_REPORTDEV request.
 *
 * The display format is
 *
 *	<name><inst> at <pname><pinst> device <dev> function <func>
 *
 * where
 *
 *	<name>		this device's name property
 *	<inst>		this device's instance number
 *	<name>		parent device's name property
 *	<inst>		parent device's instance number
 *	<dev>		this device's device number
 *	<func>		this device's function number
 */
int
report_dev(dev_info_t *dip)
{
	if (dip == (dev_info_t *)0)
		return (DDI_FAILURE);
	cmn_err(CE_CONT, "?PCI-device: %s@%s, %s%d\n",
	    ddi_node_name(dip), ddi_get_name_addr(dip),
	    ddi_driver_name(dip),
	    ddi_get_instance(dip));
	return (DDI_SUCCESS);
}


/*
 * reg property for pcimem nodes that covers the entire address
 * space for the node:  config, io, or memory.
 */
pci_regspec_t pci_pcimem_reg[3] =
{
	{PCI_ADDR_CONFIG,			0, 0, 0, 0x800000	},
	{(uint_t)(PCI_ADDR_IO|PCI_RELOCAT_B),	0, 0, 0, PCI_IO_SIZE	},
	{(uint_t)(PCI_ADDR_MEM32|PCI_RELOCAT_B), 0, 0, 0, PCI_MEM_SIZE	}
};

static int
pci_get_hose(dev_info_t *dip)
{
	return (ddi_getprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
	    "hose", -1));
}

int
map_pci_registers(pci_t *pci_p, dev_info_t *dip)
{
	return (DDI_SUCCESS);
}

void
unmap_pci_registers(pci_t *pci_p)
{}

/*
 * pci_reloc_reg
 *
 * If the "reg" entry (*pci_rp) is relocatable, lookup "assigned-addresses"
 * property to fetch corresponding relocated address.
 *
 * used by: pci_map()
 *
 * return value:
 *
 *	DDI_SUCCESS		- on success
 *	DDI_ME_INVAL		- regspec is invalid
 */
int
pci_reloc_reg(dev_info_t *dip, dev_info_t *rdip, pci_t *pci_p,
	pci_regspec_t *rp)
{
	int assign_len, assign_entries, i;
	pci_regspec_t *assign_p;
	register uint32_t phys_hi = rp->pci_phys_hi;

	if ((phys_hi & PCI_RELOCAT_B) || !(phys_hi & PCI_ADDR_MASK))
		return (DDI_SUCCESS);

	/* phys_mid must be 0 regardless space type. */
	if (rp->pci_phys_mid != 0 || rp->pci_size_hi != 0) {
		return (DDI_ME_INVAL);
	}

	if (ddi_getlongprop(DDI_DEV_T_ANY, rdip, DDI_PROP_DONTPASS,
		"assigned-addresses", (caddr_t)&assign_p, &assign_len))
		return (DDI_ME_INVAL);

	assign_entries = assign_len / sizeof (pci_regspec_t);
	for (i = 0; i < assign_entries; i++, assign_p++) {
		uint32_t space_type = phys_hi & PCI_REG_ADDR_M;
		uint32_t assign_type = assign_p->pci_phys_hi & PCI_REG_ADDR_M;
		uint32_t assign_addr = PCI_REG_BDFR_G(assign_p->pci_phys_hi);

		if (PCI_REG_BDFR_G(phys_hi) != assign_addr)
			continue;
		if (space_type == assign_type) { /* exact match */
			rp->pci_phys_low += assign_p->pci_phys_low;
			break;
		}
		if (space_type == PCI_ADDR_MEM64 &&
		    assign_type == PCI_ADDR_MEM32) {
			rp->pci_phys_low += assign_p->pci_phys_low;
			rp->pci_phys_hi ^= PCI_ADDR_MEM64 ^ PCI_ADDR_MEM32;
			break;
		}
	}
	kmem_free(assign_p - i, assign_len);
	return (i < assign_entries ? DDI_SUCCESS : DDI_ME_INVAL);
}

/*
 * use "ranges" to translate relocated pci regspec into parent space
 */
int
pci_xlate_reg(pci_t *pci_p, pci_regspec_t *pci_rp, struct regspec *new_rp)
{
	int n;
	pci_ranges_t *rng_p = pci_p->pci_ranges;
	int rng_n = pci_p->pci_ranges_length / sizeof (pci_ranges_t);

	uint32_t space_type = PCI_REG_ADDR_G(pci_rp->pci_phys_hi);
	uint32_t reg_end, reg_begin = pci_rp->pci_phys_low;
	uint32_t sz = pci_rp->pci_size_low;

	uint32_t rng_begin, rng_end;

	if (space_type == PCI_REG_ADDR_G(PCI_ADDR_CONFIG)) {
		if (reg_begin > PCI_CONF_HDR_SIZE)
			return (DDI_ME_INVAL);
		sz = sz ? MIN(sz, PCI_CONF_HDR_SIZE) : PCI_CONF_HDR_SIZE;
		reg_begin += pci_rp->pci_phys_hi;
	}
	reg_end = reg_begin + sz - 1;

	for (n = 0; n < rng_n; n++, rng_p++) {
		if (space_type != PCI_REG_ADDR_G(rng_p->child_high))
			continue;	/* not the same space type */

		rng_begin = rng_p->child_low;
		if (space_type == PCI_REG_ADDR_G(PCI_ADDR_CONFIG))
			rng_begin += rng_p->child_high;

		rng_end = rng_begin + rng_p->size_low - 1;
		if (reg_begin >= rng_begin && reg_end <= rng_end)
			break;
	}
	if (n >= rng_n)
		return (DDI_ME_REGSPEC_RANGE);

	new_rp->regspec_bustype = ((space_type == PCI_REG_ADDR_G(PCI_ADDR_IO))? 1: 0) << 28;
	new_rp->regspec_bustype |= rng_p->parent_high;
	new_rp->regspec_addr = reg_begin - rng_begin + rng_p->parent_low;
	new_rp->regspec_size = sz;

	return (DDI_SUCCESS);
}

void
pci_fm_create(pci_t *pci_p)
{
	pci_p->pci_fm_cap = DDI_FM_ACCCHK_CAPABLE | DDI_FM_DMACHK_CAPABLE;

	ddi_fm_init(pci_p->pci_dip, &pci_p->pci_fm_cap, &pci_p->pci_fm_ibc);

	mutex_init(&pci_p->pci_peek_poke_mutex, NULL, MUTEX_DRIVER,
	    (void *)pci_p->pci_fm_ibc);
}
void
pci_fm_acc_setup(ddi_map_req_t *mp, dev_info_t *rdip)
{
	uchar_t fflag;
	ddi_acc_hdl_t *hp;
	ddi_acc_impl_t *ap;

	hp = mp->map_handlep;
	ap = (ddi_acc_impl_t *)hp->ah_platform_private;
	fflag = ap->ahi_common.ah_acc.devacc_attr_access;

	DEBUG_PRINTF("%s(): mp->map_op=%x\n", __FUNCTION__, mp->map_op);
	if (mp->map_op == DDI_MO_MAP_LOCKED) {
		ndi_fmc_insert(rdip, ACC_HANDLE, (void *)hp, NULL);
		switch (fflag) {
		case DDI_FLAGERR_ACC:
			if (ap->ahi_acc_attr & DDI_ACCATTR_IO_SPACE) {
				ap->ahi_get8 = i_ddi_io_get8;
				ap->ahi_put8 = i_ddi_io_put8;
				ap->ahi_rep_get8 = i_ddi_io_rep_get8;
				ap->ahi_rep_put8 = i_ddi_io_rep_put8;
				ap->ahi_get16 = i_ddi_io_get16;
				ap->ahi_get32 = i_ddi_io_get32;
				ap->ahi_get64 = i_ddi_io_get64;
				ap->ahi_put16 = i_ddi_io_put16;
				ap->ahi_put32 = i_ddi_io_put32;
				ap->ahi_put64 = i_ddi_io_put64;
				ap->ahi_rep_get16 = i_ddi_io_rep_get16;
				ap->ahi_rep_get32 = i_ddi_io_rep_get32;
				ap->ahi_rep_get64 = i_ddi_io_rep_get64;
				ap->ahi_rep_put16 = i_ddi_io_rep_put16;
				ap->ahi_rep_put32 = i_ddi_io_rep_put32;
				ap->ahi_rep_put64 = i_ddi_io_rep_put64;
			} else {
				ap->ahi_get8 = i_ddi_get8;
				ap->ahi_put8 = i_ddi_put8;
				ap->ahi_rep_get8 = i_ddi_rep_get8;
				ap->ahi_rep_put8 = i_ddi_rep_put8;
				ap->ahi_get16 = i_ddi_get16;
				ap->ahi_get32 = i_ddi_get32;
				ap->ahi_get64 = i_ddi_get64;
				ap->ahi_put16 = i_ddi_put16;
				ap->ahi_put32 = i_ddi_put32;
				ap->ahi_put64 = i_ddi_put64;
				ap->ahi_rep_get16 = i_ddi_rep_get16;
				ap->ahi_rep_get32 = i_ddi_rep_get32;
				ap->ahi_rep_get64 = i_ddi_rep_get64;
				ap->ahi_rep_put16 = i_ddi_rep_put16;
				ap->ahi_rep_put32 = i_ddi_rep_put32;
				ap->ahi_rep_put64 = i_ddi_rep_put64;
			}
			break;
		case DDI_CAUTIOUS_ACC :
			ap->ahi_get8 = i_ddi_caut_get8;
			ap->ahi_get16 = i_ddi_caut_get16;
			ap->ahi_get32 = i_ddi_caut_get32;
			ap->ahi_get64 = i_ddi_caut_get64;
			ap->ahi_put8 = i_ddi_caut_put8;
			ap->ahi_put16 = i_ddi_caut_put16;
			ap->ahi_put32 = i_ddi_caut_put32;
			ap->ahi_put64 = i_ddi_caut_put64;
			ap->ahi_rep_get8 = i_ddi_caut_rep_get8;
			ap->ahi_rep_get16 = i_ddi_caut_rep_get16;
			ap->ahi_rep_get32 = i_ddi_caut_rep_get32;
			ap->ahi_rep_get64 = i_ddi_caut_rep_get64;
			ap->ahi_rep_put8 = i_ddi_caut_rep_put8;
			ap->ahi_rep_put16 = i_ddi_caut_rep_put16;
			ap->ahi_rep_put32 = i_ddi_caut_rep_put32;
			ap->ahi_rep_put64 = i_ddi_caut_rep_put64;
			break;
		default:
			break;
		}
	} else if (mp->map_op == DDI_MO_UNMAP) {
		ndi_fmc_remove(rdip, ACC_HANDLE, (void *)hp);
	}
}

