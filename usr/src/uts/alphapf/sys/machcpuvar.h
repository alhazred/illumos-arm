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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_MACHCPUVAR_H
#define	_SYS_MACHCPUVAR_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/inttypes.h>
#include <sys/x_call.h>
#include <sys/avintr.h>
#include <sys/pte.h>

#ifndef	_ASM

/*
 * Machine specific fields of the cpu struct
 * defined in common/sys/cpuvar.h.
 *
 * Note:  This is kinda kludgy but seems to be the best
 * of our alternatives.
 */
typedef void *cpu_pri_lev_t;

struct cpuid_info;
struct cpu_ucode_info;
struct cmi_hdl;

/*
 * A note about the hypervisor affinity bits: a one bit in the affinity mask
 * means the corresponding event channel is allowed to be serviced
 * by this cpu.
 */
struct	machcpu {
	/*
	 * x_call fields - used for interprocessor cross calls
	 */
	struct xc_msg	*xc_msgbox;
	struct xc_msg	*xc_free;
	xc_data_t	xc_data;
	uint32_t	xc_wait_cnt;
	volatile uint32_t xc_work_cnt;
	uint32_t	xc_ipis;

	int		mcpu_nodeid;		/* node-id */
	int		mcpu_pri;		/* CPU priority */
	cpu_pri_lev_t	mcpu_pri_data;		/* ptr to machine dependent */
						/* data for setting priority */
						/* level */
	kmutex_t	mcpu_ppaddr_mutex;
	struct softint	mcpu_softinfo;
	uint64_t	pil_high_start[HIGH_LEVELS];
	uint64_t	intrstat[PIL_MAX + 1][2];

	struct cpuid_info	 *mcpu_cpi;

	volatile uint32_t *mcpu_mwait;	/* MONITOR/MWAIT buffer */
	uint16_t max_cstates;		/* supported max cstates */

	struct cpu_ucode_info	*mcpu_ucode_info;

	void			*mcpu_pm_mach_state;
	struct cmi_hdl		*mcpu_cmi_hdl;
	void			*mcpu_mach_ctx_ptr;

	volatile uint32_t	mcpu_istamp;
	struct hat	*mcpu_current_hat;
	uint32_t	mcpu_asid;
	uint64_t	mcpu_asid_gen;
};

#define	NINTR_THREADS	(LOCK_LEVEL-1)	/* number of interrupt threads */
#define	MWAIT_HALTED	(1)		/* mcpu_mwait set when halting */
#define	MWAIT_RUNNING	(0)		/* mcpu_mwait set to wakeup */
#define	MWAIT_WAKEUP_IPI	(2)	/* need IPI to wakeup */
#define	MWAIT_WAKEUP(cpu)	(*((cpu)->cpu_m.mcpu_mwait) = MWAIT_RUNNING)

#endif	/* _ASM */

/* Please DON'T add any more of this namespace-poisoning sewage here */

#define	cpu_nodeid cpu_m.mcpu_nodeid
#define	cpu_pri cpu_m.mcpu_pri
#define	cpu_pri_data cpu_m.mcpu_pri_data
#define	cpu_current_hat cpu_m.mcpu_current_hat
#define	cpu_hat_info cpu_m.mcpu_hat_info
#define	cpu_ppaddr_mutex cpu_m.mcpu_ppaddr_mutex
#define	cpu_softinfo cpu_m.mcpu_softinfo
#define	cpu_asid cpu_m.mcpu_asid
#define	cpu_asid_gen cpu_m.mcpu_asid_gen
#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_MACHCPUVAR_H */
