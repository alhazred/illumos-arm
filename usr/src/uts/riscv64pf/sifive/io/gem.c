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

#include <stddef.h>
#include <sys/promif.h>
#include <sys/miiregs.h>
#include <sys/ethernet.h>
#include <sys/byteorder.h>
#include <sys/debug.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/vlan.h>
#include <sys/mac.h>
#include <sys/mac_ether.h>
#include <sys/strsun.h>
#include <sys/miiregs.h>
#include <sys/sunndi.h>
#include <sys/ndi_impldefs.h>
#include <sys/ddi_impldefs.h>
#include <sys/crc32.h>
#include <sys/sysmacros.h>
#include <sys/platmod.h>
#include <sys/gpio.h>
#include "gem.h"
#include "gemreg.h"

#define	GEM_DMA_BUFFER_SIZE	1536
#define	GEM_DMA_ALIGN		64

static ddi_device_acc_attr_t mem_acc_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_NEVERSWAP_ACC,
	DDI_STORECACHING_OK_ACC,
};

static ddi_device_acc_attr_t reg_acc_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_NEVERSWAP_ACC,
	DDI_STRICTORDER_ACC,
};

static ddi_dma_attr_t dma_attr = {
	DMA_ATTR_V0,			/* dma_attr_version	*/
	0x0000000000000000ull,		/* dma_attr_addr_lo	*/
	0x00000000FFFFFFFFull,		/* dma_attr_addr_hi	*/
	0x00000000FFFFFFFFull,		/* dma_attr_count_max	*/
	0x0000000000000001ull,		/* dma_attr_align	*/
	0x00000FFF,			/* dma_attr_burstsizes	*/
	0x00000001,			/* dma_attr_minxfer	*/
	0x00000000FFFFFFFFull,		/* dma_attr_maxxfer	*/
	0x00000000FFFFFFFFull,		/* dma_attr_seg		*/
	1,				/* dma_attr_sgllen	*/
	0x00000001,			/* dma_attr_granular	*/
	DDI_DMA_FLAGERR			/* dma_attr_flags	*/
};

static void gem_destroy(struct gem_sc *sc);
static void gem_m_stop(void *arg);

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

static void
gem_reg_write(struct gem_sc *sc, int index, uint32_t val)
{
	uint32_t *addr = sc->reg.addr + index;
	ddi_put32(sc->reg.handle, addr, val);
}

static uint32_t
gem_reg_read(struct gem_sc *sc, int index)
{
	uint32_t *addr = sc->reg.addr + index;
	return ddi_get32(sc->reg.handle, addr);
}

static void
gem_usecwait(int usec)
{
	drv_usecwait(usec);
}

static pnode_t
gem_get_node(struct gem_sc *sc)
{
	return ddi_get_nodeid(sc->dip);
}
static void
gem_mutex_enter(struct gem_sc *sc)
{
	mutex_enter(&sc->intrlock);
}
static void
gem_mutex_exit(struct gem_sc *sc)
{
	mutex_exit(&sc->intrlock);
}

static void
gem_gmac_reset(struct gem_sc *sc)
{
	pnode_t node = gem_get_node(sc);

	pnode_t child = prom_childnode(node);
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
		if (prom_get_reg(child, 0, &phy_id) != 0) {
			sc->phy_id = phy_id;
		}
	}

	gem_reg_write(sc, GEM_NWCTRL, 0);
	gem_reg_write(sc, GEM_NWCFG, 0);
	gem_reg_write(sc, GEM_NWCTRL, GEM_NWCTRL_CLRSTAT);
	gem_reg_write(sc, GEM_TXSTATUS, gem_reg_read(sc, GEM_TXSTATUS));
	gem_reg_write(sc, GEM_RXSTATUS, gem_reg_read(sc, GEM_RXSTATUS));
	gem_reg_write(sc, GEM_IDR, ~0u);
	gem_reg_write(sc, GEM_IMR, 0u);
	gem_reg_write(sc, GEM_ISR, ~0u);
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

static void
gem_gmac_init(struct gem_sc *sc)
{
	// interrupt disable
	gem_reg_write(sc, GEM_IDR, ~0u);

	// interrupt clear
	gem_reg_write(sc, GEM_ISR, ~0u);

	gem_reg_write(sc, GEM_SPADDR1LO, (sc->dev_addr[3] << 24) | (sc->dev_addr[2] << 16) | (sc->dev_addr[1] << 8) | sc->dev_addr[0]);
	gem_reg_write(sc, GEM_SPADDR1HI, (sc->dev_addr[5] << 8) | sc->dev_addr[4]);

	gem_reg_write(sc, GEM_TXSTATUS, gem_reg_read(sc, GEM_TXSTATUS));
	gem_reg_write(sc, GEM_RXSTATUS, gem_reg_read(sc, GEM_RXSTATUS));

	gem_reg_write(sc, GEM_RXQBASE, sc->rx_ring.desc.dmac_addr);
	gem_reg_write(sc, GEM_RBQPH, sc->rx_ring.desc.dmac_addr >> 32);
	gem_reg_write(sc, GEM_TXQBASE, sc->tx_ring.desc.dmac_addr);
	gem_reg_write(sc, GEM_TBQPH, sc->tx_ring.desc.dmac_addr >> 32);

	gem_reg_write(sc, GEM_IER, GEM_INT_TXERRS | GEM_INT_TXCMPL | GEM_INT_RXERRS | GEM_INT_RXCMPL);
}

