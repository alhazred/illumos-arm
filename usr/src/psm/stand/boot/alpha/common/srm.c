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
#include <sys/promif.h>
#include "prom_dev.h"
#include "srm.h"
#include "console.h"

extern uint64_t prom_dispatch(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

enum {
	CCB_GETC = 0x01,
	CCB_PUTS, CCB_RESET_TERM, CCB_SET_TERM_INT, CCB_SET_TERM_CTL, CCB_PROCESS_KEYCODE,
	CCB_CONSOLE_OPEN, CCB_CONSOLE_CLOSE,
	CCB_OPEN = 0x10,
	CCB_CLOSE, CCB_IOCTL, CCB_READ, CCB_WRITE,
	CCB_SET_ENV = 0x20,
	CCB_RESET_ENV, CCB_GET_ENV, CCB_SAVE_ENV,
	CCB_PSWITCH = 0x30,
	CCB_BIOS_EMUL = 0x32
};

int srm_getenv(int index, char *envval, int maxlen)
{
	union {
		uint64_t ret;
		struct {
			uint32_t len;
			uint32_t rsv: 29;
			uint32_t rst: 3;
		} s;
	} ccb_sts;

	ccb_sts.ret = prom_dispatch(CCB_GET_ENV, index, (uint64_t)envval, maxlen - 1, 0);

	return (ccb_sts.s.rst == 0)? 0: -1;
}

static ssize_t
srm_gets(int dev, caddr_t buf, size_t len, uint_t startblk)
{
	union {
		uint64_t ret;
		struct {
			uint32_t c;
			uint32_t rsv: 29;
			uint32_t rst: 3;
		} s;
	} ccb_sts;
	size_t left_len = len;

	while (left_len > 0) {
		ccb_sts.ret = prom_dispatch(CCB_GETC, dev, 0, 0, 0);
		if (ccb_sts.s.rst == 0) {
			*buf++ = ccb_sts.s.c & 0xFF;
			left_len--;
			break;
		} else if (ccb_sts.s.rst == 1) {
			*buf++ = ccb_sts.s.c & 0xFF;
			left_len--;
		} else {
			break;
		}
	}

	return ((len - left_len == 0)? -1: len - left_len);
}

static ssize_t
srm_puts(int dev, caddr_t buf, size_t len, uint_t startblk)
{
	union {
		uint64_t ret;
		struct {
			uint32_t len;
			uint32_t rsv: 29;
			uint32_t rst: 3;
		} s;
	} ccb_sts;
	size_t left_len = len;

	while (left_len > 0) {
		ccb_sts.ret = prom_dispatch(CCB_PUTS, dev, (uint64_t)buf, left_len, 0);
		buf += ccb_sts.s.len;
		left_len -= ccb_sts.s.len;
	}
	return len;
}

#define SRM_DEVS_MAX 8
static int srm_devs = 0;
struct {
	char name[OBP_MAXPATHLEN];
	int fd;
} srm_ctrb[SRM_DEVS_MAX];

static int
srm_open(const char *path)
{
	if (srm_devs >= SRM_DEVS_MAX)
		return -1;
	union {
		uint64_t ret;
		struct {
			uint32_t chan;
			uint32_t rsv: 30;
			uint32_t rst: 2;
		} s;
	} ccb_sts;

	char buf[OBP_MAXPATHLEN];
	strcpy(buf, path);

	if (buf[strlen(buf) - 2] == ':' && buf[strlen(buf) - 1] == 'a')
		buf[strlen(buf) - 2] = 0;

	ccb_sts.ret = prom_dispatch(CCB_OPEN, (uint64_t)buf, strlen(buf), 0, 0);

	if (ccb_sts.s.rst != 0)
		return -1;

	srm_ctrb[srm_devs].fd = ccb_sts.s.chan;
	strcpy(srm_ctrb[srm_devs].name, buf);

	if (strncmp(path, "BOOTP", strlen("BOOTP"))==0) {
		char *str = "bootp";
		prom_setprop(prom_chosennode(), "net-config-strategy", (caddr_t)str, strlen(str) + 1);
		str = "ethernet,100,rj45,full";
		prom_setprop(prom_chosennode(), "network-interface-type", (caddr_t)str, strlen(str) + 1);
	}

	return srm_devs++;
}

static int
srm_close(int dev)
{
	union {
		uint64_t ret;
		struct {
			uint32_t sbz;
			uint32_t rsv: 31;
			uint32_t rst: 1;
		} s;
	} ccb_sts;

	ccb_sts.ret = prom_dispatch(CCB_CLOSE, srm_ctrb[dev].fd, 0, 0, 0);

	if (ccb_sts.s.rst != 0)
		return -1;

	srm_ctrb[dev].fd = -1;
	memset(srm_ctrb[dev].name, 0, sizeof(srm_ctrb[dev].name));

	return 0;
}

static ssize_t
srm_read(int dev, caddr_t buf, size_t len, uint_t startblk)
{
	char local_buf[512];
	union {
		uint64_t ret;
		struct {
			uint32_t len;
			uint32_t rsv: 31;
			uint32_t rst: 1;
		} s;
	} ccb_sts;

	if (len < 512) {
		ccb_sts.ret = prom_dispatch(CCB_READ, srm_ctrb[dev].fd, 512, (uint64_t)local_buf, (uint64_t)startblk);
		memcpy(buf, local_buf, len);
		return ((ccb_sts.s.rst != 0)? -1: (ccb_sts.s.len > len)? len: ccb_sts.s.len);
	} else {
		ccb_sts.ret = prom_dispatch(CCB_READ, srm_ctrb[dev].fd, len, (uint64_t)buf, (uint64_t)startblk);
		return ((ccb_sts.s.rst != 0)? -1: ccb_sts.s.len);
	}
}

static ssize_t
srm_write(int dev, caddr_t buf, size_t len, uint_t startblk)
{
	union {
		uint64_t ret;
		struct {
			uint32_t len;
			uint32_t rsv: 31;
			uint32_t rst: 1;
		} s;
	} ccb_sts;

	ccb_sts.ret = prom_dispatch(CCB_WRITE, srm_ctrb[dev].fd, len, (uint64_t)buf, (uint64_t)startblk);

	return ((ccb_sts.s.rst != 0)? -1: ccb_sts.s.len);
}

static int
srm_getmacaddr(int dev, caddr_t ea)
{
	int i;
	char *name = srm_ctrb[dev].name;

	if (strncmp("BOOTP", name, strlen("BOOTP")) != 0)
		return -1;

	caddr_t enet_addr = name;
	for (i = 0; i < 8; i++) {
		enet_addr = strchr(enet_addr, ' ');
		if (enet_addr == NULL) {
			return -1;
		}
		enet_addr++;
	}
	if (enet_addr != NULL) {
		int hv, lv;

#define	dval(c)	(((c) >= '0' && (c) <= '9') ? ((c) - '0') : \
		 (((c) >= 'A' && (c) <= 'F') ? (10 + (c) - 'A') : \
		  (((c) >= 'a' && (c) <= 'f') ? (10 + (c) - 'a') : -1)))

		for (i = 0; i < 6; i++) {
			hv = dval(*enet_addr); enet_addr++;
			lv = dval(*enet_addr); enet_addr++;
			enet_addr++;

			if (hv == -1 || lv == -1) {
				return -1;
			}

			ea[i] = (hv << 4) | lv;
		}
#undef dval
	}
	return 0;
}

