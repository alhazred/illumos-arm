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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Determine the PCI configuration mechanism recommended by the BIOS.
 */

#include <sys/types.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/pci_impl.h>
#include <sys/ddi_subrdefs.h>
#include <sys/bootconf.h>
#include <sys/psw.h>
#include <sys/modctl.h>
#include <sys/errno.h>
#include <sys/pci.h>
#include <sys/reboot.h>
#include <sys/mutex.h>
#include "../../../../common/pci/pci_strings.h"
#include <sys/promif.h>

/*
 * Interface routines
 */
static void pci_enumerate(int);
static void pci_setup_tree(void);

static void pci_scan_bus(dev_info_t *dip, int hose, int bus);
static void
add_reg_props(dev_info_t *dip, uintptr_t conf_base, uint8_t bus, uint8_t dev, uint8_t func, int pciide);
static void
add_ppb_props(dev_info_t *dip, uintptr_t conf_base, uint8_t bus, uint8_t dev, uint8_t func);
static void add_compatible(dev_info_t *dip, uint16_t subvenid, uint16_t subdevid,
    uint16_t vendorid, uint16_t deviceid, uint8_t revid, uint32_t classcode);
static void add_model_prop(dev_info_t *dip, uint32_t classcode);
static void add_bus_range_prop(dev_info_t *dip, int bus, int sub_bus);
static void set_devpm_d0(int hose, int bus, int dev, int func);
//===============================================================
// for PYXIS
//===============================================================
//
static const pci_ranges_t pyxis_base[] = {
	{PCI_ADDR_CONFIG, 0, 0x00000000, 0x8a, 0x00000000, 0, 0x00010000},
	{PCI_ADDR_CONFIG, 0, 0x00010000, 0x8b, 0x00010000, 0, 0x00ff0000},
	{PCI_ADDR_IO,     0, 0, 0x89, 0, 0, 0x01000000},
	{PCI_ADDR_MEM32,  0, 0, 0x88, 0, 1, 0x00000000},
	{PCI_ADDR_MEM64,  0, 0, 0x88, 0, 1, 0x00000000}
};
static int get_pci_numhose()
{
	return 1;
}
static const pci_ranges_t *get_pci_ranges(int hose)
{
	if (hose == 0)
		return pyxis_base;
	return 0;
}
static int get_pci_numranges(int hose)
{
	if (hose == 0)
		return sizeof(pyxis_base) / sizeof(pyxis_base[0]);
	return 0;
}
static uint32_t maketag(int b, int d, int f)
{
	return (b << 16) | (d << 11) | (f << 8);
}
static uintptr_t get_config_base(int hose, int bus, int dev, int func)
{
	const pci_ranges_t *ranges = get_pci_ranges(hose);
	uint32_t base_offset = maketag(bus, dev, func);
	if (ranges) {
		for (int i = 0; i < get_pci_numranges(hose); i++) {
			if (PCI_REG_ADDR_G(ranges[i].child_high) == PCI_REG_ADDR_G(PCI_ADDR_CONFIG)) {
				if (ranges[i].child_low <= base_offset &&
				    base_offset < (ranges[i].child_low + ranges[i].size_low)) {
					uintptr_t conf_base = 0xfffffc0000000000ul;
					conf_base |= ((uintptr_t)ranges[i].parent_high << 32);
					conf_base |= ((uintptr_t)ranges[i].parent_low);
					conf_base += base_offset;
					conf_base -= ranges[i].child_low;
					return conf_base;
				}
			}
		}
	}
	return 0;
}
static inline uint8_t pci_conf_read8(uintptr_t conf_base, int offset)
{
	return *(volatile uint8_t *)(conf_base | offset);
}
static inline uint16_t pci_conf_read16(uintptr_t conf_base, int offset)
{
	return *(volatile uint16_t *)(conf_base | offset);
}
static inline uint32_t pci_conf_read32(uintptr_t conf_base, int offset)
{
	return *(volatile uint32_t *)(conf_base | offset);
}
static inline uint64_t pci_conf_read64(uintptr_t conf_base, int offset)
{
	return *(volatile uint64_t *)(conf_base | offset);
}
static inline void pci_conf_write8(uintptr_t conf_base, int offset, uint8_t wval)
{
	*(volatile uint8_t *)(conf_base | offset) = wval;
	__asm __volatile("mb" : : : "memory");
}
static inline void pci_conf_write16(uintptr_t conf_base, int offset, uint16_t wval)
{
	*(volatile uint16_t *)(conf_base | offset) = wval;
	__asm __volatile("mb" : : : "memory");
}
static inline void pci_conf_write32(uintptr_t conf_base, int offset, uint32_t wval)
{
	*(volatile uint32_t *)(conf_base | offset) = wval;
	__asm __volatile("mb" : : : "memory");
}
static inline void pci_conf_write64(uintptr_t conf_base, int offset, uint64_t wval)
{
	*(volatile uint64_t *)(conf_base | offset) = wval;
	__asm __volatile("mb" : : : "memory");
}
//===============================================================
static int
is_display(uint32_t classcode)
{
	static uint32_t disp_classes[] = {
		0x000100,
		0x030000,
		0x030001
	};
	int i, nclasses = sizeof (disp_classes) / sizeof (uint32_t);

	for (i = 0; i < nclasses; i++) {
		if (classcode == disp_classes[i])
			return (1);
	}
	return (0);
}

