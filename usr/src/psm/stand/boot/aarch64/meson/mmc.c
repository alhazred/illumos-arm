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

#include <stdbool.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/promif.h>
#include <sys/salib.h>
#include <sys/byteorder.h>
#include <sys/sysmacros.h>
#include <sys/controlregs.h>
#include <sys/dditypes.h>
#include <sys/devops.h>
#include <sys/sdcard/sda.h>
#include <sys/platform.h>
#include <sys/platmod.h>
#include <sys/gpio.h>
#include <util/sscanf.h>

#include "prom_dev.h"
#include "boot_plat.h"
#include "mmc.h"

#define BUFFER_SIZE	0x20000

static void
usecwait(int usec)
{
	uint64_t cnt = (read_cntpct() / (read_cntfrq() / 1000000)) + usec + 2;
	for (;;) {
		if ((read_cntpct() / (read_cntfrq() / 1000000)) > cnt)
			break;
	}
}
static void
cache_flush(void *addr, size_t size)
{
	uintptr_t end = roundup((uintptr_t)addr + size, DCACHE_LINE);
	while ((uintptr_t)addr < end) {
		flush_data_cache((uintptr_t)addr);
		addr = (void *)((uintptr_t)addr + DCACHE_LINE);
	}
	dsb(sy);
}

#define SD_EMMC_CLOCK(base) (*(volatile uint32_t *)(base))

union sd_emmc_clock {
	uint32_t dw;
	struct {
		uint32_t Cfg_div		:	6;
		uint32_t Cfg_src		:	2;
		uint32_t Cfg_co_phase		:	2;
		uint32_t Cfg_tx_phase		:	2;
		uint32_t Cfg_rx_phase		:	2;
		uint32_t Cfg_sram_pd		:	2;
		uint32_t Cfg_tx_delay		:	4;
		uint32_t Cfg_rx_delay		:	4;
		uint32_t Cfg_always_on		:	1;
		uint32_t Cfg_irq_sdio_sleep	:	1;
		uint32_t 			:	6;
	};
};

#define SD_EMMC_DELAY(base) (*(volatile uint32_t *)((base) + 0x4))
union sd_emmc_delay {
	uint32_t dw;
	struct {
		uint32_t Dly0	: 4;
		uint32_t Dly1	: 4;
		uint32_t Dly2	: 4;
		uint32_t Dly3	: 4;
		uint32_t Dly4	: 4;
		uint32_t Dly5	: 4;
		uint32_t Dly6	: 4;
		uint32_t Dly7	: 4;
	};
};

#define SD_EMMC_ADJUST(base) (*(volatile uint32_t *)((base) + 0x8))
union sd_emmc_adjust {
	uint32_t dw;
	struct {
		uint32_t Dly8		: 4;
		uint32_t Dly9		: 4;
		uint32_t Cali_sel	: 4;
		uint32_t Cali_enable	: 1;
		uint32_t Adj_enable	: 1;
		uint32_t Cali_rise	: 1;
		uint32_t		: 1;
		uint32_t Adj_delay	: 6;
		uint32_t 		: 10;
	};
};

#define SD_EMMC_CALOUT(base) (*(volatile uint32_t *)((base) + 0x10))
union sd_emmc_calout {
	uint32_t dw;
	struct {
		uint32_t Cali_idx	: 6;
		uint32_t		: 1;
		uint32_t Cali_vld	: 1;
		uint32_t Cali_setup	: 8;
		uint32_t 		: 16;
	};
};

#define SD_EMMC_START(base) (*(volatile uint32_t *)((base) + 0x40))
union sd_emmc_start {
	uint32_t dw;
	struct {
		uint32_t Desc_int	: 1;
		uint32_t Desc_busy	: 1;
		uint32_t Desc_addr	: 30;
	};
};

