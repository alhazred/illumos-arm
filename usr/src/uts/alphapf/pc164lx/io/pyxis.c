
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
 * Copyright (c) 1992, 2010, Oracle and/or its affiliates. All rights reserved.
 */
/*
 * Copyright (c) 2009-2010, Intel Corporation.
 * All rights reserved.
 */

#include <sys/pit.h>
#include <sys/strlog.h>
#include <sys/clock.h>
#include <sys/debug.h>
#include <sys/rtc.h>
#include <sys/cpupart.h>
#include <sys/cpuvar.h>
#include <sys/cpu_event.h>
#include <sys/cmt.h>
#include <sys/cpu.h>
#include <sys/disp.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/sysmacros.h>
#include <sys/memlist.h>
#include <sys/param.h>
#include <sys/promif.h>
#include <sys/cpu_pm.h>
#include <sys/mach_intr.h>
#include <vm/hat_alpha.h>
#include <sys/kdi_machimpl.h>
#include <sys/sdt.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/cpc_pcbe.h>
#include <sys/cmn_err.h>
#include <sys/bootconf.h>
#include <sys/reboot.h>
#include <sys/dtrace.h>
#include <sys/stack.h>
#include <sys/privregs.h>
#include <sys/dumphdr.h>
#include <sys/panic.h>
#include <sys/pal.h>
#include <vm/vm_dep.h>
#include <sys/clock_impl.h>
#include <sys/smp_impldefs.h>
#include <sys/hwrpb.h>
#include <sys/rtc.h>
#include <sys/modctl.h>
#include <sys/types.h>
#include <sys/machclock.h>

#define	NSEC_IN_SEC		1000000000

#define MAX_ISA_IRQ	15
#define NIRQ	40

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

static uint64_t intr_enabled[MAXIPL];

static void
psm_set_pic(uint64_t intr_setting)
{
	static uint64_t current_intr_enabled;

	_outb(MIMR_PORT, ~(uint8_t)(intr_setting));
	_outb(SIMR_PORT, ~(uint8_t)(intr_setting >> 8));

	uint64_t mask = ((current_intr_enabled ^ intr_setting) >> (MAX_ISA_IRQ + 1));
	int i = 0;
	while (mask) {
		if (mask & 1) {
			if (intr_setting & (1ull << (i + MAX_ISA_IRQ + 1))) {
				pal_cserve(0x34, i);
			} else {
				pal_cserve(0x35, i);
			}
		}
		i++;
		mask >>= 1;
	}

	current_intr_enabled = intr_setting;
}

static int
psm_chgspl_impl(int irqno, int ipl, int min_ipl, int max_ipl)
{
	if (irqno >= NIRQ)
		return 0;

	// 0 ... max_ipl - 1で割り込み有効化
	// max_ipl ... (MAXIPL - 1)で割り込み無効化
	for (int i = 0; i < MAXIPL; i++) {
		if (i < max_ipl)
			intr_enabled[i] |= (1ull << irqno);
		else
			intr_enabled[i] &= ~(1ull << irqno);
	}

	return (0);
}

static int
psm_intr_enter_impl(int ipl, int intno)
{
	int newipl;

	newipl = autovect[intno].avh_hi_pri;

	if (newipl)
		psm_set_pic(intr_enabled[newipl]);

	if (intno <= MAX_ISA_IRQ) {
		_outb(MCMD_PORT, PIC_SEOI | ((intno >= 8) ? MASTERLINE: intno));
		if (intno >= 8)
			_outb(SCMD_PORT, PIC_SEOI | (intno & 7));
	}

	if (newipl == 0)
		return (-1);

	return newipl;
}

static void
psm_intr_exit_impl(int ipl, int intno)
{
	psm_set_pic(intr_enabled[ipl]);
}

static void
psm_setspl_impl(int ipl)
{
	psm_set_pic(intr_enabled[ipl]);
}

static int
mach_softlvl_to_vect(int ipl)
{
	setsoftint = av_set_softint_pending;
	kdisetsoftint = kdi_av_set_softint_pending;

	return (-1);
}

static int
mach_clkinit(void)
{
	ulong_t clkticks = PIT_HZ / hz;

	_outb(PITCTL_PORT, (PIT_C0|PIT_NDIVMODE|PIT_READMODE));
	_outb(PITCTR0_PORT, (uchar_t)clkticks);
	_outb(PITCTR0_PORT, (uchar_t)(clkticks>>8));

	return (NSEC_IN_SEC / hz);
}

void
mach_init(void)
{
	uint8_t reg;

	for (int i = 0; i < NIRQ - (MAX_ISA_IRQ + 1); i++) {
		pal_cserve(0x35, i);
	}

	slvltovect = mach_softlvl_to_vect;
	addspl = psm_chgspl_impl;
	delspl = psm_chgspl_impl;
	setspl = psm_setspl_impl;
	setlvl = psm_intr_enter_impl;
	setlvlx = psm_intr_exit_impl;
	clkinitf = mach_clkinit;

	CPU->cpu_base_spl = 0;
	psm_chgspl_impl(MASTERLINE, 0, 0, MAXIPL);
	psm_chgspl_impl(20,         0, 0, MAXIPL);

	int ipl = CPU->cpu_m.mcpu_pri;
	psm_setspl_impl(ipl);

	// set rtc: binary, 24HR
	_outb(RTC_ADDR, RTC_A);
	_outb(RTC_DATA, RTC_DIV2 | RTC_RATE6);
	_outb(RTC_ADDR, RTC_B);
	_outb(RTC_DATA, /* RTC_PIE | */ RTC_SQWE | RTC_DM | RTC_HM);
	_outb(RTC_ADDR, RTC_C);
	(void)_inb(RTC_DATA);
}

void set_platform_defaults(void)
{
	tod_module_name = "todpc164lx";
}

static struct modlmisc modlmisc = {
	&mod_miscops, "platmod"
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};

int
_init(void)
{
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
