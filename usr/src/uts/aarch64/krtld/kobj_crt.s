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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * exit routine from linker/loader to kernel
 */

#include <sys/asm_linkage.h>
#include <sys/reboot.h>
#include "assym.h"

/*
 *  exitto is called from main() and does 1 things
 *	It then jumps directly to the just-loaded standalone.
 *	There is NO RETURN from exitto().
 */

/*
 * void exitto(caddr_t entrypoint)
 */

	.hidden romp
	.hidden ops

	ENTRY(exitto)
	mov	x2, x0
	adrp	x0, :got:romp
	ldr	x0, [x0, #:got_lo12:romp]
	ldr	x0, [x0]
	adrp	x1, :got:ops
	ldr	x1, [x1, #:got_lo12:ops]
	ldr	x1, [x1]
	adr	x30, 1f
	br	x2
1:	ret
	SET_SIZE(exitto)
