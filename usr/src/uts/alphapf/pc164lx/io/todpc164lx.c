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

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>

#include <sys/cpuvar.h>
#include <sys/clock.h>
#include <sys/debug.h>
#include <sys/rtc.h>
#include <sys/archsystm.h>
#include <sys/sysmacros.h>
#include <sys/lockstat.h>
#include <sys/stat.h>
#include <sys/sunddi.h>
#include <sys/promif.h>

static int todpc_rtcget(unsigned char *buf);
static void todpc_rtcput(unsigned char *buf);
static inline uint8_t _inb(int port)
{
	uint8_t value;
	uint8_t *ioport_addr = (uint8_t *)(0xFFFFFC8900000000UL + port);
	__asm__ __volatile__(
	    "ldbu %0, %1" : "=r" (value) : "m" (*ioport_addr) : "memory");
	__asm__ __volatile__("mb" ::: "memory");
	return value;
}

static inline void _outb(int port, uint8_t value)
{
	uint8_t *ioport_addr = (uint8_t *)(0xFFFFFC8900000000UL + port);
	__asm__ __volatile__(
	    "stb %1, %0" : "=m" (*ioport_addr) : "r" (value) : "memory");
	__asm__ __volatile__("mb" ::: "memory");
}

/*
 * The minimum sleep time till an alarm can be fired.
 * This can be tuned in /etc/system, but if the value is too small,
 * there is a danger that it will be missed if it takes too long to
 * get from the set point to sleep.  Or that it can fire quickly, and
 * generate a power spike on the hardware.  And small values are
 * probably only usefull for test setups.
 */
static int clock_min_alarm = 4;

/*
 * Machine-dependent clock routines.
 */

extern long gmt_lag;

/*
 * Write the specified time into the clock chip.
 * Must be called with tod_lock held.
 */
/*ARGSUSED*/
static void
todpc_set(timestruc_t ts)
{
	todinfo_t tod = utc_to_tod(ts.tv_sec - ggmtl());
	struct rtc_t rtc;

	ASSERT(MUTEX_HELD(&tod_lock));

	if (todpc_rtcget((unsigned char *)&rtc))
		return;

	/*
	 * rtc bytes are in binary-coded decimal, so we have to convert.
	 * We assume that we wrap the rtc year back to zero at 2000.
	 */
	rtc.rtc_yr	= tod.tod_year - YRBASE;
	rtc.rtc_mon	= tod.tod_month;
	rtc.rtc_dom	= tod.tod_day;
	rtc.rtc_dow	= tod.tod_dow;
	rtc.rtc_hr	= tod.tod_hour;
	rtc.rtc_min	= tod.tod_min;
	rtc.rtc_sec	= tod.tod_sec;

	todpc_rtcput((unsigned char *)&rtc);
}

/*
 * Read the current time from the clock chip and convert to UNIX form.
 * Assumes that the year in the clock chip is valid.
 * Must be called with tod_lock held.
 */
/*ARGSUSED*/
static timestruc_t
todpc_get()
{
	timestruc_t ts;
	todinfo_t tod;
	struct rtc_t rtc;

	ASSERT(MUTEX_HELD(&tod_lock));

	if (todpc_rtcget((unsigned char *)&rtc)) {
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
		tod_status_set(TOD_GET_FAILED);
		return (ts);
	}

	tod.tod_year	= rtc.rtc_yr + YRBASE;
	tod.tod_month	= rtc.rtc_mon;
	tod.tod_day	= rtc.rtc_dom;
	tod.tod_dow	= rtc.rtc_dow;
	tod.tod_hour	= rtc.rtc_hr;
	tod.tod_min	= rtc.rtc_min;
	tod.tod_sec	= rtc.rtc_sec;

	/* read was successful so ensure failure flag is clear */
	tod_status_clear(TOD_GET_FAILED);

	ts.tv_sec = tod_to_utc(tod) + ggmtl();
	ts.tv_nsec = 0;

	return (ts);
}

/*
 * Write the specified wakeup alarm into the clock chip.
 * Must be called with tod_lock held.
 */
