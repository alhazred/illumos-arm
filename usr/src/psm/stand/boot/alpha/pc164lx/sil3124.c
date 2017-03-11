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
 */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/salib.h>
#include <sys/promif.h>
#include <sys/pci.h>
#include <util/sscanf.h>
#include "sil3124.h"
#include "pci_dev.h"
#include "prom_dev.h"
#include <sys/sata/sata_defs.h>
#include <sys/sata/adapters/si3124/si3124reg.h>

#define	SILICON_IMAGE_VENDOR_ID	0x1095

struct sil3124_dev {
	uintptr_t sictl_global_addr;
	uintptr_t sictl_port_addr;
	int port;
	int hose;
};

static void
mdelay(int t)
{
	uintptr_t e = prom_gettime() + t + 1;
	while (prom_gettime() <= e) {}
}

static void
wait_for_change(int hose, uintptr_t reg, uint32_t mask, uint32_t val, int timeout_msec)
{
	while ((pci_read32(hose, reg) & mask) == val && timeout_msec-- > 0) {
		mdelay(1);
	}
}

static int
sil_exec_cmd(struct sil3124_dev *ctlp, void *pcmd)
{
	uint64_t paddr = virt_to_pci(pcmd);
	uint32_t irq_mask = (INTR_COMMAND_COMPLETE | INTR_COMMAND_ERROR) << 16;

	pci_write32(ctlp->hose, PORT_INTERRUPT_STATUS(ctlp, ctlp->port), irq_mask);
	pci_write32(ctlp->hose, PORT_COMMAND_ACTIVATION(ctlp, ctlp->port, 0), (uint32_t)paddr);
	pci_write32(ctlp->hose, PORT_COMMAND_ACTIVATION(ctlp, ctlp->port, 0) + 4, (uint32_t)(paddr >> 32));

	wait_for_change(ctlp->hose, PORT_INTERRUPT_STATUS(ctlp, ctlp->port), irq_mask, 0, 10000);

	if ((pci_read32(ctlp->hose, PORT_INTERRUPT_STATUS(ctlp, ctlp->port)) >> 16) & INTR_COMMAND_COMPLETE) {
		return 0;
	} else {
		return -1;
	}
}

static uint64_t
sil_cmd_read(struct sil3124_dev *ctlp, uint64_t start, uint64_t blkcnt, caddr_t buffer)
{
	si_prb_t cmdb = {0};
	si_prb_t *prb = &cmdb;

	SET_PRB_CONTROL_PKT_READ(prb);
	SET_FIS_TYPE(prb->prb_fis, REGISTER_FIS_H2D);
	SET_FIS_PMP(prb->prb_fis, 0xf);
	SET_FIS_CDMDEVCTL(prb->prb_fis, 1);
	SET_FIS_COMMAND(prb->prb_fis, SATAC_READ_DMA);
	SET_FIS_DEV_HEAD(prb->prb_fis, (SATA_ADH_LBA | ((start >> 24) & 0xf)));
	SET_FIS_CYL_HI(prb->prb_fis,   ((start >> 16) & 0xff));
	SET_FIS_CYL_LOW(prb->prb_fis,  ((start >> 8) & 0xff));
	SET_FIS_SECTOR(prb->prb_fis,   (start & 0xff));
	SET_FIS_SECTOR_COUNT(prb->prb_fis, (blkcnt & 0xff));
	prb->prb_sge0.sge_addr = virt_to_pci(buffer);
	prb->prb_sge0.sge_data_count = blkcnt * SATA_DISK_SECTOR_SIZE;
	SET_SGE_TRM(prb->prb_sge0);

	return sil_exec_cmd(ctlp, &cmdb);
}