#define SD_EMMC_CFG(base) (*(volatile uint32_t *)((base) + 0x44))
union sd_emmc_cfg {
	uint32_t dw;
	struct {
		uint32_t Cfg_bus_width		: 2;
		uint32_t Cfg_ddr		: 1;
		uint32_t Cfg_dc_ugt		: 1;
		uint32_t Cfg_bl_len		: 4;
		uint32_t Cfg_resp_timeout	: 4;
		uint32_t Cfg_rc_cc		: 4;
		uint32_t Cfg_out_fall		: 1;
		uint32_t Cfg_blk_gap_ip		: 1;
		uint32_t Cfg_sdclk_always_on	: 1;
		uint32_t Cfg_ignore_owner	: 1;
		uint32_t Cfg_chk_ds		: 1;
		uint32_t Cfg_cmd_low		: 1;
		uint32_t Cfg_stop_clk		: 1;
		uint32_t Cfg_auto_clk		: 1;
		uint32_t Cfg_txd_add_err	: 1;
		uint32_t Cfg_txd_retry		: 1;
		uint32_t Cfg_irq_ds		: 1;
		uint32_t Cfg_err_abor		: 1;
		uint32_t Cfg_ip_txd_adj		: 4;
	};
};

#define SD_EMMC_STATUS(base) (*(volatile uint32_t *)((base) + 0x48))
union sd_emmc_status {
	uint32_t dw;
	struct {
		uint32_t Rxd_err		: 8;
		uint32_t Txd_err		: 1;
		uint32_t Desc_err		: 1;
		uint32_t Resp_err		: 1;
		uint32_t Resp_timeout		: 1;
		uint32_t Desc_timeout		: 1;
		uint32_t End_of_Chain		: 1;
		uint32_t Resp_status		: 1;
		uint32_t IRQ_sdio		: 1;
		uint32_t DAT_i			: 8;
		uint32_t CMD_i			: 1;
		uint32_t DS			: 1;
		uint32_t Bus_fsm		: 4;
		uint32_t Desc_Busy		: 1;
		uint32_t Core_Busy		: 1;
	};
};

#define SD_EMMC_IRQ_EN(base) (*(volatile uint32_t *)((base) + 0x4c))
union sd_emmc_irq_en {
	uint32_t dw;
	struct {
		uint32_t En_Rxd_err		: 8;
		uint32_t En_Txd_err		: 1;
		uint32_t En_Desc_err		: 1;
		uint32_t En_Resp_err		: 1;
		uint32_t En_Resp_timeout	: 1;
		uint32_t En_Desc_timeout	: 1;
		uint32_t En_End_of_Chain	: 1;
		uint32_t En_Resp_status		: 1;
		uint32_t En_IRQ_sdio		: 1;
		uint32_t Cfg_secure		: 1;
		uint32_t			: 15;
	};
};

#define SD_EMMC_CMD_CFG(base) (*(volatile uint32_t *)((base) + 0x50))
union sd_emmc_cmd_cfg {
	uint32_t dw;
	struct {
		uint32_t Length		: 9;
		uint32_t Block_mode	: 1;
		uint32_t R1b		: 1;
		uint32_t End_of_chain	: 1;
		uint32_t Timeout	: 4;
		uint32_t No_resp	: 1;
		uint32_t No_cmd		: 1;
		uint32_t Data_io	: 1;
		uint32_t Data_wr	: 1;
		uint32_t Resp_nocrc	: 1;
		uint32_t Resp_128	: 1;
		uint32_t Resp_num	: 1;
		uint32_t Data_num	: 1;
		uint32_t Cmd_index	: 6;
		uint32_t Error		: 1;
		uint32_t Owner		: 1;
	};
};

#define SD_EMMC_CMD_ARG(base) (*(volatile uint32_t *)((base) + 0x54))
#define SD_EMMC_CMD_DAT(base) (*(volatile uint32_t *)((base) + 0x58))
#define SD_EMMC_CMD_RSP0(base) (*(volatile uint32_t *)((base) + 0x5c))
#define SD_EMMC_CMD_RSP1(base) (*(volatile uint32_t *)((base) + 0x60))
#define SD_EMMC_CMD_RSP2(base) (*(volatile uint32_t *)((base) + 0x64))
#define SD_EMMC_CMD_RSP3(base) (*(volatile uint32_t *)((base) + 0x68))

static int
sd_emmc_pinmux(pnode_t node, const char *pinname)
{
	int len;
	int pinctrl_index = prom_get_prop_index(node, "pinctrl-names", pinname);
	if (pinctrl_index < 0)
		return -1;
	char buf[80];
	sprintf(buf, "pinctrl-%d", pinctrl_index);

	len = prom_getproplen(node, buf);
	if (len != sizeof(uint32_t))
		return -1;
	uint32_t pinctrl;
	prom_getprop(node, buf, (caddr_t)&pinctrl);
	pnode_t pinctrl_node;
	pinctrl_node = prom_findnode_by_phandle(htonl(pinctrl));
	if (pinctrl_node < 0)
		return -1;

	return plat_pinmux_set(pinctrl_node);
}

