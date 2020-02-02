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
#include <sys/byteorder.h>
#include <sys/debug.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/sunndi.h>
#include <sys/ndi_impldefs.h>
#include <sys/ddi_impldefs.h>
#include <sys/sysmacros.h>
#include <sys/platmod.h>
#include <sys/gpio.h>
#include "mmc_spi.h"


static ddi_device_acc_attr_t reg_acc_attr = {
	DDI_DEVICE_ATTR_V0,
	DDI_NEVERSWAP_ACC,
	DDI_STRICTORDER_ACC,
};

static void
spi_reg_write(struct spi_sc *sc, size_t offset, uint32_t val)
{
	uint32_t *addr = sc->addr + offset / sizeof(uint32_t);
	ddi_put32(sc->handle, addr, val);
}

static uint32_t
spi_reg_read(struct spi_sc *sc, size_t offset)
{
	uint32_t *addr = sc->addr + offset / sizeof(uint32_t);
	return ddi_get32(sc->handle, addr);
}

#define	SPI_CLK	(500 * 1000 * 1000)

#define SPI_SCKDIV	(0x0)
union spi_sckdiv {
	uint32_t dw;
	struct {
		uint32_t div		:	12;
		uint32_t		:	20;
	};
};

#define SPI_SCKMODE	(0x4)
union spi_sckmode {
	uint32_t dw;
	struct {
		uint32_t pha		:	1;
		uint32_t pol		:	1;
		uint32_t		:	20;
	};
};

#define SPI_CSID	(0x10)
union spi_csid {
	uint32_t dw;
	struct {
		uint32_t csid		:	32;
	};
};

#define SPI_CSDEF	(0x14)
union spi_csdef {
	uint32_t dw;
	struct {
		uint32_t csdef		:	32;
	};
};

#define SPI_CSMODE	(0x18)
union spi_csmode {
	uint32_t dw;
	struct {
		uint32_t mode		:	2;
		uint32_t		:	30;
	};
};

#define SPI_DELAY0	(0x28)
union spi_delay0 {
	uint32_t dw;
	struct {
		uint32_t cssck		:	8;
		uint32_t		:	8;
		uint32_t sckcs		:	8;
		uint32_t		:	8;
	};
};

#define SPI_DELAY1	(0x2c)
union spi_delay1 {
	uint32_t dw;
	struct {
		uint32_t intercs	:	8;
		uint32_t		:	8;
		uint32_t interxfr	:	8;
		uint32_t		:	8;
	};
};

#define SPI_FMT	(0x40)
union spi_fmt {
	uint32_t dw;
	struct {
		uint32_t proto		:	2;	// 0: single, 1: dual, 2: quad
		uint32_t endian		:	1;	// 0: MSB, 1: LSB
		uint32_t dir		:	1;	// 0: RX, 1: TX
		uint32_t		:	12;
		uint32_t len		:	4;
		uint32_t		:	12;
	};
};

#define SPI_TXDATA	(0x48)
union spi_txdata {
	uint32_t dw;
	struct {
		uint32_t data		:	8;
		uint32_t		:	23;
		uint32_t full		:	1;
	};
};

#define SPI_RXDATA	(0x4c)
union spi_rxdata {
	uint32_t dw;
	struct {
		uint32_t data		:	8;
		uint32_t		:	23;
		uint32_t empty		:	1;
	};
};

#define SPI_TXMARK	(0x50)
union spi_txmark {
	uint32_t dw;
	struct {
		uint32_t txmark		:	3;
		uint32_t		:	29;
	};
};

#define SPI_RXMARK	(0x54)
union spi_rxmark {
	uint32_t dw;
	struct {
		uint32_t rxmark		:	3;
		uint32_t		:	29;
	};
};

#define SPI_IE		(0x70)
union spi_ie {
	uint32_t dw;
	struct {
		uint32_t txwn		:	1;
		uint32_t rxwn		:	1;
		uint32_t		:	30;
	};
};

