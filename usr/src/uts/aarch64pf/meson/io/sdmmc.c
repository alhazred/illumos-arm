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
 * Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
 */

#include <sys/types.h>
#include <sys/ksynch.h>
#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/inttypes.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/sdcard/sda.h>
#include <sys/kstat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/platform.h>
#include <sys/platmod.h>
#include <sys/promif.h>
#include <sys/byteorder.h>
#include <sys/gpio.h>
#include <sys/controlregs.h>
#include <sys/sysmacros.h>

#define CAPAB_HIGH_SPEED	(1u << 0)
#define CAPAB_4BIT_DATA		(1u << 1)
#define CAPAB_8BIT_DATA		(1u << 2)

static void
cache_flush(void *addr, size_t size)
{
	size_t line_size = CTR_TO_DATA_LINESIZE(read_ctr_el0());
	for (uintptr_t v = (uintptr_t)addr & ~(line_size - 1); v < (uintptr_t)addr + size; v += line_size) {
		flush_data_cache(v);
	}
	dmb(sy);
}
typedef	struct sdstats	sdstats_t;
typedef	struct sdslot	sdslot_t;
typedef	struct sdhost	sdhost_t;

struct sdstats {
	kstat_named_t	ks_ncmd;
	kstat_named_t	ks_ixfr;
	kstat_named_t	ks_oxfr;
	kstat_named_t	ks_ibytes;
	kstat_named_t	ks_obytes;
	kstat_named_t	ks_npio;
	kstat_named_t	ks_ndma;
	kstat_named_t	ks_nmulti;
	kstat_named_t	ks_baseclk;
	kstat_named_t	ks_cardclk;
	kstat_named_t	ks_tmusecs;
	kstat_named_t	ks_width;
	kstat_named_t	ks_flags;
	kstat_named_t	ks_capab;
};

#define	SDFLAG_FORCE_PIO		(1U << 0)
#define	SDFLAG_FORCE_DMA		(1U << 1)

/*
 * Per slot state.
 */
struct sdslot {
	pnode_t			ss_node;
	sdhost_t		*ss_host;
	int			ss_num;
	ddi_acc_handle_t	ss_acch;
	caddr_t 		ss_regva;
	uint8_t			ss_tmoutclk;
	uint32_t		ss_ocr;		/* OCR formatted voltages */
	uint16_t		ss_mode;
	boolean_t		ss_suspended;
	sdstats_t		ss_stats;
#define	ss_ncmd			ss_stats.ks_ncmd.value.ui64
#define	ss_ixfr			ss_stats.ks_ixfr.value.ui64
#define	ss_oxfr			ss_stats.ks_oxfr.value.ui64
#define	ss_ibytes		ss_stats.ks_ibytes.value.ui64
#define	ss_obytes		ss_stats.ks_obytes.value.ui64
#define	ss_ndma			ss_stats.ks_ndma.value.ui64
#define	ss_npio			ss_stats.ks_npio.value.ui64
#define	ss_nmulti		ss_stats.ks_nmulti.value.ui64

#define	ss_baseclk		ss_stats.ks_baseclk.value.ui32
#define	ss_cardclk		ss_stats.ks_cardclk.value.ui32
#define	ss_tmusecs		ss_stats.ks_tmusecs.value.ui32
#define	ss_width		ss_stats.ks_width.value.ui32
#define	ss_flags		ss_stats.ks_flags.value.ui32
#define	ss_capab		ss_stats.ks_capab.value.ui32
	kstat_t			*ss_ksp;

	/*
	 * Command in progress
	 */
	uint8_t			*ss_kvaddr;
	int			ss_blksz;
	uint16_t		ss_resid;	/* in blocks */
	int			ss_rcnt;

	/* scratch buffer, to receive extra PIO data */
	caddr_t			ss_bounce;
	ddi_dma_handle_t	ss_bufdmah;
	ddi_acc_handle_t	ss_bufacch;
	ddi_dma_cookie_t	ss_bufdmac;

	int			power_level;
	struct gpio_ctrl	*gpio_cd;
	struct gpio_ctrl	*gpio_ro;
	struct gpio_ctrl	*gpio_power;
	struct gpio_ctrl	*gpio_volsw;
	struct gpio_ctrl	*gpio_dat3;
};

/*
 * This allocates a rather large chunk of contiguous memory for DMA.
 * But doing so means that we'll almost never have to resort to PIO.
 */
#define	SDHOST_BOUNCESZ		65536
#define	SDHOST_MAXSLOTS		1

/*
 * Per controller state.
 */
struct sdhost {
	int			sh_numslots;
	sdslot_t		sh_slots[SDHOST_MAXSLOTS];
	sda_host_t		*sh_host;
	pnode_t			sh_node;
	dev_info_t		*sh_dip;
	kmutex_t		sh_lock;
	kcondvar_t		sh_cv;
	kcondvar_t		sh_waitcv;
	int			sh_use;
	int			sh_waiter;

	ddi_acc_handle_t	sh_handle;
	caddr_t			sh_addr;
	ddi_intr_handle_t	sh_ihandle;

	uint32_t		sh_status;
};

