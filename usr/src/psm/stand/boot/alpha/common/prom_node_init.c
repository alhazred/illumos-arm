/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
#include <sys/promif.h>
#include <sys/promimpl.h>
#include <sys/systm.h>
#include <sys/salib.h>
#include <sys/bootvfs.h>
#include <util/getoptstr.h>
#include "boot_plat.h"
#include <libfdt.h>
#include <sys/platnames.h>
#include <stdbool.h>
#include <sys/pal.h>
#include "srm.h"

void
prom_node_init(void)
{
	const char *str;
	uint32_t val;
	size_t dtb_size = 0x80000;

	void *fdtp = (void *)(SEGKPM_BASE + memlist_get(dtb_size, MMU_PAGESIZE, &pfreelistp));
	fdt_create_empty_tree(fdtp, dtb_size);
	set_fdtp(fdtp);

	pnode_t root = prom_rootnode();

	// root property
	val = htonl(2);
	prom_setprop(root, "#address-cells", (caddr_t)&val, sizeof(val));
	val = htonl(2);
	prom_setprop(root, "#size-cells", (caddr_t)&val, sizeof(val));

	// cpus
	pnode_t cpus = prom_add_subnode(prom_rootnode(), "cpus");
	val = htonl(1);
	prom_setprop(cpus, "#address-cells", (caddr_t)&val, sizeof(val));
	val = htonl(0);
	prom_setprop(cpus, "#size-cells", (caddr_t)&val, sizeof(val));
	for (int i = 0; i < hwrpb->pcs_num; i++) {
		char buf[40];
		struct pcs *pcs = (struct pcs *)((caddr_t)hwrpb + hwrpb->pcs_off + hwrpb->pcs_size * hwrpb->cpu_id);
		if (!(pcs->stat_flags & PCS_STAT_PP))
			continue;
		sprintf(buf, "cpu@%d", i);

		pnode_t cpu = prom_add_subnode(cpus, buf);
		str = "cpu";
		prom_setprop(cpu, "device_type", (caddr_t)str, strlen(str) + 1);
		str = ((pal_whami() == i)? "ok": "disable");
		prom_setprop(cpu, "status", (caddr_t)str, strlen(str) + 1);
		val = htonl(i);
		prom_setprop(cpu, "reg", (caddr_t)&val, sizeof(val));
		val = htonl(hwrpb->counter);
		prom_setprop(cpu, "clock-frequency", (caddr_t)&val, sizeof(val));
	}

	// memory
	pnode_t memory = prom_add_subnode(prom_rootnode(), "memory");
	{
		const struct memlist *listp = pinstalledp;
		while (listp) {
			uint32_t reg[4];
			reg[0] = htonl(listp->ml_address >> 32);
			reg[1] = htonl(listp->ml_address);
			reg[2] = htonl(listp->ml_size >> 32);
			reg[3] = htonl(listp->ml_size);
			prom_setprop(memory, "reg", (caddr_t)reg, sizeof(reg));
			listp = listp->ml_next;
		}
	}

	pnode_t chosen = prom_add_subnode(prom_rootnode(), "chosen");
	{
		static char def_bootargs[256];
		srm_getenv(ENV_BOOTED_OSFLAGS, def_bootargs, sizeof(def_bootargs));
		if (strlen(def_bootargs) > 0)
			prom_setprop(chosen, "bootargs", (caddr_t)def_bootargs, strlen(def_bootargs) + 1);
	}

	prom_node_fixup();
}