static void
gem_gmac_update(struct gem_sc *sc)
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
	if (sc->phy_duplex)
		netcfg |= GEM_NWCFG_FDX;
	else
		netcfg &= ~GEM_NWCFG_FDX;
	gem_reg_write(sc, GEM_NWCFG, netcfg);
}


static void
gem_gmac_enable(struct gem_sc *sc)
{
	sc->nwctrl |= GEM_NWCTRL_TXENA;
	sc->nwctrl |= GEM_NWCTRL_RXENA;
	gem_reg_write(sc, GEM_NWCTRL, sc->nwctrl);
}

static void
gem_gmac_disable(struct gem_sc *sc)
{
	sc->nwctrl &= ~GEM_NWCTRL_TXENA;
	sc->nwctrl &= ~GEM_NWCTRL_RXENA;
	gem_reg_write(sc, GEM_NWCTRL, sc->nwctrl);
}

static void
gem_free_tx(struct gem_sc *sc, int idx)
{
	if (sc->tx_ring.mp[idx]) {
		freemsg(sc->tx_ring.mp[idx]);
		sc->tx_ring.mp[idx] = NULL;
	}
}

static void
gem_free_packet(struct gem_packet *pkt)
{
	struct gem_sc *sc = pkt->sc;
	if (sc->running && sc->rx_pkt_num < RX_PKT_NUM_MAX) {
		pkt->mp = desballoc((unsigned char *)pkt->dma.addr, GEM_DMA_BUFFER_SIZE, BPRI_MED, &pkt->free_rtn);
	} else {
		pkt->mp = NULL;
	}
	if (pkt->mp == NULL) {
		ddi_dma_unbind_handle(pkt->dma.dma_handle);
		ddi_dma_mem_free(&pkt->dma.mem_handle);
		ddi_dma_free_handle(&pkt->dma.dma_handle);
		kmem_free(pkt, sizeof(struct gem_packet));
	} else {
		mutex_enter(&sc->rx_pkt_lock);
		pkt->next = sc->rx_pkt_free;
		sc->rx_pkt_free = pkt;
		sc->rx_pkt_num++;
		mutex_exit(&sc->rx_pkt_lock);
	}
}

static struct gem_packet *
gem_alloc_packet(struct gem_sc *sc)
{
	struct gem_packet *pkt;
	ddi_dma_attr_t desc_dma_attr = dma_attr;
	desc_dma_attr.dma_attr_align = GEM_DMA_ALIGN;

	mutex_enter(&sc->rx_pkt_lock);
	pkt = sc->rx_pkt_free;
	if (pkt) {
		sc->rx_pkt_free = pkt->next;
		sc->rx_pkt_num--;
	}
	mutex_exit(&sc->rx_pkt_lock);

	if (pkt == NULL) {
		pkt = (struct gem_packet *)kmem_zalloc(sizeof(struct gem_packet), KM_NOSLEEP);
		if (pkt) {
			if (ddi_dma_alloc_handle(sc->dip, &desc_dma_attr, DDI_DMA_SLEEP, 0, &pkt->dma.dma_handle) != DDI_SUCCESS) {
				kmem_free(pkt, sizeof(struct gem_packet));
				pkt= NULL;
			}
		}

		if (pkt) {
			if (ddi_dma_mem_alloc(pkt->dma.dma_handle, GEM_DMA_BUFFER_SIZE, &mem_acc_attr, DDI_DMA_CONSISTENT, DDI_DMA_SLEEP, 0,
				    &pkt->dma.addr, &pkt->dma.size, &pkt->dma.mem_handle)) {
				ddi_dma_free_handle(&pkt->dma.dma_handle);
				kmem_free(pkt, sizeof(struct gem_packet));
				pkt= NULL;
			} else {
				ASSERT(pkt->dma.size >= GEM_DMA_BUFFER_SIZE);
	 		}
		}

		if (pkt) {
			ddi_dma_cookie_t cookie;
			uint_t ccount;
			int result = ddi_dma_addr_bind_handle(pkt->dma.dma_handle, NULL, pkt->dma.addr, pkt->dma.size, DDI_DMA_RDWR | DDI_DMA_CONSISTENT,
			    DDI_DMA_SLEEP, NULL, &cookie, &ccount);
			if (result == DDI_DMA_MAPPED) {
				ASSERT(ccount == 1);
				pkt->dma.dmac_addr = cookie.dmac_laddress;
				ASSERT((cookie.dmac_laddress & (GEM_DMA_ALIGN - 1)) == 0);
				ASSERT(cookie.dmac_size <= GEM_DMA_BUFFER_SIZE);
				pkt->sc = sc;
				pkt->free_rtn.free_func = gem_free_packet;
				pkt->free_rtn.free_arg = (char *)pkt;

				pkt->mp = desballoc((unsigned char *)pkt->dma.addr, GEM_DMA_BUFFER_SIZE, BPRI_MED, &pkt->free_rtn);
				if (pkt->mp == NULL) {
					ddi_dma_unbind_handle(pkt->dma.dma_handle);
					ddi_dma_mem_free(&pkt->dma.mem_handle);
					ddi_dma_free_handle(&pkt->dma.dma_handle);
					kmem_free(pkt, sizeof(struct gem_packet));
					pkt= NULL;
				}
			} else {
				ddi_dma_mem_free(&pkt->dma.mem_handle);
				ddi_dma_free_handle(&pkt->dma.dma_handle);
				kmem_free(pkt, sizeof(struct gem_packet));
				pkt= NULL;
			}
		}
	}

	return pkt;
}