#define	PROPSET(x)							\
	(ddi_prop_get_int(DDI_DEV_T_ANY, dip,				\
	DDI_PROP_DONTPASS | DDI_PROP_NOTPROM, x, 0) != 0)

#define REG_WRITE(shp, reg, val) \
	ddi_put32((shp)->sh_handle, (void *)((shp)->sh_addr + (reg)), (val))

#define REG_READ(shp, reg) \
	ddi_get32((shp)->sh_handle, (void *)((shp)->sh_addr + (reg)))

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

union sd_emmc_start {
	uint32_t dw;
	struct {
		uint32_t Desc_int	: 1;
		uint32_t Desc_busy	: 1;
		uint32_t Desc_addr	: 30;
	};
};

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

#define SD_EMMC_CLOCK		0x00
#define SD_EMMC_DELAY		0x04
#define SD_EMMC_ADJUST		0x08
#define SD_EMMC_CALOUT		0x10
#define SD_EMMC_START		0x40
#define SD_EMMC_CFG		0x44
#define SD_EMMC_STATUS		0x48
#define SD_EMMC_IRQ_EN		0x4c
#define SD_EMMC_CMD_CFG		0x50
#define SD_EMMC_CMD_ARG		0x54
#define SD_EMMC_CMD_DAT		0x58
#define SD_EMMC_CMD_RSP0	0x5c
#define SD_EMMC_CMD_RSP1	0x60
#define SD_EMMC_CMD_RSP2	0x64
#define SD_EMMC_CMD_RSP3	0x68

static int sdhost_probe(dev_info_t *);
static int sdhost_attach(dev_info_t *, ddi_attach_cmd_t);
static int sdhost_detach(dev_info_t *, ddi_detach_cmd_t);
static int sdhost_quiesce(dev_info_t *);
static int sdhost_suspend(dev_info_t *);
static int sdhost_resume(dev_info_t *);

static void sdhost_enable_interrupts(sdslot_t *);
static void sdhost_disable_interrupts(sdslot_t *);
static int sdhost_setup_intr(dev_info_t *, sdhost_t *);
static uint_t sdhost_intr(caddr_t, caddr_t);
static int sdhost_init_slot(sdhost_t *, int, pnode_t);
static void sdhost_uninit_slot(sdhost_t *, int);
static sda_err_t sdhost_soft_reset(sdslot_t *, uint8_t);
static sda_err_t sdhost_set_clock(sdslot_t *, uint32_t);
static sda_err_t sdhost_set_width(sdslot_t *, uint32_t);
static void sdhost_xfer_done(sdslot_t *, sda_err_t);
static sda_err_t sdhost_wait_cmd(sdslot_t *, sda_cmd_t *);
static uint_t sdhost_slot_intr(sdslot_t *);

static sda_err_t sdhost_cmd(void *, sda_cmd_t *);
static sda_err_t sdhost_getprop(void *, sda_prop_t, uint32_t *);
static sda_err_t sdhost_setprop(void *, sda_prop_t, uint32_t);
static sda_err_t sdhost_poll(void *);
static sda_err_t sdhost_reset(void *);
static sda_err_t sdhost_halt(void *);

static struct dev_ops sdhost_dev_ops = {
	DEVO_REV,			/* devo_rev */
	0,				/* devo_refcnt */
	ddi_no_info,			/* devo_getinfo */
	nulldev,			/* devo_identify */
	sdhost_probe,			/* devo_probe */
	sdhost_attach,			/* devo_attach */
	sdhost_detach,			/* devo_detach */
	nodev,				/* devo_reset */
	NULL,				/* devo_cb_ops */
	NULL,				/* devo_bus_ops */
	NULL,				/* devo_power */
	sdhost_quiesce,			/* devo_quiesce */
};

static struct modldrv sdhost_modldrv = {
	&mod_driverops,			/* drv_modops */
	"Standard SD Host Controller",	/* drv_linkinfo */
	&sdhost_dev_ops			/* drv_dev_ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,			/* ml_rev */
	{ &sdhost_modldrv, NULL }	/* ml_linkage */
};

static struct sda_ops sdhost_ops = {
	SDA_OPS_VERSION,
	sdhost_cmd,			/* so_cmd */
	sdhost_getprop,			/* so_getprop */
	sdhost_setprop,			/* so_setprop */
	sdhost_poll,			/* so_poll */
	sdhost_reset,			/* so_reset */
	sdhost_halt,			/* so_halt */
};

static ddi_device_acc_attr_t sdhost_regattr = {
	DDI_DEVICE_ATTR_V0,	/* devacc_attr_version */
	DDI_STRUCTURE_LE_ACC,	/* devacc_attr_endian_flags */
	DDI_STRICTORDER_ACC,	/* devacc_attr_dataorder */
	DDI_DEFAULT_ACC,	/* devacc_attr_access */
};
static ddi_device_acc_attr_t sdhost_bufattr = {
	DDI_DEVICE_ATTR_V0,	/* devacc_attr_version */
	DDI_NEVERSWAP_ACC,	/* devacc_attr_endian_flags */
	DDI_STRICTORDER_ACC,	/* devacc_attr_dataorder */
	DDI_DEFAULT_ACC,	/* devacc_attr_access */
};