#define SPI_IP		(0x74)
union spi_ip {
	uint32_t dw;
	struct {
		uint32_t txwn		:	1;
		uint32_t rxwn		:	1;
		uint32_t		:	30;
	};
};

#define SPI_FCTRL	(0x60)
union spi_fctrl {
	uint32_t dw;
	struct {
		uint32_t en		:	1;
		uint32_t		:	31;
	};
};

static void
spi_set_mode(struct spi_sc *sc, int mode)
{
	union spi_sckmode spi_sckmode = {0};
	spi_sckmode.pol = (mode & 2) >> 1;
	spi_sckmode.pha = mode & 1;
	spi_reg_write(sc, SPI_SCKMODE, spi_sckmode.dw);
}

static void
spi_set_mode0(struct spi_sc *sc)
{
	spi_set_mode(sc, 0);
}

static void
spi_csmode_auto(struct spi_sc *sc)
{
	union spi_csmode spi_csmode = {0};
	spi_csmode.mode = 0;	// auto
	spi_reg_write(sc, SPI_CSMODE, spi_csmode.dw);
}

static void
spi_csmode_off(struct spi_sc *sc)
{
	union spi_csmode spi_csmode = {0};
	spi_csmode.mode = 3;	// off
	spi_reg_write(sc, SPI_CSMODE, spi_csmode.dw);
}

static void
spi_set_clock(struct spi_sc *sc, uint32_t freq)
{
	union spi_sckdiv spi_sckdiv = {0};
	spi_sckdiv.div = (SPI_CLK + 2 * freq - 1) / (2 * freq) - 1;
	spi_reg_write(sc, SPI_SCKDIV, spi_sckdiv.dw);
}

static void
spi_clear_rfifo(struct spi_sc *sc)
{
	for (;;) {
		union spi_rxdata spi_rxdata = { spi_reg_read(sc, SPI_RXDATA) };
		if (spi_rxdata.empty)
			break;
	}
}

static void
spi_setup(struct spi_sc *sc, uint32_t freq)
{
	union spi_ie spi_ie = {0};
	spi_reg_write(sc, SPI_IE, spi_ie.dw);

	union spi_delay0 spi_delay0 = {0};
	spi_delay0.cssck = 1;
	spi_delay0.sckcs = 1;
	spi_reg_write(sc, SPI_DELAY0, spi_delay0.dw);

	union spi_delay1 spi_delay1 = {0};
	spi_delay1.intercs = 1;
	spi_delay1.interxfr = 0;
	spi_reg_write(sc, SPI_DELAY1, spi_delay1.dw);

	union spi_fctrl spi_fctrl = {0};
	spi_reg_write(sc, SPI_FCTRL, spi_fctrl.dw);

	union spi_csdef spi_csdef;
	spi_csdef.dw = spi_reg_read(sc, SPI_CSDEF);
	spi_csdef.csdef |= (1u << sc->cs);
	spi_reg_write(sc, SPI_CSDEF, spi_csdef.dw);

	union spi_csid spi_csid = {0};
	spi_csid.csid = sc->cs;
	spi_reg_write(sc, SPI_CSID, spi_csid.dw);

	union spi_fmt spi_fmt = {0};
	spi_fmt.proto = 0;
	spi_fmt.endian = 0;
	spi_fmt.dir = 0;
	spi_fmt.len = 8;
	spi_reg_write(sc, SPI_FMT, spi_fmt.dw);

	spi_set_clock(sc, freq);

	spi_clear_rfifo(sc);
}