static bool
gem_alloc_desc_ring(struct gem_sc *sc, struct gem_dma *desc_dma, size_t align, size_t size)
{
	ddi_dma_attr_t desc_dma_attr = dma_attr;
	desc_dma_attr.dma_attr_align = align;

	if (ddi_dma_alloc_handle(sc->dip, &desc_dma_attr, DDI_DMA_SLEEP, 0, &desc_dma->dma_handle) != DDI_SUCCESS) {
		return false;
	}

	if (ddi_dma_mem_alloc(desc_dma->dma_handle, size, &mem_acc_attr, DDI_DMA_CONSISTENT, DDI_DMA_SLEEP, 0,
		    &desc_dma->addr, &desc_dma->size, &desc_dma->mem_handle)) {
		return false;
	}

	ddi_dma_cookie_t cookie;
	uint_t ccount;
	int result = ddi_dma_addr_bind_handle(
	    desc_dma->dma_handle, NULL, desc_dma->addr, desc_dma->size, DDI_DMA_RDWR | DDI_DMA_CONSISTENT,
	    DDI_DMA_SLEEP, NULL, &cookie, &ccount);
	if (result == DDI_DMA_MAPPED) {
		ASSERT(ccount == 1);
	} else {
		return false;
	}
	ASSERT(desc_dma->size >= size);
	desc_dma->dmac_addr = cookie.dmac_laddress;

	return true;
}

static void
gem_free_desc_ring(struct gem_dma *desc_dma)
{
	if (desc_dma->dmac_addr)
		ddi_dma_unbind_handle(desc_dma->dma_handle);
	desc_dma->dmac_addr = 0;

	if (desc_dma->mem_handle)
		ddi_dma_mem_free(&desc_dma->mem_handle);
	desc_dma->mem_handle = 0;

	if (desc_dma->dma_handle)
		ddi_dma_free_handle(&desc_dma->dma_handle);
	desc_dma->dma_handle = 0;
}

static bool
gem_alloc_buffer(struct gem_sc *sc)
{
	int len;
	size_t desc_size = sc->is64b_desc? sizeof(struct gem_desc64): sizeof(struct gem_desc);

	for (int index = 0; index < RX_DESC_NUM; index++) {
		struct gem_packet *pkt = gem_alloc_packet(sc);
		if (!pkt)
			return false;
		sc->rx_ring.pkt[index] = pkt;

		struct gem_desc *desc_p = (struct gem_desc *)(sc->rx_ring.desc.addr + desc_size * index);
		desc_p->addr = pkt->dma.dmac_addr;
		if (index  == RX_DESC_NUM - 1)
			desc_p->addr |= DESC_0_RX_WRAP;
		desc_p->ctrl = 0;
		if (sc->is64b_desc) {
			struct gem_desc64 *desc64_p = (struct gem_desc64 *)(sc->rx_ring.desc.addr + desc_size * index);
			desc64_p->addrhi = pkt->dma.dmac_addr >> 32;
			desc64_p->rsv = 0;
		}
	}
	sc->rx_ring.index = 0;

	for (int index = 0; index < TX_DESC_NUM; index++) {
		struct gem_packet *pkt = gem_alloc_packet(sc);
		if (!pkt)
			return false;
		sc->tx_ring.pkt[index] = pkt;
		struct gem_desc *desc_p = (struct gem_desc *)(sc->tx_ring.desc.addr + desc_size * index);
		desc_p->addr = pkt->dma.dmac_addr;
		desc_p->ctrl = DESC_1_USED;
		if (index == TX_DESC_NUM - 1)
			desc_p->ctrl |= DESC_1_TX_WRAP;
		if (sc->is64b_desc) {
			struct gem_desc64 *desc64_p = (struct gem_desc64 *)(sc->tx_ring.desc.addr + desc_size * index);
			desc64_p->addrhi = pkt->dma.dmac_addr >> 32;
			desc64_p->rsv = 0;
		}
	}
	sc->tx_ring.head = 0;
	sc->tx_ring.tail = 0;

	return true;
}