static ddi_dma_attr_t dma_attr = {
	DMA_ATTR_V0,			/* dma_attr_version	*/
	0x0000000000000000ull,		/* dma_attr_addr_lo	*/
	0x00000000FFFFFFFFull,		/* dma_attr_addr_hi	*/
	0x00000000FFFFFFFFull,		/* dma_attr_count_max	*/
	0x0000000000001000ull,		/* dma_attr_align	*/
	0x00000000,			/* dma_attr_burstsizes	*/
	0x00000001,			/* dma_attr_minxfer	*/
	0x00000000FFFFFFFFull,		/* dma_attr_maxxfer	*/
	0x00000000FFFFFFFFull,		/* dma_attr_seg		*/
	1,				/* dma_attr_sgllen	*/
	0x00000001,			/* dma_attr_granular	*/
	DDI_DMA_FLAGERR			/* dma_attr_flags	*/
};

int
_init(void)
{
	int	rv;

	sda_host_init_ops(&sdhost_dev_ops);

	if ((rv = mod_install(&modlinkage)) != 0) {
		sda_host_fini_ops(&sdhost_dev_ops);
	}

	return (rv);
}

int
_fini(void)
{
	int	rv;

	if ((rv = mod_remove(&modlinkage)) == 0) {
		sda_host_fini_ops(&sdhost_dev_ops);
	}
	return (rv);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

int
sdhost_probe(dev_info_t *dip)
{
	int len;
	char buf[80];
	pnode_t node = ddi_get_nodeid(dip);
	if (node < 0)
		return (DDI_PROBE_FAILURE);

	len = prom_getproplen(node, "status");
	if (len <= 0 || len >= sizeof(buf))
		return (DDI_PROBE_FAILURE);

	prom_getprop(node, "status", (caddr_t)buf);
	if (strcmp(buf, "ok") != 0 && (strcmp(buf, "okay") != 0))
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

static pnode_t
sdhost_get_pinmux(struct sdhost *shp, const char *pinname)
{
	pnode_t node = shp->sh_node;
	int len;
	int pinctrl_index = prom_get_prop_index(node, "pinctrl-names", pinname);
	if (pinctrl_index < 0)
		return OBP_NONODE;
	char buf[80];
	sprintf(buf, "pinctrl-%d", pinctrl_index);
	len = prom_getproplen(node, buf);
	if (len != sizeof(uint32_t))
		return OBP_NONODE;
	uint32_t pinctrl;
	prom_getprop(node, buf, (caddr_t)&pinctrl);
	return prom_findnode_by_phandle(ntohl(pinctrl));
}

int
sdhost_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	sdhost_t		*shp;
	uint8_t			slotinfo;
	uint8_t			bar;
	int			i;
	int			rv;

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
	default:
		return (DDI_FAILURE);
	}

	/*
	 * Soft state allocation.
	 */
	shp = kmem_zalloc(sizeof (*shp), KM_SLEEP);
	ddi_set_driver_private(dip, shp);

	shp->sh_node = ddi_get_nodeid(dip);
	shp->sh_dip = dip;
	mutex_init(&shp->sh_lock, NULL, MUTEX_DRIVER, NULL);
	cv_init(&shp->sh_cv, NULL, CV_DRIVER, NULL);
	cv_init(&shp->sh_waitcv, NULL, CV_DRIVER, NULL);
	shp->sh_use = -1;
	shp->sh_waiter = 0;

	if (ddi_regs_map_setup(shp->sh_dip, 0, &shp->sh_addr, 0, 0, &sdhost_regattr, &shp->sh_handle) != DDI_SUCCESS) {
		goto failed;
	}
	int actual;
	if (ddi_intr_alloc(shp->sh_dip, &shp->sh_ihandle, DDI_INTR_TYPE_FIXED, 0, 1, &actual, DDI_INTR_ALLOC_STRICT) != DDI_SUCCESS) {
		goto failed;
	}

	if (ddi_intr_add_handler(shp->sh_ihandle, sdhost_intr, shp, NULL) != DDI_SUCCESS) {
		goto failed;
	}

	/*
	 * Reset the "slot number", so uninit slot works properly.
	 */
	for (i = 0; i < SDHOST_MAXSLOTS; i++) {
		shp->sh_slots[i].ss_num = -1;
	}

	pnode_t slot_node = prom_childnode(shp->sh_node);
	shp->sh_numslots = 0;
	for (i = 0; i < SDHOST_MAXSLOTS; i++) {
		if (slot_node <= 0)
			break;
		shp->sh_numslots++;
		slot_node = prom_nextnode(slot_node);
	}

	shp->sh_host = sda_host_alloc(dip, shp->sh_numslots, &sdhost_ops, NULL);
	if (shp->sh_host == NULL) {
		cmn_err(CE_WARN, "Failed allocating SD host structure");
		goto failed;
	}

	pnode_t pinctrl_node = sdhost_get_pinmux(shp, "sd_all_pins");
	if (pinctrl_node < 0)
		pinctrl_node = sdhost_get_pinmux(shp, "emmc_all_pins");
	if (pinctrl_node > 0)
		plat_pinmux_set(pinctrl_node);

	union sd_emmc_clock _sd_emmc_clock = { .dw = 0};
	_sd_emmc_clock.Cfg_always_on = 1;
	_sd_emmc_clock.Cfg_co_phase = 2;
	_sd_emmc_clock.Cfg_src = 0;
	_sd_emmc_clock.Cfg_div = 1;
	REG_WRITE(shp, SD_EMMC_CLOCK, _sd_emmc_clock.dw);
	drv_usecwait(200);

	union sd_emmc_cfg _sd_emmc_cfg;
	_sd_emmc_cfg.dw = REG_READ(shp, SD_EMMC_CFG);
	_sd_emmc_cfg.Cfg_bus_width = 0;
	_sd_emmc_cfg.Cfg_bl_len = 9;	// 512
	_sd_emmc_cfg.Cfg_resp_timeout = 0x7;
	_sd_emmc_cfg.Cfg_rc_cc = 4;
	REG_WRITE(shp, SD_EMMC_CFG, _sd_emmc_cfg.dw);
	drv_usecwait(200);

	REG_WRITE(shp, SD_EMMC_STATUS, REG_READ(shp, SD_EMMC_STATUS));

	union sd_emmc_irq_en _sd_emmc_irq_en = { .dw = 0};
	_sd_emmc_irq_en.En_Rxd_err = 1;
	_sd_emmc_irq_en.En_Txd_err = 1;
	_sd_emmc_irq_en.En_Desc_err = 1;
	_sd_emmc_irq_en.En_Resp_err = 1;
	_sd_emmc_irq_en.En_Resp_timeout = 1;
	_sd_emmc_irq_en.En_Desc_timeout = 1;
	_sd_emmc_irq_en.En_End_of_Chain = 1;
	REG_WRITE(shp, SD_EMMC_IRQ_EN, _sd_emmc_irq_en.dw);

	/*
	 * Configure slots, this also maps registers, enables
	 * interrupts, etc.  Most of the hardware setup is done here.
	 */
	slot_node = prom_childnode(shp->sh_node);
	for (i = 0; i < shp->sh_numslots; i++) {
		if (sdhost_init_slot(shp, i, slot_node) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "Failed initializing slot %d", i);
			goto failed;
		}
		slot_node = prom_nextnode(slot_node);
	}

	ddi_report_dev(dip);

	/*
	 * Enable device interrupts at the DDI layer.
	 */
	rv = ddi_intr_enable(shp->sh_ihandle);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN, "Failed enabling interrupts");
		goto failed;
	}

	/*
	 * Mark the slots online with the framework.  This will cause
	 * the framework to probe them for the presence of cards.
	 */
	if (sda_host_attach(shp->sh_host) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "Failed attaching to SDA framework");
		(void) ddi_intr_disable(shp->sh_ihandle);
		goto failed;
	}

	return (DDI_SUCCESS);

