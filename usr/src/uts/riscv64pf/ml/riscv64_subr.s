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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sys/errno.h>
#include "assym.h"

#define THREADP(reg)			\
	   mrs reg, tpidr_el1

/*
 * int setjmp(label_t *lp)
 * void longjmp(label_t *lp)
 *
 * Setjmp and longjmp implement non-local gotos using state vectors
 * type label_t.
 */
	ENTRY(setjmp)
	sd	ra, 8*LABEL_REG_PC(a0)
	sd	sp, 8*LABEL_REG_SP(a0)
	sd	s0, 8*LABEL_REG_S0(a0)
	sd	s1, 8*LABEL_REG_S1(a0)
	sd	s2, 8*LABEL_REG_S2(a0)
	sd	s3, 8*LABEL_REG_S3(a0)
	sd	s4, 8*LABEL_REG_S4(a0)
	sd	s5, 8*LABEL_REG_S5(a0)
	sd	s6, 8*LABEL_REG_S6(a0)
	sd	s7, 8*LABEL_REG_S7(a0)
	sd	s8, 8*LABEL_REG_S8(a0)
	sd	s9, 8*LABEL_REG_S9(a0)
	sd	s10, 8*LABEL_REG_S10(a0)
	sd	s11, 8*LABEL_REG_S11(a0)
	li	a0, 0
	ret
	SET_SIZE(setjmp)

	ENTRY(longjmp)
	ld	ra, 8*LABEL_REG_PC(a0)
	ld	sp, 8*LABEL_REG_SP(a0)
	ld	s0, 8*LABEL_REG_S0(a0)
	ld	s1, 8*LABEL_REG_S1(a0)
	ld	s2, 8*LABEL_REG_S2(a0)
	ld	s3, 8*LABEL_REG_S3(a0)
	ld	s4, 8*LABEL_REG_S4(a0)
	ld	s5, 8*LABEL_REG_S5(a0)
	ld	s6, 8*LABEL_REG_S6(a0)
	ld	s7, 8*LABEL_REG_S7(a0)
	ld	s8, 8*LABEL_REG_S8(a0)
	ld	s9, 8*LABEL_REG_S9(a0)
	ld	s10, 8*LABEL_REG_S10(a0)
	ld	s11, 8*LABEL_REG_S11(a0)
	li	a0, 1
	ret
	SET_SIZE(longjmp)

/*
 * int on_fault(label_t *ljb)
 * void no_fault(void)
 *
 * Catch lofault faults. Like setjmp except it returns one
 * if code following causes uncorrectable fault. Turned off
 * by calling no_fault().
 */
	ENTRY(on_fault)
	fence
	la	a2, .Lcatch_fault
	sd	a0, T_ONFAULT(tp)
	sd	a2, T_LOFAULT(tp)
	j	setjmp
.Lcatch_fault:
	ld	a0, T_ONFAULT(tp)
	fence
	sd	zero, T_ONFAULT(tp)
	sd	zero, T_LOFAULT(tp)
	j	longjmp
	SET_SIZE(on_fault)

	ENTRY(no_fault)
	fence
	sd	zero, T_ONFAULT(tp)
	sd	zero, T_LOFAULT(tp)
	ret
	SET_SIZE(no_fault)

/*
 * void on_trap_trampoline(void)
 * int on_trap(on_trap_data_t *otp, uint_t prot)
 *
 * Default trampoline code for on_trap() (see <sys/ontrap.h>).  We just
 * do a longjmp(&curthread->t_ontrap->ot_jmpbuf) if this is ever called.
 */
	ENTRY(on_trap_trampoline)
	ld	a0, T_ONTRAP(tp)
	addi	a0, a0, OT_JMPBUF
	j	longjmp
	SET_SIZE(on_trap_trampoline)


	ENTRY(on_trap)
	fence
	sh	a1, OT_PROT(a0)		/* otp->ot_prot = prot */
	sh	zero, OT_TRAP(a0)	/* otp->ot_trap = 0 */
	la	a1, on_trap_trampoline	/* r1 = &on_trap_trampoline */
	sd	a1, OT_TRAMPOLINE(a0)	/* otp->ot_trampoline = r1 */
	sd	zero, OT_HANDLE(a0)	/* otp->ot_handle = NULL */
	sd	zero, OT_PAD1(a0)	/* otp->ot_pad1 = NULL */

	ld	a2, T_ONTRAP(tp)
	beq	a2, a0, 1f
	sd	a2, OT_PREV(a0)		/*   otp->ot_prev = r1->t_ontrap */
	sd	a0, T_ONTRAP(tp)	/*   r1->t_ontrap = otp */
	fence
1:	addi	a0, a0, OT_JMPBUF
	j	setjmp
	SET_SIZE(on_trap)

/*
 * greg_t getfp(void)
 * return the current frame pointer
 */

	ENTRY(getfp)
	mv	a0, fp
	ret
	SET_SIZE(getfp)


	ENTRY(_insque)
	ld	t0, (0 * 8)(a1)	// predp->forw
	sd	a1, (1 * 8)(a0)	// entryp->back = predp
	sd	t0, (0 * 8)(a0)	// entryp->forw = predp->forw
	sd	a0, (0 * 8)(a1)	// predp->forw = entryp
	sd	a0, (1 * 8)(t0)	// predp->forw->back = entryp
	ret
	SET_SIZE(_insque)

	ENTRY(_remque)
	ld	t0, (0 * 8)(a0)	// entryp->forw
	ld	t1, (1 * 8)(a0)	// entryp->back
	sd	t0, (0 * 8)(t1)	// entryp->back->forw = entryp->forw
	sd	t1, (1 * 8)(t0)	// entryp->forw->back = entryp->back
	ret
	SET_SIZE(_remque)

	/*
	 * dtrace_icookie_t
	 * dtrace_interrupt_disable(void)
	 */
	ENTRY(dtrace_interrupt_disable)
	csrrci	a0, sstatus, SSR_SIE
	andi	a0, a0, SSR_SIE
	ret
	SET_SIZE(dtrace_interrupt_disable)

	/*
	 * void
	 * dtrace_interrupt_enable(dtrace_icookie_t cookie)
	 */
	ENTRY(dtrace_interrupt_enable)
	andi	a0, a0, SSR_SIE
	beqz	a0, 1f
	csrs	sstatus, a0
1:	ret
	SET_SIZE(dtrace_interrupt_enable)

	ENTRY(dtrace_membar_consumer)
	fence
	ret
	SET_SIZE(dtrace_membar_consumer)

	ENTRY(dtrace_membar_producer)
	fence
	ret
	SET_SIZE(dtrace_membar_producer)

	ENTRY_NP(return_instr)
	ret
	SET_SIZE(return_instr)

	ENTRY_NP(ftrace_interrupt_disable)
	csrrci	a0, sstatus, SSR_SIE
	andi	a0, a0, SSR_SIE
	ret
	SET_SIZE(ftrace_interrupt_disable)

	ENTRY_NP(ftrace_interrupt_enable)
	andi	a0, a0, SSR_SIE
	beqz	a0, 1f
	csrs	sstatus, a0
1:	ret
	SET_SIZE(ftrace_interrupt_enable)


	ENTRY(prefetch_page_w)
	ret
        SET_SIZE(prefetch_page_w)

	ENTRY(prefetch_page_r)
	ret
        SET_SIZE(prefetch_page_r)

	ENTRY(prefetch_smap_w)
	ret
        SET_SIZE(prefetch_smap_w)