static void
spi_send(struct spi_sc *sc, const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		// wait
		for (;;) {
			union spi_txdata spi_txdata = { spi_reg_read(sc, SPI_TXDATA) };
			if (!spi_txdata.full) {
				break;
			}
		}
		// send
		{
			union spi_txdata spi_txdata = {0};
			spi_txdata.data = buf[i];
			spi_reg_write(sc, SPI_TXDATA, spi_txdata.dw);
		}
		// wait recv
		for (;;) {
			union spi_rxdata spi_rxdata = { spi_reg_read(sc, SPI_RXDATA) };
			if (!spi_rxdata.empty)
				break;
		}
	}
}

static void
spi_recv(struct spi_sc *sc, uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		// wait
		for (;;) {
			union spi_txdata spi_txdata = { spi_reg_read(sc, SPI_TXDATA) };
			if (!spi_txdata.full) {
				break;
			}
		}
		// send
		{
			union spi_txdata spi_txdata = {0};
			spi_txdata.data = 0xff;
			spi_reg_write(sc, SPI_TXDATA, spi_txdata.dw);
		}
		// wait recv
		for (;;) {
			union spi_rxdata spi_rxdata = { spi_reg_read(sc, SPI_RXDATA) };
			if (!spi_rxdata.empty) {
				buf[i] = spi_rxdata.data;
				break;
			}
		}
	}
}

static uint8_t
crc7(uint8_t crc, const uint8_t *buf, int len)
{
	static const uint8_t crc7_table[] = {
		0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
		0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
		0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
		0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
		0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
		0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
		0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
		0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
		0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
		0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
		0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
		0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
		0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
		0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
		0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
		0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
		0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
		0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
		0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
		0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
		0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
		0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
		0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
		0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
		0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
		0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
		0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
		0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
		0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
		0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
		0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
		0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
	};

	for (int i = 0; i < len; i++)
		crc = crc7_table[((crc << 1) ^ buf[i]) & 0xff];

	return crc;
}

static uint16_t
crc16(uint16_t crc, const uint8_t *buf, int len)
{
	static const uint16_t crc16_table[] = {
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
	};

	for (int i = 0; i < len; i++)
		crc = crc16_table[((crc >> 8) ^ buf[i]) & 0xff] ^ (crc << 8);

	return crc;
}

static void
mmc_spi_send_cmd(struct mmc_sc *sc, int index, uint32_t arg)
{
	uint8_t buf[7];
	buf[0] = 0xff;
	buf[1] = 0x40 | index;
	buf[2] = (arg >> 24) & 0xff;
	buf[3] = (arg >> 16) & 0xff;
	buf[4] = (arg >> 8) & 0xff;
	buf[5] = (arg & 0xff);
	buf[6] = (crc7(0, &buf[1], 5) << 1) | 0x01;
	spi_send(&sc->spi, buf, sizeof(buf));
}

#define MMC_SPI_R1_IDLE			(1 << 0)
#define MMC_SPI_R1_ERASE_RESET		(1 << 1)
#define MMC_SPI_R1_ILLEGAL_COMMAND	(1 << 2)
#define MMC_SPI_R1_COM_CRC_ERR		(1 << 3)
#define MMC_SPI_R1_ERASE_SEQUENCE_ERR	(1 << 4)
#define MMC_SPI_R1_ADDRESS_ERR		(1 << 5)
#define MMC_SPI_R1_PARAMETER_ERR	(1 << 6)
#define MMC_SPI_R1_ERROR	( \
    MMC_SPI_R1_ERASE_RESET | \
    MMC_SPI_R1_ILLEGAL_COMMAND | \
    MMC_SPI_R1_COM_CRC_ERR | \
    MMC_SPI_R1_ERASE_SEQUENCE_ERR | \
    MMC_SPI_R1_ADDRESS_ERR | \
    MMC_SPI_R1_PARAMETER_ERR)

#define MMC_SPI_RETRY 3000000
#define MMC_SPI_CMD_RETRY 8

static uint8_t
mmc_spi_wait_r1(struct mmc_sc *sc)
{
	uint8_t c;
	spi_recv(&sc->spi, &c, 1);
	for (int i = 0; i < MMC_SPI_CMD_RETRY; i++) {
		spi_recv(&sc->spi, &c, 1);
		if ((c & 0x80) == 0)
			return c;
	}
	return c;
}