failed:
	if (shp->sh_ihandle != NULL) {
		(void) ddi_intr_remove_handler(shp->sh_ihandle);
		(void) ddi_intr_free(shp->sh_ihandle);
	}
	for (i = 0; i < shp->sh_numslots; i++)
		sdhost_uninit_slot(shp, i);
	if (shp->sh_host != NULL)
		sda_host_free(shp->sh_host);
	kmem_free(shp, sizeof (*shp));

	return (DDI_FAILURE);
}

int
sdhost_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	return (DDI_SUCCESS);
}

int
sdhost_quiesce(dev_info_t *dip)
{
	return (DDI_SUCCESS);
}

int
sdhost_suspend(dev_info_t *dip)
{
	return (DDI_SUCCESS);
}

int
sdhost_resume(dev_info_t *dip)
{
	return (DDI_SUCCESS);
}

sda_err_t
sdhost_set_clock(sdslot_t *ss, uint32_t hz)
{
	if (hz < prom_get_prop_int(ss->ss_node, "f_min",   400000))
		hz = prom_get_prop_int(ss->ss_node, "f_min",   400000);
	if (hz > prom_get_prop_int(ss->ss_node, "f_max",   50000000))
		hz = prom_get_prop_int(ss->ss_node, "f_max",   50000000);

	static const uint32_t sd_emmc_clocks[] = {
		24000000,
		1000000000,
	};
	sdhost_t	*shp = ss->ss_host;
	int clk_src = ((hz > 12000000)? 1: 0);
	ss->ss_baseclk = sd_emmc_clocks[clk_src];
	ss->ss_cardclk = hz;
	uint32_t div = hz? ss->ss_baseclk / hz: 0;

	union sd_emmc_clock _sd_emmc_clock;
	_sd_emmc_clock.dw = REG_READ(shp, SD_EMMC_CLOCK);
	_sd_emmc_clock.Cfg_src = clk_src;
	_sd_emmc_clock.Cfg_div = div;
	REG_WRITE(shp, SD_EMMC_CLOCK, _sd_emmc_clock.dw);
	drv_usecwait(200);
	return SDA_EOK;
}

