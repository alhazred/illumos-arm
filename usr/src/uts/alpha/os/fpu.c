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
 * Copyright (c) 1992, 2011, Oracle and/or its affiliates. All rights reserved.
 */

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc. */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T   */
/*		All Rights Reserved				*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation		*/
/*		All Rights Reserved				*/

/*
 * Copyright (c) 2009, Intel Corporation.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/regset.h>
#include <sys/privregs.h>
#include <sys/psw.h>
#include <sys/trap.h>
#include <sys/fault.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/pcb.h>
#include <sys/lwp.h>
#include <sys/cpuvar.h>
#include <sys/thread.h>
#include <sys/disp.h>
#include <sys/siginfo.h>
#include <sys/archsystm.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/fp.h>

static void
fp_new_lwp(kthread_id_t t, kthread_id_t ct)
{
	pcb_t *pcb = &ttolwp(t)->lwp_pcb;
	pcb_t *cpcb = &ttolwp(ct)->lwp_pcb;
	struct fpu_ctx *fp = &pcb->pcb_fpu;
	struct fpu_ctx *cfp = &cpcb->pcb_fpu;

	if (t == curthread) {
		fp_save(pcb);
	}
	bcopy(fp, cfp, sizeof (*cfp));
	cpcb->pcb_hw.flags |= PCB_HW_FLAGS_FEN;
	installctx(ct, cpcb, fp_save, fp_restore, fp_new_lwp, fp_new_lwp, NULL, NULL);
}

int
fp_fenflt(void)
{
	cmn_err(CE_PANIC, "%s: called\n", __func__);
	return (0);
}

void fp_init(void)
{
	pcb_t *pcb = &ttolwp(curthread)->lwp_pcb;
	bzero(&pcb->pcb_fpu.fpu_regs, sizeof(pcb->pcb_fpu.fpu_regs));
	pcb->pcb_fpu.fpu_regs.kfpu_cr = FPCR_INIT;
	pcb->pcb_hw.flags |= PCB_HW_FLAGS_FEN;
	pal_wrfen(1);
	fp_restore(pcb);

	installctx(curthread, pcb, fp_save, fp_restore, fp_new_lwp, fp_new_lwp, NULL, NULL);
}
