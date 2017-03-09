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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma once

#include <sys/csr.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This file describes the cpu's privileged register set, and
 * how the machine state is saved on the stack when a trap occurs.
 */

#define REGOFF_RA	(0*8)
#define REGOFF_SP	(1*8)
#define REGOFF_GP	(2*8)
#define REGOFF_TP	(3*8)
#define REGOFF_T0	(4*8)
#define REGOFF_T1	(5*8)
#define REGOFF_T2	(6*8)
#define REGOFF_S0	(7*8)
#define REGOFF_S1	(8*8)
#define REGOFF_A0	(9*8)
#define REGOFF_A1	(10*8)
#define REGOFF_A2	(11*8)
#define REGOFF_A3	(12*8)
#define REGOFF_A4	(13*8)
#define REGOFF_A5	(14*8)
#define REGOFF_A6	(15*8)
#define REGOFF_A7	(16*8)
#define REGOFF_S2	(17*8)
#define REGOFF_S3	(18*8)
#define REGOFF_S4	(19*8)
#define REGOFF_S5	(20*8)
#define REGOFF_S6	(21*8)
#define REGOFF_S7	(22*8)
#define REGOFF_S8	(23*8)
#define REGOFF_S9	(24*8)
#define REGOFF_S10	(25*8)
#define REGOFF_S11	(26*8)
#define REGOFF_T3	(27*8)
#define REGOFF_T4	(28*8)
#define REGOFF_T5	(29*8)
#define REGOFF_T6	(30*8)
#define REGOFF_PC	(31*8)
#define REGOFF_SSR	(32*8)
#define REG_FRAME	(34*8)

#ifndef _ASM

/*
 * This is NOT the structure to use for general purpose debugging;
 * see /proc for that.  This is NOT the structure to use to decode
 * the ucontext or grovel about in a core file; see <sys/regset.h>.
 */

struct regs {
	greg_t	r_ra;
	greg_t	r_sp;
	greg_t	r_gp;
	greg_t	r_tp;
	greg_t	r_t0;
	greg_t	r_t1;
	greg_t	r_t2;
	greg_t	r_s0;
	greg_t	r_s1;
	greg_t	r_a0;
	greg_t	r_a1;
	greg_t	r_a2;
	greg_t	r_a3;
	greg_t	r_a4;
	greg_t	r_a5;
	greg_t	r_a6;
	greg_t	r_a7;
	greg_t	r_s2;
	greg_t	r_s3;
	greg_t	r_s4;
	greg_t	r_s5;
	greg_t	r_s6;
	greg_t	r_s7;
	greg_t	r_s8;
	greg_t	r_s9;
	greg_t	r_s10;
	greg_t	r_s11;
	greg_t	r_t3;
	greg_t	r_t4;
	greg_t	r_t5;
	greg_t	r_t6;
	greg_t	r_pc;
	greg_t	r_ssr;
};

#ifdef _KERNEL
#define	lwptoregs(lwp)	((struct regs *)((lwp)->lwp_regs))
#define	USERMODE(ssr)	(((ssr) & SSR_SPP) == 0)

#endif /* _KERNEL */

#else	/* !_ASM */

#if defined(_MACHDEP)