static int
is_isa(uint16_t vid, uint16_t did, uint32_t clscode)
{
	if (((clscode >> 16) == PCI_CLASS_BRIDGE) && (((clscode >> 8) & 0xff) == PCI_BRIDGE_ISA)) {
		return 1;
	} else if (vid == 0x8086 && did == 0x484) {
		return 1;
	}
	return 0;
}

static int
is_pciide(uint8_t basecl, uint8_t subcl, uint8_t revid,
    uint16_t venid, uint16_t devid, uint16_t subvenid, uint16_t subdevid)
{
	struct ide_table {	/* table for PCI_MASS_OTHER */
		uint16_t venid;
		uint16_t devid;
	} *entry;

	/* XXX SATA and other devices: need a way to add dynamically */
	static struct ide_table ide_other[] = {
		{0x1095, 0x3112},
		{0x1095, 0x3114},
		{0x1095, 0x3512},
		{0x1095, 0x680},	/* Sil0680 */
		{0x1283, 0x8211},	/* ITE 8211F is subcl PCI_MASS_OTHER */
		{0, 0}
	};

	if (basecl != PCI_CLASS_MASS)
		return (0);

	if (subcl == PCI_MASS_IDE) {
		return (1);
	}

	if (subcl != PCI_MASS_OTHER && subcl != PCI_MASS_SATA) {
		return (0);
	}

	entry = &ide_other[0];
	while (entry->venid) {
		if (entry->venid == venid && entry->devid == devid)
			return (1);
		entry++;
	}
	return (0);
}