static struct gpio_ctrl *
sd_emmc_gpio(pnode_t node, const char *name)
{
	uint32_t gpio_buf[3];
	int len = prom_getproplen(node, name);
	if (len != sizeof(gpio_buf))
		return NULL;
	prom_getprop(node, name, (caddr_t)&gpio_buf);
	struct gpio_ctrl *gpio = (struct gpio_ctrl*)malloc(sizeof(struct gpio_ctrl));
	if (!gpio)
		return NULL;
	gpio->node = prom_findnode_by_phandle(ntohl(gpio_buf[0]));
	gpio->pin = ntohl(gpio_buf[1]);
	gpio->flags = ntohl(gpio_buf[2]);
	return gpio;
}

static uint32_t sd_emmc_clocks[] = {
	  24000000,
	1000000000,
};

static void
sd_emmc_set_clock(uintptr_t base, int clock)
{
	union sd_emmc_clock _sd_emmc_clock;
	int clk_src = ((clock > 12000000)? 1: 0);
	_sd_emmc_clock.dw = SD_EMMC_CLOCK(base);
	_sd_emmc_clock.Cfg_src = clk_src;
	_sd_emmc_clock.Cfg_div = sd_emmc_clocks[clk_src] / clock;
	SD_EMMC_CLOCK(base) = _sd_emmc_clock.dw;
	usecwait(200);
}

static void
sd_emmc_set_bus_width(uintptr_t base, int bus_width)
{
	union sd_emmc_cfg _sd_emmc_cfg;
	_sd_emmc_cfg.dw = SD_EMMC_CFG(base);
	switch (bus_width) {
	case 1: _sd_emmc_cfg.Cfg_bus_width = 0; break;
	case 2: _sd_emmc_cfg.Cfg_bus_width = 3; break;
	case 4: _sd_emmc_cfg.Cfg_bus_width = 1; break;
	case 8: _sd_emmc_cfg.Cfg_bus_width = 2; break;
	}
	SD_EMMC_CFG(base) = _sd_emmc_cfg.dw;
}

static int
sd_emmc_send_cmd(uintptr_t base, struct sda_cmd *cmd)
{
	union sd_emmc_cmd_cfg cmd_cfg = {0};
	uint32_t data_addr = 0;

	cmd_cfg.Cmd_index = cmd->sc_index;
	switch (cmd->sc_rtype) {
	case R0:
		cmd_cfg.No_resp = 1;
		break;
	case R1b:
		cmd_cfg.R1b = 1;
		break;
	case R2:
		cmd_cfg.Resp_128 = 1;
		break;
	case R3:
	case R4:
		cmd_cfg.Resp_nocrc = 1;
		break;
	default:
		break;
	}

	if (cmd->sc_flags & (SDA_CMDF_READ | SDA_CMDF_WRITE)) {
		cmd_cfg.Data_io = 1;
		cmd_cfg.Data_wr = !!(cmd->sc_flags & SDA_CMDF_WRITE);
		data_addr = (uint32_t)(uintptr_t)cmd->sc_kvaddr;
		cache_flush(cmd->sc_kvaddr, cmd->sc_nblks * cmd->sc_blksz);
		if (cmd->sc_nblks > 1) {
			cmd_cfg.Block_mode = 1;
			cmd_cfg.Length = cmd->sc_nblks;
		} else {
			cmd_cfg.Block_mode = 0;
			cmd_cfg.Length = cmd->sc_blksz;
		}
	}

	SD_EMMC_STATUS(base) = SD_EMMC_STATUS(base);

	cmd_cfg.End_of_chain = 1;
	cmd_cfg.Owner = 1;

	SD_EMMC_CMD_CFG(base) = cmd_cfg.dw;
	SD_EMMC_CMD_DAT(base) = data_addr;
	SD_EMMC_CMD_ARG(base) = cmd->sc_argument; // start

	union sd_emmc_status _sd_emmc_status;
	for (;;) {
		_sd_emmc_status.dw = SD_EMMC_STATUS(base);
		if (_sd_emmc_status.End_of_Chain) {
			break;
		}
	}
	if (_sd_emmc_status.Rxd_err ||
	    _sd_emmc_status.Txd_err ||
	    _sd_emmc_status.Desc_err ||
	    _sd_emmc_status.Resp_err ||
	    _sd_emmc_status.Resp_timeout ||
	    _sd_emmc_status.Desc_timeout) {
		prom_printf("%s:%d status %08x\n", __func__,__LINE__, _sd_emmc_status.dw);
		return -1;
	}

	if (cmd->sc_flags & SDA_CMDF_READ) {
		cache_flush(cmd->sc_kvaddr, cmd->sc_nblks * cmd->sc_blksz);
	}

	switch (cmd->sc_rtype) {
	case R0:
		break;
	case R2:
		cmd->sc_response[3] = SD_EMMC_CMD_RSP0(base);
		cmd->sc_response[2] = SD_EMMC_CMD_RSP1(base);
		cmd->sc_response[1] = SD_EMMC_CMD_RSP2(base);
		cmd->sc_response[0] = SD_EMMC_CMD_RSP3(base);
		break;
	default:
		cmd->sc_response[0] = SD_EMMC_CMD_RSP0(base);
		break;
	}

        return 0;
}

