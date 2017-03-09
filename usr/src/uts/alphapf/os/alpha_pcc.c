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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/disp.h>
#include <sys/var.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/archsystm.h>
#include <sys/cpuvar.h>
#include <sys/clock.h>
#include <sys/atomic.h>
#include <sys/lockstat.h>
#include <sys/dtrace.h>
#include <sys/time.h>
#include <sys/panic.h>
#include <sys/cpu.h>
#include <sys/pal.h>
#include <sys/archsystm.h>
#include <sys/hwrpb.h>

#define	NSEC_SHIFT 5

hrtime_t
pcc_gethrtimeunscaled(void)
{
	static volatile hrtime_t pcc_hrtime_base = 0;
	register hrtime_t last;
	register uint32_t offset;

	do {
		last = pcc_hrtime_base;
		offset = (uint32_t)(__builtin_alpha_rpcc() - last);
	} while (atomic_cas_64((uint64_t *)&pcc_hrtime_base, last, last + offset) != last);

	return last + offset;
}

static inline hrtime_t
pcc_scaletime(hrtime_t pcc)
{
	hrtime_t sec = pcc / hwrpb->counter;
	pcc -= sec * hwrpb->counter;
	return sec * NANOSEC + pcc * NANOSEC / hwrpb->counter;
}

static inline hrtime_t
pcc_unscaletime(hrtime_t nsec)
{
	hrtime_t sec = nsec / NANOSEC;
	nsec -= sec * NANOSEC;
	return sec * hwrpb->counter + nsec * hwrpb->counter / NANOSEC;
}

hrtime_t
pcc_gethrtime(void)
{
	hrtime_t hrt;
	hrt = pcc_gethrtimeunscaled();
	return pcc_scaletime(hrt);
}

hrtime_t
dtrace_gethrtime(void)
{
	hrtime_t hrt;
	hrt = pcc_gethrtimeunscaled();
	return (hrt);
}

/*
 * Convert a nanosecond based timestamp to pcc
 */
uint64_t
pcc_unscalehrtime(hrtime_t nsec)
{
	return pcc_unscaletime(nsec);
}

/* Convert a pcc timestamp to nanoseconds */
void
pcc_scalehrtime(hrtime_t *pcc)
{
	*pcc  = pcc_scaletime(*pcc);
}

/*
 * The following converts nanoseconds of highres-time to ticks
 */

static inline uint64_t
hrtime2tick(hrtime_t ts)
{
	hrtime_t q = ts / NANOSEC;
	hrtime_t r = ts - (q * NANOSEC);

	return (q * hwrpb->counter + ((r * hwrpb->counter) / NANOSEC));
}

/*
 * This is used to convert scaled high-res time from nanoseconds to
 * unscaled hardware ticks.  (Read from hardware timestamp counter)
 */

uint64_t
unscalehrtime(hrtime_t ts)
{
	uint64_t unscale = 0;
	hrtime_t rescale;
	hrtime_t diff = ts;

	while (diff > (nsec_per_tick)) {
		unscale += hrtime2tick(diff);
		rescale = pcc_scaletime(unscale);
		diff = ts - rescale;
	}

	return (unscale);
}