static int
mmc_spi_go_idle_state(struct mmc_sc *sc)
{
	mmc_spi_send_cmd(sc, 0, 0);
	int r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;
	if (!(r & MMC_SPI_R1_IDLE))
		return -1;
	return 0;
}

static int
mmc_spi_crc_on_off(struct mmc_sc *sc, bool on)
{
	mmc_spi_send_cmd(sc, 59, on? 1: 0);
	int r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;
	return 0;
}

static int
mmc_spi_send_if_cond(struct mmc_sc *sc)
{
	mmc_spi_send_cmd(sc, 8, 0x000001aa);
	int r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;

	uint8_t buf[4];
	spi_recv(&sc->spi, buf, 4);
	uint32_t v = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	if ((v & 0xFFF) != 0x1aa)
		return -1;
	return 0;
}

static int
mmc_spi_read_ocr(struct mmc_sc *sc)
{
	int r;
	for (;;) {
		mmc_spi_send_cmd(sc, 55, 0);
		r = mmc_spi_wait_r1(sc);
		if (r & 0x80)
			return -1;
		if (r & MMC_SPI_R1_ERROR)
			return -1;

		mmc_spi_send_cmd(sc, 41, 0x40000000);
		r = mmc_spi_wait_r1(sc);
		if (r & 0x80)
			return -1;
		if (r & MMC_SPI_R1_ERROR)
			return -1;
		if (!(r & MMC_SPI_R1_IDLE))
			break;
	}

	mmc_spi_send_cmd(sc, 58, 0);
	r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;

	uint8_t buf[4];
	spi_recv(&sc->spi, buf, 4);
	sc->ocr = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	return 0;
}

static int
mmc_spi_send_csd(struct mmc_sc *sc)
{
	mmc_spi_send_cmd(sc, 9, 0);
	int r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;

	int i;
	for (i = 0; i < MMC_SPI_RETRY; i++) {
		uint8_t c;
		spi_recv(&sc->spi, &c, 1);
		if (c == 0xfe)
			break;
	}
	if (i == MMC_SPI_RETRY)
		return -1;

	spi_recv(&sc->spi, sc->csd, sizeof(sc->csd));
	uint8_t crcb[2];
	spi_recv(&sc->spi, crcb, 2);
	uint16_t crc = (crcb[0] << 8) | crcb[1];
	if (crc != crc16(0, sc->csd, sizeof(sc->csd)))
		return -1;

	return 0;
}

static int
mmc_spi_send_cid(struct mmc_sc *sc)
{
	mmc_spi_send_cmd(sc, 10, 0);
	int r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;

	int i;
	for (i = 0; i < MMC_SPI_RETRY; i++) {
		uint8_t c;
		spi_recv(&sc->spi, &c, 1);
		if (c == 0xfe)
			break;
	}
	if (i == MMC_SPI_RETRY)
		return -1;

	spi_recv(&sc->spi, sc->cid, sizeof(sc->cid));
	uint8_t crcb[2];
	spi_recv(&sc->spi, crcb, 2);
	uint16_t crc = (crcb[0] << 8) | crcb[1];
	if (crc != crc16(0, sc->cid, sizeof(sc->cid)))
		return -1;

	return 0;
}

static int
mmc_spi_set_blocklen(struct mmc_sc *sc, uint32_t blklen)
{
	mmc_spi_send_cmd(sc, 16, blklen);
	int r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;
	return 0;
}