static int
sil_cmd_soft_reset(struct sil3124_dev *ctlp)
{
	pci_write32(ctlp->hose, PORT_STATUS(ctlp, ctlp->port), PORT_CONTROL_SET_BITS_PORT_INITIALIZE);
	wait_for_change(ctlp->hose, PORT_STATUS(ctlp, ctlp->port), PORT_CONTROL_SET_BITS_PORT_INITIALIZE, PORT_CONTROL_SET_BITS_PORT_INITIALIZE, 100);
	wait_for_change(ctlp->hose, PORT_STATUS(ctlp, ctlp->port), PORT_STATUS_BITS_PORT_READY, 0, 100);

	if ((pci_read32(ctlp->hose, PORT_STATUS(ctlp, ctlp->port)) & (PORT_CONTROL_SET_BITS_PORT_INITIALIZE | PORT_STATUS_BITS_PORT_READY)) != PORT_STATUS_BITS_PORT_READY)
		return -1;

	pci_write32(ctlp->hose, PORT_INTERRUPT_ENABLE_CLEAR(ctlp, ctlp->port), INTR_COMMAND_COMPLETE | INTR_COMMAND_ERROR);

	si_prb_t cmdb = {0};
	si_prb_t *prb = &cmdb;

	SET_PRB_CONTROL_SOFT_RESET(prb);
	SET_FIS_TYPE(prb->prb_fis, REGISTER_FIS_H2D);
	SET_FIS_PMP(prb->prb_fis, 0xf);

	return sil_exec_cmd(ctlp, prb);
}


#define SIL3124_DEVS_MAX 8
static int sil3124_devs = 0;
struct {
	struct sil3124_dev *ctlp;
} sil3124_ctrb[SIL3124_DEVS_MAX];

static int
sil3124_open(const char *path)
{
	unsigned hose;
	unsigned vid;
	unsigned did;
	unsigned device;
	unsigned disk;

	if (sil3124_devs >= SIL3124_DEVS_MAX)
		return -1;

	if (sscanf(path, "/pci@%d,0/pci%x,%x@%x,0/disk@%x,0:a", &hose, &vid, &did, &device, &disk) != 5 &&
	    sscanf(path, "/pci@%d,0/pci%x,%x@%x,0/sd@%x,0:a", &hose, &vid, &did, &device, &disk) != 5) {
		return -1;
	}

	int bus  = (device >> 16) & 0xFF;
	int slot = (device >> 11) & 0x1F;
	int func = (device >>  8) & 0x07;
	uintptr_t conf_base = get_config_base(hose, bus, slot, func);

	struct sil3124_dev *ctlp = kmem_alloc(sizeof(struct sil3124_dev), 0);
	if (ctlp == NULL)
		return -1;

	ctlp->sictl_global_addr = (pci_conf_read32(conf_base, PCI_CONF_BASE0) & 0xffffff80);
	ctlp->sictl_port_addr = (pci_conf_read32(conf_base, PCI_CONF_BASE2) & 0xfffffc00);
	ctlp->port = disk;
	ctlp->hose = hose;

	if (pci_conf_read16(conf_base, PCI_CONF_VENID) != SILICON_IMAGE_VENDOR_ID ||
	    pci_conf_read16(conf_base, PCI_CONF_DEVID) != SI3124_DEV_ID) {
		return -1;
	}
	if (pci_conf_read16(conf_base, PCI_CONF_SUBVENID) != vid ||
	    pci_conf_read16(conf_base, PCI_CONF_SUBSYSID) != did) {
		return -1;
	}

	uint16_t comm = pci_conf_read16(conf_base, PCI_CONF_COMM);
	comm |= (PCI_COMM_ME | PCI_COMM_SERR_ENABLE | PCI_COMM_PARITY_DETECT | PCI_COMM_MAE);
	comm &= ~PCI_COMM_IO;
	pci_conf_write16(conf_base, PCI_CONF_COMM, comm);

	pci_write32(ctlp->hose, GLOBAL_CONTROL_REG(ctlp), 0);

	pci_write32(ctlp->hose, PORT_CONTROL_CLEAR(ctlp, ctlp->port), PORT_CONTROL_CLEAR_BITS_PORT_RESET);
	wait_for_change(ctlp->hose, PORT_STATUS(ctlp, ctlp->port), PORT_CONTROL_SET_BITS_PORT_RESET, PORT_CONTROL_SET_BITS_PORT_RESET, 100);
	if (pci_read32(ctlp->hose, PORT_STATUS(ctlp, ctlp->port)) & PORT_CONTROL_SET_BITS_PORT_RESET) {
		return -1;
	}

	for (int cnt = 0; cnt < 100; cnt++) {
		if ((pci_read32(ctlp->hose, PORT_SSTATUS(ctlp, ctlp->port)) & 0xF) == 0x3)
			break;
		mdelay(1);
	}

	if ((pci_read32(ctlp->hose, PORT_SSTATUS(ctlp, ctlp->port)) & 0xf) != 0x3) {
		return -1;
	}

	wait_for_change(ctlp->hose, PORT_STATUS(ctlp, ctlp->port), PORT_STATUS_BITS_PORT_READY, PORT_STATUS_BITS_PORT_READY, 100);
	if ((pci_read32(ctlp->hose, PORT_STATUS(ctlp, ctlp->port)) & PORT_STATUS_BITS_PORT_READY) != PORT_STATUS_BITS_PORT_READY) {
		return -1;
	}

	pci_write32(ctlp->hose, PORT_CONTROL_CLEAR(ctlp, ctlp->port), PORT_CONTROL_CLEAR_BITS_INTR_NCoR);
	pci_write32(ctlp->hose, PORT_CONTROL_CLEAR(ctlp, ctlp->port), PORT_CONTROL_SET_BITS_PM_ENABLE | PORT_CONTROL_SET_BITS_RESUME);

	pci_write32(ctlp->hose, PORT_STATUS(ctlp, ctlp->port), PORT_CONTROL_SET_BITS_DEV_RESET);
	wait_for_change(ctlp->hose, PORT_STATUS(ctlp, ctlp->port), PORT_CONTROL_SET_BITS_DEV_RESET, PORT_CONTROL_SET_BITS_DEV_RESET, 100);
	if (pci_read32(ctlp->hose, PORT_STATUS(ctlp, ctlp->port)) & PORT_CONTROL_SET_BITS_DEV_RESET) {
		return -1;
	}
	pci_write32(ctlp->hose, GLOBAL_CONTROL_REG(ctlp),
	    pci_read32(ctlp->hose, GLOBAL_CONTROL_REG(ctlp)) | (1 << disk));

	if (sil_cmd_soft_reset(ctlp) < 0)
		return -1;

	sil3124_ctrb[sil3124_devs].ctlp = ctlp;

	return sil3124_devs++;
}