sda_err_t
sdhost_set_width(sdslot_t *ss, uint32_t val)
{
	sda_err_t	rv = SDA_EOK;
	sdhost_t	*shp = ss->ss_host;

	union sd_emmc_cfg _sd_emmc_cfg;
	switch (val) {
	case 1:
	case 4:
		ss->ss_width = val;
		_sd_emmc_cfg.dw = REG_READ(shp, SD_EMMC_CFG);
		_sd_emmc_cfg.Cfg_bus_width = val / 4;
		REG_WRITE(shp, SD_EMMC_CFG, _sd_emmc_cfg.dw);
		break;
	default:
		rv = SDA_EINVAL;
	}
	return rv;
}

sda_err_t
sdhost_soft_reset(sdslot_t *ss, uint8_t bits)
{
	return (SDA_ETIME);
}

void
sdhost_disable_interrupts(sdslot_t *ss)
{
}

void
sdhost_enable_interrupts(sdslot_t *ss)
{
}

int
sdhost_setup_intr(dev_info_t *dip, sdhost_t *shp)
{
	return (DDI_FAILURE);
}

void
sdhost_xfer_done(sdslot_t *ss, sda_err_t errno)
{
	sda_host_transfer(ss->ss_host->sh_host, ss->ss_num, errno);
}



uint_t
sdhost_intr(caddr_t arg1, caddr_t arg2)
{
	sdhost_t	*shp = (void *)arg1;

	mutex_enter(&shp->sh_lock);

	uint32_t status = REG_READ(shp, SD_EMMC_STATUS);
	REG_WRITE(shp, SD_EMMC_STATUS, status);
	if (shp->sh_use != -1) {
		shp->sh_status |= status;
		cv_signal(&shp->sh_cv);
	}

	mutex_exit(&shp->sh_lock);

	return DDI_INTR_CLAIMED;
}

static struct gpio_ctrl *
sdhost_gpio(sdslot_t *ss, const char *name)
{
	pnode_t node = ss->ss_node;
	uint32_t gpio_buf[3];
	int len = prom_getproplen(node, name);
	if (len != sizeof(gpio_buf))
		return NULL;
	prom_getprop(node, name, (caddr_t)&gpio_buf);
	struct gpio_ctrl *gpio = (struct gpio_ctrl*)kmem_zalloc(sizeof(struct gpio_ctrl), KM_SLEEP);
	gpio->node = prom_findnode_by_phandle(ntohl(gpio_buf[0]));
	gpio->pin = ntohl(gpio_buf[1]);
	gpio->flags = ntohl(gpio_buf[2]);
	return gpio;
}

