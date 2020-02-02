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
 * Copyright 2019 Hayashi Naoyuki
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/promif.h>
#include <sys/salib.h>
#include <sys/byteorder.h>
#include <sys/sysmacros.h>

#include <sys/miiregs.h>
#include <sys/ethernet.h>
#include <sys/gpio.h>
#include <sys/platmod.h>
#include "prom_dev.h"
#include "gem.h"
#include "gemreg.h"
#include "boot_plat.h"

#define TX_DESC_NUM 32
#define RX_DESC_NUM 48

#define ENET_ALIGN  DCACHE_LINE
#define BUFFER_SIZE 1536

#define DEFAULT_MAC_ADDRESS {0x70, 0xb3, 0xd5, 0x92, 0xf0, 0x95}

struct gem_desc
{
	uint32_t addr;
	uint32_t ctrl;
};
struct gem_desc64
{
	uint32_t addr;
	uint32_t ctrl;
	uint32_t addrhi;
	uint32_t rsv;
};


struct gem_sc
{
	uintptr_t base;
	uint8_t mac_addr[6];
	int phy_id;
	int phy_speed;
	int phy_fullduplex;
	bool is64b_desc;

	uint32_t nwctrl;
	uintptr_t tx_desc_base;
	uintptr_t rx_desc_base;
	int tx_index;
	int rx_head;
	caddr_t tx_buffer;
	caddr_t rx_buffer;
	paddr_t tx_buffer_phys;
	paddr_t tx_desc_phys;
	paddr_t rx_desc_phys;
	paddr_t rx_buffer_phys;
};

static struct gem_sc *gem_dev[3];

static void
gem_reg_write(struct gem_sc *sc, int index, uint32_t val)
{
	*((volatile uint32_t *)sc->base + index) = val;
}

static uint32_t
gem_reg_read(struct gem_sc *sc, int index)
{
	return *((volatile uint32_t *)sc->base + index);
}

static struct gem_desc *
gem_get_tx_desc(struct gem_sc *sc, int index)
{
	return (struct gem_desc *)(sc->tx_desc_base + index * ((sc->is64b_desc)? sizeof(struct gem_desc64): sizeof(struct gem_desc)));
}
static struct gem_desc *
gem_get_rx_desc(struct gem_sc *sc, int index)
{
	return (struct gem_desc *)(sc->rx_desc_base + index * ((sc->is64b_desc)? sizeof(struct gem_desc64): sizeof(struct gem_desc)));
}
static void
gem_chip_reset(struct gem_sc *sc)
{
	gem_reg_write(sc, GEM_NWCTRL, 0);
	gem_reg_write(sc, GEM_NWCFG, 0);
	gem_reg_write(sc, GEM_NWCTRL, GEM_NWCTRL_CLRSTAT);
	gem_reg_write(sc, GEM_TXSTATUS, gem_reg_read(sc, GEM_TXSTATUS));
	gem_reg_write(sc, GEM_RXSTATUS, gem_reg_read(sc, GEM_RXSTATUS));
	gem_reg_write(sc, GEM_IDR, ~0u);
	gem_reg_write(sc, GEM_HASHLO, 0);
	gem_reg_write(sc, GEM_HASHHI, 0);
	gem_reg_write(sc, GEM_RXQBASE, 0);
	gem_reg_write(sc, GEM_RBQPH, 0);
	gem_reg_write(sc, GEM_TXQBASE, 0);
	gem_reg_write(sc, GEM_TBQPH, 0);
	gem_reg_write(sc, GEM_NWCFG, GEM_NWCFG_MDC_CLK_DIV96);
	sc->nwctrl = GEM_NWCTRL_MPENA;
	gem_reg_write(sc, GEM_NWCTRL, sc->nwctrl);
	sc->is64b_desc = ((gem_reg_read(sc, GEM_DESCONF6) & GEM_DESCONF6_64B_MASK) != 0);
}
static uint16_t
gem_mii_read(struct gem_sc *sc, int offset)
{
	if (!(gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE))
		return 0xffff;

	gem_reg_write(sc, GEM_PHYMNTNC,
	    GEM_PHYMNTNC_SOF |
	    GEM_PHYMNTNC_CODE |
	    GEM_PHYMNTNC_OP_R |
	    (sc->phy_id << GEM_PHYMNTNC_ADDR_SHFT) |
	    (offset << GEM_PHYMNTNC_REG_SHIFT)
	    );

	while (!(gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE)) {}

	return gem_reg_read(sc, GEM_PHYMNTNC) & 0xffff;
}