static void found_dev(dev_info_t *parent, int hose, int bus, int dev, int func)
{
	uint8_t header, revid, basecls, subcls, proginf;
	uint16_t vid, did, svid, sdid;
	uint32_t clscode;
	char nodename[32], unitaddr[5];
	dev_info_t *dip;
	int secbus;
	uintptr_t conf_base = get_config_base(hose, bus, dev, func);

	vid = pci_conf_read16(conf_base, PCI_CONF_VENID);
	did = pci_conf_read16(conf_base, PCI_CONF_DEVID);
	header = pci_conf_read8(conf_base, PCI_CONF_HEADER);

	switch (header & PCI_HEADER_TYPE_M) {
	case PCI_HEADER_ZERO:
		svid = pci_conf_read16(conf_base, PCI_CONF_SUBVENID);
		sdid = pci_conf_read16(conf_base, PCI_CONF_SUBSYSID);
		break;
	case PCI_HEADER_CARDBUS:
		svid = pci_conf_read16(conf_base, PCI_CBUS_SUBVENID);
		sdid = pci_conf_read16(conf_base, PCI_CBUS_SUBSYSID);
		break;
	default:
		svid = 0;
		sdid = 0;
		break;
	}

	clscode = pci_conf_read32(conf_base, PCI_CONF_REVID) >> 8;
	basecls = clscode >> 16;
	subcls = (clscode >> 8) & 0xff;
	proginf = clscode & 0xff;
	revid = pci_conf_read8(conf_base, PCI_CONF_REVID);

	if (is_display(clscode))
		snprintf(nodename, sizeof(nodename), "display");
	else if (is_isa(vid, did, clscode))
		snprintf(nodename, sizeof(nodename), "isa");
	else if (svid != 0)
		snprintf(nodename, sizeof(nodename), "pci%x,%x", svid, sdid);
	else
		snprintf(nodename, sizeof(nodename), "pci%x,%x", vid, did);

	ndi_devi_alloc_sleep(parent, nodename, DEVI_SID_NODEID, &dip);

	ndi_prop_update_int(DDI_DEV_T_NONE, dip, "device-id", did);
	ndi_prop_update_int(DDI_DEV_T_NONE, dip, "vendor-id", vid);
	ndi_prop_update_int(DDI_DEV_T_NONE, dip, "revision-id", revid);
	ndi_prop_update_int(DDI_DEV_T_NONE, dip, "class-code", clscode);
	if (func == 0)
		(void) snprintf(unitaddr, sizeof(unitaddr), "%x", dev);
	else
		(void) snprintf(unitaddr, sizeof(unitaddr), "%x,%x", dev, func);

	if (svid != 0) {
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "subsystem-id", sdid);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "subsystem-vendor-id", svid);
	}

	ndi_prop_update_string(DDI_DEV_T_NONE, dip, "unit-address", unitaddr);

	if (is_display(clscode))
		ndi_prop_update_string(DDI_DEV_T_NONE, dip, "device_type", "display");

	if ((header & PCI_HEADER_TYPE_M) == PCI_HEADER_ZERO) {
		uint8_t mingrant = pci_conf_read8(conf_base, PCI_CONF_MIN_G);
		uint8_t maxlatency = pci_conf_read8(conf_base, PCI_CONF_MAX_L);

		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "min-grant", mingrant);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "max-latency", maxlatency);
	}

	{
		uint8_t intr = pci_conf_read8(conf_base, PCI_CONF_IPIN);
		if (intr != 0) {
			int iline = pci_conf_read8(conf_base, PCI_CONF_ILINE);
			ndi_prop_update_int(DDI_DEV_T_NONE, dip, "interrupts", iline);
		}
	}

	{
		int power[2] = {1, 1};
		uint16_t status = pci_conf_read16(conf_base, PCI_CONF_STAT);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "devsel-speed", (status & PCI_STAT_DEVSELT) >> 9);
		if (status & PCI_STAT_FBBC)
			ndi_prop_create_boolean(DDI_DEV_T_NONE, dip, "fast-back-to-back");
		if (status & PCI_STAT_66MHZ)
			ndi_prop_create_boolean(DDI_DEV_T_NONE, dip, "66mhz-capable");
		if (status & PCI_STAT_UDF)
			ndi_prop_create_boolean(DDI_DEV_T_NONE, dip, "udf-supported");
		ndi_prop_update_int_array(DDI_DEV_T_NONE, dip, "power-consumption", power, 2);
		set_devpm_d0(hose, bus, dev, func);
	}

	add_model_prop(dip, clscode);
	add_compatible(dip, svid, sdid, vid, did, revid, clscode);

	if ((basecls == PCI_CLASS_BRIDGE) && (subcls == PCI_BRIDGE_PCI))
		add_ppb_props(dip, conf_base, bus, dev, func);

	if (is_pciide(basecls, subcls, revid, vid, did, svid, sdid)) {
		if (ddi_compatible_driver_major(dip, NULL) == (major_t)-1) {
			ndi_devi_set_nodename(dip, "pci-ide", 0);
		}
	}

	DEVI_SET_PCI(dip);
	add_reg_props(dip, conf_base, bus, dev, func, is_pciide(basecls, subcls, revid, vid, did, svid, sdid));
	ndi_devi_bind_driver(dip, 0);

	if (is_pciide(basecls, subcls, revid, vid, did, svid, sdid)) {
		dev_info_t *cdip;
		ndi_prop_update_string(DDI_DEV_T_NONE, dip, "device_type", "pci-ide");
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#address-cells", 1);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#size-cells", 0);
		ndi_devi_alloc_sleep(dip, "ide", (pnode_t)DEVI_SID_NODEID, &cdip);
		ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "reg", 0);
		ndi_devi_bind_driver(cdip, 0);
		ndi_devi_alloc_sleep(dip, "ide", (pnode_t)DEVI_SID_NODEID, &cdip);
		ndi_prop_update_int(DDI_DEV_T_NONE, cdip, "reg", 1);
		ndi_devi_bind_driver(cdip, 0);
	}
	if (is_isa(vid, did, clscode)) {
		ndi_prop_update_string(DDI_DEV_T_NONE, dip, "device_type", "isa");
	}
	if ((basecls == PCI_CLASS_BRIDGE) && (subcls == PCI_BRIDGE_PCI)) {
		pci_scan_bus(dip, hose, pci_conf_read8(conf_base, PCI_BCNF_SECBUS));
	}
}

static void pci_scan_bus(dev_info_t *dip, int hose, int bus)
{
	for (int dev = 0; dev < PCI_MAX_DEVICES; dev++) {
		int func, nfunc = 1;
		for (func = 0; func < nfunc; func++) {
			uintptr_t conf_base = get_config_base(hose, bus, dev, func);
			if (pci_conf_read32(conf_base, PCI_CONF_VENID) == 0xffffffff)
				continue;
			found_dev(dip, hose, bus, dev, func);
			if (func == 0 &&
			    (pci_conf_read8(conf_base, PCI_CONF_HEADER) & PCI_HEADER_MULTI))
				nfunc = PCI_MAX_FUNCTIONS;
		}
	}
}