static void
gem_free_buffer(struct gem_sc *sc)
{
	for (int i = 0; i < TX_DESC_NUM; i++) {
		struct gem_packet *pkt = sc->tx_ring.pkt[i];
		if (pkt) {
			freemsg(pkt->mp);
			sc->tx_ring.pkt[i] = NULL;
		}
		gem_free_tx(sc, i);
	}

	for (int i = 0; i < RX_DESC_NUM; i++) {
		struct gem_packet *pkt = sc->rx_ring.pkt[i];
		if (pkt) {
			freemsg(pkt->mp);
			sc->rx_ring.pkt[i] = NULL;
		}
	}

	mutex_enter(&sc->rx_pkt_lock);
	for (;;) {
		struct gem_packet *pkt = sc->rx_pkt_free;
		if (pkt == NULL)
			break;
		sc->rx_pkt_free = pkt->next;
		sc->rx_pkt_num--;
		mutex_exit(&sc->rx_pkt_lock);
		freemsg(pkt->mp);
		mutex_enter(&sc->rx_pkt_lock);
	}
	mutex_exit(&sc->rx_pkt_lock);
}

static bool
gem_get_macaddr(struct gem_sc *sc)
{
	if (prom_getproplen(gem_get_node(sc), "local-mac-address") == 6) {
		prom_getprop(gem_get_node(sc), "local-mac-address", (caddr_t)sc->dev_addr);
	}
	int i = 0;
	for (; i < 6; i++)
		if (sc->dev_addr[i] != 0)
			break;
	if (i == 6) {
		uint8_t default_macaddr[6] = DEFAULT_MAC_ADDRESS;
		memcpy(sc->dev_addr, default_macaddr, 6);
		prom_setprop(gem_get_node(sc), "local-mac-address", (caddr_t)sc->dev_addr, 6);
	}
	return true;
}

static void
gem_destroy(struct gem_sc *sc)
{
	if (sc->intr_handle) {
		ddi_intr_disable(sc->intr_handle);
		ddi_intr_remove_handler(sc->intr_handle);
		ddi_intr_free(sc->intr_handle);
	}
	sc->intr_handle = 0;

	if (sc->mii_handle)
		mii_free(sc->mii_handle);
	sc->mii_handle = 0;

	if (sc->mac_handle) {
		mac_unregister(sc->mac_handle);
		mac_free(sc->macp);
	}
	sc->mac_handle = 0;

	gem_free_buffer(sc);

	gem_free_desc_ring(&sc->tx_ring.desc);
	gem_free_desc_ring(&sc->rx_ring.desc);

	if (sc->reg.handle)
		ddi_regs_map_free(&sc->reg.handle);
	sc->reg.handle = 0;

	ddi_set_driver_private(sc->dip, NULL);
	struct gem_mcast *mc;
	while ((mc = list_head(&sc->mcast)) != NULL) {
		list_remove(&sc->mcast, mc);
		kmem_free(mc, sizeof (*mc));
	}
	list_destroy(&sc->mcast);
	mutex_destroy(&sc->intrlock);
	mutex_destroy(&sc->rx_pkt_lock);
	kmem_free(sc, sizeof (*sc));
}

static bool
gem_init(struct gem_sc *sc)
{
	gem_gmac_reset(sc);

	if (!gem_get_macaddr(sc))
		return false;

	size_t desc_size = sc->is64b_desc? sizeof(struct gem_desc64): sizeof(struct gem_desc);
	if (!gem_alloc_desc_ring(sc, &sc->rx_ring.desc, desc_size, desc_size * RX_DESC_NUM))
		return false;

	if (!gem_alloc_desc_ring(sc, &sc->tx_ring.desc, desc_size, desc_size * TX_DESC_NUM))
		return false;

	return true;
}

static void
gem_mii_write(void *arg, uint8_t phy, uint8_t reg, uint16_t value)
{
	struct gem_sc *sc = arg;

	gem_mutex_enter(sc);

	if (gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE) {
		gem_reg_write(sc, GEM_PHYMNTNC,
		    GEM_PHYMNTNC_SOF |
		    GEM_PHYMNTNC_CODE |
		    GEM_PHYMNTNC_OP_W |
		    (phy << GEM_PHYMNTNC_ADDR_SHFT) |
		    (reg << GEM_PHYMNTNC_REG_SHIFT) |
		    value
		    );
		int i;
		for (i = 0; i < 1000; i++) {
			gem_usecwait(100);
			if (gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE)
				break;
		}
		if (i == 1000) {
			cmn_err(CE_WARN, "%s%d: MII write failed",
			    ddi_driver_name(sc->dip), ddi_get_instance(sc->dip));
		}
	} else {
		cmn_err(CE_WARN, "%s%d: MII busy",
		    ddi_driver_name(sc->dip), ddi_get_instance(sc->dip));
	}

	gem_mutex_exit(sc);
}

