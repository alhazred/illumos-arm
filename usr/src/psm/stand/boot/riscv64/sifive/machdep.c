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
#include <sys/sysmacros.h>
#include <sys/machparam.h>
#include <sys/memlist.h>
#include <sys/memlist_impl.h>
#include <sys/csr.h>
#include <sys/saio.h>
#include <sys/bootsyms.h>
#include <sys/fcntl.h>
#include <sys/platform.h>
#include <sys/platnames.h>
#include <alloca.h>
#include <netinet/inetutil.h>
#include <sys/bootvfs.h>
#include "ramdisk.h"
#include "boot_plat.h"
#include "gem.h"
#include "mmc.h"
#include <libfdt.h>
#include <sys/memlist.h>
#ifndef rounddown
#define	rounddown(x, y)	(((x)/(y))*(y))
#endif

extern char _BootScratch[];
extern char _RamdiskStart[];
extern char _RamdiskStart[];
extern char _RamdiskEnd[];
extern char filename[];
static char zfs_bootfs[256];
char v2args_buf[V2ARGS_BUF_SZ];
char *v2args = v2args_buf;
extern char *bootp_response;
extern volatile uint64_t boot_hartmask;
extern volatile uintptr_t kernel_entrypoint;
extern volatile uint64_t kernel_satp;
extern struct xboot_info xboot_info;

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
extern	void init_physmem_common(void);

void
setup_aux(void)
{
}

static void
add_iomap(pnode_t node, void *arg)
{
	if (prom_is_compatible(node, (const char *)arg)) {
		int index = 0;
		for (;;) {
			uint64_t base;
			uint64_t size;
			if (prom_get_reg_address(node, index, &base) == 0 &&
			    prom_get_reg_size(node, index, &size) == 0) {
				uint64_t addr = rounddown(base, MMU_PAGESIZE);
				uint64_t len = roundup(base + size, MMU_PAGESIZE) - addr;
				if (!memlist_find(piolistp, addr)) {
					prom_printf("add io %p %p for %s\n", addr, len, arg);
					memlist_add_span(addr, len, &piolistp);
				}
			} else {
				break;
			}
			index++;
		}
	}
}

static void
update_enable_hartmask(pnode_t node, void *arg)
{
	int len;

	len = prom_getproplen(node, "device_type");
	if (len <= 0) return;
	char *device_type = __builtin_alloca(len);
	prom_getprop(node, "device_type", device_type);
	if (strcmp("cpu", device_type) != 0)
		return;

	len = prom_getproplen(node, "mmu-type");
	if (len <= 0) return;
	char *mmu_type = __builtin_alloca(len);
	prom_getprop(node, "mmu-type", mmu_type);
	if (strcmp("riscv,sv39", mmu_type) != 0 &&
	    strcmp("riscv,sv48", mmu_type) != 0)
		return;

	uint64_t base;
	if (prom_get_reg_address(node, 0, &base) != 0)
		return;

	__sync_fetch_and_or((uint64_t*)arg, (1ul << base));
}

void
init_physmem(void)
{
	init_physmem_common();
}
void
init_iolist(void)
{
	prom_walk(add_iomap, "sifive,fu540-c000-gem");
	prom_walk(add_iomap, "sifive,spi0");
	prom_walk(add_iomap, "sifive,i2c0");
	prom_walk(add_iomap, "sifive,uart0");
	prom_walk(add_iomap, "sifive,pwm0");
	prom_walk(add_iomap, "sifive,fu540-c000-prci");
	prom_walk(add_iomap, "sifive,plic-1.0.0");
}
void exitto(int (*entrypoint)())
{
	for (struct memlist *ml = plinearlistp; ml != NULL; ml = ml->ml_next) {
		uintptr_t pa = ml->ml_address;
		uintptr_t sz = ml->ml_size;
		map_phys(PTE_A | PTE_D | PTE_G | PTE_W | PTE_R, (caddr_t)(SEGKPM_BASE + pa), pa, sz);
	}
	for (struct memlist *ml = piolistp; ml != NULL; ml = ml->ml_next) {
		uintptr_t pa = ml->ml_address;
		uintptr_t sz = ml->ml_size;
		map_phys(PTE_A | PTE_D | PTE_G | PTE_W | PTE_R, (caddr_t)(SEGKPM_BASE + pa), pa, sz);
	}
	fini_memory();

	uint64_t v;
	const char *str;

	v = htonll((uint64_t)_RamdiskStart);
	prom_setprop(prom_chosennode(), "ramdisk_start", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)_RamdiskEnd);
	prom_setprop(prom_chosennode(), "ramdisk_end", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)pfreelistp);
	prom_setprop(prom_chosennode(), "phys-avail", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)pinstalledp);
	prom_setprop(prom_chosennode(), "phys-installed", (caddr_t)&v, sizeof(v));
	v = htonll((uint64_t)pscratchlistp);
	prom_setprop(prom_chosennode(), "boot-scratch", (caddr_t)&v, sizeof(v));
	if (bootp_response) {
		uint_t blen = strlen(bootp_response) / 2;
		char *pktbuf = alloca(blen);
		hexascii_to_octet(bootp_response, blen * 2, pktbuf, &blen);
		prom_setprop(prom_chosennode(), "bootp-response", pktbuf, blen);
	} else {
	}
	str = "";
	prom_setprop(prom_chosennode(), "boot-args", (caddr_t)str, strlen(str) + 1);
	str = "";
	prom_setprop(prom_chosennode(), "bootargs", (caddr_t)str, strlen(str) + 1);
	str = filename;
	prom_setprop(prom_chosennode(), "whoami", (caddr_t)str, strlen(str) + 1);
	str = filename;
	prom_setprop(prom_chosennode(), "boot-file", (caddr_t)str, strlen(str) + 1);

	if (prom_getproplen(prom_chosennode(), "impl-arch-name") < 0) {
		str = "riscv64";
		prom_setprop(prom_chosennode(), "impl-arch-name", (caddr_t)str, strlen(str) + 1);
	}

	str = get_mfg_name();
	prom_setprop(prom_chosennode(), "mfg-name", (caddr_t)str, strlen(str) + 1);
	str = "115200,8,n,1,-";
	prom_setprop(prom_chosennode(), "ttya-mode", (caddr_t)str, strlen(str) + 1);
	prom_setprop(prom_chosennode(), "ttyb-mode", (caddr_t)str, strlen(str) + 1);

	str = "serial0:115200n8";		// board
	prom_setprop(prom_chosennode(), "stdout-path", (caddr_t)str, strlen(str) + 1);

	xboot_info.bi_fdt = SEGKPM_BASE + (uint64_t)get_fdtp();

	uint64_t hartmask = 0;
	prom_walk(update_enable_hartmask, &hartmask);
	while (hartmask != boot_hartmask) {
		asm volatile ("":::"memory");
	}

	int tp;
	asm volatile ("mv %0, tp":"=r"(tp)::);

	kernel_satp = csr_read_satp();
	asm volatile ("fence":::"memory");
	kernel_entrypoint = (uintptr_t)entrypoint;
	entrypoint(&xboot_info, hartmask);
}