#define __SAVE_USER_REGS(reg) \
	addi	sp, sp, -REG_FRAME		; \
	sd	ra, REGOFF_RA(sp)		; \
	csrrw	ra, sscratch, zero		; \
	sd	ra, REGOFF_SP(sp)		; \
	sd	gp, REGOFF_GP(sp)		; \
	sd	tp, REGOFF_TP(sp)		; \
	sd	t0, REGOFF_T0(sp)		; \
	sd	t1, REGOFF_T1(sp)		; \
	sd	t2, REGOFF_T2(sp)		; \
	sd	s0, REGOFF_S0(sp)		; \
	sd	s1, REGOFF_S1(sp)		; \
	sd	a0, REGOFF_A0(sp)		; \
	sd	a1, REGOFF_A1(sp)		; \
	sd	a2, REGOFF_A2(sp)		; \
	sd	a3, REGOFF_A3(sp)		; \
	sd	a4, REGOFF_A4(sp)		; \
	sd	a5, REGOFF_A5(sp)		; \
	sd	a6, REGOFF_A6(sp)		; \
	sd	a7, REGOFF_A7(sp)		; \
	sd	s2, REGOFF_S2(sp)		; \
	sd	s3, REGOFF_S3(sp)		; \
	sd	s4, REGOFF_S4(sp)		; \
	sd	s5, REGOFF_S5(sp)		; \
	sd	s6, REGOFF_S6(sp)		; \
	sd	s7, REGOFF_S7(sp)		; \
	sd	s8, REGOFF_S8(sp)		; \
	sd	s9, REGOFF_S9(sp)		; \
	sd	s10, REGOFF_S10(sp)		; \
	sd	s11, REGOFF_S11(sp)		; \
	sd	t3, REGOFF_T3(sp)		; \
	sd	t4, REGOFF_T4(sp)		; \
	sd	t5, REGOFF_T5(sp)		; \
	sd	t6, REGOFF_T6(sp)		; \
	csrr	reg, sepc			; \
	sd	reg, REGOFF_PC(sp)		; \
	csrr	reg, sstatus			; \
	sd	reg, REGOFF_SSR(sp)

#define	__RESTORE_USER_REGS(reg) \
	ld	reg, REGOFF_PC(sp)		; \
	csrw	sepc, reg			; \
	ld	reg, REGOFF_SSR(sp)		; \
	csrw	sstatus, reg			; \
	ld	reg, REGOFF_SP(sp)		; \
	csrw	sscratch, reg			; \
	ld	ra, REGOFF_RA(sp)		; \
	ld	gp, REGOFF_GP(sp)		; \
	ld	tp, REGOFF_TP(sp)		; \
	ld	t0, REGOFF_T0(sp)		; \
	ld	t1, REGOFF_T1(sp)		; \
	ld	t2, REGOFF_T2(sp)		; \
	ld	s0, REGOFF_S0(sp)		; \
	ld	s1, REGOFF_S1(sp)		; \
	ld	a0, REGOFF_A0(sp)		; \
	ld	a1, REGOFF_A1(sp)		; \
	ld	a2, REGOFF_A2(sp)		; \
	ld	a3, REGOFF_A3(sp)		; \
	ld	a4, REGOFF_A4(sp)		; \
	ld	a5, REGOFF_A5(sp)		; \
	ld	a6, REGOFF_A6(sp)		; \
	ld	a7, REGOFF_A7(sp)		; \
	ld	s2, REGOFF_S2(sp)		; \
	ld	s3, REGOFF_S3(sp)		; \
	ld	s4, REGOFF_S4(sp)		; \
	ld	s5, REGOFF_S5(sp)		; \
	ld	s6, REGOFF_S6(sp)		; \
	ld	s7, REGOFF_S7(sp)		; \
	ld	s8, REGOFF_S8(sp)		; \
	ld	s9, REGOFF_S9(sp)		; \
	ld	s10, REGOFF_S10(sp)		; \
	ld	s11, REGOFF_S11(sp)		; \
	ld	t3, REGOFF_T3(sp)		; \
	ld	t4, REGOFF_T4(sp)		; \
	ld	t5, REGOFF_T5(sp)		; \
	ld	t6, REGOFF_T6(sp)		; \
	addi	sp, sp, REG_FRAME		; \
	csrrw	sp, sscratch, sp

