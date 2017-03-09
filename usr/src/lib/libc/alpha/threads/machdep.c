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
 * Copyright (c) 2004, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2017 Hayashi Naoyuki
 */

#include "thr_uberdata.h"
#include <procfs.h>
#include <ucontext.h>
#include <setjmp.h>

extern int getlwpstatus(thread_t, lwpstatus_t *);
extern int putlwpregs(thread_t, prgregset_t);

/* ARGSUSED2 */
void *
setup_top_frame(void *stk, size_t stksize, ulwp_t *ulwp)
{
	uint64_t *stack;

	/*
	 * Top-of-stack must be rounded down to STACK_ALIGN
	 */
	stack = (uint64_t *)(((uintptr_t)stk + stksize) & ~(STACK_ALIGN-1));

	return (stack);
}

int
setup_context(ucontext_t *ucp, void *(*func)(ulwp_t *),
	ulwp_t *ulwp, caddr_t stk, size_t stksize)
{
	uint64_t *stack;

	/* clear the context */
	(void) memset(ucp, 0, sizeof (*ucp));

	/*
	 * Setup the top stack frame.
	 * If this fails, pass the problem up to the application.
	 */
	if ((stack = setup_top_frame(stk, stksize, ulwp)) == NULL)
		return (ENOMEM);

	/* fill in registers of interest */
	ucp->uc_flags |= UC_CPU;
	ucp->uc_mcontext.gregs[REG_PC] = (greg_t)func;
	ucp->uc_mcontext.gregs[REG_PV] = (greg_t)func;
	ucp->uc_mcontext.gregs[REG_SP] = (greg_t)stack;
	ucp->uc_mcontext.gregs[REG_RA] = (greg_t)_lwp_start;
	ucp->uc_mcontext.gregs[REG_A0] = (greg_t)ulwp;
	ucp->uc_mcontext.gregs[REG_UQ] = (greg_t)ulwp;

	return (0);
}

/*
 * Machine-dependent startup code for a newly-created thread.
 */
void *
_thrp_setup(ulwp_t *self)
{
	self->ul_ustack.ss_sp = (void *)(self->ul_stktop - self->ul_stksiz);
	self->ul_ustack.ss_size = self->ul_stksiz;
	self->ul_ustack.ss_flags = 0;
	(void) setustack(&self->ul_ustack);

	update_sched(self);
	tls_setup();

	/* signals have been deferred until now */
	sigon(self);

	if (self->ul_cancel_pending == 2 && !self->ul_cancel_disabled)
		return (NULL);	/* cancelled by pthread_create() */
	return (self->ul_startpc(self->ul_startarg));
}

extern void _getfcr(uint64_t *);
void
_fpinherit(ulwp_t *ulwp)
{
	_getfcr(&ulwp->ul_fpuenv.fctrl);
}

void
getgregs(ulwp_t *ulwp, gregset_t rs)
{
	lwpstatus_t status;

	if (getlwpstatus(ulwp->ul_lwpid, &status) == 0) {
		rs[REG_S0] = status.pr_reg[REG_S0];
		rs[REG_S1] = status.pr_reg[REG_S1];
		rs[REG_S2] = status.pr_reg[REG_S2];
		rs[REG_S3] = status.pr_reg[REG_S3];
		rs[REG_S4] = status.pr_reg[REG_S4];
		rs[REG_S5] = status.pr_reg[REG_S5];
		rs[REG_S6] = status.pr_reg[REG_S6];
		rs[REG_SP] = status.pr_reg[REG_SP];
		rs[REG_RA] = status.pr_reg[REG_RA];
		rs[REG_PC] = status.pr_reg[REG_PC];
	} else {
		rs[REG_S0] = 0;
		rs[REG_S1] = 0;
		rs[REG_S2] = 0;
		rs[REG_S3] = 0;
		rs[REG_S4] = 0;
		rs[REG_S5] = 0;
		rs[REG_S6] = 0;
		rs[REG_SP] = 0;
		rs[REG_RA] = 0;
		rs[REG_PC] = 0;
	}
}

void
setgregs(ulwp_t *ulwp, gregset_t rs)
{
	lwpstatus_t status;

	if (getlwpstatus(ulwp->ul_lwpid, &status) == 0) {
		status.pr_reg[REG_S0] = rs[REG_S0];
		status.pr_reg[REG_S1] = rs[REG_S1];
		status.pr_reg[REG_S2] = rs[REG_S2];
		status.pr_reg[REG_S3] = rs[REG_S3];
		status.pr_reg[REG_S4] = rs[REG_S4];
		status.pr_reg[REG_S5] = rs[REG_S5];
		status.pr_reg[REG_S6] = rs[REG_S6];
		status.pr_reg[REG_SP] = rs[REG_SP];
		status.pr_reg[REG_RA] = rs[REG_RA];
		status.pr_reg[REG_PC] = rs[REG_PC];
		putlwpregs(ulwp->ul_lwpid, status.pr_reg);
	}
}

int
__csigsetjmp(sigjmp_buf env, int savemask, greg_t sp,
    greg_t s0, greg_t s1, greg_t s2, greg_t s3,
    greg_t s4, greg_t s5, greg_t s6, greg_t ra)
{
	ucontext_t *ucp = (ucontext_t *)env;
	ulwp_t *self = curthread;
	getcontext(ucp);

	ucp->uc_link = self->ul_siglink;
	if (self->ul_ustack.ss_flags & SS_ONSTACK)
		ucp->uc_stack = self->ul_ustack;
	else {
		ucp->uc_stack.ss_sp =
		    (void *)(self->ul_stktop - self->ul_stksiz);
		ucp->uc_stack.ss_size = self->ul_stksiz;
		ucp->uc_stack.ss_flags = 0;
	}
	ucp->uc_flags = UC_STACK | UC_MCONTEXT;
	if (savemask) {
		ucp->uc_flags |= UC_SIGMASK;
		enter_critical(self);
		ucp->uc_sigmask = self->ul_sigmask;
		exit_critical(self);
	}
	ucp->uc_mcontext.gregs[REG_SP] = sp;
	ucp->uc_mcontext.gregs[REG_S0] = s0;
	ucp->uc_mcontext.gregs[REG_S1] = s1;
	ucp->uc_mcontext.gregs[REG_S2] = s2;
	ucp->uc_mcontext.gregs[REG_S3] = s3;
	ucp->uc_mcontext.gregs[REG_S4] = s4;
	ucp->uc_mcontext.gregs[REG_S5] = s5;
	ucp->uc_mcontext.gregs[REG_S6] = s6;
	ucp->uc_mcontext.gregs[REG_RA] = ra;
	ucp->uc_mcontext.gregs[REG_PC] = ra;
	return (0);
}

void
smt_pause(void)
{
}