static int
mmc_spi_recv_data(struct mmc_sc *sc, caddr_t buf)
{
	int i;
	for (i = 0; i < MMC_SPI_RETRY; i++) {
		uint8_t c;
		spi_recv(&sc->spi, &c, 1);
		if (c == 0xfe)
			break;
	}
	if (i == MMC_SPI_RETRY)
		return -1;

	spi_recv(&sc->spi, (uint8_t *)buf, 512);
	uint8_t crcb[2];
	spi_recv(&sc->spi, (uint8_t *)crcb, 2);
	uint16_t crc = (crcb[0] << 8) | crcb[1];
	if (crc != crc16(0, (uint8_t*)buf, 512))
		return -1;

	return 0;
}

static int
mmc_spi_read_single_block(struct mmc_sc *sc, caddr_t buf, uint32_t blkaddr)
{
	int r;
	mmc_spi_send_cmd(sc, 17, blkaddr);
	r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;

	return mmc_spi_recv_data(sc, buf);
}

static int
mmc_spi_read_multiple_block(struct mmc_sc *sc, caddr_t buf, uint32_t blkaddr, uint32_t blknum)
{
	int r;
	mmc_spi_send_cmd(sc, 18, blkaddr);
	r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;

	uint32_t x = blknum;
	while (blknum) {
		if (mmc_spi_recv_data(sc, buf) != 0) {
			mmc_spi_send_cmd(sc, 12, 0);
			mmc_spi_wait_r1(sc);
			return -1;
		}
		buf += 512;
		blknum--;
	}

	mmc_spi_send_cmd(sc, 12, 0);
	r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;

	return 0;
}

static int
mmc_spi_send_data(struct mmc_sc *sc, const caddr_t buf, bool multi)
{
	uint8_t tok[2] = {0xff, multi? 0xfc :0xfe};
	uint16_t crc = crc16(0, (uint8_t*)buf, 512);
	uint8_t crcb[2] = {crc >> 8, crc & 0xff};
	spi_send(&sc->spi, tok, sizeof(tok));
	spi_send(&sc->spi, (uint8_t*)buf, 512);
	spi_send(&sc->spi, crcb, sizeof(crcb));

	uint8_t c;

	int i;
	for (i = 0; i < MMC_SPI_CMD_RETRY; i++) {
		spi_recv(&sc->spi, &c, 1);
		if ((c & 0x10) == 0) { // data response token
			if ((c & 0xf) != ((2 << 1) | 1)) { // data accepted
				return -1;
			}
			break;
		}
	}
	if (i == MMC_SPI_CMD_RETRY)
		return -1;

	spi_recv(&sc->spi, &c, 1);
	for (i = 0; i < MMC_SPI_RETRY; i++) {
		spi_recv(&sc->spi, &c, 1);
		if (c == 0xff)
			return 0;
	}

	return -1;
}

static int
mmc_spi_write_single_block(struct mmc_sc *sc, caddr_t buf, uint32_t blkaddr)
{
	int r;
	mmc_spi_send_cmd(sc, 24, blkaddr);
	r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;

	return mmc_spi_send_data(sc, buf, false);
}

static int
mmc_spi_write_multiple_block(struct mmc_sc *sc, caddr_t buf, uint32_t blkaddr, uint32_t blknum)
{
	int r;
	mmc_spi_send_cmd(sc, 25, blkaddr);
	r = mmc_spi_wait_r1(sc);
	if (r & 0x80)
		return -1;
	if (r & MMC_SPI_R1_ERROR)
		return -1;

	while (blknum > 0) {
		int ret = mmc_spi_send_data(sc, buf, true);
		if (ret < 0) {
			return -1;
		}

		blknum--;
		buf += 512;
	}

	uint8_t tok[2] = {0xff, 0xfd};
	spi_send(&sc->spi, tok, sizeof(tok));

	uint8_t c;
	spi_recv(&sc->spi, &c, 1);

	int i;
	for (i = 0; i < MMC_SPI_RETRY; i++) {
		spi_recv(&sc->spi, &c, 1);
		if (c == 0xff)
			return 0;
	}

	return -1;
}