static int get_max_bus(int hose)
{
	int max_bus = 0;
	int bus = 0;
	for (int dev = 0; dev < PCI_MAX_DEVICES; dev++) {
		int func, nfunc = 1;
		for (func = 0; func < nfunc; func++) {
			uintptr_t conf_base = get_config_base(hose, bus, dev, func);
			if (pci_conf_read32(conf_base, PCI_CONF_VENID) == 0xffffffff)
				continue;
			if ((pci_conf_read8(conf_base, PCI_CONF_HEADER) & PCI_HEADER_TYPE_M) == PCI_HEADER_PPB) {
				uint8_t subbus = pci_conf_read8(conf_base, PCI_BCNF_SUBBUS);
				if (max_bus < subbus)
					max_bus = subbus;
			}
			if (func == 0 &&
			    (pci_conf_read8(conf_base, PCI_CONF_HEADER) & PCI_HEADER_MULTI))
				nfunc = PCI_MAX_FUNCTIONS;
		}
	}
	return max_bus;
}


static void pci_setup_tree(void)
{
	int hose;

	for (hose = 0; hose < get_pci_numhose(); hose++)
	{
		struct regspec pci_regs = {0};
		pci_ranges_t pci_ranges[6] = {0};
		const pci_ranges_t *ranges;
		dev_info_t *dip;
		int max_bus;
		int nrange = 0;

		ranges = get_pci_ranges(hose);
		if (ranges) {
			for (int i = 0; i < get_pci_numranges(hose); i++) {
				pci_ranges[nrange++] = ranges[i];
			}
		}

		ndi_devi_alloc_sleep(ddi_root_node(), "pci", (pnode_t)DEVI_SID_NODEID, &dip);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#address-cells", 3);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#size-cells", 2);
		pci_regs.regspec_bustype = hose;
		ndi_prop_update_int_array(DDI_DEV_T_NONE, dip, "reg", (int *)&pci_regs, sizeof(pci_regs) / sizeof(int));
		ndi_prop_update_int_array(DDI_DEV_T_NONE, dip, "ranges", (int *)pci_ranges, nrange * sizeof (pci_ranges_t) / sizeof(int));
		ndi_prop_update_string(DDI_DEV_T_NONE, dip, "device_type", "pci");
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, "hose", hose);
		max_bus = get_max_bus(hose);
		add_bus_range_prop(dip, 0, max_bus);
		ndi_devi_bind_driver(dip, 0);
		pci_scan_bus(dip, hose, 0);
	}
}

static void
pci_enumerate(int reprogram)
{
	if (reprogram) {
		return;
	}

	pci_setup_tree();
}

static void
add_bus_range_prop(dev_info_t *dip, int bus, int sub_bus)
{
	int bus_range[2];

	bus_range[0] = bus;
	bus_range[1] = sub_bus;
	ndi_prop_update_int_array(DDI_DEV_T_NONE, dip, "bus-range", (int *)bus_range, 2);
}

static void
set_devpm_d0(int hose, int bus, int dev, int func)
{
	uint16_t status;
	uint8_t header;
	uint8_t cap_ptr;
	uint8_t cap_id;
	uint16_t pmcsr;

	uintptr_t conf_base = get_config_base(hose, bus, dev, func);
	status = pci_conf_read8(conf_base, PCI_CONF_STAT);
	if (!(status & PCI_STAT_CAP))
		return;	/* No capabilities list */

	header = pci_conf_read8(conf_base, PCI_CONF_HEADER) & PCI_HEADER_TYPE_M;
	if (header == PCI_HEADER_CARDBUS)
		cap_ptr = pci_conf_read8(conf_base, PCI_CBUS_CAP_PTR);
	else
		cap_ptr = pci_conf_read8(conf_base, PCI_CONF_CAP_PTR);
	/*
	 * Walk the capabilities list searching for a PM entry.
	 */
	while (cap_ptr != PCI_CAP_NEXT_PTR_NULL && cap_ptr >= PCI_CAP_PTR_OFF) {
		cap_ptr &= PCI_CAP_PTR_MASK;
		cap_id = pci_conf_read8(conf_base, cap_ptr + PCI_CAP_ID);
		if (cap_id == PCI_CAP_ID_PM) {
			pmcsr = pci_conf_read16(conf_base, cap_ptr + PCI_PMCSR);
			pmcsr &= ~(PCI_PMCSR_STATE_MASK);
			pmcsr |= PCI_PMCSR_D0; /* D0 state */
			pci_conf_write16(conf_base, cap_ptr + PCI_PMCSR, pmcsr);
			break;
		}
		cap_ptr = pci_conf_read8(conf_base, cap_ptr + PCI_CAP_NEXT_PTR);
	}

}


