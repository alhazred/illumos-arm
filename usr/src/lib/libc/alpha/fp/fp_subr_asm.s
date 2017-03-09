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
	.file	"fp_subr.s"

#include <SYS.h>

	ENTRY(__get_fpcr)
	excb
	mf_fpcr $f0
	excb
	stt	$f0, 0(a0)
	RET
	SET_SIZE(__get_fpcr)

	ENTRY(__set_fpcr)
	ldt	$f0, 0(a0)
	excb
	mt_fpcr $f0
	excb
	RET
	SET_SIZE(__set_fpcr)

	ENTRY(__dabs)
	ldt	$f0, 0(a0)
	fabs	$f0, $f0
	RET
	SET_SIZE(__dabs)