static int
mmc_bd_read(void *arg, bd_xfer_t *xfer)
{
	if (xfer->x_flags & BD_XFER_POLL)
		return (EIO);

	int r;
	struct mmc_sc *sc = arg;
	mutex_enter(&sc->intrlock);
	if (xfer->x_nblks > 1) {
		r = mmc_spi_read_multiple_block(sc, xfer->x_kaddr, xfer->x_blkno, xfer->x_nblks);
	} else {
		r = mmc_spi_read_single_block(sc, xfer->x_kaddr, xfer->x_blkno);
	}
	mutex_exit(&sc->intrlock);

	if (r != 0)
		return EIO;

	bd_xfer_done(xfer, 0);

	return 0;
}

static int
mmc_bd_write(void *arg, bd_xfer_t *xfer)
{
	if (xfer->x_flags & BD_XFER_POLL)
		return (EIO);

	int r;
	struct mmc_sc *sc = arg;
	mutex_enter(&sc->intrlock);
	if (xfer->x_nblks > 1) {
		r = mmc_spi_write_multiple_block(sc, xfer->x_kaddr, xfer->x_blkno, xfer->x_nblks);
	} else {
		r = mmc_spi_write_single_block(sc, xfer->x_kaddr, xfer->x_blkno);
	}
	mutex_exit(&sc->intrlock);

	if (r != 0)
		return EIO;

	bd_xfer_done(xfer, 0);

	return 0;
}

static uint32_t
extract_128b(const uint8_t *buf, int start, int width)
{
	uint32_t v = 0;
	for (int i = 0; i < width; i++) {
		uint8_t bit = (buf[(127 - (start + i)) / 8] >> ((start + i) % 8)) & 1;
		v |= (bit << i);
	}
	return v;
}

static void
mmc_bd_driveinfo(void *arg, bd_drive_t *drive)
{
	struct mmc_sc *sc = arg;
	drive->d_qsize = 4;
	drive->d_maxxfer = 65536;
	drive->d_removable = B_FALSE;
	drive->d_hotpluggable = B_FALSE;
	drive->d_target = 0;

	switch (extract_128b(sc->cid, 120, 8)) {
	case 0x01: drive->d_vendor = "Panasonic"; break;
	case 0x02: drive->d_vendor = "Toshiba"; break;
	case 0x03: drive->d_vendor = "SanDisk"; break;
	case 0x1b: drive->d_vendor = "Samsung"; break;
	case 0x1d: drive->d_vendor = "AData"; break;
	case 0x27: drive->d_vendor = "Phison"; break;
	case 0x28: drive->d_vendor = "Lexar"; break;
	case 0x31: drive->d_vendor = "Silicon Power"; break;
	case 0x41: drive->d_vendor = "Kingston"; break;
	case 0x74: drive->d_vendor = "Transcend"; break;
	default: drive->d_vendor = "unknown"; break;
	}

	drive->d_vendor_len = strlen(drive->d_vendor);
	drive->d_product = kmem_zalloc(6, KM_SLEEP);
	drive->d_product[4] = extract_128b(sc->cid, 64, 8);
	drive->d_product[3] = extract_128b(sc->cid, 72, 8);
	drive->d_product[2] = extract_128b(sc->cid, 80, 8);
	drive->d_product[1] = extract_128b(sc->cid, 88, 8);
	drive->d_product[0] = extract_128b(sc->cid, 96, 8);
	drive->d_product_len = strlen(drive->d_product);
	uint32_t serial = extract_128b(sc->cid, 24, 32);
	drive->d_serial = kmem_zalloc(9, KM_SLEEP);
	sprintf(drive->d_serial, "%08x", serial);
	drive->d_serial_len = 8;
	uint32_t rev = extract_128b(sc->cid, 56, 8);
	drive->d_revision = kmem_zalloc(6, KM_SLEEP);
	sprintf(drive->d_revision, "%d.%d", rev >> 4, rev & 0xF);
	drive->d_revision_len = strlen(drive->d_revision);
}