int
sdhost_init_slot(sdhost_t *shp, int num, pnode_t node)
{
	sdslot_t	*ss;
	uint32_t	capab;
	uint32_t	clk;
	char		ksname[16];
	size_t		blen;
	unsigned	ndmac;
	int		rv;
	dev_info_t	*dip = shp->sh_dip;

	/*
	 * Register the private state.
	 */
	ss = &shp->sh_slots[num];
	ss->ss_host = shp;
	ss->ss_num = num;
	ss->ss_node = node;
	sda_host_set_private(shp->sh_host, num, ss);

	/*
	 * Initialize core data structure, locks, etc.
	 */

	/*
	 * Set up DMA.
	 */
	rv = ddi_dma_alloc_handle(dip, &dma_attr, DDI_DMA_SLEEP, NULL, &ss->ss_bufdmah);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN, "Failed to alloc dma handle (%d)!", rv);
		return (DDI_FAILURE);
	}

	rv = ddi_dma_mem_alloc(ss->ss_bufdmah, SDHOST_BOUNCESZ,
	    &sdhost_bufattr, DDI_DMA_CONSISTENT, DDI_DMA_SLEEP, NULL,
	    &ss->ss_bounce, &blen, &ss->ss_bufacch);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN, "Failed to alloc bounce buffer (%d)!", rv);
		return (DDI_FAILURE);
	}

	rv = ddi_dma_addr_bind_handle(ss->ss_bufdmah, NULL, ss->ss_bounce,
	    blen, DDI_DMA_RDWR | DDI_DMA_CONSISTENT, DDI_DMA_SLEEP, NULL,
	    &ss->ss_bufdmac, &ndmac);
	if ((rv != DDI_DMA_MAPPED) || (ndmac != 1)) {
		cmn_err(CE_WARN, "Failed to bind DMA bounce buffer (%d, %u)!",
		    rv, ndmac);
		return (DDI_FAILURE);
	}

	/*
	 * Set up virtual kstats.
	 */
	(void) snprintf(ksname, sizeof (ksname), "slot%d", num);
	ss->ss_ksp = kstat_create(ddi_driver_name(dip), ddi_get_instance(dip),
	    ksname, "misc", KSTAT_TYPE_NAMED,
	    sizeof (sdstats_t) / sizeof (kstat_named_t), KSTAT_FLAG_VIRTUAL);
	if (ss->ss_ksp != NULL) {
		sdstats_t	*sp = &ss->ss_stats;
		ss->ss_ksp->ks_data = sp;
		ss->ss_ksp->ks_private = ss;
		ss->ss_ksp->ks_lock = &shp->sh_lock;
		/* counters are 64 bits wide */
		kstat_named_init(&sp->ks_ncmd, "ncmd", KSTAT_DATA_UINT64);
		kstat_named_init(&sp->ks_ixfr, "ixfr", KSTAT_DATA_UINT64);
		kstat_named_init(&sp->ks_oxfr, "oxfr", KSTAT_DATA_UINT64);
		kstat_named_init(&sp->ks_ibytes, "ibytes", KSTAT_DATA_UINT64);
		kstat_named_init(&sp->ks_obytes, "obytes", KSTAT_DATA_UINT64);
		kstat_named_init(&sp->ks_npio, "npio", KSTAT_DATA_UINT64);
		kstat_named_init(&sp->ks_ndma, "ndma", KSTAT_DATA_UINT64);
		kstat_named_init(&sp->ks_nmulti, "nmulti", KSTAT_DATA_UINT64);
		/* these aren't counters -- leave them at 32 bits */
		kstat_named_init(&sp->ks_cardclk, "cardclk", KSTAT_DATA_UINT32);
		kstat_named_init(&sp->ks_tmusecs, "tmusecs", KSTAT_DATA_UINT32);
		kstat_named_init(&sp->ks_width, "width", KSTAT_DATA_UINT32);
		kstat_named_init(&sp->ks_flags, "flags", KSTAT_DATA_UINT32);
		kstat_named_init(&sp->ks_capab, "capab", KSTAT_DATA_UINT32);
		kstat_install(ss->ss_ksp);
	}

	ss->gpio_cd = sdhost_gpio(ss, "gpio_cd");
	ss->gpio_ro = sdhost_gpio(ss, "gpio_ro");
	ss->gpio_power = sdhost_gpio(ss, "gpio_power");
	ss->gpio_volsw = sdhost_gpio(ss, "gpio_volsw");
	ss->gpio_dat3 = sdhost_gpio(ss, "gpio_dat3");
	ss->power_level = prom_get_prop_int(ss->ss_node, "power_level", 0);
	ss->ss_ocr = prom_get_prop_int(ss->ss_node, "ocr_avail", OCR_33_34V | OCR_18_19V);

	if (ss->gpio_cd) {
		if (plat_gpio_direction_input(ss->gpio_cd) < 0)
			return (DDI_FAILURE);
	}

	if (ss->gpio_ro) {
		if (plat_gpio_direction_input(ss->gpio_ro) < 0)
			return (DDI_FAILURE);
		if (plat_gpio_set_pullup(ss->gpio_ro, 1) < 0)
			return (DDI_FAILURE);
	}

	if (ss->gpio_power) {
		if (plat_gpio_direction_output(ss->gpio_power, !ss->power_level) < 0)
			return (DDI_FAILURE);
	}

	if (ss->gpio_volsw) {
		if (plat_gpio_direction_output(ss->gpio_volsw, 0) < 0)
			return (DDI_FAILURE);
	}

	ss->ss_capab = 0;
	int len = prom_getproplen(ss->ss_node, "caps");
	if (len > 0) {
		char *prop = __builtin_alloca(len);
		prom_getprop(node, "caps", prop);
		int offeset = 0;
		int index = 0;
		while (offeset < len) {
			//prom_printf("%s:%d %s\n", __func__,__LINE__, prop + offeset);
			if (strcmp("MMC_CAP_SD_HIGHSPEED", prop + offeset) == 0)
				ss->ss_capab |= CAPAB_HIGH_SPEED;
			else if (strcmp("MMC_CAP_MMC_HIGHSPEED", prop + offeset) == 0)
				ss->ss_capab |= CAPAB_HIGH_SPEED;
			else if (strcmp("MMC_CAP_4_BIT_DATA", prop + offeset) == 0)
				ss->ss_capab |= CAPAB_4BIT_DATA;
			else if (strcmp("MMC_CAP_8_BIT_DATA", prop + offeset) == 0)
				ss->ss_capab |= CAPAB_8BIT_DATA;

			offeset += strlen(prop + offeset) + 1;
		}
	}

	sdhost_set_clock(ss, prom_get_prop_int(ss->ss_node, "f_min",   400000));
	sdhost_set_width(ss, 1);

	return (DDI_SUCCESS);
}

void
sdhost_uninit_slot(sdhost_t *shp, int num)
{
}

void
sdhost_get_response(sdslot_t *ss, sda_cmd_t *cmdp)
{
}

sda_err_t
sdhost_wait_cmd(sdslot_t *ss, sda_cmd_t *cmdp)
{
	return (SDA_ETIME);
}

sda_err_t
sdhost_poll(void *arg)
{
	return (SDA_EOK);
}