#define __SAVE_SV_REGS(reg) \
	csrrw	sp, sscratch, sp		; \
	addi	sp, sp, -REG_FRAME		; \
	sd	ra, REGOFF_RA(sp)		; \
	addi	ra, sp, REG_FRAME		; \
	sd	ra, REGOFF_SP(sp)		; \
	sd	gp, REGOFF_GP(sp)		; \
	sd	tp, REGOFF_TP(sp)		; \
	sd	t0, REGOFF_T0(sp)		; \
	sd	t1, REGOFF_T1(sp)		; \
	sd	t2, REGOFF_T2(sp)		; \
	sd	s0, REGOFF_S0(sp)		; \
	sd	s1, REGOFF_S1(sp)		; \
	sd	a0, REGOFF_A0(sp)		; \
	sd	a1, REGOFF_A1(sp)		; \
	sd	a2, REGOFF_A2(sp)		; \
	sd	a3, REGOFF_A3(sp)		; \
	sd	a4, REGOFF_A4(sp)		; \
	sd	a5, REGOFF_A5(sp)		; \
	sd	a6, REGOFF_A6(sp)		; \
	sd	a7, REGOFF_A7(sp)		; \
	sd	s2, REGOFF_S2(sp)		; \
	sd	s3, REGOFF_S3(sp)		; \
	sd	s4, REGOFF_S4(sp)		; \
	sd	s5, REGOFF_S5(sp)		; \
	sd	s6, REGOFF_S6(sp)		; \
	sd	s7, REGOFF_S7(sp)		; \
	sd	s8, REGOFF_S8(sp)		; \
	sd	s9, REGOFF_S9(sp)		; \
	sd	s10, REGOFF_S10(sp)		; \
	sd	s11, REGOFF_S11(sp)		; \
	sd	t3, REGOFF_T3(sp)		; \
	sd	t4, REGOFF_T4(sp)		; \
	sd	t5, REGOFF_T5(sp)		; \
	sd	t6, REGOFF_T6(sp)		; \
	csrr	reg, sepc			; \
	sd	reg, REGOFF_PC(sp)		; \
	csrr	reg, sstatus			; \
	sd	reg, REGOFF_SSR(sp)

#define	__RESTORE_SV_REGS(reg) \
	ld	reg, REGOFF_PC(sp)		; \
	csrw	sepc, reg			; \
	ld	reg, REGOFF_SSR(sp)		; \
	csrw	sstatus, reg			; \
	ld	reg, REGOFF_SP(sp)		; \
	csrw	sscratch, reg			; \
	ld	ra, REGOFF_RA(sp)		; \
	ld	gp, REGOFF_GP(sp)		; \
	ld	tp, REGOFF_TP(sp)		; \
	ld	t0, REGOFF_T0(sp)		; \
	ld	t1, REGOFF_T1(sp)		; \
	ld	t2, REGOFF_T2(sp)		; \
	ld	s0, REGOFF_S0(sp)		; \
	ld	s1, REGOFF_S1(sp)		; \
	ld	a0, REGOFF_A0(sp)		; \
	ld	a1, REGOFF_A1(sp)		; \
	ld	a2, REGOFF_A2(sp)		; \
	ld	a3, REGOFF_A3(sp)		; \
	ld	a4, REGOFF_A4(sp)		; \
	ld	a5, REGOFF_A5(sp)		; \
	ld	a6, REGOFF_A6(sp)		; \
	ld	a7, REGOFF_A7(sp)		; \
	ld	s2, REGOFF_S2(sp)		; \
	ld	s3, REGOFF_S3(sp)		; \
	ld	s4, REGOFF_S4(sp)		; \
	ld	s5, REGOFF_S5(sp)		; \
	ld	s6, REGOFF_S6(sp)		; \
	ld	s7, REGOFF_S7(sp)		; \
	ld	s8, REGOFF_S8(sp)		; \
	ld	s9, REGOFF_S9(sp)		; \
	ld	s10, REGOFF_S10(sp)		; \
	ld	s11, REGOFF_S11(sp)		; \
	ld	t3, REGOFF_T3(sp)		; \
	ld	t4, REGOFF_T4(sp)		; \
	ld	t5, REGOFF_T5(sp)		; \
	ld	t6, REGOFF_T6(sp)		; \
	addi	sp, sp, REG_FRAME		; \
	csrrw	sp, sscratch, zero

#endif	/* _MACHDEP */

/*
 * Used to set rflags to known values at the head of an
 * interrupt gate handler, i.e. interrupts are -already- disabled.
 */
#define	INTGATE_INIT_KERNEL_FLAGS

#endif	/* !_ASM */

#ifdef __cplusplus
}
#endif

