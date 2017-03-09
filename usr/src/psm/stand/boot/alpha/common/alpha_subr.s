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
 */

#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include "assym.h"


	ENTRY(load_pcb)
	lda	sp, -(8*8)(sp)
	stq	ra, (8*0)(sp)
	stq	s0, (8*1)(sp)
	stq	s1, (8*2)(sp)
	stq	s2, (8*3)(sp)
	stq	s3, (8*4)(sp)
	stq	s4, (8*5)(sp)
	stq	s5, (8*6)(sp)
	stq	s6, (8*7)(sp)

	stq	sp, PCB_KSP(a1)
	call_pal PAL_swpctx

	ldq	s6, (8*7)(sp)
	ldq	s5, (8*6)(sp)
	ldq	s4, (8*5)(sp)
	ldq	s3, (8*4)(sp)
	ldq	s2, (8*3)(sp)
	ldq	s1, (8*2)(sp)
	ldq	s0, (8*1)(sp)
	ldq	ra, (8*0)(sp)
	lda	sp, (8*8)(sp)
	ret
	SET_SIZE(load_pcb)

/*
 * prom_dispatch()
 */
	ENTRY(prom_dispatch)
	ldgp	gp, 0(pv)	/* set gp */
	lda	sp, -(8*2)(sp)
	stq	ra, (8*0)(sp)
	lda	t0, hwrpb		/* t0 <- hwrpb */
	ldq	t1, 0(t0)		/* t1 <- HWRPB_ADDR */
	ldq	t0, RPB_CRB_OFF(t1)	/* t0 <- CRB Offset */
	addq	t1, t0, t1		/* t1 <- CRB */
	ldq	pv, 0(t1)		/* pv <- DISPATCH Procedure Value */
	ldq	t0, 8(pv)		/* t0 <- DISPATCH Entry */
	jsr	ra, (t0)		/* call!! */
	ldq	ra, (8*0)(sp)
	lda	sp, (8*2)(sp)
	ret
	SET_SIZE(prom_dispatch)

/*
 * prom_fixup()
 */
	ENTRY(prom_fixup)
	ldgp	gp, 0(pv)	/* set gp */
	lda	sp, -(8*2)(sp)
	stq	ra, (8*0)(sp)
	lda	t0, hwrpb		/* t0 <- hwrpb */
	ldq	t1, 0(t0)		/* t1 <- HWRPB_ADDR */
	ldq	t0, RPB_CRB_OFF(t1)	/* t0 <- CRB Offset */
	addq	t1, t0, t1		/* t1 <- CRB */
	ldq	pv, 16(t1)		/* pv <- FIXUP Procedure Value */
	ldq	t0, 8(pv)		/* t0 <- FIXUP Entry */
	jsr	ra, (t0)		/* call!! */
	ldq	ra, (8*0)(sp)
	lda	sp, (8*2)(sp)
	ret
	SET_SIZE(prom_fixup)

/*
 * splimp()
 */
	ENTRY(splimp)
	lda	a0, 0x7
	ret
	SET_SIZE(splimp)

/*
 * splnet()
 */
	ENTRY(splnet)
	lda	a0, 0x7
	ret
	SET_SIZE(splnet)

/*
 * splx()
 */
	ENTRY(splx)
	ret
	SET_SIZE(splx)