extern void get_boot_zpool(char *);
static void 
set_zfs_bootfs(void)
{
	get_boot_zpool(zfs_bootfs);
	prom_setprop(prom_chosennode(), "zfs-bootfs", (caddr_t)zfs_bootfs, strlen(zfs_bootfs) + 1);
}

static void
set_rootfs(char *bpath, char *fstype)
{
	char *str;
	prom_printf("bpath=%s\n", bpath);
	if (strcmp(fstype, "nfs") == 0) {
		str = "nfsdyn";
		prom_setprop(prom_chosennode(), "fstype", (caddr_t)str, strlen(str) + 1);
	} else {
		str = fstype;
		prom_setprop(prom_chosennode(), "fstype", (caddr_t)str, strlen(str) + 1);
		str = bpath;
		if (strstr(str, "blkdev") == NULL) {
			static char buf[256];
			sprintf(buf, "%s/blkdev@0", str);
			str = buf;
		}
		prom_setprop(prom_chosennode(), "bootpath", (caddr_t)str, strlen(str) + 1);
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

/*
 * Return the manufacturer name for this platform.
 *
 * This is exported (solely) as the rootnode name property in
 * the kernel's devinfo tree via the 'mfg-name' boot property.
 * So it's only used by boot, not the boot blocks.
 */
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
		prom_printf("mfg_name=%s\n", mfgname);
		return (mfgname);
	}

	return ("Unknown");
}

static int
prom_search_dev(pnode_t nodeid, const char *dev, char *name)
{
	if (prom_is_compatible(nodeid, dev)) {
		int namelen = prom_getproplen(nodeid, "name");
		if (namelen > 1) {
			strcpy(name, "/");
			name += strlen(name);
			prom_getprop(nodeid, "name", name);
			int reglen = prom_getproplen(nodeid, "reg");
			if (reglen > 0) {
				name += strlen(name);
				strcpy(name, "@");
				name += strlen(name);
				uint64_t base;
				prom_get_reg(nodeid, 0, &base);
				sprintf(name, "%lx", base);
			}
		}
		return 0;
	}

	pnode_t child = prom_childnode(nodeid);
	while (child > 0) {
		int namelen = prom_getproplen(child, "name");
		if (namelen > 0) {
			strcpy(name, "/");
			prom_getprop(nodeid, "name", name + 1);
			if (name[1] == 0) {
				name[0] = 0;
			} else {
				int reglen = prom_getproplen(nodeid, "reg");
				if (reglen > 0) {
					name += strlen(name);
					strcpy(name, "@");
					name += strlen(name);
					uint64_t base;
					prom_get_reg(nodeid, 0, &base);
					sprintf(name, "%lx", base);
				}
			}
		}
		if (prom_search_dev(child, dev, name + strlen(name)) == 0) {
			return 0;
		}
		child = prom_nextnode(child);
	}
	name[0] = 0;
	return -1;
}

char *
get_default_bootpath(void)
{
	static char def_bootpath[80];
	prom_search_dev(prom_rootnode(), "sifive,fu540-c000-gem", def_bootpath);
	return def_bootpath;
}

void _reset(void)
{
	prom_printf("%s:%d\n",__func__,__LINE__);
	for (;;) {}
}

void init_machdev(void)
{
	char *str;
	str = "SUNW,sifive";
	prom_setprop(prom_chosennode(), "impl-arch-name", (caddr_t)str, strlen(str) + 1);
	prom_setprop(prom_rootnode(), "mfg-name", (caddr_t)str, strlen(str) + 1);
	int namelen = prom_getproplen(prom_rootnode(), "compatible");
	namelen += strlen(str) + 1;
	char *compatible = __builtin_alloca(namelen);
	strcpy(compatible, str);
	prom_getprop(prom_rootnode(), "compatible", compatible + strlen(str) + 1);
	prom_setprop(prom_rootnode(), "compatible", compatible, namelen);

	init_gem();
	init_mmc();
}