static void
gem_mii_write(struct gem_sc *sc, int offset, uint16_t val)
{
	if (!(gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE))
		return;

	gem_reg_write(sc, GEM_PHYMNTNC,
	    GEM_PHYMNTNC_SOF |
	    GEM_PHYMNTNC_CODE |
	    GEM_PHYMNTNC_OP_W |
	    (sc->phy_id << GEM_PHYMNTNC_ADDR_SHFT) |
	    (offset << GEM_PHYMNTNC_REG_SHIFT) |
	    val
	    );

	while (!(gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE)) {}
}

static void
gem_media_update(struct gem_sc *sc)
{
	uint32_t netcfg = gem_reg_read(sc, GEM_NWCFG);
	switch (sc->phy_speed) {
	case 1000:
		netcfg |= GEM_NWCFG_GIGA;
		netcfg |= GEM_NWCFG_100BASE;
		break;
	case 100:
		netcfg &= ~GEM_NWCFG_GIGA;
		netcfg |= GEM_NWCFG_100BASE;
		break;
	case 10:
	default:
		netcfg &= ~GEM_NWCFG_GIGA;
		netcfg &= ~GEM_NWCFG_100BASE;
		break;
	}
	if (sc->phy_fullduplex)
		netcfg |= GEM_NWCFG_FDX;
	else
		netcfg &= ~GEM_NWCFG_FDX;
	gem_reg_write(sc, GEM_NWCFG, netcfg);
}

static int
gem_phy_reset(struct gem_sc *sc)
{
	uint16_t advert = gem_mii_read(sc, MII_AN_ADVERT) & 0x1F;
	advert |= MII_ABILITY_100BASE_TX_FD;
	advert |= MII_ABILITY_100BASE_TX;
	advert |= MII_ABILITY_10BASE_T_FD;
	advert |= MII_ABILITY_10BASE_T;
	uint16_t gigctrl =  MII_MSCONTROL_1000T_FD | MII_MSCONTROL_1000T;

	gem_mii_write(sc, MII_AN_ADVERT, advert);
	gem_mii_write(sc, MII_MSCONTROL, gigctrl);

	uint16_t bmcr = MII_CONTROL_ANE | MII_CONTROL_RSAN | MII_CONTROL_1GB | MII_CONTROL_FDUPLEX;
	gem_mii_write(sc, MII_CONTROL, bmcr);

	int i;
	uint16_t bmsr = 0;
	for (i = 0; i < 10000; i++) {
		uint_t s = prom_gettime();
		while (prom_gettime() < s + 2) {}
		bmsr = gem_mii_read(sc, MII_STATUS);
		if (bmsr == 0xffff)
			continue;
		if (bmsr & MII_STATUS_LINKUP)
			break;
	}
	if (i == 10000 || !(bmsr & MII_STATUS_LINKUP))
		return -1;

	uint16_t lpar = gem_mii_read(sc, MII_AN_LPABLE);
	uint16_t msstat = gem_mii_read(sc, MII_MSSTATUS);
	if (msstat & MII_MSSTATUS_LP1000T_FD) {
		sc->phy_speed = 1000;
		sc->phy_fullduplex = 1;
	} else if (msstat & MII_MSSTATUS_LP1000T) {
		sc->phy_speed = 1000;
		sc->phy_fullduplex = 0;
	} else if (lpar & MII_ABILITY_100BASE_TX_FD) {
		sc->phy_speed = 100;
		sc->phy_fullduplex = 1;
	} else if (lpar & MII_ABILITY_100BASE_TX) {
		sc->phy_speed = 100;
		sc->phy_fullduplex = 0;
	} else if (lpar & MII_ABILITY_10BASE_T_FD) {
		sc->phy_speed = 10;
		sc->phy_fullduplex = 1;
	} else if (lpar & MII_ABILITY_10BASE_T) {
		sc->phy_speed = 10;
		sc->phy_fullduplex = 0;
	} else {
		sc->phy_speed = 0;
		sc->phy_fullduplex = 0;
	}
	gem_media_update(sc);

	return 0;
}

