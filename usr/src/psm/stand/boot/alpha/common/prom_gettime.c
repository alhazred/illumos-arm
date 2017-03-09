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

#include <sys/promif.h>
#include <sys/sysmacros.h>

extern uint64_t tick_per_count;

u_int prom_gettime(void)
{
	static uint64_t val;
	uint64_t newVal;
	uint64_t prevVal;

	do {
		uint32_t pcc = (uint32_t)__builtin_alpha_rpcc();
		prevVal = val;
		newVal = prevVal + (pcc - (uint32_t)prevVal);
	} while (!__sync_bool_compare_and_swap(&val, prevVal, newVal));

	return newVal / tick_per_count;
}