extern const struct pci_class_strings_s class_pci[];
extern int class_pci_items;

static void
add_model_prop(dev_info_t *dip, uint32_t classcode)
{
	const char *desc;
	int i;
	uint8_t baseclass = classcode >> 16;
	uint8_t subclass = (classcode >> 8) & 0xff;
	uint8_t progclass = classcode & 0xff;

	if ((baseclass == PCI_CLASS_MASS) && (subclass == PCI_MASS_IDE)) {
		desc = "IDE controller";
	} else {
		for (desc = 0, i = 0; i < class_pci_items; i++) {
			if ((baseclass == class_pci[i].base_class) &&
			    (subclass == class_pci[i].sub_class) &&
			    (progclass == class_pci[i].prog_class)) {
				desc = class_pci[i].actual_desc;
				break;
			}
		}
		if (i == class_pci_items)
			desc = "Unknown class of pci/pnpbios device";
	}

	ndi_prop_update_string(DDI_DEV_T_NONE, dip, "model", (char *)desc);
}
#define	COMPAT_BUFSIZE	512
static void
add_compatible(dev_info_t *dip, uint16_t subvenid, uint16_t subdevid,
    uint16_t vendorid, uint16_t deviceid, uint8_t revid, uint32_t classcode)
{
	int i = 0;
	int size = COMPAT_BUFSIZE;
	char *compat[13];
	char *buf, *curr;

	curr = buf = kmem_alloc(size, KM_SLEEP);

	if (subvenid) {
		compat[i++] = curr;	/* form 0 */
		snprintf(curr, size, "pci%x,%x.%x.%x.%x", vendorid, deviceid, subvenid, subdevid, revid);
		size -= strlen(curr) + 1;
		curr += strlen(curr) + 1;

		compat[i++] = curr;	/* form 1 */
		snprintf(curr, size, "pci%x,%x.%x.%x", vendorid, deviceid, subvenid, subdevid);
		size -= strlen(curr) + 1;
		curr += strlen(curr) + 1;

		compat[i++] = curr;	/* form 2 */
		snprintf(curr, size, "pci%x,%x", subvenid, subdevid);
		size -= strlen(curr) + 1;
		curr += strlen(curr) + 1;
	}
	compat[i++] = curr;	/* form 3 */
	snprintf(curr, size, "pci%x,%x.%x", vendorid, deviceid, revid);
	size -= strlen(curr) + 1;
	curr += strlen(curr) + 1;

	compat[i++] = curr;	/* form 4 */
	snprintf(curr, size, "pci%x,%x", vendorid, deviceid);
	size -= strlen(curr) + 1;
	curr += strlen(curr) + 1;

	compat[i++] = curr;	/* form 5 */
	snprintf(curr, size, "pciclass,%06x", classcode);
	size -= strlen(curr) + 1;
	curr += strlen(curr) + 1;

	compat[i++] = curr;	/* form 6 */
	snprintf(curr, size, "pciclass,%04x", (classcode >> 8));
	size -= strlen(curr) + 1;
	curr += strlen(curr) + 1;

	ndi_prop_update_string_array(DDI_DEV_T_NONE, dip, "compatible", compat, i);
	kmem_free(buf, COMPAT_BUFSIZE);
}

