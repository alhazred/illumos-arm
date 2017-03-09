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

#ifndef	_SYS_PRIVREGS_H
#define	_SYS_PRIVREGS_H


#ifdef __cplusplus
extern "C" {
#endif

/*
 * This file describes the cpu's privileged register set, and
 * how the machine state is saved on the stack when a trap occurs.
 */

#ifndef _ASM

/*
 * This is NOT the structure to use for general purpose debugging;
 * see /proc for that.  This is NOT the structure to use to decode
 * the ucontext or grovel about in a core file; see <sys/regset.h>.
 */

struct regs {
	greg_t	r_a3;
	greg_t	r_a4;
	greg_t	r_a5;
	greg_t	r_t8;
	greg_t	r_t9;
	greg_t	r_t10;
	greg_t	r_t11;
	greg_t	r_ra;
	greg_t	r_pv;
	greg_t	r_at;
	greg_t	r_v0;
	greg_t	r_t0;
	greg_t	r_t1;
	greg_t	r_t2;
	greg_t	r_t3;
	greg_t	r_t4;
	greg_t	r_t5;
	greg_t	r_t6;
	greg_t	r_t7;
	greg_t	r_s0;
	greg_t	r_s1;
	greg_t	r_s2;
	greg_t	r_s3;
	greg_t	r_s4;
	greg_t	r_s5;
	greg_t	r_s6;
	greg_t	update;
	greg_t	pad;

	greg_t	r_ps;
	greg_t	r_pc;
	greg_t	r_gp;
	greg_t	r_a0;
	greg_t	r_a1;
	greg_t	r_a2;
};

#ifdef _KERNEL
#define	lwptoregs(lwp)	((struct regs *)((lwp)->lwp_regs))

#endif /* _KERNEL */

#else	/* !_ASM */

#if defined(_MACHDEP)

#define REGOFF_A3	(0*8)
#define REGOFF_A4	(1*8)
#define REGOFF_A5	(2*8)
#define REGOFF_T8	(3*8)
#define REGOFF_T9	(4*8)
#define REGOFF_T10	(5*8)
#define REGOFF_T11	(6*8)
#define REGOFF_RA	(7*8)
#define REGOFF_PV	(8*8)
#define REGOFF_AT	(9*8)
#define REGOFF_V0	(10*8)
#define REGOFF_T0	(11*8)
#define REGOFF_T1	(12*8)
#define REGOFF_T2	(13*8)
#define REGOFF_T3	(14*8)
#define REGOFF_T4	(15*8)
#define REGOFF_T5	(16*8)
#define REGOFF_T6	(17*8)
#define REGOFF_T7	(18*8)
#define REGOFF_S0	(19*8)
#define REGOFF_S1	(20*8)
#define REGOFF_S2	(21*8)
#define REGOFF_S3	(22*8)
#define REGOFF_S4	(23*8)
#define REGOFF_S5	(24*8)
#define REGOFF_S6	(25*8)
#define REGOFF_UPDATE	(26*8)

#define REGOFF_PS	(28*8)	/* HW frame */
#define REGOFF_PC	(29*8)	/* HW frame */
#define REGOFF_GP	(30*8)	/* HW frame */
#define REGOFF_A0	(31*8)	/* HW frame */
#define REGOFF_A1	(32*8)	/* HW frame */
#define REGOFF_A2	(33*8)	/* HW frame */

#define __SAVE_REGS			\
	lda	sp, -REGOFF_PS(sp);	\
	stq	a3, REGOFF_A3(sp);	\
	stq	a4, REGOFF_A4(sp);	\
	stq	a5, REGOFF_A5(sp);	\
	stq	t8, REGOFF_T8(sp);	\
	stq	t9, REGOFF_T9(sp);	\
	stq	t10, REGOFF_T10(sp);	\
	stq	t11, REGOFF_T11(sp);	\
	stq	ra, REGOFF_RA(sp);	\
	stq	pv, REGOFF_PV(sp);	\
	stq	AT, REGOFF_AT(sp);	\
	stq	v0, REGOFF_V0(sp);	\
	stq	t0, REGOFF_T0(sp);	\
	stq	t1, REGOFF_T1(sp);	\
	stq	t2, REGOFF_T2(sp);	\
	stq	t3, REGOFF_T3(sp);	\
	stq	t4, REGOFF_T4(sp);	\
	stq	t5, REGOFF_T5(sp);	\
	stq	t6, REGOFF_T6(sp);	\
	stq	t7, REGOFF_T7(sp);	\
	stq	s0, REGOFF_S0(sp);	\
	stq	s1, REGOFF_S1(sp);	\
	stq	s2, REGOFF_S2(sp);	\
	stq	s3, REGOFF_S3(sp);	\
	stq	s4, REGOFF_S4(sp);	\
	stq	s5, REGOFF_S5(sp);	\
	stq	s6, REGOFF_S6(sp);	\
	stq_c	t6, REGOFF_UPDATE(sp)

#define __SAVE_SYSCALL_REGS		\
	lda	sp, -REGOFF_PS(sp);	\
	stq	v0, REGOFF_V0(sp);	\
	stq	a0, REGOFF_A0(sp);	\
	stq	a1, REGOFF_A1(sp);	\
	stq	a2, REGOFF_A2(sp);	\
	stq	a3, REGOFF_A3(sp);	\
	stq	a4, REGOFF_A4(sp);	\
	stq	a5, REGOFF_A5(sp);	\
	stq	ra, REGOFF_RA(sp);	\
	stq	pv, REGOFF_PV(sp);	\
	stq	s0, REGOFF_S0(sp);	\
	stq	s1, REGOFF_S1(sp);	\
	stq	s2, REGOFF_S2(sp);	\
	stq	s3, REGOFF_S3(sp);	\
	stq	s4, REGOFF_S4(sp);	\
	stq	s5, REGOFF_S5(sp);	\
	stq	s6, REGOFF_S6(sp);	\
	stq	zero, REGOFF_UPDATE(sp);

#define	__RESTORE_SYSCALL_REGS		\
	ldq	v0, REGOFF_V0(sp);	\
	ldq	a0, REGOFF_A0(sp);	\
	ldq	a1, REGOFF_A1(sp);	\
	ldq	a2, REGOFF_A2(sp);	\
	ldq	a3, REGOFF_A3(sp);	\
	ldq	a4, REGOFF_A4(sp);	\
	ldq	a5, REGOFF_A5(sp);	\
	ldq	ra, REGOFF_RA(sp);	\
	ldq	pv, REGOFF_PV(sp);	\
	ldq	t1, REGOFF_T1(sp);	\
	ldq	t2, REGOFF_T2(sp);	\
	lda	sp, REGOFF_PS(sp)

#define	__RESTORE_REGS			\
	ldq	a3, REGOFF_A3(sp);	\
	ldq	a4, REGOFF_A4(sp);	\
	ldq	a5, REGOFF_A5(sp);	\
	ldq	t8, REGOFF_T8(sp);	\
	ldq	t9, REGOFF_T9(sp);	\
	ldq	t10, REGOFF_T10(sp);	\
	ldq	t11, REGOFF_T11(sp);	\
	ldq	ra, REGOFF_RA(sp);	\
	ldq	pv, REGOFF_PV(sp);	\
	ldq	AT, REGOFF_AT(sp);	\
	ldq	v0, REGOFF_V0(sp);	\
	ldq	t0, REGOFF_T0(sp);	\
	ldq	t1, REGOFF_T1(sp);	\
	ldq	t2, REGOFF_T2(sp);	\
	ldq	t3, REGOFF_T3(sp);	\
	ldq	t4, REGOFF_T4(sp);	\
	ldq	t5, REGOFF_T5(sp);	\
	ldq	t6, REGOFF_T6(sp);	\
	ldq	t7, REGOFF_T7(sp);	\
	ldq	s0, REGOFF_S0(sp);	\
	ldq	s1, REGOFF_S1(sp);	\
	ldq	s2, REGOFF_S2(sp);	\
	ldq	s3, REGOFF_S3(sp);	\
	ldq	s4, REGOFF_S4(sp);	\
	ldq	s5, REGOFF_S5(sp);	\
	ldq	s6, REGOFF_S6(sp);	\
	lda	sp, REGOFF_PS(sp)


/*
 * Push register state onto the stack. If we've
 * interrupted userland, do a swapgs as well.
 */
#define	INTR_PUSH

#define	INTR_POP

#define	USER_POP

#define	USER32_POP

#define	DFTRAP_PUSH

#endif	/* _MACHDEP */

/*
 * Used to set rflags to known values at the head of an
 * interrupt gate handler, i.e. interrupts are -already- disabled.
 */
#define	INTGATE_INIT_KERNEL_FLAGS

#endif	/* !_ASM */

#include <sys/controlregs.h>

#ifdef __cplusplus
}
#endif

#endif	/* !_SYS_PRIVREGS_H */
