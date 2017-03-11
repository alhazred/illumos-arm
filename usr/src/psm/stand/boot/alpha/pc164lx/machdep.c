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
#include <sys/param.h>
#include <sys/promif.h>
#include <sys/salib.h>
#include <sys/bootconf.h>
#include <sys/boot.h>
#include <sys/bootinfo.h>
#include <sys/hwrpb.h>
#include <sys/sysmacros.h>
#include <sys/pte.h>
#include <sys/machparam.h>
#include <sys/pci.h>
#include <sys/pal.h>
#include <alloca.h>
#include <netinet/inetutil.h>
#include <sys/bootvfs.h>
#include <sys/bootsyms.h>
#include <libfdt.h>

#include "boot_plat.h"
#include "srm.h"
#include "pci_dev.h"
#include "sil3124.h"

#define V2ARGS_BUF_SZ OBP_MAXPATHLEN
pte_t *l1_ptbl;

struct rpb *hwrpb = (struct rpb *)HWRPB_ADDR;

extern char _RamdiskStart[];
extern char _RamdiskEnd[];
extern char filename[];
static struct xboot_info xboot_info;
static char zfs_bootfs[256];
char v2args_buf[V2ARGS_BUF_SZ];
char *v2args = v2args_buf;
extern char *bootp_response;
extern uint64_t boot_args[3];

extern	int (*readfile(int fd, int print))();
extern	void kmem_init(void);
extern	void *kmem_alloc(size_t, int);
extern	void kmem_free(void *, size_t);
extern	void get_boot_args(char *buf);
extern	void setup_bootops(void);
extern	struct	bootops bootops;
extern	void exitto(int (*entrypoint)());
extern	int openfile(char *filename);
extern int determine_fstype_and_mountroot(char *);
extern	ssize_t xread(int, char *, size_t);
extern	void _reset(void);

void update_hwrpb_cksum(void)
{
	uint64_t *p, sum = 0;
	int i;

	for (i = 0, p = (uint64_t *) hwrpb;
			i < (offsetof(struct rpb, checksum) / sizeof(uint64_t));
			i++, p++)
		sum += *p;

	hwrpb->checksum = sum;
}

void
setup_aux(void)
{
}

#define	PYXIS_PCI_WnBASE(n)	(*(volatile uint32_t *)(0xfffffc8760000400UL + 0x100 * (n)))
#define	PYXIS_PCI_WnMASK(n)	(*(volatile uint32_t *)(0xfffffc8760000440UL + 0x100 * (n)))
#define	PYXIS_PCI_TnBASE(n)	(*(volatile uint32_t *)(0xfffffc8760000480UL + 0x100 * (n)))
#define	PYXIS_PCI_TBIA		(*(volatile uint32_t *)0xfffffc8760000100UL)
#define	PYXIS_CTRL		(*(volatile uint32_t *)0xfffffc8740000100UL)
#define	PYXIS_CTRL_PCI_LOOP_EN	(1<<2)

#define	PYXIS_PCI_DENSE(x)	(*(volatile uint32_t *)(0xfffffc8600000000UL + (x)))

uint64_t memlist_get(uint64_t size, int align, struct memlist **listp);
static uint64_t alloc_pci_sg(uint64_t size)
{
	if (size < MMU_PAGESIZE)
	{
		size = MMU_PAGESIZE;
	}
	uint64_t tbase = memlist_get(size, 64 * 1024, &pfreelistp);
	bzero((void *)(0xfffffc0000000000ul + tbase), size);
	return tbase;
}

static void pci_tbi(void)
{
	volatile uint32_t ctrl;
	int i;

	__asm__ __volatile__("mb":::"memory");
	ctrl = PYXIS_CTRL;
	PYXIS_CTRL = ctrl | PYXIS_CTRL_PCI_LOOP_EN;
	__asm__ __volatile__("mb":::"memory");

	for (i = 0; i < 12; i++) {
		PYXIS_PCI_DENSE(0x08000000 + i * 0x8000);
	}

	__asm__ __volatile__("mb":::"memory");
	PYXIS_CTRL = ctrl;
	__asm__ __volatile__("mb":::"memory");
}