static uint16_t
gem_mii_read(void *arg, uint8_t phy, uint8_t reg)
{
	struct gem_sc *sc = arg;

	uint16_t data = 0xffff;

	gem_mutex_enter(sc);

	if (gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE) {
		gem_reg_write(sc, GEM_PHYMNTNC,
		    GEM_PHYMNTNC_SOF |
		    GEM_PHYMNTNC_CODE |
		    GEM_PHYMNTNC_OP_R |
		    (phy << GEM_PHYMNTNC_ADDR_SHFT) |
		    (reg << GEM_PHYMNTNC_REG_SHIFT)
		    );
		int i;
		for (i = 0; i < 1000; i++) {
			gem_usecwait(100);
			if (gem_reg_read(sc, GEM_NWSTATUS) & GEM_NWSTATUS_IDLE) {
				data = gem_reg_read(sc, GEM_PHYMNTNC) & 0xffff;
				break;
			}
		}
		if (i == 1000) {
			cmn_err(CE_WARN, "%s%d: MII read failed",
			    ddi_driver_name(sc->dip), ddi_get_instance(sc->dip));
		}
	} else {
		cmn_err(CE_WARN, "%s%d: MII busy",
		    ddi_driver_name(sc->dip), ddi_get_instance(sc->dip));
	}

	gem_mutex_exit(sc);

	return data;
}

