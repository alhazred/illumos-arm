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
#include <sys/types.h>
#include <sys/promif.h>
#include <sys/salib.h>
#include <sys/systm.h>
#include <sys/byteorder.h>
#include <alloca.h>
#include "pci_dev.h"
#include "boot_plat.h"
#include "../../../../../common/pci/pci_strings.h"

#define	PYXIS_PCI_WnBASE(n)	(*(volatile uint32_t *)(0xfffffc8760000400UL + 0x100 * (n)))
#define	PYXIS_PCI_WnMASK(n)	(*(volatile uint32_t *)(0xfffffc8760000440UL + 0x100 * (n)))
#define	PYXIS_PCI_TnBASE(n)	(*(volatile uint32_t *)(0xfffffc8760000480UL + 0x100 * (n)))
#define	PYXIS_PCI_TBIA		(*(volatile uint32_t *)0xfffffc8760000100UL)
#define	PYXIS_CTRL		(*(volatile uint32_t *)0xfffffc8740000100UL)
#define	PYXIS_CTRL_PCI_LOOP_EN	(1<<2)
#define	PYXIS_PCI_DENSE(x)	(*(volatile uint32_t *)(0xfffffc8600000000UL + (x)))


static const pci_ranges_t pyxis_base[] = {
	{PCI_ADDR_CONFIG, 0, 0x00000000, 0x8a, 0x00000000, 0, 0x00010000},
	{PCI_ADDR_CONFIG, 0, 0x00010000, 0x8b, 0x00010000, 0, 0x00ff0000},
	{PCI_ADDR_IO,     0, 0, 0x89, 0, 0, 0x01000000},
	{PCI_ADDR_MEM32,  0, 0, 0x88, 0, 1, 0x00000000},
	{PCI_ADDR_MEM64,  0, 0, 0x88, 0, 1, 0x00000000}
};
#if 0
static int get_pci_numhose()
{
	return 1;
}
#endif
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
uintptr_t get_config_base(int hose, int bus, int dev, int func)
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

uint8_t pci_conf_read8(uintptr_t conf_base, int offset)
{
	return *(volatile uint8_t *)(conf_base | offset);
}

uint16_t pci_conf_read16(uintptr_t conf_base, int offset)
{
	return *(volatile uint16_t *)(conf_base | offset);
}

uint32_t pci_conf_read32(uintptr_t conf_base, int offset)
{
	return *(volatile uint32_t *)(conf_base | offset);
}

void pci_conf_write8(uintptr_t conf_base, int offset, uint8_t v)
{
	*(volatile uint8_t *)(conf_base | offset) = v;
}

void pci_conf_write16(uintptr_t conf_base, int offset, uint16_t v)
{
	*(volatile uint16_t *)(conf_base | offset) = v;
}

void pci_conf_write32(uintptr_t conf_base, int offset, uint32_t v)
{
	*(volatile uint32_t *)(conf_base | offset) = v;
}

uint8_t pci_read8(int hose, uintptr_t addr)
{
	return *(volatile uint8_t *)(0xfffffc0000000000 + 0x8800000000 + addr);
}

uint16_t pci_read16(int hose, uintptr_t addr)
{
	return *(volatile uint16_t *)(0xfffffc0000000000 + 0x8800000000 + addr);
}

uint32_t pci_read32(int hose, uintptr_t addr)
{
	return *(volatile uint32_t *)(0xfffffc0000000000 + 0x8800000000 + addr);
}

void pci_write8(int hose, uintptr_t addr,  uint8_t v)
{
	*(volatile uint8_t *)(0xfffffc0000000000 + 0x8800000000 + addr) = v;
}

void pci_write16(int hose, uintptr_t addr, uint16_t v)
{
	*(volatile uint16_t *)(0xfffffc0000000000 + 0x8800000000 + addr) = v;
}

void pci_write32(int hose, uintptr_t addr, uint32_t v)
{
	*(volatile uint32_t *)(0xfffffc0000000000 + 0x8800000000 + addr) = v;
}

uint64_t virt_to_pci(void *va)
{
	uintptr_t addr = (uintptr_t)va;
	uint64_t pa;
	if (0xfffffc0000000000 <= addr && addr < 0xfffffe0000000000) {
		pa = (addr - 0xfffffc0000000000);
	} else {
		uint64_t *ptb = (uint64_t *)(VPT_BASE | (addr >> 10));
		pa = ((((*ptb) >> 32) << 13) | (addr & 0x1fff));
	}
	return (pa + 0x40000000);
}

#if 0
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

static void
add_model_prop(pnode_t node, uint32_t classcode)
{
	const char *str;
	int i;
	uint8_t baseclass = classcode >> 16;
	uint8_t subclass = (classcode >> 8) & 0xff;
	uint8_t progclass = classcode & 0xff;
	extern const struct pci_class_strings_s class_pci[];
	extern int class_pci_items;

	if ((baseclass == PCI_CLASS_MASS) && (subclass == PCI_MASS_IDE)) {
		str = "IDE controller";
	} else {
		for (i = 0; i < class_pci_items; i++) {
			if ((baseclass == class_pci[i].base_class) &&
			    (subclass == class_pci[i].sub_class) &&
			    (progclass == class_pci[i].prog_class)) {
				str = class_pci[i].actual_desc;
				break;
			}
		}
		if (i == class_pci_items)
			str = "Unknown class of pci/pnpbios device";
	}

	prom_setprop(node, "model", (caddr_t)str, strlen(str) + 1);
}

