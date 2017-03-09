
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

#define	PSMI_1_7
#include <sys/smp_impldefs.h>
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
uint_t cp_haltset_fanout = 0;
int gethrtime_hires = 0;
int tsc_gethrtime_initted = 0;
uint_t	cpu_freq;
uint64_t cpu_freq_hz;
extern void return_instr();
static int mach_softlvl_to_vect(int ipl);
/*
 *	Local function prototypes
 */
static int mp_disable_intr(processorid_t cpun);
static void mp_enable_intr(processorid_t cpun);
static hrtime_t dummy_hrtime(void);
static void dummy_scalehrtime(hrtime_t *);
static uint64_t dummy_unscalehrtime(hrtime_t);
static int mach_translate_irq(dev_info_t *dip, int irqno);

extern int cpuid_get_coreid(cpu_t *);
extern int cpuid_get_chipid(cpu_t *);

/*
 *	PSM functions initialization
 */
void (*psm_shutdownf)(int, int)	= (void (*)(int, int))return_instr;
void (*psm_preshutdownf)(int, int) = (void (*)(int, int))return_instr;
void (*psm_notifyf)(int)	= (void (*)(int))return_instr;
void (*psm_set_idle_cpuf)(int)	= (void (*)(int))return_instr;
void (*psm_unset_idle_cpuf)(int) = (void (*)(int))return_instr;
void (*picinitf)() 		= return_instr;
int (*clkinitf)(int, int *) 	= (int (*)(int, int *))return_instr;
int (*ap_mlsetup)() 		= (int (*)(void))return_instr;
void (*send_dirintf)() 		= return_instr;
void (*setspl)(int)		= (void (*)(int))return_instr;
int (*addspl)(int, int, int, int) = (int (*)(int, int, int, int))return_instr;
int (*delspl)(int, int, int, int) = (int (*)(int, int, int, int))return_instr;
int (*get_pending_spl)(void)	= (int (*)(void))return_instr;
int (*addintr)(void *, int, avfunc, char *, int, caddr_t, caddr_t,
    uint64_t *, dev_info_t *) = NULL;
void (*remintr)(void *, int, avfunc, int) = NULL;
void (*kdisetsoftint)(int, struct av_softinfo *)=
	(void (*)(int, struct av_softinfo *))return_instr;
void (*setsoftint)(int, struct av_softinfo *)=
	(void (*)(int, struct av_softinfo *))return_instr;
int (*slvltovect)(int)		= (int (*)(int))return_instr;
int (*setlvl)(int, int)	= (int (*)(int, int))return_instr;
void (*setlvlx)(int, int)	= (void (*)(int, int))return_instr;
int (*psm_disable_intr)(int)	= mp_disable_intr;
void (*psm_enable_intr)(int)	= mp_enable_intr;
hrtime_t (*gethrtimef)(void)	= dummy_hrtime;
hrtime_t (*gethrtimeunscaledf)(void)	= dummy_hrtime;
void (*scalehrtimef)(hrtime_t *)	= dummy_scalehrtime;
uint64_t (*unscalehrtimef)(hrtime_t)	= dummy_unscalehrtime;
int (*psm_translate_irq)(dev_info_t *, int) = mach_translate_irq;
void (*gethrestimef)(timestruc_t *) = pc_gethrestime;
void (*psm_notify_error)(int, char *) = (void (*)(int, char *))NULL;
int (*psm_get_clockirq)(int) = NULL;
int (*psm_get_ipivect)(int, int) = NULL;
uchar_t (*psm_get_ioapicid)(uchar_t) = NULL;
uint32_t (*psm_get_localapicid)(uint32_t) = NULL;
uchar_t (*psm_xlate_vector_by_irq)(uchar_t) = NULL;

int (*psm_clkinit)(int) = NULL;
void (*psm_timer_reprogram)(hrtime_t) = NULL;
void (*psm_timer_enable)(void) = NULL;
void (*psm_timer_disable)(void) = NULL;
void (*psm_post_cyclic_setup)(void *arg) = NULL;

void (*notify_error)(int, char *) = (void (*)(int, char *))return_instr;
void (*hrtime_tick)(void)	= return_instr;

int
pg_plat_hw_shared(cpu_t *cp, pghw_type_t hw)
{
	return (0);
}