static int
gem_probe(dev_info_t *dip)
{
	int len;
	char buf[80];
	pnode_t node = ddi_get_nodeid(dip);
	if (node < 0)
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

static void
gem_mii_notify(void *arg, link_state_t link)
{
	struct gem_sc *sc = arg;
	uint32_t gmac;
	uint32_t gpcr;
	link_flowctrl_t fc;
	link_duplex_t duplex;
	int speed;

	fc = mii_get_flowctrl(sc->mii_handle);
	duplex = mii_get_duplex(sc->mii_handle);
	speed = mii_get_speed(sc->mii_handle);

	gem_mutex_enter(sc);

	if (link == LINK_STATE_UP) {
		sc->phy_speed = speed;
		sc->phy_duplex = duplex;
		gem_gmac_update(sc);
	} else {
		sc->phy_speed = -1;
		sc->phy_duplex = LINK_DUPLEX_UNKNOWN;
	}

	gem_mutex_exit(sc);

	mac_link_update(sc->mac_handle, link);
}

static void
gem_mii_reset(void *arg)
{
	struct gem_sc *sc = arg;
	int phy = mii_get_addr(sc->mii_handle);

	gem_mii_write(sc, phy, 0x0d, 0x7);
	gem_mii_write(sc, phy, 0x0e, 0x3c);
	gem_mii_write(sc, phy, 0x0d, 0x4007);
	gem_mii_write(sc, phy, 0x0e, 0);

	uint16_t v = gem_mii_read(sc, phy, 9);
	gem_mii_write(sc, phy, 9, v & ~(1u << 9));
}

static mii_ops_t gem_mii_ops = {
	MII_OPS_VERSION,
	gem_mii_read,
	gem_mii_write,
	gem_mii_notify,
	gem_mii_reset	/* reset */
};

static int
gem_phy_install(struct gem_sc *sc)
{
	sc->mii_handle = mii_alloc(sc, sc->dip, &gem_mii_ops);
	if (sc->mii_handle == NULL) {
		return (DDI_FAILURE);
	}
	//mii_set_pauseable(sc->mii_handle, B_FALSE, B_FALSE);

	return DDI_SUCCESS;
}

static mblk_t *
gem_send(struct gem_sc *sc, mblk_t *mp)
{
	if (((sc->tx_ring.head - sc->tx_ring.tail + TX_DESC_NUM) % TX_DESC_NUM) == (TX_DESC_NUM - 2)) {
		return mp;
	}

	int index = sc->tx_ring.head;
	size_t mblen = 0;
	sc->tx_ring.mp[index] = mp;
	struct gem_packet *pkt = sc->tx_ring.pkt[index];

	caddr_t addr = pkt->dma.addr;
	for (mblk_t *bp = mp; bp != NULL; bp = bp->b_cont) {
		size_t frag_len = MBLKL(bp);
		if (frag_len == 0)
			continue;
		memcpy(addr, bp->b_rptr, frag_len);
		addr += frag_len;
		mblen += frag_len;
	}
	size_t desc_size = sc->is64b_desc? sizeof(struct gem_desc64): sizeof(struct gem_desc);
	volatile struct gem_desc *desc_p = (volatile struct gem_desc *)(sc->tx_ring.desc.addr + desc_size * index);
	asm volatile ("fence":::"memory");
	desc_p->ctrl = (desc_p->ctrl & DESC_1_TX_WRAP) | DESC_1_TX_LAST | ((mblen < ETHERMIN) ? ETHERMIN: mblen);
	sc->tx_ring.head = (index + 1) % TX_DESC_NUM;

	return (NULL);
}

static mblk_t *
gem_m_tx(void *arg, mblk_t *mp)
{
	struct gem_sc *sc = arg;
	mblk_t *nmp;

	gem_mutex_enter(sc);

	int count = 0;
	while (mp != NULL) {
		nmp = mp->b_next;
		mp->b_next = NULL;
		if ((mp = gem_send(sc, mp)) != NULL) {
			mp->b_next = nmp;
			break;
		}
		mp = nmp;
		count++;
	}

	if (count != 0) {
		gem_reg_write(sc, GEM_NWCTRL, sc->nwctrl | GEM_NWCTRL_TXSTART);
	}

	gem_mutex_exit(sc);

	return (mp);
}


static mblk_t *
gem_rx_intr(struct gem_sc *sc)
{
	int index = sc->rx_ring.index;

	mblk_t *mblk_head = NULL;
	mblk_t **mblk_tail = &mblk_head;

	size_t desc_size = sc->is64b_desc? sizeof(struct gem_desc64): sizeof(struct gem_desc);
	for (;;) {
		size_t len = 0;
		volatile struct gem_desc *desc_p = (struct gem_desc *)(sc->rx_ring.desc.addr + desc_size * index);
		uint32_t status = desc_p->addr;
		asm volatile ("fence":::"memory");
		if ((status & DESC_0_RX_OWNERSHIP) == 0) {
			break;
		}
		uint32_t ctrl = desc_p->ctrl;

		if ((ctrl & (DESC_1_RX_SOF | DESC_1_RX_EOF)) == (DESC_1_RX_SOF | DESC_1_RX_EOF)) {
			len = (ctrl & DESC_1_LENGTH);
		}

		if (len > 0) {
			struct gem_packet *pkt = gem_alloc_packet(sc);
			if (pkt) {
				mblk_t *mp = sc->rx_ring.pkt[index]->mp;
				*mblk_tail = mp;
				mblk_tail = &mp->b_next;
				mp->b_wptr += len;
				sc->rx_ring.pkt[index] = pkt;
			}
		}

		{
			struct gem_packet *pkt = sc->rx_ring.pkt[index];
			desc_p->ctrl = 0;
			asm volatile ("fence":::"memory");
			desc_p->addr = pkt->dma.dmac_addr | (status & DESC_0_RX_WRAP);
		}
		index = ((index + 1) % RX_DESC_NUM);
	}

	sc->rx_ring.index = index;

	return mblk_head;
}


static int
gem_tx_intr(struct gem_sc *sc)
{
	int index = sc->tx_ring.tail;
	int ret = 0;
	size_t desc_size = sc->is64b_desc? sizeof(struct gem_desc64): sizeof(struct gem_desc);
	while (index != sc->tx_ring.head) {
		volatile struct gem_desc *desc_p = (volatile struct gem_desc *)(sc->tx_ring.desc.addr + desc_size * index);
		uint32_t ctrl = desc_p->ctrl;
		asm volatile ("fence":::"memory");
		if ((ctrl & DESC_1_USED) == 0)
			break;
		gem_free_tx(sc, index);
		index = (index + 1) % TX_DESC_NUM;
		ret++;
	}
	sc->tx_ring.tail = index;
	return ret;
}

static uint_t
gem_intr(caddr_t arg, caddr_t unused)
{
	struct gem_sc *sc = (struct gem_sc *)arg;

	gem_mutex_enter(sc);

	for (;;) {
		uint32_t isr = gem_reg_read(sc, GEM_ISR);
		gem_reg_write(sc, GEM_ISR, isr);

//		prom_printf("%s:%d isr=%x %d %d %d\n",__func__,__LINE__,isr, sc->tx_ring.head, sc->tx_ring.tail, sc->rx_ring.index);
		isr &= (GEM_INT_TXERRS | GEM_INT_TXCMPL | GEM_INT_RXERRS | GEM_INT_RXCMPL);
		if (isr == 0)
			break;

		if (sc->running == 0)
			break;

		if (isr & (GEM_INT_RXERRS | GEM_INT_RXCMPL)) {
			mblk_t *mp = gem_rx_intr(sc);
			if (mp) {
				gem_mutex_exit(sc);
				mac_rx(sc->mac_handle, NULL, mp);
				gem_mutex_enter(sc);
			}
		}

		if (sc->running == 0)
			break;

		if (isr & (GEM_INT_TXERRS | GEM_INT_TXCMPL)) {
			int tx = 0;

			tx = gem_tx_intr(sc);

			if (tx) {
				gem_mutex_exit(sc);
				mac_tx_update(sc->mac_handle);
				gem_mutex_enter(sc);
			}
		}
	}

	gem_mutex_exit(sc);

	return (DDI_INTR_CLAIMED);
}


static int gem_attach(dev_info_t *dip, ddi_attach_cmd_t cmd);
static int
gem_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_DETACH:
		break;
	default:
		return (DDI_FAILURE);
	}
	struct gem_sc *sc = ddi_get_driver_private(dip);

	gem_m_stop(sc);

	if (mac_disable(sc->mac_handle) != 0)
		return (DDI_FAILURE);

	gem_destroy(sc);

	return DDI_SUCCESS;
}

static int
gem_quiesce(dev_info_t *dip)
{
	cmn_err(CE_WARN, "%s%d: gem_quiesce is not implemented",
	    ddi_driver_name(dip), ddi_get_instance(dip));
	return DDI_FAILURE;
}