static int
gem_setup_buffer(struct gem_sc *sc)
{
	size_t size = 0;
	size_t desc_size = ((sc->is64b_desc)? sizeof(struct gem_desc64): sizeof(struct gem_desc));
	size += desc_size * TX_DESC_NUM;
	size += desc_size * RX_DESC_NUM;
	size += BUFFER_SIZE * TX_DESC_NUM;
	size += BUFFER_SIZE * RX_DESC_NUM;

	size_t alloc_size = size + 2 * MMU_PAGESIZE;
	uintptr_t orig_addr = (uintptr_t)kmem_alloc(alloc_size, 0);
	uintptr_t buf_addr = roundup(orig_addr, MMU_PAGESIZE);
	size_t buf_size = roundup(size, MMU_PAGESIZE);
	uintptr_t buf_vaddr = memlist_get(buf_size, MMU_PAGESIZE, &ptmplistp);

	map_phys(PTE_A | PTE_D | PTE_G | PTE_W | PTE_R, (caddr_t)buf_vaddr, buf_addr, buf_size);
	size_t offset = 0;
	sc->tx_desc_phys = (paddr_t)(buf_addr + offset);
	sc->tx_desc_base = buf_vaddr + offset;
	offset += desc_size * TX_DESC_NUM;

	sc->rx_desc_phys = (paddr_t)(buf_addr + offset);
	sc->rx_desc_base = buf_vaddr + offset;
	offset += desc_size * RX_DESC_NUM;

	sc->tx_buffer_phys = (paddr_t)(buf_addr + offset);
	sc->tx_buffer = (caddr_t)(buf_vaddr + offset);
	offset += BUFFER_SIZE * TX_DESC_NUM;

	sc->rx_buffer_phys = (paddr_t)(buf_addr + offset);
	sc->rx_buffer = (caddr_t)(buf_vaddr + offset);
	offset += BUFFER_SIZE * RX_DESC_NUM;

	memset((void *)sc->tx_desc_base, 0, desc_size * TX_DESC_NUM);
	memset((void *)sc->rx_desc_base, 0, desc_size * RX_DESC_NUM);
	memset((void *)sc->tx_buffer, 0, BUFFER_SIZE * TX_DESC_NUM);
	memset((void *)sc->rx_buffer, 0, BUFFER_SIZE * RX_DESC_NUM);

	for (int i = 0; i < TX_DESC_NUM; i++) {
		struct gem_desc *desc = gem_get_tx_desc(sc, i);
		desc->addr = sc->tx_buffer_phys + i * BUFFER_SIZE;
		desc->ctrl = DESC_1_USED;
		if (i == TX_DESC_NUM - 1)
			desc->ctrl |= DESC_1_TX_WRAP;
		if (sc->is64b_desc) {
			((struct gem_desc64 *)desc)->addrhi = (sc->tx_buffer_phys >> 32);
			((struct gem_desc64 *)desc)->rsv = 0;
		}
	}
	for (int i = 0; i < RX_DESC_NUM; i++) {
		struct gem_desc *desc = gem_get_rx_desc(sc, i);
		desc->addr = sc->rx_buffer_phys + i * BUFFER_SIZE;
		if (i == RX_DESC_NUM - 1)
			desc->addr |= DESC_0_RX_WRAP;
		desc->ctrl = 0;
		if (sc->is64b_desc) {
			((struct gem_desc64 *)desc)->addrhi = (sc->rx_buffer_phys >> 32);
			((struct gem_desc64 *)desc)->rsv = 0;
		}
	}

	return 0;
}

