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
#include <sys/pal.h>
#include <sys/errno.h>
#include "assym.h"


/*
 * int setjmp(label_t *lp)
 * void longjmp(label_t *lp)
 *
 * Setjmp and longjmp implement non-local gotos using state vectors
 * type label_t.
 */
	ENTRY(setjmp)
	stq	s0, (LABEL_REG_S0*8)(a0)
	stq	s1, (LABEL_REG_S1*8)(a0)
	stq	s2, (LABEL_REG_S2*8)(a0)
	stq	s3, (LABEL_REG_S3*8)(a0)
	stq	s4, (LABEL_REG_S4*8)(a0)
	stq	s5, (LABEL_REG_S5*8)(a0)
	stq	s6, (LABEL_REG_S6*8)(a0)
	stq	ra, (LABEL_REG_PC*8)(a0)
	stq	sp, (LABEL_REG_SP*8)(a0)
	mov	zero, v0
	ret
	SET_SIZE(setjmp)

	ENTRY(longjmp)
	ldq	s0, (LABEL_REG_S0*8)(a0)
	ldq	s1, (LABEL_REG_S1*8)(a0)
	ldq	s2, (LABEL_REG_S2*8)(a0)
	ldq	s3, (LABEL_REG_S3*8)(a0)
	ldq	s4, (LABEL_REG_S4*8)(a0)
	ldq	s5, (LABEL_REG_S5*8)(a0)
	ldq	s6, (LABEL_REG_S6*8)(a0)
	ldq	ra, (LABEL_REG_PC*8)(a0)
	ldq	sp, (LABEL_REG_SP*8)(a0)
	ldiq	v0, 1
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
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)	// v0 <- curthread()
	br	t0, Lon_fault
	.align 4
Lcatch_fault:
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)	// v0 <- curthread()
	ldq	a0, T_ONFAULT(v0)
	mb
	stq	zero, T_LOFAULT(v0)
	stq	zero, T_ONFAULT(v0)
	br	zero, longjmp
	.align 4
Lon_fault:
	stq	t0, T_LOFAULT(v0)
	stq	a0, T_ONFAULT(v0)
	br	zero, setjmp
	SET_SIZE(on_fault)

	ENTRY(no_fault)
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)	// v0 <- curthread()
	mb
	stq	zero, T_LOFAULT(v0)
	stq	zero, T_ONFAULT(v0)
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
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)	// v0 <- curthread()
	ldq	a0, T_ONTRAP(v0)
	addq	a0, OT_JMPBUF, a0
	br	zero, longjmp
	SET_SIZE(on_trap_trampoline)


	ENTRY(on_trap)
	LDGP(pv)
	mb
	stw	a1, OT_PROT(a0)
	stw	zero, OT_TRAP(a0)
	lda	a1, on_trap_trampoline
	stq	a1, OT_TRAMPOLINE(a0)
	stq	zero, OT_HANDLE(a0)
	stq	zero, OT_PAD1(a0)
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)	// v0 <- curthread()
	ldq	a2, T_ONTRAP(v0)
	cmpeq	a0, a2, t0
	bne	t0, 0f
	stq	a2, OT_PREV(a0)
	stq	a0, T_ONTRAP(v0)
	mb
0:	addq	a0, OT_JMPBUF, a0
	br	zero, setjmp
	SET_SIZE(on_trap)


/*
 * greg_t getfp(void)
 * return the current frame pointer
 * Alphaでは常にfpが設定されるとは限らないので、spを使用する。
 */

	ENTRY(getfp)
	mov	sp, v0
	ret
	SET_SIZE(getfp)


#define	SETPRI(level) \
	mov	level, a0;	\
	lda	pv, do_splx;	\
	jmp	zero, (pv)