static int
srm_match(const char *name)
{
	if (strncmp("SCSI", name, strlen("SCSI")) == 0)
		return 1;
	if (strncmp("BOOTP", name, strlen("BOOTP")) == 0)
		return 1;
	return 0;
}

static int
stdin_match(const char *name)
{
	return !strcmp(name, "stdin");
}

static int
stdout_match(const char *name)
{
	return !strcmp(name, "stdout");
}

static int
console_open(const char *name)
{
	union {
		char b[8];
		uint64_t qw;
	} buf;
	buf.qw = 0;
	srm_getenv(ENV_TTY_DEV, buf.b, sizeof(buf));

	return buf.b[0] - '0';
}

static struct prom_dev stdin_dev =
{
	.match = stdin_match,
	.open = console_open,
	.read = srm_gets,
};

static struct prom_dev stdout_dev =
{
	.match = stdout_match,
	.open = console_open,
	.write = srm_puts,
};

static struct prom_dev srm_dev =
{
	.match = srm_match,
	.open = srm_open,
	.read = srm_read,
	.write = srm_write,
	.close = srm_close,
	.getmacaddr = srm_getmacaddr,
};

void init_console(void)
{
	prom_register(&stdout_dev);
	prom_register(&stdin_dev);
	prom_register(&srm_dev);
}