static int
mmc_bd_mediainfo(void *arg, bd_media_t *media)
{
	struct mmc_sc *sc = arg;
	uint64_t csz;
	uint32_t cmult;
	uint32_t read_bl_len;
	uint64_t size;
	if (sc->ocr & (1u << 30)) {
		csz = extract_128b(sc->csd, 48, 22);
		read_bl_len = extract_128b(sc->csd, 80, 4);
		size = (csz + 1) * (512 * 1024);
	} else {
		csz = extract_128b(sc->csd, 62, 12);
		cmult = extract_128b(sc->csd, 47, 3);
		read_bl_len = extract_128b(sc->csd, 80, 4);
		size = ((csz + 1) * (1 << (cmult + 2))) * read_bl_len;
	}

	media->m_nblks = size / 512;
	media->m_blksize = 512;
	media->m_readonly = B_FALSE;
	media->m_solidstate = B_TRUE;
	return (0);
}

static bd_ops_t mmc_bd_ops = {
	BD_OPS_VERSION_0,
	mmc_bd_driveinfo,
	mmc_bd_mediainfo,
	NULL,			/* devid_init */
	NULL,			/* sync_cache */
	mmc_bd_read,
	mmc_bd_write,
};

static void
mmc_destroy(struct mmc_sc *sc)
{
	if (sc->bdh) {
		bd_free_handle(sc->bdh);
	}
	if (sc->spi.handle)
		ddi_regs_map_free(&sc->spi.handle);
	mutex_destroy(&sc->intrlock);
	kmem_free(sc, sizeof (*sc));
}

static int
mmc_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_DETACH:
		break;
	default:
		return (DDI_FAILURE);
	}
	struct mmc_sc *sc = ddi_get_driver_private(dip);

	bd_detach_handle(sc->bdh);

	ddi_set_driver_private(sc->dip, NULL);
	mmc_destroy(sc);

	return DDI_SUCCESS;
}

static int
mmc_quiesce(dev_info_t *dip)
{
	cmn_err(CE_WARN, "%s%d: mmc_quiesce is not implemented",
	    ddi_driver_name(dip), ddi_get_instance(dip));
	return DDI_FAILURE;
}

