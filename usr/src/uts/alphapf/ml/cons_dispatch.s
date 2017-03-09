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
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include "assym.h"

/*
 * cons_dispatch()
 */
	ENTRY(cons_dispatch)
	LDGP(pv)
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
	SET_SIZE(cons_dispatch)

	ENTRY(cons_reboot)
	call_pal PAL_halt
0:	br	zero, 0b
	SET_SIZE(cons_reboot)