static void
gem_update_filter(struct gem_sc *sc)
{
	uint32_t hash[2] = {0};
	uint32_t nwcfg = gem_reg_read(sc, GEM_NWCFG);

	if (nwcfg & GEM_NWCFG_PROMISC) {
		hash[0] = 0xffffffff;
		hash[1] = 0xffffffff;
	} else if (nwcfg & GEM_NWCFG_MCAST_HASH) {
		for (struct gem_mcast *mc = list_head(&sc->mcast); mc; mc = list_next(&sc->mcast, mc)) {
			int v = 0;
			for (int i = 0; i < 6; i++)
				for (int j = 0; j < 8; j++)
					v ^= ((((mc->addr[(i + 6 * j) / 8] >> ((i + 6 * j) % 8))) & 1) << i);
			hash[v / 32] |= (1u << (v % 32));
		}
	}
	gem_reg_write(sc, GEM_HASHLO, hash[0]);
	gem_reg_write(sc, GEM_HASHHI, hash[1]);
}

static int
gem_m_setpromisc(void *a, boolean_t b)
{
	struct gem_sc *sc = a;
	gem_mutex_enter(sc);

	uint32_t nwcfg = gem_reg_read(sc, GEM_NWCFG);
	if (b)
		nwcfg |= GEM_NWCFG_PROMISC;
	else
		nwcfg &= ~GEM_NWCFG_PROMISC;
	gem_reg_write(sc, GEM_NWCFG, nwcfg);

	gem_update_filter(sc);

	gem_mutex_exit(sc);

	return 0;
}

static int
gem_m_multicst(void *a, boolean_t b, const uint8_t *c)
{
	struct gem_sc *sc = a;
	struct gem_mcast *mc;

	gem_mutex_enter(sc);

	if (b) {
		mc = kmem_alloc(sizeof (*mc), KM_NOSLEEP);
		if (!mc) {
			gem_mutex_exit(sc);
			return ENOMEM;
		}

		memcpy(mc->addr, c, sizeof(mc->addr));
		list_insert_head(&sc->mcast, mc);
	} else {
		for (mc = list_head(&sc->mcast); mc; mc = list_next(&sc->mcast, mc)) {
			if (memcmp(mc->addr, c, sizeof(mc->addr)) == 0) {
				list_remove(&sc->mcast, mc);
				kmem_free(mc, sizeof (*mc));
				break;
			}
		}
	}

	uint32_t nwcfg = gem_reg_read(sc, GEM_NWCFG);
	if (list_head(&sc->mcast))
		nwcfg |= GEM_NWCFG_MCAST_HASH;
	else
		nwcfg &= ~GEM_NWCFG_MCAST_HASH;
	gem_reg_write(sc, GEM_NWCFG, nwcfg);

	gem_update_filter(sc);

	gem_mutex_exit(sc);
	return 0;
}

static int
gem_m_unicst(void *arg, const uint8_t *dev_addr)
{
	struct gem_sc *sc = arg;

	gem_mutex_enter(sc);

	memcpy(sc->dev_addr, dev_addr, sizeof(sc->dev_addr));

	gem_gmac_disable(sc);

	gem_reg_write(sc, GEM_SPADDR1LO, (sc->dev_addr[3] << 24) | (sc->dev_addr[2] << 16) | (sc->dev_addr[1] << 8) | sc->dev_addr[0]);
	gem_reg_write(sc, GEM_SPADDR1HI, (sc->dev_addr[5] << 8) | sc->dev_addr[4]);

	gem_gmac_enable(sc);

	gem_mutex_exit(sc);

	return 0;
}

static int
gem_m_start(void *arg)
{
	struct gem_sc *sc = arg;

	gem_mutex_enter(sc);

	if (!gem_alloc_buffer(sc)) {
		gem_mutex_exit(sc);
		return ENOMEM;
	}
	gem_gmac_init(sc);
	gem_gmac_enable(sc);

	sc->running = 1;

	if (ddi_intr_enable(sc->intr_handle) != DDI_SUCCESS) {
		sc->running = 0;
		gem_gmac_disable(sc);
		gem_free_buffer(sc);
		gem_mutex_exit(sc);
		return EIO;
	}

	gem_mutex_exit(sc);

	mii_start(sc->mii_handle);

	return 0;
}

static void
gem_m_stop(void *arg)
{
	struct gem_sc *sc = arg;

	mii_stop(sc->mii_handle);

	gem_mutex_enter(sc);

	ddi_intr_disable(sc->intr_handle);

	sc->running = 0;
	gem_gmac_disable(sc);
	gem_free_buffer(sc);

	gem_mutex_exit(sc);
}

static int
gem_m_getstat(void *arg, uint_t stat, uint64_t *val)
{
	struct gem_sc *sc = arg;
	return mii_m_getstat(sc->mii_handle, stat, val);
}

static int
gem_m_setprop(void *arg, const char *name, mac_prop_id_t num, uint_t sz, const void *val)
{
	struct gem_sc *sc = arg;
	return mii_m_setprop(sc->mii_handle, name, num, sz, val);
}