sda_err_t
sdhost_cmd(void *arg, sda_cmd_t *cmd)
{
	sdslot_t	*ss = arg;
	uint16_t	command;
	uint16_t	mode;
	sda_err_t	rv;
	sdhost_t	*shp = ss->ss_host;

	uint32_t data_addr = 0;
	union sd_emmc_cmd_cfg cmd_cfg = {0};
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

	mutex_enter(&shp->sh_lock);

	while (shp->sh_use != -1) {
		shp->sh_waiter++;
		cv_wait(&shp->sh_waitcv, &shp->sh_lock);
		shp->sh_waiter--;
	}

	if (ss->ss_suspended) {
		mutex_exit(&shp->sh_lock);
		return (SDA_ESUSPENDED);
	}

	if (cmd->sc_flags & (SDA_CMDF_READ | SDA_CMDF_WRITE)) {
		//prom_printf("%s:%d %d\n", __func__,__LINE__, cmd->sc_index);
		//prom_printf("%s:%d cmd->sc_ndmac = %d\n", __func__,__LINE__, cmd->sc_ndmac);
		//prom_printf("%s:%d cmd->sc_blksz = %d\n", __func__,__LINE__, cmd->sc_blksz);
		//prom_printf("%s:%d cmd->sc_nblks = %d\n", __func__,__LINE__, cmd->sc_nblks);
		//prom_printf("%s:%d cmd->sc_argument = %08x\n", __func__,__LINE__, cmd->sc_argument);
		cmd_cfg.Data_io = 1;
		cmd_cfg.Data_wr = !!(cmd->sc_flags & SDA_CMDF_WRITE);

		if (cmd->sc_ndmac) {
			data_addr = cmd->sc_dmac.dmac_address;
			cache_flush(cmd->sc_kvaddr, cmd->sc_nblks * cmd->sc_blksz);
		} else {
			data_addr = ss->ss_bufdmac.dmac_address;
			if (cmd->sc_flags & SDA_CMDF_WRITE)
				memcpy(ss->ss_bounce, cmd->sc_kvaddr, cmd->sc_nblks * cmd->sc_blksz);
			cache_flush(ss->ss_bounce, cmd->sc_nblks * cmd->sc_blksz);
		}

		if (cmd->sc_nblks > 1) {
			cmd_cfg.Block_mode = 1;
			cmd_cfg.Length = cmd->sc_nblks;
			if (ss->ss_blksz != cmd->sc_blksz) {
				union sd_emmc_cfg _sd_emmc_cfg;
				_sd_emmc_cfg.dw = REG_READ(shp, SD_EMMC_CFG);
				_sd_emmc_cfg.Cfg_bl_len = 31 - __builtin_clz(cmd->sc_blksz);
				REG_WRITE(shp, SD_EMMC_CFG, _sd_emmc_cfg.dw);
				ss->ss_blksz = cmd->sc_blksz;
			}
		} else {
			cmd_cfg.Block_mode = 0;
			cmd_cfg.Length = cmd->sc_blksz;
		}
	}

	shp->sh_use = ss->ss_num;
	shp->sh_status = 0;

	cmd_cfg.End_of_chain = 1;
	cmd_cfg.Owner = 1;

	REG_WRITE(shp, SD_EMMC_STATUS, REG_READ(shp, SD_EMMC_STATUS));
	//prom_printf("%s:%d %08x\n", __func__,__LINE__, cmd_cfg.dw);
	//prom_printf("%s:%d %08x\n", __func__,__LINE__, data_addr);
	//prom_printf("%s:%d %08x\n", __func__,__LINE__, cmd->sc_argument);
	REG_WRITE(shp, SD_EMMC_CMD_CFG, cmd_cfg.dw);
	REG_WRITE(shp, SD_EMMC_CMD_DAT, data_addr);
	REG_WRITE(shp, SD_EMMC_CMD_ARG, cmd->sc_argument);	// start

	union sd_emmc_status _sd_emmc_status;
	for (;;) {
		cv_wait(&shp->sh_cv, &shp->sh_lock);

		_sd_emmc_status.dw = shp->sh_status;
		if (_sd_emmc_status.End_of_Chain)
			break;
	}

	if (
	    _sd_emmc_status.Rxd_err ||
	    _sd_emmc_status.Txd_err ||
	    _sd_emmc_status.Desc_err ||
	    _sd_emmc_status.Resp_err) {
		rv = SDA_EPROTO;
		cmn_err(CE_WARN, "%s:%d status=%08x",__func__,__LINE__, _sd_emmc_status.dw);
	} else if (
	    _sd_emmc_status.Resp_timeout ||
	    _sd_emmc_status.Desc_timeout) {
		rv = SDA_ETIME;
		cmn_err(CE_WARN, "%s:%d status=%08x",__func__,__LINE__, _sd_emmc_status.dw);
	} else {
		// ok
		if (cmd->sc_flags & SDA_CMDF_READ) {
			if (cmd->sc_ndmac) {
				cache_flush(cmd->sc_kvaddr, cmd->sc_nblks * cmd->sc_blksz);
			} else {
				cache_flush(ss->ss_bounce, cmd->sc_nblks * cmd->sc_blksz);
				memcpy(cmd->sc_kvaddr, ss->ss_bounce, cmd->sc_nblks * cmd->sc_blksz);
			}
		}
		rv = SDA_EOK;
	}

	if (cmd->sc_flags & (SDA_CMDF_DAT))
		sdhost_xfer_done(ss, rv);

	if (rv == SDA_EOK) {
		switch (cmd->sc_rtype) {
		case R0:
			break;
		case R2:
			cmd->sc_response[0] = REG_READ(shp, SD_EMMC_CMD_RSP0);
			cmd->sc_response[1] = REG_READ(shp, SD_EMMC_CMD_RSP1);
			cmd->sc_response[2] = REG_READ(shp, SD_EMMC_CMD_RSP2);
			cmd->sc_response[3] = REG_READ(shp, SD_EMMC_CMD_RSP3);

			break;
		default:
			cmd->sc_response[0] = REG_READ(shp, SD_EMMC_CMD_RSP0);
			break;
		}
	}

	if (cmd->sc_index == CMD_READ_MULTI || cmd->sc_index == CMD_WRITE_MULTI) {
		union sd_emmc_cmd_cfg stop_cfg = {
			.Cmd_index = CMD_STOP_TRANSMIT,
			.R1b = 1,
			.End_of_chain = 1,
			.Owner = 1,
		};

		shp->sh_status = 0;

		REG_WRITE(shp, SD_EMMC_CMD_CFG, stop_cfg.dw);
		REG_WRITE(shp, SD_EMMC_CMD_ARG, 0);

		for (;;) {
			cv_wait(&shp->sh_cv, &shp->sh_lock);
			_sd_emmc_status.dw = shp->sh_status;
			if (_sd_emmc_status.End_of_Chain)
				break;
		}
	}


	shp->sh_use = -1;
	if (shp->sh_waiter)
		cv_signal(&shp->sh_waitcv);
	mutex_exit(&shp->sh_lock);

	return (rv);
}