#define	COMPAT_BUFSIZE	512
static void
add_compatible(pnode_t node, uint16_t subvenid, uint16_t subdevid,
    uint16_t vendorid, uint16_t deviceid, uint8_t revid, uint32_t classcode)
{
	char *buf, *curr;

	curr = buf = alloca(COMPAT_BUFSIZE);

	if (subvenid) {
		sprintf(curr, "pci%x,%x.%x.%x.%x", vendorid, deviceid, subvenid, subdevid, revid);
		curr += strlen(curr) + 1;

		sprintf(curr, "pci%x,%x.%x.%x", vendorid, deviceid, subvenid, subdevid);
		curr += strlen(curr) + 1;

		sprintf(curr, "pci%x,%x", subvenid, subdevid);
		curr += strlen(curr) + 1;
	}
	sprintf(curr, "pci%x,%x.%x", vendorid, deviceid, revid);
	curr += strlen(curr) + 1;

	sprintf(curr, "pci%x,%x", vendorid, deviceid);
	curr += strlen(curr) + 1;

	sprintf(curr, "pciclass,%06x", classcode);
	curr += strlen(curr) + 1;

	sprintf(curr, "pciclass,%04x", (classcode >> 8));
	curr += strlen(curr) + 1;

	prom_setprop(node, "compatible", (caddr_t)buf, curr - buf);
}

static void pci_scan_bus(pnode_t pci_bus, int hose, int bus);

static void
found_dev(pnode_t parent, int hose, int bus, int dev, int func)
{
	uint8_t header, revid, basecls, subcls;
	uint16_t vid, did, svid, sdid;
	uint32_t clscode;
	char nodename[32], unitaddr[5];
	uintptr_t conf_base = get_config_base(hose, bus, dev, func);

	const char *str;

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
	revid = pci_conf_read8(conf_base, PCI_CONF_REVID);

	if (is_display(clscode))
		snprintf(nodename, sizeof(nodename), "display");
	else if (is_isa(vid, did, clscode))
		snprintf(nodename, sizeof(nodename), "isa");
	else if (svid != 0)
		snprintf(nodename, sizeof(nodename), "pci%x,%x", svid, sdid);
	else
		snprintf(nodename, sizeof(nodename), "pci%x,%x", vid, did);

	pnode_t node = prom_add_subnode(parent, nodename);

	str = unitaddr;
	prom_setprop(node, "unit-address", (caddr_t)str, strlen(str) + 1);

	set_devpm_d0(hose, bus, dev, func);

	add_model_prop(node, clscode);
	add_compatible(node, svid, sdid, vid, did, revid, clscode);

	str = "ok";
	prom_setprop(node, "status", (caddr_t)str, strlen(str) + 1);

	if ((basecls == PCI_CLASS_BRIDGE) && (subcls == PCI_BRIDGE_PCI)) {
		str = "pci";
		prom_setprop(node, "device_type", (caddr_t)str, strlen(str) + 1);
	}
	if (is_isa(vid, did, clscode)) {
		str = "isa";
		prom_setprop(node, "device_type", (caddr_t)str, strlen(str) + 1);
	}

	if ((basecls == PCI_CLASS_BRIDGE) && (subcls == PCI_BRIDGE_PCI)) {
		pci_scan_bus(node, hose, pci_conf_read8(conf_base, PCI_BCNF_SECBUS));
	}
}

static void
pci_scan_bus(pnode_t pci_bus, int hose, int bus)
{
	for (int dev = 0; dev < PCI_MAX_DEVICES; dev++) {
		int func, nfunc = 1;
		for (func = 0; func < nfunc; func++) {
			uintptr_t conf_base = get_config_base(hose, bus, dev, func);
			if (pci_conf_read32(conf_base, PCI_CONF_VENID) == 0xffffffff)
				continue;
			found_dev(pci_bus, hose, bus, dev, func);
			if (func == 0 &&
			    (pci_conf_read8(conf_base, PCI_CONF_HEADER) & PCI_HEADER_MULTI))
				nfunc = PCI_MAX_FUNCTIONS;
		}
	}
}

void
prom_pci_setup(void)
{
	for (int hose = 0; hose < get_pci_numhose(); hose++)
	{
		const char *str;
		uint32_t val;

		prom_printf("%s: hose%d Probe\n", __FUNCTION__, hose);

		char buf[80];
		sprintf(buf, "pci@%d", hose);
		prom_add_subnode(prom_rootnode(), buf);
		pnode_t pci_bus = prom_finddevice(buf);
		val = TO_FDT_ENDIAN(hose);
		prom_setprop(pci_bus, "hose", (caddr_t)&val, sizeof(val));
		str = "pci";
		prom_setprop(pci_bus, "device_type", (caddr_t)str, strlen(str) + 1);
		str = "ok";
		prom_setprop(pci_bus, "status", (caddr_t)str, strlen(str) + 1);

		pci_scan_bus(pci_bus, hose, 0);
	}
}
#endif