static int
gem_match(const char *name)
{
	pnode_t node = prom_finddevice(name);
	if (node <= 0)
		return 0;
	if (prom_is_compatible(node, "sifive,fu540-c000-gem"))
		return 1;
	return 0;
}
static int
gem_open(const char *name)
{
	if (prom_finddevice(name) <= 0)
		return -1;
	if (!prom_is_compatible(prom_finddevice(name), "sifive,fu540-c000-gem"))
		return -1;

	int fd;

	for (fd = 0; fd < sizeof(gem_dev) / sizeof(gem_dev[0]); fd++) {
		if (gem_dev[fd] == NULL)
			break;
	}
	if (fd == sizeof(gem_dev) / sizeof(gem_dev[0]))
		return -1;
	struct gem_sc *sc = kmem_alloc(sizeof(struct gem_sc), 0);
	memset(sc, 0, sizeof(struct gem_sc));

	if (prom_getproplen(prom_finddevice(name), "local-mac-address") == 6) {
		prom_getprop(prom_finddevice(name), "local-mac-address", (caddr_t)sc->mac_addr);
	}
	int i = 0;
	for (; i < 6; i++)
		if (sc->mac_addr[i] != 0)
			break;
	if (i == 6) {
		uint8_t default_macaddr[6] = DEFAULT_MAC_ADDRESS;
		memcpy(sc->mac_addr, default_macaddr, 6);
		prom_setprop(prom_finddevice(name), "local-mac-address", (caddr_t)sc->mac_addr, 6);
	}

	uint64_t regbase;
	uint64_t regsize;
	if (prom_get_reg_address(prom_finddevice(name), 0, &regbase) != 0)
		return -1;
	if (prom_get_reg_size(prom_finddevice(name), 0, &regsize) != 0)
		return -1;

	regsize = roundup(regsize, MMU_PAGESIZE);
	sc->base = memlist_get(regsize, MMU_PAGESIZE, &ptmplistp);
	map_phys(PTE_A | PTE_D | PTE_G | PTE_W | PTE_R, (caddr_t)sc->base, regbase, regsize);

	pnode_t child = prom_childnode(prom_finddevice(name));
	if (child > 0) {
		int len = prom_getproplen(child, "reset-gpios");
		if (len > 0) {
			uint32_t *gpio = (uint32_t *)__builtin_alloca(len);
			prom_getprop(child, "reset-gpios", (caddr_t)gpio);
			struct gpio_ctrl gpioc;
			gpioc.node = prom_findnode_by_phandle(ntohl(gpio[0]));
			gpioc.pin = ntohl(gpio[1]);
			plat_gpio_direction_output(&gpioc, htonl(gpio[2]));
		}

		uint64_t phy_id;
		if (prom_get_reg_address(child, 0, &phy_id) != 0) {
			sc->phy_id = phy_id;
		}
	}

	gem_chip_reset(sc);

	if (gem_phy_reset(sc) < 0) {
		return -1;
	}
	if (gem_setup_buffer(sc) < 0) {
		return -1;
	}

	uint32_t dmacfg = gem_reg_read(sc, GEM_DMACFG);
	if (sc->is64b_desc)
		dmacfg |= GEM_DMACFG_ADDR_64B;
	dmacfg |= (((BUFFER_SIZE / GEM_DMACFG_RBUFSZ_MUL) << GEM_DMACFG_RBUFSZ_S) & GEM_DMACFG_RBUFSZ_M);
	gem_reg_write(sc, GEM_DMACFG, dmacfg);

	gem_reg_write(sc, GEM_RXQBASE, sc->rx_desc_phys);
	gem_reg_write(sc, GEM_RBQPH, sc->rx_desc_phys >> 32);
	gem_reg_write(sc, GEM_TXQBASE, sc->tx_desc_phys);
	gem_reg_write(sc, GEM_TBQPH, sc->tx_desc_phys >> 32);

	gem_reg_write(sc, GEM_SPADDR1LO, (sc->mac_addr[3] << 24) | (sc->mac_addr[2] << 16) | (sc->mac_addr[1] << 8) | sc->mac_addr[0]);
	gem_reg_write(sc, GEM_SPADDR1HI, (sc->mac_addr[5] << 8) | sc->mac_addr[4]);

	sc->nwctrl |= GEM_NWCTRL_TXENA;
	sc->nwctrl |= GEM_NWCTRL_RXENA;
	gem_reg_write(sc, GEM_NWCTRL, sc->nwctrl);

	char *str;
	str = "bootp";
	prom_setprop(prom_chosennode(), "net-config-strategy", (caddr_t)str, strlen(str) + 1);
	str = "ethernet,100,rj45,full";
	prom_setprop(prom_chosennode(), "network-interface-type", (caddr_t)str, strlen(str) + 1);
	str = "Ethernet controller";
	prom_setprop(prom_finddevice(name), "model", (caddr_t)str, strlen(str) + 1);
	str = "okay";
	prom_setprop(prom_finddevice(name), "status", (caddr_t)str, strlen(str) + 1);

	gem_dev[fd] = sc;
	return fd;
}