static int
gem_m_getprop(void *arg, const char *name, mac_prop_id_t num, uint_t sz, void *val)
{
	struct gem_sc *sc = arg;
	return mii_m_getprop(sc->mii_handle, name, num, sz, val);
}

static void
gem_m_propinfo(void *arg, const char *name, mac_prop_id_t num, mac_prop_info_handle_t prh)
{
	struct gem_sc *sc = arg;
	mii_m_propinfo(sc->mii_handle, name, num, prh);
}

static void
gem_m_ioctl(void *arg, queue_t *wq, mblk_t *mp)
{
	struct gem_sc *sc = arg;
	if (mii_m_loop_ioctl(sc->mii_handle, wq, mp))
		return;

	miocnak(wq, mp, 0, EINVAL);
}

extern struct mod_ops mod_driverops;

DDI_DEFINE_STREAM_OPS(gem_devops, nulldev, gem_probe, gem_attach,
    gem_detach, nodev, NULL, D_MP, NULL, gem_quiesce);

static struct modldrv gem_modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	"gem",			/* short description */
	&gem_devops		/* driver specific ops */
};

static struct modlinkage gem_modlinkage = {
	MODREV_1,		/* ml_rev */
	{ &gem_modldrv, NULL }	/* ml_linkage */
};

static mac_callbacks_t gem_m_callbacks = {
	MC_SETPROP | MC_GETPROP | MC_PROPINFO,	/* mc_callbacks */
	gem_m_getstat,	/* mc_getstat */
	gem_m_start,		/* mc_start */
	gem_m_stop,		/* mc_stop */
	gem_m_setpromisc,	/* mc_setpromisc */
	gem_m_multicst,	/* mc_multicst */
	gem_m_unicst,		/* mc_unicst */
	gem_m_tx,		/* mc_tx */
	NULL,
	gem_m_ioctl,		/* mc_ioctl */
	NULL,			/* mc_getcapab */
	NULL,			/* mc_open */
	NULL,			/* mc_close */
	gem_m_setprop,
	gem_m_getprop,
	gem_m_propinfo
};

static int
gem_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_ATTACH:
		break;
	default:
		return (DDI_FAILURE);
	}

	struct gem_sc *sc = kmem_zalloc(sizeof(struct gem_sc), KM_SLEEP);
	ddi_set_driver_private(dip, sc);
	sc->dip = dip;

	mutex_init(&sc->intrlock, NULL, MUTEX_DRIVER, NULL);
	mutex_init(&sc->rx_pkt_lock, NULL, MUTEX_DRIVER, NULL);
	list_create(&sc->mcast, sizeof (struct gem_mcast), offsetof(struct gem_mcast, node));

	if (ddi_regs_map_setup(sc->dip, 0, (caddr_t*)&sc->reg.addr, 0, 0, &reg_acc_attr, &sc->reg.handle) != DDI_SUCCESS) {
		goto err_exit;
	}

	gem_mutex_enter(sc);
	if (!gem_init(sc)) {
		gem_mutex_exit(sc);
		goto err_exit;
	}
	gem_mutex_exit(sc);

	mac_register_t *macp;
	if ((macp = mac_alloc(MAC_VERSION)) == NULL) {
		goto err_exit;
	}
	sc->macp = macp;

	macp->m_type_ident = MAC_PLUGIN_IDENT_ETHER;
	macp->m_driver = sc;
	macp->m_dip = dip;
	macp->m_src_addr = sc->dev_addr;
	macp->m_callbacks = &gem_m_callbacks;
	macp->m_min_sdu = 0;
	macp->m_max_sdu = ETHERMTU;
	macp->m_margin = VLAN_TAGSZ;

	if (mac_register(macp, &sc->mac_handle) != 0) {
		mac_free(sc->macp);
		sc->mac_handle = 0;
		goto err_exit;
	}

	if (gem_phy_install(sc) != DDI_SUCCESS) {
		goto err_exit;
	}

	int actual;
	if (ddi_intr_alloc(dip, &sc->intr_handle, DDI_INTR_TYPE_FIXED, 0, 1, &actual, DDI_INTR_ALLOC_STRICT) != DDI_SUCCESS) {
		goto err_exit;
	}

	if (ddi_intr_add_handler(sc->intr_handle, gem_intr, sc, NULL) != DDI_SUCCESS) {
		ddi_intr_free(sc->intr_handle);
		sc->intr_handle = 0;
		goto err_exit;
	}

	return DDI_SUCCESS;
err_exit:
	gem_destroy(sc);
	return (DDI_FAILURE);
}

int
_init(void)
{
	int i;

	mac_init_ops(&gem_devops, "platmac");

	if ((i = mod_install(&gem_modlinkage)) != 0) {
		mac_fini_ops(&gem_devops);
	}
	return (i);
}

int
_fini(void)
{
	int i;

	if ((i = mod_remove(&gem_modlinkage)) == 0) {
		mac_fini_ops(&gem_devops);
	}
	return (i);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&gem_modlinkage, modinfop));
}