static void
todpc_setalarm(timestruc_t ts)
{
	struct rtc_t rtc;
	int delta, asec, amin, ahr, adom, amon;

	ASSERT(MUTEX_HELD(&tod_lock));

	/* A delay of zero is not allowed */
	if (ts.tv_sec == 0)
		return;

	/* Make sure that we delay no less than the minimum time */
	if (ts.tv_sec < clock_min_alarm)
		ts.tv_sec = clock_min_alarm;

	if (todpc_rtcget((unsigned char *)&rtc))
		return;

	/*
	 * Compute alarm secs, mins and hrs, and where appropriate, dom
	 * and mon.  rtc bytes are in binary-coded decimal, so we have
	 * to convert.
	 */
	delta = ts.tv_sec + rtc.rtc_sec;
	asec = delta % 60;

	delta = (delta / 60) + rtc.rtc_min;
	amin = delta % 60;

	delta = (delta / 60) + rtc.rtc_hr;
	ahr  = delta % 24;

	if (delta >= 24) {
		prom_printf("No day alarm - set to end of today!\n");
		asec = 59;
		amin = 59;
		ahr  = 23;
	}

	rtc.rtc_asec = asec;
	rtc.rtc_amin = amin;
	rtc.rtc_ahr  = ahr;

	rtc.rtc_statusb |= RTC_AIE;	/* Enable alarm interrupt */

	todpc_rtcput((unsigned char *)&rtc);
}

/*
 * Clear an alarm.  This is effectively setting an alarm of 0.
 */
static void
todpc_clralarm()
{
	timestruc_t ts = {0};
	mutex_enter(&tod_lock);
	todpc_setalarm(ts);
	mutex_exit(&tod_lock);
}

/*
 * Routine to read contents of real time clock to the specified buffer.
 * Returns ENXIO if clock not valid, or EAGAIN if clock data cannot be read
 * else 0.
 * The routine will busy wait for the Update-In-Progress flag to clear.
 * On completion of the reads the Seconds register is re-read and the
 * UIP flag is rechecked to confirm that an clock update did not occur
 * during the accesses.  Routine will error exit after 256 attempts.
 * (See bugid 1158298.)
 * Routine returns RTC_NREG (which is 15) bytes of data, as given in the
 * technical reference.  This data includes both time and status registers.
 */

static int
todpc_rtcget(unsigned char *buf)
{
	unsigned char	reg;
	int		i;

	ASSERT(MUTEX_HELD(&tod_lock));

	_outb(RTC_ADDR, RTC_D);
	reg = _inb(RTC_DATA);
	if ((reg & RTC_VRT) == 0)
		return (ENXIO);

	for (i = 0; i < 256; i++) {
		int s = splhi();
		int valid = 0;
		_outb(RTC_ADDR, RTC_A);
		reg = _inb(RTC_DATA);
		if ((reg & RTC_UIP) == 0) {
			for (int j = 0; j < RTC_NREG; j++) {
				_outb(RTC_ADDR, j);
				buf[j] = _inb(RTC_DATA);
			}
			_outb(RTC_ADDR, 0);
			reg = _inb(RTC_DATA);
			if (reg == ((struct rtc_t *)buf)->rtc_sec &&
			    (((struct rtc_t *)buf)->rtc_statusa & RTC_UIP) == 0) {
				splx(s);
				return (0);
			}
		}
		splx(s);
		drv_usecwait(60);
	}
	return (EAGAIN);
}

/*
 * This routine writes the contents of the given buffer to the real time
 * clock.  It is given RTC_NREGP bytes of data, which are the 10 bytes used
 * to write the time and set the alarm.  It should be called with the priority
 * raised to 5.
 */
static void
todpc_rtcput(unsigned char *buf)
{
	unsigned char	reg;
	int		i;
	unsigned char	tmp;

	_outb(RTC_ADDR, RTC_B);
	reg = _inb(RTC_DATA);
	_outb(RTC_ADDR, RTC_B);
	_outb(RTC_DATA, reg | RTC_SET);

	for (i = 0; i < RTC_NREGP; i++) {
		_outb(RTC_ADDR, i);
		_outb(RTC_DATA, buf[i]);
	}

	_outb(RTC_ADDR, RTC_B);
	reg = _inb(RTC_DATA);
	_outb(RTC_ADDR, RTC_B);
	_outb(RTC_DATA, reg & ~RTC_SET);
}

static struct modlmisc modlmisc = {
	&mod_miscops, "todpc164lx"
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};

int
_init(void)
{
	extern tod_ops_t tod_ops;

	tod_ops.tod_get = todpc_get;
	tod_ops.tod_set = todpc_set;
	tod_ops.tod_set_watchdog_timer = NULL;
	tod_ops.tod_clear_watchdog_timer = NULL;
	tod_ops.tod_set_power_alarm = todpc_setalarm;
	tod_ops.tod_clear_power_alarm = todpc_clralarm;

	return mod_install(&modlinkage);
}

int
_fini(void)
{
	return (EBUSY);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}