static int
gem_close(int dev)
{
	if (!(0 <= dev && dev < sizeof(gem_dev) / sizeof(gem_dev[0])))
		return -1;

	struct gem_sc *sc = gem_dev[dev];
	if (!sc)
		return -1;

	gem_chip_reset(sc);

	gem_dev[dev] = NULL;
	return 0;
}

static int
gem_getmacaddr(ihandle_t dev, caddr_t ea)
{
	if (!(0 <= dev && dev < sizeof(gem_dev) / sizeof(gem_dev[0])))
		return -1;

	struct gem_sc *sc = gem_dev[dev];
	if (!sc)
		return -1;
	memcpy(ea, sc->mac_addr, 6);
	return 0;
}

static ssize_t
gem_send(int dev, caddr_t data, size_t packet_length, uint_t startblk)
{
	if (!(0 <= dev && dev < sizeof(gem_dev) / sizeof(gem_dev[0])))
		return -1;

	struct gem_sc *sc = gem_dev[dev];
	if (!sc)
		return -1;

	int index = sc->tx_index;
	volatile struct gem_desc *tx_desc = (volatile struct gem_desc *)gem_get_tx_desc(sc, index);
	uint32_t ctrl = tx_desc->ctrl;
	while ((ctrl & DESC_1_USED) == 0) {
		ctrl = tx_desc->ctrl;
	}
	asm volatile ("fence":::"memory");
	caddr_t buffer = sc->tx_buffer + BUFFER_SIZE * index;
	memcpy(buffer, data, packet_length);
	asm volatile ("fence":::"memory");
	tx_desc->ctrl = (ctrl & DESC_1_TX_WRAP) | DESC_1_TX_LAST | ((packet_length < ETHERMIN) ? ETHERMIN: packet_length);

	gem_reg_write(sc, GEM_NWCTRL, sc->nwctrl | GEM_NWCTRL_TXSTART);
	sc->tx_index = (sc->tx_index + 1) % TX_DESC_NUM;

	return packet_length;
}

static ssize_t
gem_recv(int dev, caddr_t buf, size_t buf_len, uint_t startblk)
{
	if (!(0 <= dev && dev < sizeof(gem_dev) / sizeof(gem_dev[0])))
		return -1;

	struct gem_sc *sc = gem_dev[dev];
	if (!sc)
		return -1;
	int index = sc->rx_head;
	size_t len = 0;

	volatile struct gem_desc *rx_desc = (volatile struct gem_desc *)gem_get_rx_desc(sc, index);
	uint32_t status = rx_desc->addr;
	asm volatile ("fence":::"memory");
	if ((status & DESC_0_RX_OWNERSHIP) == 0) {
		return 0;
	}
	uint32_t ctrl = rx_desc->ctrl;

	if ((ctrl & (DESC_1_RX_SOF | DESC_1_RX_EOF)) == (DESC_1_RX_SOF | DESC_1_RX_EOF)) {
		len = (ctrl & DESC_1_LENGTH);
		if (len >= 64) {
			caddr_t buffer = sc->rx_buffer + BUFFER_SIZE * index;
			memcpy(buf, buffer, len);
		}
	}
	rx_desc->ctrl = 0;
	asm volatile ("fence":::"memory");
	rx_desc->addr = status & ~DESC_0_RX_OWNERSHIP;

	index = (index + 1) % RX_DESC_NUM;
	sc->rx_head = index;

	return len;
}

static struct prom_dev gem_prom_dev =
{
	.match = gem_match,
	.open = gem_open,
	.write = gem_send,
	.read = gem_recv,
	.close = gem_close,
	.getmacaddr = gem_getmacaddr,
};

void init_gem(void)
{
	prom_register(&gem_prom_dev);
}

