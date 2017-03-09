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

#include <sys/boot.h>
#include <sys/promif.h>
#include <sys/pal.h>
#include <sys/salib.h>
#include <sys/hwrpb.h>
#include "boot_plat.h"

char *default_name = "alpha";
char *default_path = "/platform/alpha/kernel";
uint64_t tick_per_count;

boolean_t
is_netdev(char *devpath)
{
	if (strncmp(devpath, "BOOTP", strlen("BOOTP")) == 0) {
		return B_TRUE;
	} else if (devpath[0] == '/') {
		pnode_t node = prom_finddevice(devpath);
		if (node > 0) {
			int len = prom_getproplen(node, "model");
			if (len > 0) {
				char *buf = __builtin_alloca(len);
				prom_getprop(node, "model", buf);
				if (strcmp(buf, "Ethernet controller") == 0) {
					return B_TRUE;
				}
			}
		}
	}
	return B_FALSE;
}

void
fiximp(void)
{
	extern int use_align;

	use_align = 1;

	tick_per_count = hwrpb->counter / 1000000;	// hack
}

void _reset(void)
{
	asm volatile ("call_pal %0"
	    :
	    : "i"(PAL_halt)
	    : "memory");
	for (;;) {}
}