int
pg_plat_cpus_share(cpu_t *cpu_a, cpu_t *cpu_b, pghw_type_t hw)
{
	switch (hw) {
	case PGHW_IPIPE:
	case PGHW_CHIP:
		return (pg_plat_hw_instance_id(cpu_a, hw) ==
		    pg_plat_hw_instance_id(cpu_b, hw));
	}
	return (0);
}

id_t
pg_plat_hw_instance_id(cpu_t *cpu, pghw_type_t hw)
{
	switch (hw) {
	case PGHW_IPIPE:
	case PGHW_CHIP:
	case PGHW_CACHE:
		return (cpu->cpu_id);
	}
	return (-1);
}

/*
 * Rank the relative importance of optimizing for hw1 or hw2
 */
pghw_type_t
pg_plat_hw_rank(pghw_type_t hw1, pghw_type_t hw2)
{
	int i;
	int rank1 = 0;
	int rank2 = 0;

	static pghw_type_t hw_hier[] = {
		PGHW_IPIPE,
		PGHW_CHIP,
		PGHW_CACHE,
		PGHW_NUM_COMPONENTS
	};

	for (i = 0; hw_hier[i] != PGHW_NUM_COMPONENTS; i++) {
		if (hw_hier[i] == hw1)
			rank1 = i;
		if (hw_hier[i] == hw2)
			rank2 = i;
	}

	if (rank1 > rank2)
		return (hw1);
	else
		return (hw2);
}

/*
 * Override the default CMT dispatcher policy for the specified
 * hardware sharing relationship
 */
/* ARGSUSED */
pg_cmt_policy_t
pg_plat_cmt_policy(pghw_type_t hw)
{
	/* Accept the default polices */
	return (CMT_NO_POLICY);
}

id_t
pg_plat_get_core_id(cpu_t *cp)
{
	return (pg_plat_hw_instance_id(cp, PGHW_IPIPE));
}

/*
 * Return number of counter events requested to measure hardware capacity and
 * utilization and setup CPC requests for specified CPU as needed
 *
 * May return 0 when platform or processor specific code knows that no CPC
 * events should be programmed on this CPU or -1 when platform or processor
 * specific code doesn't know which counter events are best to use and common
 * code should decide for itself
 */
int
cu_plat_cpc_init(cpu_t *cp, kcpc_request_list_t *reqs, int nreqs)
{
	const char	*impl_name;

	/*
	 * Return error if pcbe_ops not set
	 */
	if (pcbe_ops == NULL)
		return (-1);

	return (0);
}

static int
mp_disable_intr(int cpun)
{
	/*
	 * switch to the offline cpu
	 */
	affinity_set(cpun);
	/*
	 * raise ipl to just below cross call
	 */
	splx(XC_SYS_PIL - 1);
	/*
	 *	set base spl to prevent the next swtch to idle from
	 *	lowering back to ipl 0
	 */
	CPU->cpu_intr_actv |= (1 << (XC_SYS_PIL - 1));
	set_base_spl();
	affinity_clear();
	return (DDI_SUCCESS);
}

static void
mp_enable_intr(int cpun)
{
	/*
	 * switch to the online cpu
	 */
	affinity_set(cpun);
	/*
	 * clear the interrupt active mask
	 */
	CPU->cpu_intr_actv &= ~(1 << (XC_SYS_PIL - 1));
	set_base_spl();
	(void) spl0();
	affinity_clear();
}
/*
 * Routine to ensure initial callers to hrtime gets 0 as return
 */
static hrtime_t
dummy_hrtime(void)
{
	return (0);
}

/* ARGSUSED */
static void
dummy_scalehrtime(hrtime_t *ticks)
{}

static uint64_t
dummy_unscalehrtime(hrtime_t nsecs)
{
	return ((uint64_t)nsecs);
}
/*ARGSUSED*/
static int
mach_translate_irq(dev_info_t *dip, int irqno)
{
	return (irqno);	/* default to NO translation */
}

int
mach_cpu_start(struct cpu *cp, void *ctx)
{
	return 0;
}
/*
 * Create cpu device node in device tree and online it.
 * Return created dip with reference count held if requested.
 */
int
mach_cpu_create_device_node(struct cpu *cp, dev_info_t **dipp)
{
	return 0;
}

void
cmp_set_nosteal_interval(void)
{
	/* Set the nosteal interval (used by disp_getbest()) to 100us */
	nosteal_nsec = 100000UL;
}