static int
sil3124_close(int dev)
{
	if (!sil3124_ctrb[dev].ctlp)
		return -1;
	sil3124_ctrb[dev].ctlp = NULL;
	return 0;
}

static ssize_t
sil3124_read(int dev, caddr_t addr, size_t len, uint_t blknr)
{
	struct sil3124_dev *ctlp = sil3124_ctrb[dev].ctlp;
	uint_t start = blknr;
	uint_t blks = len / SATA_DISK_SECTOR_SIZE;
	do {
		uint_t num = (blks > 0xff)? 0xff: blks;
		if (sil_cmd_read(ctlp, start, num, addr))
			return -1;
		start += num;
		blks -= num;
		addr += SATA_DISK_SECTOR_SIZE * num;
	} while (blks != 0);

	return len;
}

static int
sil3124_match(const char *path)
{
	unsigned hose;
	unsigned vid;
	unsigned did;
	unsigned device;
	unsigned disk;

	if (sscanf(path, "/pci@%d,0/pci%x,%x@%x,0/disk@%x,0:a", &hose, &vid, &did, &device, &disk) != 5 &&
	    sscanf(path, "/pci@%d,0/pci%x,%x@%x,0/sd@%x,0:a", &hose, &vid, &did, &device, &disk) != 5)
		return 0;
	if (vid != SILICON_IMAGE_VENDOR_ID)
		return 0;
	return 1;
}

static struct prom_dev sil3124_device =
{
	.match = sil3124_match,
	.open = sil3124_open,
	.read = sil3124_read,
	.close = sil3124_close,
};

void init_sil3124(void)
{
	prom_register(&sil3124_device);
}