static void
setup_pci_window(void)
{
	int i;
	struct {
		uint64_t base;
		uint64_t size;
		uint64_t attr;
		uint64_t pci_base;
	} def_value[4] = {
		{0x00800000, 0x00800000, 3},
		{0x40000000, 0x40000000, 1, 0},
		{0x08000000, 0x00200000, 3},
		{0x20000000, 0x20000000, 3}
	};

	for (i = 0; i < sizeof (def_value) / sizeof(def_value[0]); i++) {
		PYXIS_PCI_WnBASE(i) = 0x0;
		__asm__ __volatile__("mb":::"memory");
		if (def_value[i].attr & 1) {
			PYXIS_PCI_WnMASK(i) = def_value[i].size - 0x00100000;
			if (def_value[i].attr & 2) {
				PYXIS_PCI_TnBASE(i) = alloc_pci_sg(def_value[i].size / 0x2000 * 8) >> 2;
			} else {
				PYXIS_PCI_TnBASE(i) = def_value[i].pci_base;
			}
			__asm__ __volatile__("mb":::"memory");
			PYXIS_PCI_WnBASE(i) = def_value[i].base | def_value[i].attr;
		}
	}

	{
		int tbi_index = 2;
		uint64_t *tbase = (uint64_t *)(0xfffffc0000000000ul + (PYXIS_PCI_TnBASE(tbi_index) << 2));
		for (i = 0; i < def_value[tbi_index].size / 0x2000; i++) {
			tbase[i] = ((PYXIS_PCI_TnBASE(tbi_index) << 2) >> 12) | 1;
		}
	}
	__asm__ __volatile__("mb":::"memory");
	pci_tbi();
}

void exitto(int (*entrypoint)())
{
	uint64_t v;
	const char *str;

	setup_pci_window();

	phandle_t chosen = prom_chosennode();

	v = htonll((uint64_t)_RamdiskStart);
	prom_setprop(chosen, "ramdisk_start", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)_RamdiskEnd);
	prom_setprop(chosen, "ramdisk_end", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)pfreelistp);
	prom_setprop(chosen, "phys-avail", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)pinstalledp);
	prom_setprop(chosen, "phys-installed", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)pscratchlistp);
	prom_setprop(chosen, "boot-scratch", (caddr_t)&v, sizeof(v));
	if (bootp_response) {
		uint_t blen = strlen(bootp_response) / 2;
		char *pktbuf = alloca(blen);
		hexascii_to_octet(bootp_response, blen * 2, pktbuf, &blen);
		prom_setprop(chosen, "bootp-response", pktbuf, blen);
	} else {
	}
	str = "";
	prom_setprop(chosen, "boot-args", (caddr_t)str, strlen(str) + 1);
	str = "";
	prom_setprop(chosen, "bootargs", (caddr_t)str, strlen(str) + 1);
	str = filename;
	prom_setprop(chosen, "whoami", (caddr_t)str, strlen(str) + 1);
	str = filename;
	prom_setprop(chosen, "boot-file", (caddr_t)str, strlen(str) + 1);

	if (prom_getproplen(chosen, "impl-arch-name") < 0) {
		str = get_mfg_name();
		prom_setprop(chosen, "impl-arch-name", (caddr_t)str, strlen(str) + 1);
	}

	str = get_mfg_name();
	prom_setprop(chosen, "mfg-name", (caddr_t)str, strlen(str) + 1);
	str = "9600,8,n,1,-";
	prom_setprop(chosen, "ttya-mode", (caddr_t)str, strlen(str) + 1);
	prom_setprop(chosen, "ttyb-mode", (caddr_t)str, strlen(str) + 1);

	xboot_info.bi_fdt = (uint64_t)get_fdtp();
	entrypoint(&xboot_info);
}