static int
mmc_probe(dev_info_t *dip)
{
	int len;
	char buf[80];
	pnode_t nodeid = ddi_get_nodeid(dip);
	if (nodeid < 0)
		return (DDI_PROBE_FAILURE);
	uint64_t regbase;
	if (prom_get_reg(nodeid, 0, &regbase) != 0)
		return (DDI_PROBE_FAILURE);

	pnode_t child = prom_childnode(nodeid);
	while (child > 0) {
		if (prom_is_compatible(child, "mmc-spi-slot")) {
			break;
		}
		child = prom_nextnode(child);
	}
	if (child <= 0)
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

static int
mmc_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_ATTACH:
		break;
	default:
		return (DDI_FAILURE);
	}

	struct mmc_sc *sc = kmem_zalloc(sizeof(struct mmc_sc), KM_SLEEP);
	ddi_set_driver_private(dip, sc);

	pnode_t child = prom_childnode(ddi_get_nodeid(dip));
	while (child > 0) {
		if (prom_is_compatible(child, "mmc-spi-slot"))
			break;
		child = prom_nextnode(child);
	}
	sc->dip = dip;
	sc->spi.dip = dip;

	uint64_t cs = 0;
	if (prom_get_reg(ddi_get_nodeid(sc->spi.dip), 0, &cs) == 0) {
		cs = ntohll(cs);
	}
	sc->spi.cs = cs;

	mutex_init(&sc->intrlock, NULL, MUTEX_DRIVER, NULL);
	if (ddi_regs_map_setup(sc->spi.dip, 0, (caddr_t*)&sc->spi.addr, 0, 0, &reg_acc_attr, &sc->spi.handle) != DDI_SUCCESS) {
		goto err_exit;
	}

	pnode_t node = child;
	uint32_t spi_freq = 20000000;
	int len = prom_getproplen(node, "spi-max-frequency");
	if (len == sizeof(spi_freq)) {
		prom_getprop(node, "spi-max-frequency", (caddr_t)&spi_freq);
		spi_freq = ntohl(spi_freq);
	}

	spi_setup(&sc->spi, 400 * 1000);
	spi_set_mode0(&sc->spi);
	spi_csmode_off(&sc->spi);
	for (int i = 0; i < 100; i++) {
		uint8_t c = 0xff;
		spi_send(&sc->spi, &c, 1);
	}
	spi_csmode_auto(&sc->spi);
	if (mmc_spi_go_idle_state(sc) != 0) {
		cmn_err(CE_WARN, "%s%d: mmc_spi_go_idle_state faild",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		goto err_exit;
	}
	if (mmc_spi_crc_on_off(sc, true) != 0) {
		cmn_err(CE_WARN, "%s%d: mmc_spi_crc_on_off faild",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		goto err_exit;
	}
	if (mmc_spi_send_if_cond(sc) != 0) {
		cmn_err(CE_WARN, "%s%d: mmc_spi_send_if_cond faild",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		goto err_exit;
	}
	if (mmc_spi_read_ocr(sc) != 0) {
		cmn_err(CE_WARN, "%s%d: mmc_spi_read_ocr faild",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		goto err_exit;
	}
	if (mmc_spi_send_csd(sc) != 0) {
		cmn_err(CE_WARN, "%s%d: mmc_spi_send_csd faild",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		goto err_exit;
	}
	if (mmc_spi_send_cid(sc) != 0) {
		cmn_err(CE_WARN, "%s%d: mmc_spi_send_cid faild",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		goto err_exit;
	}

	if (mmc_spi_set_blocklen(sc, 512) != 0) {
		cmn_err(CE_WARN, "%s%d: mmc_spi_set_blocklen faild",
		    ddi_driver_name(dip), ddi_get_instance(dip));
		goto err_exit;
	}

	spi_set_clock(&sc->spi, spi_freq);

	sc->bdh = bd_alloc_handle(sc, &mmc_bd_ops, NULL, KM_SLEEP);
	ASSERT(sc->bdh);

	bd_attach_handle(sc->spi.dip, sc->bdh);

	return DDI_SUCCESS;
err_exit:
	mmc_destroy(sc);
	return (DDI_FAILURE);
}

static struct dev_ops mmc_dev_ops = {
	DEVO_REV,			/* devo_rev */
	0,				/* devo_refcnt */
	ddi_no_info,			/* devo_getinfo */
	nulldev,			/* devo_identify */
	mmc_probe,			/* devo_probe */
	mmc_attach,			/* devo_attach */
	mmc_detach,			/* devo_detach */
	nodev,				/* devo_reset */
	NULL,				/* devo_cb_ops */
	NULL,				/* devo_bus_ops */
	NULL,				/* devo_power */
	mmc_quiesce,			/* devo_quiesce */
};

static struct modldrv mmc_modldrv = {
	&mod_driverops,			/* drv_modops */
	"SiFive SPI MMC",		/* drv_linkinfo */
	&mmc_dev_ops			/* drv_dev_ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,			/* ml_rev */
	{ &mmc_modldrv, NULL }	/* ml_linkage */
};

int
_init(void)
{
	int i;

	bd_mod_init(&mmc_dev_ops);
	if ((i = mod_install(&modlinkage)) != 0) {
		bd_mod_fini(&mmc_dev_ops);
	}
	return (i);
}

int
_fini(void)
{
	int i;

	if ((i = mod_remove(&modlinkage)) == 0) {
		bd_mod_fini(&mmc_dev_ops);
	}
	return (i);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}