#define	RAISE(level) \
	mov	level, a0;	\
	lda	pv, splr;	\
	jmp	zero, (pv)

	ENTRY(spl8)
	LDGP(pv)
	SETPRI(15)
	SET_SIZE(spl8)

	ENTRY(spl7)
	LDGP(pv)
	RAISE(13)
	SET_SIZE(spl7)

	ENTRY(splzs)
	LDGP(pv)
	SETPRI(12)
	SET_SIZE(splzs)

	ENTRY(splhi)
	ALTENTRY(splhigh)
	ALTENTRY(spl6)
	ALTENTRY(i_ddi_splhigh)
	LDGP(pv)
	RAISE(DISP_LEVEL)
	SET_SIZE(i_ddi_splhigh)
	SET_SIZE(spl6)
	SET_SIZE(splhigh)
	SET_SIZE(splhi)

	ENTRY(spl0)
	LDGP(pv)
	SETPRI(0)
	SET_SIZE(spl0)

	ENTRY(splx)
	LDGP(pv)
	lda	pv, do_splx
	jmp	zero, (pv)
	SET_SIZE(splx)


	ENTRY(_insque)
	ldq	t0, 0(a1)	// predp->forw
	stq	a1, 8(a0)	// entryp->back = predp
	stq	t0, 0(a0)	// entryp->forw = predp->forw
	stq	a0, 0(a1)	// predp->forw = entryp
	stq	a0, 8(t0)	// predp->forw->back = entryp
	ret
	SET_SIZE(_insque)

	ENTRY(_remque)
	ldq	t0, 0(a0)	// entryp->forw
	ldq	t1, 8(a0)	// entryp->back
	stq	t0, 0(t1)	// entryp->back->forw = entryp->forw
	stq	t1, 8(t0)	// entryp->forw->back = entryp->back
	ret
	SET_SIZE(_remque)

	/*
	 * dtrace_icookie_t
	 * dtrace_interrupt_disable(void)
	 */
	ENTRY(dtrace_interrupt_disable)
	mov	0x6, a0
	call_pal PAL_swpipl
	ret
	SET_SIZE(dtrace_interrupt_disable)

	/*
	 * void
	 * dtrace_interrupt_enable(dtrace_icookie_t cookie)
	 */
	ENTRY(dtrace_interrupt_enable)
	call_pal PAL_swpipl
	ret
	SET_SIZE(dtrace_interrupt_enable)

	ENTRY(dtrace_membar_consumer)
	mb
	ret
	SET_SIZE(dtrace_membar_consumer)

	ENTRY(dtrace_membar_producer)
	mb
	ret
	SET_SIZE(dtrace_membar_producer)

	ENTRY(switch_sp_and_call)
	mov	sp, t0
	mov	a0, sp
	lda	sp, -2*8(sp)
	stq	ra, 0*8(sp)
	stq	t0, 1*8(sp)
	mov	a1, pv
	mov	a2, a0
	mov	a3, a1
	jsr	ra, (pv)
	ldq	ra, 0*8(sp)
	ldq	sp, 1*8(sp)
	ret
	SET_SIZE(switch_sp_and_call)


	ENTRY_NP(return_instr)
	ret
	SET_SIZE(return_instr)


	ENTRY_NP(ftrace_interrupt_disable)
	mov	6, a0
	call_pal PAL_swpipl
	ret
	SET_SIZE(ftrace_interrupt_disable)

	ENTRY_NP(ftrace_interrupt_enable)
	call_pal PAL_swpipl
	ret
	SET_SIZE(ftrace_interrupt_enable)

	ENTRY_NP(pal_clrfen)
	call_pal PAL_clrfen
	ret
	SET_SIZE(pal_clrfen)

	ENTRY_NP(pal_imb)
	call_pal PAL_imb
	ret
	SET_SIZE(pal_imb)

	ENTRY_NP(pal_bpt)
	call_pal PAL_bpt
	ret
	SET_SIZE(pal_bpt)

	ENTRY_NP(pal_halt)
	call_pal PAL_halt
	ret
	SET_SIZE(pal_halt)

	ENTRY_NP(pal_draina)
	call_pal PAL_draina
	ret
	SET_SIZE(pal_draina)

	ENTRY_NP(pal_rdunique)
	call_pal PAL_rdunique
	ret
	SET_SIZE(pal_rdunique)

	ENTRY_NP(pal_wrunique)
	call_pal PAL_wrunique
	ret
	SET_SIZE(pal_wrunique)

	ENTRY_NP(pal_rdmces)
	call_pal PAL_rdmces
	ret
	SET_SIZE(pal_rdmces)

	ENTRY_NP(pal_rdps)
	call_pal PAL_rdps
	ret
	SET_SIZE(pal_rdps)

	ENTRY_NP(pal_rdusp)
	call_pal PAL_rdusp
	ret
	SET_SIZE(pal_rdusp)

	ENTRY_NP(pal_rdval)
	call_pal PAL_rdval
	ret
	SET_SIZE(pal_rdval)

	ENTRY_NP(pal_swpctx)
	call_pal PAL_swpctx
	ret
	SET_SIZE(pal_swpctx)

	ENTRY_NP(pal_swpipl)
	call_pal PAL_swpipl
	ret
	SET_SIZE(pal_swpipl)

	ENTRY_NP(__tbia)
	call_pal PAL_tbi
	ret
	SET_SIZE(__tbia)

	ENTRY_NP(__tbis)
	call_pal PAL_tbi
	ret
	SET_SIZE(__tbis)

	ENTRY_NP(pal_whami)
	call_pal PAL_whami
	ret
	SET_SIZE(pal_whami)

	ENTRY_NP(pal_wrfen)
	call_pal PAL_wrfen
	ret
	SET_SIZE(pal_wrfen)

	ENTRY_NP(pal_wripir)
	call_pal PAL_ipir
	ret
	SET_SIZE(pal_wripir)

	ENTRY_NP(pal_wrusp)
	call_pal PAL_wrusp
	ret
	SET_SIZE(pal_wrusp)

	ENTRY_NP(pal_wrmces)
	call_pal PAL_wrmces
	ret
	SET_SIZE(pal_wrmces)

	ENTRY_NP(pal_wrval)
	call_pal PAL_wrval
	ret
	SET_SIZE(pal_wrval)

	ENTRY_NP(pal_wrvptptr)
	call_pal PAL_wrvptptr
	ret
	SET_SIZE(pal_wrvptptr)

	ENTRY_NP(pal_cflush)
	call_pal PAL_cflush
	ret
	SET_SIZE(pal_cflush)

	ENTRY_NP(pal_wrent)
	call_pal PAL_wrent
	ret
	SET_SIZE(pal_wrent)

	ENTRY_NP(pal_cserve)
	call_pal PAL_cserve
	ret
	SET_SIZE(pal_cserve)

	ENTRY_NP(pal_wrasn)
	call_pal PAL_wrasn
	ret
	SET_SIZE(pal_wrasn)