static int
atoi(const char *p)
{
	int n;
	int c = *p++, neg = 0;

	while (isspace(c)) {
		c = *p++;
	}
	if (!isdigit(c)) {
		switch (c) {
		case '-':
			neg++;
			/* FALLTHROUGH */
		case '+':
			c = *p++;
		}
	}
	for (n = 0; isdigit(c); c = *p++) {
		n *= 10; /* two steps to avoid unnecessary overflow */
		n += '0' - c; /* accum neg to avoid surprises at MAX */
	}
	return (neg ? n : -n);
}
extern void get_boot_zpool(char *);
void set_zfs_bootfs(void)
{
	get_boot_zpool(zfs_bootfs);
	phandle_t chosen = prom_chosennode();
	prom_setprop(chosen, "zfs-bootfs", (caddr_t)zfs_bootfs, strlen(zfs_bootfs) + 1);
	prom_printf("zfs_bootfs=%s\n", zfs_bootfs);
}
void set_rootfs(char *bpath, char *fstype)
{
	phandle_t chosen = prom_chosennode();

	if (strcmp(fstype, "nfs") == 0) {
		char *str = "nfsdyn";
		prom_setprop(chosen, "fstype", (caddr_t)str, strlen(str) + 1);
	} else {
		char *str = fstype;
		prom_setprop(chosen, "fstype", (caddr_t)str, strlen(str) + 1);

		char *dev = strtok(bpath, " ");
		if (strcmp(dev, "SCSI") == 0) {
			char boot_dev[256];
			int bus = atoi(strtok(NULL, " "));
			int slot = atoi(strtok(NULL, " "));
			int func = atoi(strtok(NULL, " "));
			int disk = atoi(strtok(NULL, " "));
			uintptr_t conf_base = get_config_base(0, bus, slot, func);
			uint16_t vid = pci_conf_read16(conf_base, PCI_CONF_VENID);
			uint16_t did = pci_conf_read16(conf_base, PCI_CONF_DEVID);
			uint16_t svid;
			uint16_t sdid;
			uint8_t header = pci_conf_read8(conf_base, PCI_CONF_HEADER);
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
			if (svid != 0)
				sprintf(boot_dev, "/pci@0,0/pci%x,%x@%x,0/sd@%x,0:a", svid, sdid, ((uint32_t)bus << 16) | ((uint32_t)slot << 11) | ((uint32_t)func << 8), disk, 0);
			else
				sprintf(boot_dev, "/pci@0,0/pci%x,%x@%x,0/sd@%x,0:a", vid, did, ((uint32_t)bus << 16) | ((uint32_t)slot << 11) | ((uint32_t)func << 8), disk, 0);
			str = boot_dev;
			prom_setprop(chosen, "bootpath", (caddr_t)str, strlen(str) + 1);
		} else if (dev[0] == '/') {
			str = bpath;
			prom_setprop(chosen, "bootpath", (caddr_t)str, strlen(str) + 1);
		}
	}
}

void
load_ramdisk(void *virt, const char *name)
{
	static char	tmpname[MAXPATHLEN];

	if (determine_fstype_and_mountroot(prom_bootpath()) == VFS_SUCCESS) {
		set_rootfs(prom_bootpath(), get_default_fs()->fsw_name);
		if (strcmp(get_default_fs()->fsw_name, "zfs") == 0)
			set_zfs_bootfs();

		strcpy(tmpname, name);
		int fd = openfile(tmpname);
		if (fd >= 0) {
			prom_printf("%s:%d open %s\n",__func__,__LINE__, tmpname);
			struct stat st;
			if (fstat(fd, &st) == 0) {
				xread(fd, (char *)virt, st.st_size);
			}
		} else {
			prom_printf("open failed %s\n", tmpname);
			prom_reset();
		}
		closeall(1);
		unmountroot();
	} else {
		prom_printf("mountroot failed\n");
		prom_reset();
	}
}

#define	MAXNMLEN	80		/* # of chars in an impl-arch name */

char *
get_mfg_name(void)
{
	pnode_t n;
	int len;

	static char mfgname[MAXNMLEN];

	if ((n = prom_rootnode()) != OBP_NONODE &&
	    (len = prom_getproplen(n, "mfg-name")) > 0 && len < MAXNMLEN) {
		(void) prom_getprop(n, "mfg-name", mfgname);
		mfgname[len] = '\0'; /* broken clones don't terminate name */
		return (mfgname);
	}

	return ("Unknown");
}

void prom_node_fixup(void)
{
	const char *mfgname = "SUNW,pc164lx";
	const char *model = "AlphaPC 164LX";
	const char *str;

	pnode_t root = prom_finddevice("/");

	// root property
	str = mfgname;
	prom_setprop(root, "mfg-name", (caddr_t)str, strlen(str) + 1);
	str = model;
	prom_setprop(root, "model", (caddr_t)str, strlen(str) + 1);
	str = mfgname;
	prom_setprop(root, "compatible", (caddr_t)str, strlen(str) + 1);
}

char *
get_default_bootpath(void)
{
	static char def_bootpath[80];

	memset(def_bootpath, 0, sizeof(def_bootpath));
	srm_getenv(ENV_BOOTED_DEV, def_bootpath, sizeof(def_bootpath));

	return def_bootpath;
}

void init_machdev(void)
{
	init_sil3124();
}