static void
add_ppb_props(dev_info_t *dip, uintptr_t conf_base, uint8_t bus, uint8_t dev, uint8_t func)
{
	char *dev_type;
	int i;
	uint32_t val, io_range[2], mem_range[2], pmem_range[2];
	uint8_t secbus = pci_conf_read8(conf_base, PCI_BCNF_SECBUS);
	uint8_t subbus = pci_conf_read8(conf_base, PCI_BCNF_SUBBUS);
	uint8_t progclass;
	ppb_ranges_t range[3];
	int range_idx = 0;

	ASSERT(secbus <= subbus);

	ndi_prop_update_string(DDI_DEV_T_NONE, dip, "device_type", "pci");
	ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#address-cells", 3);
	ndi_prop_update_int(DDI_DEV_T_NONE, dip, "#size-cells", 2);

	val = pci_conf_read8(conf_base, PCI_BCNF_IO_BASE_LOW);
	io_range[0] = ((val & 0xf0) << 8);
	val = pci_conf_read8(conf_base, PCI_BCNF_IO_LIMIT_LOW);
	io_range[1]  = ((val & 0xf0) << 8) | 0xFFF;
	if (io_range[0] != 0 && io_range[0] < io_range[1]) {
		range[range_idx].child_high = range[range_idx].parent_high = PCI_ADDR_IO | PCI_REG_REL_M;
		range[range_idx].child_mid = range[range_idx].parent_mid = 0;
		range[range_idx].child_low = range[range_idx].parent_low = io_range[0];
		range[range_idx].size_high = 0;
		range[range_idx].size_low = io_range[1] - io_range[0] + 1;
		range_idx++;
	}

	val = pci_conf_read16(conf_base, PCI_BCNF_MEM_BASE);
	mem_range[0] = ((val & 0xFFF0) << 16);
	val = pci_conf_read16(conf_base, PCI_BCNF_MEM_LIMIT);
	mem_range[1] = ((val & 0xFFF0) << 16) | 0xFFFFF;
	if (mem_range[0] != 0 && mem_range[0] < mem_range[1]) {
		range[range_idx].child_high = range[range_idx].parent_high = PCI_ADDR_MEM32 | PCI_REG_REL_M;
		range[range_idx].child_mid = range[range_idx].parent_mid = 0;
		range[range_idx].child_low = range[range_idx].parent_low = mem_range[0];
		range[range_idx].size_high = 0;
		range[range_idx].size_low = mem_range[1] - mem_range[0] + 1;
		range_idx++;
	}

	val = pci_conf_read16(conf_base, PCI_BCNF_PF_BASE_LOW);
	pmem_range[0] = ((val & 0xFFF0) << 16);
	val = pci_conf_read16(conf_base, PCI_BCNF_PF_LIMIT_LOW);
	pmem_range[1] = ((val & 0xFFF0) << 16) | 0xFFFFF;
	if (pmem_range[0] != 0 && pmem_range[0] < pmem_range[1]) {
		range[range_idx].child_high = range[range_idx].parent_high = PCI_ADDR_MEM32 | PCI_REG_REL_M | PCI_REG_PF_M;
		range[range_idx].child_mid = range[range_idx].parent_mid = 0;
		range[range_idx].child_low = range[range_idx].parent_low = pmem_range[0];
		range[range_idx].size_high = 0;
		range[range_idx].size_low = pmem_range[1] - pmem_range[0] + 1;
		range_idx++;
	}
	if (range_idx > 0)
		ndi_prop_update_int_array(DDI_DEV_T_NONE, dip,
		    "ranges", (int *)range, sizeof(ppb_ranges_t) * range_idx / sizeof (int));

	add_bus_range_prop(dip, secbus, subbus);
}
/*
 * Adjust the reg properties for a dual channel PCI-IDE device.
 *
 * NOTE: don't do anything that changes the order of the hard-decodes
 * and programmed BARs. The kernel driver depends on these values
 * being in this order regardless of whether they're for a 'native'
 * mode BAR or not.
 */
/*
 * config info for pci-ide devices
 */
static struct {
	uint8_t  native_mask;	/* 0 == 'compatibility' mode, 1 == native */
	uint8_t  bar_offset;	/* offset for alt status register */
	uint16_t addr;		/* compatibility mode base address */
	uint16_t length;	/* number of ports for this BAR */
} pciide_bar[] = {
	{ 0x01, 0, 0x1f0, 8 },	/* primary lower BAR */
	{ 0x01, 2, 0x3f6, 1 },	/* primary upper BAR */
	{ 0x04, 0, 0x170, 8 },	/* secondary lower BAR */
	{ 0x04, 2, 0x376, 1 }	/* secondary upper BAR */
};

static int
pciIdeAdjustBAR(uint8_t progcl, int index, uint32_t *basep, uint32_t *lenp)
{
	int hard_decode = 0;

	/*
	 * Adjust the base and len for the BARs of the PCI-IDE
	 * device's primary and secondary controllers. The first
	 * two BARs are for the primary controller and the next
	 * two BARs are for the secondary controller. The fifth
	 * and sixth bars are never adjusted.
	 */
	if (index >= 0 && index <= 3) {
		*lenp = pciide_bar[index].length;

		if (progcl & pciide_bar[index].native_mask) {
			*basep += pciide_bar[index].bar_offset;
		} else {
			*basep = pciide_bar[index].addr;
			hard_decode = 1;
		}
	}

	/*
	 * if either base or len is zero make certain both are zero
	 */
	if (*basep == 0 || *lenp == 0) {
		*basep = 0;
		*lenp = 0;
		hard_decode = 0;
	}

	return (hard_decode);
}