struct mmc_slot {
	pnode_t			node;
	int			ss_blksz;
	int			ss_rcnt;
	uint32_t		ss_ocr;
	uint16_t		ss_resid;
	uint16_t		ss_mode;
	uint32_t		ss_cardclk;
	uint32_t		ss_width;
	uint32_t		ss_capab;

	uint32_t		f_min;
	uint32_t		f_max;
	uint32_t		ocr_avail;
	uint32_t		ocr;
	uint32_t		rca;
	uint32_t		s_maxclk;
	uint32_t		csd[4];
	uint32_t		scr[2];
	uint64_t		capacity;

	int power_level;
	struct gpio_ctrl	*gpio_cd;
	struct gpio_ctrl	*gpio_ro;
	struct gpio_ctrl	*gpio_power;
	struct gpio_ctrl	*gpio_volsw;
	struct gpio_ctrl	*gpio_dat3;
};

struct mmc_sc {
	pnode_t node;
	uintptr_t base;
	struct mmc_slot slot;
	char *buffer;
};

static struct mmc_sc mmc_dev[3];

static int
mmc_open(const char *name)
{
	pnode_t node;
	pnode_t slot_node;
	int fd;
	int len;
	uint32_t addr;
	char buf[80];
	struct mmc_sc *sc;
	uint64_t base;
	int i;

	for (fd = 0; fd < sizeof(mmc_dev) / sizeof(mmc_dev[0]); fd++) {
		if (mmc_dev[fd].node == 0)
			break;
	}

	if (fd == sizeof(mmc_dev) / sizeof(mmc_dev[0]))
		return -1;

	if (sscanf(name, "/soc/sd@%x/blkdev@0", &addr) == 1) {
		sprintf(buf, "/soc/sd@%x", addr);
	} else if (sscanf(name, "/soc/emmc@%x/blkdev@0", &addr) == 1) {
		sprintf(buf, "/soc/emmc@%x", addr);
	} else {
		return -1;
	}

	node = prom_finddevice(buf);
	if (node <= 0)
		return -1;

	len = prom_getproplen(node, "status");
	if (len <= 0 || len >= sizeof(buf))
		return -1;
	prom_getprop(node, "status", (caddr_t)buf);
	if (strcmp(buf, "ok") != 0 && strcmp(buf, "okay") != 0)
		return -1;

	if (prom_get_reg(node, 0, &base) < 0)
		return -1;

	slot_node = prom_childnode(node);
	if (slot_node < 0)
		return -1;

	char *buffer = malloc(BUFFER_SIZE + 2 * DCACHE_LINE);
	buffer = (char *)roundup((uintptr_t)buffer, DCACHE_LINE);

	sc = &mmc_dev[fd];
	sc->node = node;
	sc->base = base;
	sc->slot.node = slot_node;
	sc->buffer = buffer;

	sc->slot.gpio_cd = sd_emmc_gpio(sc->slot.node, "gpio_cd");
	sc->slot.gpio_ro = sd_emmc_gpio(sc->slot.node, "gpio_ro");
	sc->slot.gpio_power = sd_emmc_gpio(sc->slot.node, "gpio_power");
	sc->slot.gpio_volsw = sd_emmc_gpio(sc->slot.node, "gpio_volsw");
	sc->slot.gpio_dat3 = sd_emmc_gpio(sc->slot.node, "gpio_dat3");
	sc->slot.power_level = prom_get_prop_int(sc->slot.node, "power_level", 0);

	sc->slot.f_min = prom_get_prop_int(sc->slot.node, "f_min",   400000);
	sc->slot.f_max = prom_get_prop_int(sc->slot.node, "f_max", 50000000);
	sc->slot.ocr_avail = prom_get_prop_int(sc->slot.node, "ocr_avail", OCR_33_34V | OCR_32_33V | OCR_31_32V | OCR_18_19V);
	sc->slot.ss_cardclk = sc->slot.f_min;
	sc->slot.ss_width = 1;

	if (sd_emmc_pinmux(node, "sd_all_pins") < 0)
		if (sd_emmc_pinmux(node, "emmc_all_pins") < 0)
			return -1;

	if (sc->slot.gpio_cd) {
		if (plat_gpio_direction_input(sc->slot.gpio_cd) < 0)
			return -1;
		if (plat_gpio_get(sc->slot.gpio_cd)) {
			prom_printf("%s:%d card not detected\n", __func__,__LINE__);
			return -1;
		}
	}

	if (sc->slot.gpio_ro) {
		if (plat_gpio_direction_input(sc->slot.gpio_ro) < 0)
			return -1;
		if (plat_gpio_set_pullup(sc->slot.gpio_ro, 1) < 0)
			return -1;
	}

	if (sc->slot.gpio_power) {
		if (plat_gpio_direction_output(sc->slot.gpio_power, !sc->slot.power_level) < 0)
			return -1;
	}

	if (sc->slot.gpio_volsw) {
		if (plat_gpio_direction_output(sc->slot.gpio_volsw, 0) < 0)
			return -1;
	}

	// init
	int clk_src = 0;
	union sd_emmc_clock _sd_emmc_clock = { .dw = 0};
	_sd_emmc_clock.Cfg_always_on = 1;
	_sd_emmc_clock.Cfg_co_phase = 2;
	_sd_emmc_clock.Cfg_src = clk_src;
	_sd_emmc_clock.Cfg_div = sd_emmc_clocks[clk_src] / sc->slot.ss_cardclk;
	SD_EMMC_CLOCK(sc->base) = _sd_emmc_clock.dw;
	usecwait(200);

	union sd_emmc_cfg _sd_emmc_cfg;
	_sd_emmc_cfg.dw = SD_EMMC_CFG(sc->base);
	_sd_emmc_cfg.Cfg_bus_width = 0;
	_sd_emmc_cfg.Cfg_bl_len = 9;	// 512 (1 << 9)
	_sd_emmc_cfg.Cfg_resp_timeout = 0x7;
	_sd_emmc_cfg.Cfg_rc_cc = 4;
	SD_EMMC_CFG(sc->base) = _sd_emmc_cfg.dw;
	usecwait(200);
	SD_EMMC_IRQ_EN(sc->base) = 0; //_sd_emmc_irq_en.dw;

	// power on
	if (sc->slot.gpio_power) {
		if (plat_gpio_direction_output(sc->slot.gpio_power, sc->slot.power_level) < 0)
			return -1;
	}

	usecwait(200);
	{
		struct sda_cmd cmd = {
			.sc_index = CMD_GO_IDLE,
			.sc_rtype = R0,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
	}

	{
		struct sda_cmd cmd = {
			.sc_index = CMD_SEND_IF_COND,
			.sc_rtype = R7,
			.sc_argument = ((!!(sc->slot.ocr_avail & OCR_HI_MASK)) << 8) | 0xaa,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
		if ((cmd.sc_response[0] & 0xff) != 0xaa)
			return -1;
	}

	for (i = 0; i < 1000; i++) {
		struct sda_cmd acmd = {
			.sc_index = CMD_APP_CMD,
			.sc_rtype = R1,
		};
		if (sd_emmc_send_cmd(sc->base, &acmd) < 0)
			return -1;

		struct sda_cmd cmd = {
			.sc_index = ACMD_SD_SEND_OCR,
			.sc_rtype = R3,
			.sc_argument = (sc->slot.ocr_avail & OCR_HI_MASK) | OCR_CCS,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
		if (cmd.sc_response[0] & OCR_POWER_UP) {
			sc->slot.ocr = cmd.sc_response[0];
			sc->slot.rca = 0;
			break;
		}
		usecwait(1000);
	}
	if (i >= 1000)
		return -1;

	{
		struct sda_cmd cmd = {0};
		cmd.sc_index = CMD_BCAST_CID;
		cmd.sc_rtype = R2;
		cmd.sc_argument = 0;

		sd_emmc_send_cmd(sc->base, &cmd);
		uint32_t cid[4];
		memcpy(cid, cmd.sc_response, sizeof(cid));
#if 0
		prom_printf("Man %06x Snr %04x%04x",
		    cid[0] >> 24, (cid[2] & 0xffff),
		    (cid[3] >> 16) & 0xffff);
		prom_printf("%c%c%c%c%c%c", cid[0] & 0xff,
		    (cid[1] >> 24), (cid[1] >> 16) & 0xff,
		    (cid[1] >> 8) & 0xff, cid[1] & 0xff,
		    (cid[2] >> 24) & 0xff);
		prom_printf("%d.%d\n", (cid[2] >> 20) & 0xf,
		    (cid[2] >> 16) & 0xf);
#endif
	}

	{
		struct sda_cmd cmd = {
			.sc_index = CMD_SEND_RCA,
			.sc_rtype = R6,
			.sc_argument = sc->slot.rca << 16,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
		sc->slot.rca = (cmd.sc_response[0] >> 16) & 0xffff;
		//prom_printf("%s:%d rca=%08x\n", __func__,__LINE__, sc->slot.rca);
	}

	{
		struct sda_cmd cmd = {
			.sc_index = CMD_SEND_CSD,
			.sc_rtype = R2,
			.sc_argument = sc->slot.rca << 16,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
		memcpy(sc->slot.csd, cmd.sc_response, sizeof(sc->slot.csd));

		static const uint32_t	mult[16] = { 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
		static const uint32_t	units[8] = { 10000, 100000, 1000000, 10000000, 0, 0, 0, 0, };
		sc->slot.s_maxclk = units[sc->slot.csd[0] & 0x7] * mult[(sc->slot.csd[0] >> 3) & 0xF];
		//prom_printf("%s:%d maxclk=%d\n", __func__,__LINE__, sc->slot.s_maxclk);
		sc->slot.ss_blksz = (1 << ((sc->slot.csd[1] >> 16) & 0xF));
		//prom_printf("%s:%d blksz=%d\n", __func__,__LINE__, sc->slot.ss_blksz);
		//prom_printf("%s:%d write blksz=%d\n", __func__,__LINE__, (1 << ((sc->slot.csd[3] >> 22) & 0xF)));
		if (sc->slot.ocr & OCR_CCS) {
			uint64_t csize = (((sc->slot.csd[1] & 0x3f) << 16) | (sc->slot.csd[2] >> 16));
			uint64_t cmult = 8;
			sc->slot.capacity = ((csize + 1) << (cmult + 2));
			sc->slot.capacity *= sc->slot.ss_blksz;
		} else {
			uint64_t csize = (((sc->slot.csd[1] & 0x3ff) << 2) | (sc->slot.csd[2] >> 30));
			uint64_t cmult = ((sc->slot.csd[2] >> 15) & 0x7);
			sc->slot.capacity = ((csize + 1) << (cmult + 2));
			sc->slot.capacity *= sc->slot.ss_blksz;
		}
		//prom_printf("%s:%d capacity=%ld\n", __func__,__LINE__, sc->slot.capacity);
	}

	{
		struct sda_cmd cmd = {
			.sc_index = CMD_SEND_CSD,
			.sc_rtype = R2,
			.sc_argument = sc->slot.rca << 16,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
		memcpy(sc->slot.csd, cmd.sc_response, sizeof(sc->slot.csd));

		static const uint32_t	mult[16] = { 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
		static const uint32_t	units[8] = { 10000, 100000, 1000000, 10000000, 0, 0, 0, 0, };
		sc->slot.s_maxclk = units[sc->slot.csd[0] & 0x7] * mult[(sc->slot.csd[0] >> 3) & 0xF];
		//prom_printf("%s:%d maxclk=%d\n", __func__,__LINE__, sc->slot.s_maxclk);
		sc->slot.ss_blksz = (1 << ((sc->slot.csd[1] >> 16) & 0xF));
		//prom_printf("%s:%d blksz=%d\n", __func__,__LINE__, sc->slot.ss_blksz);
		//prom_printf("%s:%d write blksz=%d\n", __func__,__LINE__, (1 << ((sc->slot.csd[3] >> 22) & 0xF)));
		if (sc->slot.ocr & OCR_CCS) {
			uint64_t csize = (((sc->slot.csd[1] & 0x3f) << 16) | (sc->slot.csd[2] >> 16));
			uint64_t cmult = 8;
			sc->slot.capacity = ((csize + 1) << (cmult + 2));
			sc->slot.capacity *= sc->slot.ss_blksz;
		} else {
			uint64_t csize = (((sc->slot.csd[1] & 0x3ff) << 2) | (sc->slot.csd[2] >> 30));
			uint64_t cmult = ((sc->slot.csd[2] >> 15) & 0x7);
			sc->slot.capacity = ((csize + 1) << (cmult + 2));
			sc->slot.capacity *= sc->slot.ss_blksz;
		}
		//prom_printf("%s:%d capacity=%ld\n", __func__,__LINE__, sc->slot.capacity);
	}

	for (i = 0; i < 1000; i++) {
		struct sda_cmd cmd = {
			.sc_index = CMD_SEND_STATUS,
			.sc_rtype = R1,
			.sc_argument = sc->slot.rca << 16,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) == 0) {
			if ((cmd.sc_response[0] & R1_READY_FOR_DATA) && (R1_STATE(cmd.sc_response[0]) != 7))
				break;
		}
		usecwait(1000);
	}
	if (i >= 1000)
		return -1;
	{
		struct sda_cmd cmd = {
			.sc_index = CMD_SELECT_CARD,
			.sc_rtype = R1,
			.sc_argument = sc->slot.rca << 16,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
	}

	{
		struct sda_cmd acmd = {
			.sc_index = CMD_APP_CMD,
			.sc_rtype = R1,
			.sc_argument = sc->slot.rca << 16,
		};
		if (sd_emmc_send_cmd(sc->base, &acmd) < 0)
			return -1;
	}

	for (i = 0; i < 3; i++) {
		memset(buffer, 0, MMU_PAGESIZE);
		struct sda_cmd cmd = {
			.sc_index = ACMD_SEND_SCR,
			.sc_rtype = R1,

			.sc_nblks = 1,
			.sc_blksz = 8,
			.sc_flags = SDA_CMDF_READ,
			.sc_kvaddr = buffer,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) == 0)
			break;
	}
	if (i >= 3)
		return -1;
	sc->slot.scr[0] = ntohl(*(uint32_t *)(buffer + 0));
	sc->slot.scr[1] = ntohl(*(uint32_t *)(buffer + 4));

	for (i = 0; i < 4; i++) {
		memset(buffer, 0, MMU_PAGESIZE);
		struct sda_cmd cmd = {
			.sc_index = CMD_SWITCH_FUNC,
			.sc_rtype = R1,
			.sc_nblks = 1,
			.sc_blksz = 64,
			.sc_flags = SDA_CMDF_READ,
			.sc_kvaddr = buffer,
		};
		cmd.sc_argument = (0u << 31) | 0xffffff;
		cmd.sc_argument &= ~(0xf << (0 << 2));
		cmd.sc_argument |=  (0x1 << (0 << 2));
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
		if (!(ntohl(*(uint32_t *)(buffer + 4 * 7)) & 0x00020000))
			break;
	}
	if (i >= 4)
		return -1;
	if ((ntohl(*(uint32_t *)(buffer + 4 * 3)) & 0x00020000)) {
		memset(buffer, 0, MMU_PAGESIZE);
		prom_printf("High Speed supported\n");
		// high speed
		struct sda_cmd cmd = {
			.sc_index = CMD_SWITCH_FUNC,
			.sc_rtype = R1,
			.sc_nblks = 1,
			.sc_blksz = 64,
			.sc_flags = SDA_CMDF_READ,
			.sc_kvaddr = buffer,
		};
		cmd.sc_argument = (1u << 31) | 0xffffff;
		cmd.sc_argument &= ~(0xf << (0 << 2));
		cmd.sc_argument |=  (0x1 << (0 << 2));
		if (sd_emmc_send_cmd(sc->base, &cmd) == 0) {
			if ((ntohl(*(uint32_t *)(buffer + 4 * 4)) & 0x0f000000) == 0x01000000) {
				sc->slot.s_maxclk = 50000000;
			}
		}
	}

	// 4bit
	if (sc->slot.scr[0] & 0x00040000) {
		prom_printf("4bit width\n");
		struct sda_cmd acmd = {
			.sc_index = CMD_APP_CMD,
			.sc_rtype = R1,
			.sc_argument = sc->slot.rca << 16,
		};
		if (sd_emmc_send_cmd(sc->base, &acmd) < 0)
			return -1;

		struct sda_cmd cmd = {
			.sc_index = ACMD_SET_BUS_WIDTH,
			.sc_rtype = R1,
			.sc_argument = 2,
		};
		if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
			return -1;
		sc->slot.ss_width = 4;
		sd_emmc_set_bus_width(sc->base, sc->slot.ss_width);
	}
	sd_emmc_set_clock(sc->base, sc->slot.s_maxclk);

	return fd;
}

static ssize_t
mmc_read(int dev, caddr_t buf, size_t buf_len, uint_t startblk)
{
	size_t read_size = buf_len;
	struct mmc_sc *sc = &mmc_dev[dev];
	if ((uintptr_t)buf & (DCACHE_LINE - 1)) {
		while (read_size / BUFFER_SIZE) {
			{
				struct sda_cmd cmd = {
					.sc_index = CMD_READ_MULTI,
					.sc_rtype = R1,
					.sc_nblks = BUFFER_SIZE / sc->slot.ss_blksz,
					.sc_blksz = sc->slot.ss_blksz,
					.sc_flags = SDA_CMDF_READ,
					.sc_kvaddr = sc->buffer,
				};
				if (sc->slot.ocr & OCR_CCS) {
					cmd.sc_argument = startblk;
				} else {
					cmd.sc_argument = startblk * sc->slot.ss_blksz;
				}
				if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
					return -1;
			}
			memcpy(buf, sc->buffer, BUFFER_SIZE);
			{
				struct sda_cmd cmd = {
					.sc_index = CMD_STOP_TRANSMIT,
					.sc_rtype = R1b,
				};
				if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
					return -1;
			}
			buf += BUFFER_SIZE;
			startblk += BUFFER_SIZE / sc->slot.ss_blksz;
			read_size -= BUFFER_SIZE;
		}

		{
			struct sda_cmd cmd = {
				.sc_index = CMD_READ_MULTI,
				.sc_rtype = R1,
				.sc_nblks = read_size / sc->slot.ss_blksz,
				.sc_blksz = sc->slot.ss_blksz,
				.sc_flags = SDA_CMDF_READ,
				.sc_kvaddr = sc->buffer,
			};
			if (sc->slot.ocr & OCR_CCS) {
				cmd.sc_argument = startblk;
			} else {
				cmd.sc_argument = startblk * sc->slot.ss_blksz;
			}
			if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
				return -1;
		}
		memcpy(buf, sc->buffer, read_size);
		{
			struct sda_cmd cmd = {
				.sc_index = CMD_STOP_TRANSMIT,
				.sc_rtype = R1b,
			};
			if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
				return -1;
		}
	} else {
		{
			struct sda_cmd cmd = {
				.sc_index = CMD_READ_MULTI,
				.sc_rtype = R1,
				.sc_nblks = read_size / sc->slot.ss_blksz,
				.sc_blksz = sc->slot.ss_blksz,
				.sc_flags = SDA_CMDF_READ,
				.sc_kvaddr = buf,
			};
			if (sc->slot.ocr & OCR_CCS) {
				cmd.sc_argument = startblk;
			} else {
				cmd.sc_argument = startblk * sc->slot.ss_blksz;
			}
			if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
				return -1;
		}

		{
			struct sda_cmd cmd = {
				.sc_index = CMD_STOP_TRANSMIT,
				.sc_rtype = R1b,
			};
			if (sd_emmc_send_cmd(sc->base, &cmd) < 0)
				return -1;
		}
	}
	return buf_len;
}

static int
mmc_match(const char *path)
{
	const char *cmp;

	cmp = "/soc/sd@";
	if (strncmp(path, cmp, strlen(cmp)) == 0)
		return 1;

	cmp = "/soc/emmc@";
	if (strncmp(path, cmp, strlen(cmp)) == 0)
		return 1;

	return 0;
}

static struct prom_dev mmc_prom_dev =
{
	.match = mmc_match,
	.open = mmc_open,
	.read = mmc_read,
};

void init_mmc(void)
{
	prom_register(&mmc_prom_dev);
}

