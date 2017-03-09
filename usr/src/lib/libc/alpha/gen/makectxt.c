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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma weak _makecontext = makecontext

#include <stdarg.h>
#include <ucontext.h>
#include <sys/stack.h>
#include <unistd.h>

extern void __resumecontext(void);

void
makecontext(ucontext_t *ucp, void (*func)(), int argc, ...)
{
	long *sp;
	va_list ap;
	size_t size;
	int pusharg = (argc > 6 ? argc - 6 : 0);
	greg_t tmp;
	int i;

	ucp->uc_mcontext.gregs[REG_PC] =
	    ucp->uc_mcontext.gregs[REG_T12] = (greg_t)func;

	size = sizeof (long) * (pusharg + 1);

	sp = (long *)(((uintptr_t)ucp->uc_stack.ss_sp +
	    ucp->uc_stack.ss_size - size) & ~(STACK_ENTRY_ALIGN - 1));

	if (((uintptr_t)sp & (STACK_ALIGN - 1ul)) == 0)
		sp -= STACK_ENTRY_ALIGN / sizeof (*sp);

	ucp->uc_mcontext.gregs[REG_SP] = (greg_t)sp;

	va_start(ap, argc);

	for (i = 0; i < argc; i++) {
		tmp = va_arg(ap, long);
		switch (i) {
		case 0:
			ucp->uc_mcontext.gregs[REG_A0] = tmp;
			break;
		case 1:
			ucp->uc_mcontext.gregs[REG_A1] = tmp;
			break;
		case 2:
			ucp->uc_mcontext.gregs[REG_A2] = tmp;
			break;
		case 3:
			ucp->uc_mcontext.gregs[REG_A3] = tmp;
			break;
		case 4:
			ucp->uc_mcontext.gregs[REG_A4] = tmp;
			break;
		case 5:
			ucp->uc_mcontext.gregs[REG_A5] = tmp;
			break;
		default:
			*sp++ = tmp;
			break;
		}
	}

	va_end(ap);

	ucp->uc_mcontext.gregs[REG_RA] = (long)__resumecontext;
}
