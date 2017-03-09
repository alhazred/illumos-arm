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
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
 */
/*
 * Copyright (c) 2010, Intel Corporation.
 * All rights reserved.
 */

/*
 * Welcome to the world of the "real mode platter".
 * See also startup.c, mpcore.s and apic.c for related routines.
 */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/cpuvar.h>
#include <sys/cpu_module.h>
#include <sys/kmem.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/controlregs.h>
#include <sys/smp_impldefs.h>
#include <sys/sysmacros.h>
#include <sys/promif.h>
#include <sys/cpu.h>
#include <sys/cpu_event.h>
#include <sys/sunndi.h>
#include <sys/fs/dv_node.h>
#include <vm/hat_alpha.h>
#include <vm/as.h>

extern cpuset_t cpu_ready_set;

extern int  mp_start_cpu_common(cpu_t *cp, boolean_t boot);
extern void *(*cpu_pause_func)(void *);

/*
 * Fill up the real mode platter to make it easy for real mode code to
 * kick it off. This area should really be one passed by boot to kernel
 * and guaranteed to be below 1MB and aligned to 16 bytes. Should also
 * have identical physical and virtual address in paged mode.
 */
static ushort_t *warm_reset_vector = NULL;

int
mach_cpucontext_init(void)
{
	return (0);
}

void
mach_cpucontext_fini(void)
{
}

void *
mach_cpucontext_xalloc(struct cpu *cp, int optype)
{
	return (NULL);
}

void
mach_cpucontext_xfree(struct cpu *cp, void *arg, int err, int optype)
{
}

void *
mach_cpucontext_alloc(struct cpu *cp)
{
	return NULL;
}

void
mach_cpucontext_free(struct cpu *cp, void *arg, int err)
{
}

/*
 * "Enter monitor."  Called via cross-call from stop_other_cpus().
 */
void
mach_cpu_halt(char *msg)
{
	if (msg)
		prom_printf("%s\n", msg);

	/*CONSTANTCONDITION*/
	while (1)
		;
}

void
mach_cpu_idle(void)
{
}

void
mach_cpu_pause(volatile char *safe)
{
}

/*
 * Power on the target CPU.
 */
int
mp_cpu_poweron(struct cpu *cp)
{
	return (ENOTSUP);
}

#define	MP_CPU_DETACH_MAX_TRIES		5
#define	MP_CPU_DETACH_DELAY		100

static int
mp_cpu_detach_driver(dev_info_t *dip)
{
	return 0;
}

/*
 * Power off the target CPU.
 * Note: cpu_lock will be released and then reacquired.
 */
int
mp_cpu_poweroff(struct cpu *cp)
{
	return (ENOTSUP);
}