sda_err_t
sdhost_getprop(void *arg, sda_prop_t prop, uint32_t *val)
{
	sdslot_t	*ss = arg;
	sdhost_t	*shp = ss->ss_host;
	sda_err_t	rv = SDA_EOK;

	mutex_enter(&shp->sh_lock);

	if (ss->ss_suspended) {
		mutex_exit(&shp->sh_lock);
		return (SDA_ESUSPENDED);
	}
	switch (prop) {
	case SDA_PROP_INSERTED:
		if (ss->gpio_cd) {
			if (plat_gpio_get(ss->gpio_cd))
				*val = B_FALSE;
			else
				*val = B_TRUE;
		} else {
			*val = B_TRUE;
		}
		break;

	case SDA_PROP_WPROTECT:
		if (ss->gpio_ro) {
			if (plat_gpio_get(ss->gpio_ro))
				*val = B_TRUE;
			else
				*val = B_FALSE;
		} else {
			*val = B_FALSE;
		}
		break;

	case SDA_PROP_OCR:
		*val = ss->ss_ocr;
		break;

	case SDA_PROP_CLOCK:
		*val = ss->ss_cardclk;
		break;

	case SDA_PROP_CAP_HISPEED:
		if ((ss->ss_capab & CAPAB_HIGH_SPEED) != 0) {
			*val = B_TRUE;
		} else {
			*val = B_FALSE;
		}
		break;

	case SDA_PROP_CAP_4BITS:
		if ((ss->ss_capab & CAPAB_4BIT_DATA) != 0) {
			*val = B_TRUE;
		} else {
			*val = B_FALSE;
		}
		break;

	case SDA_PROP_CAP_8BITS:
		if ((ss->ss_capab & CAPAB_8BIT_DATA) != 0) {
			*val = B_TRUE;
		} else {
			*val = B_FALSE;
		}
		break;

	case SDA_PROP_CAP_NOPIO:
		*val = B_FALSE;
		break;

	case SDA_PROP_CAP_INTR:
		*val = B_FALSE;
		break;

	default:
		rv = SDA_ENOTSUP;
		break;
	}
	mutex_exit(&shp->sh_lock);

	return (rv);
}

sda_err_t
sdhost_setprop(void *arg, sda_prop_t prop, uint32_t val)
{
	sdslot_t	*ss = arg;
	sdhost_t	*shp = ss->ss_host;
	sda_err_t	rv = SDA_EOK;

	mutex_enter(&shp->sh_lock);
	while (shp->sh_use != -1) {
		shp->sh_waiter++;
		cv_wait(&shp->sh_waitcv, &shp->sh_lock);
		shp->sh_waiter--;
	}
	if (ss->ss_suspended) {
		mutex_exit(&shp->sh_lock);
		return (SDA_ESUSPENDED);
	}

	switch (prop) {
	case SDA_PROP_CLOCK:
		rv = sdhost_set_clock(arg, val);
		break;

	case SDA_PROP_BUSWIDTH:
		rv = sdhost_set_width(arg, val);
		break;

	case SDA_PROP_OCR:
		if ((val & ss->ss_ocr) & OCR_33_34V) {
			plat_gpio_set(ss->gpio_volsw, 0);
		} else if ((val & ss->ss_ocr) & OCR_18_19V) {
			plat_gpio_set(ss->gpio_volsw, 1);
		} else {
			rv = SDA_EINVAL;
		}
		break;

	case SDA_PROP_HISPEED:
	case SDA_PROP_LED:
		break;

	default:
		rv = SDA_ENOTSUP;
		break;
	}

	mutex_exit(&shp->sh_lock);
	return (rv);
}

sda_err_t
sdhost_reset(void *arg)
{
	return (SDA_EOK);
}

sda_err_t
sdhost_halt(void *arg)
{
	return (SDA_EOK);
}