static void
add_reg_props(dev_info_t *dip, uintptr_t conf_base, uint8_t bus, uint8_t dev, uint8_t func, int pciide)
{
	uint8_t baseclass, subclass, progclass, header;
	uint16_t bar_sz;
	uint32_t value = 0, len, devloc;
	uint32_t base, base_hi;
	uint16_t offset, end;
	int max_basereg, j;
	uint32_t phys_hi;

	pci_regspec_t regs[16] = {{0}};
	pci_regspec_t assigned[15] = {{0}};
	int nreg, nasgn;

	devloc = ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8);
	regs[0].pci_phys_hi = devloc;
	nreg = 1;	/* rest of regs[0] is all zero */
	nasgn = 0;

	baseclass = pci_conf_read8(conf_base, PCI_CONF_BASCLASS);
	subclass = pci_conf_read8(conf_base, PCI_CONF_SUBCLASS);
	progclass = pci_conf_read8(conf_base, PCI_CONF_PROGCLASS);
	header = pci_conf_read8(conf_base, PCI_CONF_HEADER) & PCI_HEADER_TYPE_M;

	switch (header) {
	case PCI_HEADER_ZERO:
		max_basereg = PCI_BASE_NUM;
		break;
	case PCI_HEADER_PPB:
		max_basereg = PCI_BCNF_BASE_NUM;
		break;
	case PCI_HEADER_CARDBUS:
		max_basereg = PCI_CBUS_BASE_NUM;
		break;
	default:
		max_basereg = 0;
		break;
	}

	end = PCI_CONF_BASE0 + max_basereg * sizeof (uint32_t);
	for (j = 0, offset = PCI_CONF_BASE0; offset < end; j++, offset += bar_sz) {
		uint32_t command;

		/* determine the size of the address space */
		base = pci_conf_read32(conf_base, offset);
		if (baseclass != PCI_CLASS_BRIDGE) {
			command = pci_conf_read16(conf_base, PCI_CONF_COMM);
			pci_conf_write16(conf_base, PCI_CONF_COMM, command & ~(PCI_COMM_MAE | PCI_COMM_IO));
		}
		pci_conf_write32(conf_base, offset, 0xffffffff);
		value = pci_conf_read32(conf_base, offset);
		pci_conf_write32(conf_base, offset, base);
		if (baseclass != PCI_CLASS_BRIDGE)
			pci_conf_write16(conf_base, PCI_CONF_COMM, command);

		/* construct phys hi,med.lo, size hi, lo */
		if ((pciide && j < 4) || (base & PCI_BASE_SPACE_IO)) {
			int hard_decode = 0;
			/* i/o space */
			bar_sz = PCI_BAR_SZ_32;
			value &= PCI_BASE_IO_ADDR_M;
			len = value & -value;

			/* XXX Adjust first 4 IDE registers */
			if (pciide) {
				if (subclass != PCI_MASS_IDE)
					progclass = (PCI_IDE_IF_NATIVE_PRI | PCI_IDE_IF_NATIVE_SEC);
				hard_decode = pciIdeAdjustBAR(progclass, j, &base, &len);
			} else if (value == 0) {
				/* skip base regs with size of 0 */
				continue;
			}

			regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = len;

			regs[nreg].pci_phys_hi = PCI_ADDR_IO | devloc | (hard_decode ? PCI_RELOCAT_B : offset);
			regs[nreg].pci_phys_low = hard_decode ?  base & PCI_BASE_IO_ADDR_M : 0;
			assigned[nasgn].pci_phys_hi = PCI_RELOCAT_B | regs[nreg].pci_phys_hi;
			assigned[nasgn].pci_phys_low = base & PCI_BASE_IO_ADDR_M;
			nreg++, nasgn++;
		} else {
			/* memory space */
			if ((base & PCI_BASE_TYPE_M) == PCI_BASE_TYPE_ALL) {
				bar_sz = PCI_BAR_SZ_64;
				base_hi = pci_conf_read32(conf_base, offset + 4);
				phys_hi = PCI_ADDR_MEM64;
			} else {
				bar_sz = PCI_BAR_SZ_32;
				base_hi = 0;
				phys_hi = PCI_ADDR_MEM32;
			}

			/* skip base regs with size of 0 */
			value &= PCI_BASE_M_ADDR_M;

			if (value == 0)
				continue;

			len = value & -value;
			regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = len;

			phys_hi |= (devloc | offset);
			if (base & PCI_BASE_PREF_M)
				phys_hi |= PCI_PREFETCH_B;

			regs[nreg].pci_phys_hi = phys_hi;
			assigned[nasgn].pci_phys_hi = PCI_RELOCAT_B | regs[nreg].pci_phys_hi;
			assigned[nasgn].pci_phys_mid = base_hi;
			assigned[nasgn].pci_phys_low = base & PCI_BASE_M_ADDR_M;
			nreg++, nasgn++;
		}
	}
	switch (header) {
	case PCI_HEADER_ZERO:
		offset = PCI_CONF_ROM;
		break;
	case PCI_HEADER_PPB:
		offset = PCI_BCNF_ROM;
		break;
	default: /* including PCI_HEADER_CARDBUS */
		goto done;
	}

	base = pci_conf_read32(conf_base, offset);
	pci_conf_write32(conf_base, offset, PCI_BASE_ROM_ADDR_M);
	value = pci_conf_read32(conf_base, offset);
	pci_conf_write32(conf_base, offset, base);

	if (value & PCI_BASE_ROM_ENABLE)
		value &= PCI_BASE_ROM_ADDR_M;
	else
		value = 0;

	if (value != 0) {
		regs[nreg].pci_phys_hi = (PCI_ADDR_MEM32 | devloc) + offset;
		assigned[nasgn].pci_phys_hi = (PCI_RELOCAT_B | PCI_ADDR_MEM32 | devloc) + offset;
		base &= PCI_BASE_ROM_ADDR_M;
		assigned[nasgn].pci_phys_low = base;
		len = value & -value;
		regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = len;
		nreg++, nasgn++;
	}

	/*
	 * Account for "legacy" (alias) video adapter resources
	 */

	/* add the three hard-decode, aliased address spaces for VGA */
	if ((baseclass == PCI_CLASS_DISPLAY && subclass == PCI_DISPLAY_VGA) ||
	    (baseclass == PCI_CLASS_NONE && subclass == PCI_NONE_VGA)) {

		/* VGA hard decode 0x3b0-0x3bb */
		regs[nreg].pci_phys_hi = assigned[nasgn].pci_phys_hi = (PCI_RELOCAT_B | PCI_ALIAS_B | PCI_ADDR_IO | devloc);
		regs[nreg].pci_phys_low = assigned[nasgn].pci_phys_low = 0x3b0;
		regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = 0xc;
		nreg++, nasgn++;

		/* VGA hard decode 0x3c0-0x3df */
		regs[nreg].pci_phys_hi = assigned[nasgn].pci_phys_hi = (PCI_RELOCAT_B | PCI_ALIAS_B | PCI_ADDR_IO | devloc);
		regs[nreg].pci_phys_low = assigned[nasgn].pci_phys_low = 0x3c0;
		regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = 0x20;
		nreg++, nasgn++;

		/* Video memory */
		regs[nreg].pci_phys_hi = assigned[nasgn].pci_phys_hi = (PCI_RELOCAT_B | PCI_ALIAS_B | PCI_ADDR_MEM32 | devloc);
		regs[nreg].pci_phys_low = assigned[nasgn].pci_phys_low = 0xa0000;
		regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = 0x20000;
		nreg++, nasgn++;
	}

	/* add the hard-decode, aliased address spaces for 8514 */
	if ((baseclass == PCI_CLASS_DISPLAY) && (subclass == PCI_DISPLAY_VGA) && (progclass & PCI_DISPLAY_IF_8514)) {
		/* hard decode 0x2e8 */
		regs[nreg].pci_phys_hi = assigned[nasgn].pci_phys_hi = (PCI_RELOCAT_B | PCI_ALIAS_B | PCI_ADDR_IO | devloc);
		regs[nreg].pci_phys_low = assigned[nasgn].pci_phys_low = 0x2e8;
		regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = 0x1;
		nreg++, nasgn++;

		/* hard decode 0x2ea-0x2ef */
		regs[nreg].pci_phys_hi = assigned[nasgn].pci_phys_hi = (PCI_RELOCAT_B | PCI_ALIAS_B | PCI_ADDR_IO | devloc);
		regs[nreg].pci_phys_low = assigned[nasgn].pci_phys_low = 0x2ea;
		regs[nreg].pci_size_low = assigned[nasgn].pci_size_low = 0x6;
		nreg++, nasgn++;
	}

done:
	ndi_prop_update_int_array(DDI_DEV_T_NONE, dip, "reg", (int *)regs, nreg * sizeof (pci_regspec_t) / sizeof (int));
	ndi_prop_update_int_array(DDI_DEV_T_NONE, dip, "assigned-addresses", (int *)assigned, nasgn * sizeof (pci_regspec_t) / sizeof (int));
}




static struct modlmisc modlmisc = {
	&mod_miscops, "PCI BIOS interface"
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};

int
_init(void)
{
	int	err;

	if ((err = mod_install(&modlinkage)) != 0)
		return (err);

	impl_bus_add_probe(pci_enumerate);
	return (0);
}

int
_fini(void)
{
	int	err;

	if ((err = mod_remove(&modlinkage)) != 0)
		return (err);

	impl_bus_delete_probe(pci_enumerate);
	return (0);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

