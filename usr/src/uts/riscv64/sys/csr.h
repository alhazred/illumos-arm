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

#pragma once

#if !defined(_ASM)
#include <sys/types.h>
#include <asm/csr.h>

#define SSR_SD		(1ul << 63)
#define SSR_UXL_MASK	(3ul << 32)
#define SSR_UXL_32	(1ul << 32)
#define SSR_UXL_64	(2ul << 32)
#define SSR_MXR		(1ul << 19)
#define SSR_SUM		(1ul << 18)
#define SSR_XS_MASK	(3ul << 15)
#define SSR_XS_OFF	(0ul << 15)
#define SSR_XS_INIT	(1ul << 15)
#define SSR_XS_CLEAN	(2ul << 15)
#define SSR_XS_DIRTY	(3ul << 15)
#define SSR_FS_MASK	(3ul << 13)
#define SSR_FS_OFF	(0ul << 13)
#define SSR_FS_INIT	(1ul << 13)
#define SSR_FS_CLEAN	(2ul << 13)
#define SSR_FS_DIRTY	(3ul << 13)
#define SSR_SPP		(1ul << 8)
#define SSR_SPIE	(1ul << 5)
#define SSR_UPIE	(1ul << 4)	// user mode interrupt
#define SSR_SIE		(1ul << 1)
#define SSR_UIE		(1ul << 0)	// user mode interrupt

#define SSR_USERINIT	(SSR_XS_OFF | SSR_FS_INIT | SSR_SPIE)

#define SATP_MODE_MASK	(0xFul << 60)
#define SATP_MODE_BARE	(0x0ul << 60)
#define SATP_MODE_SV39	(0x8ul << 60)
#define SATP_MODE_SV48	(0x9ul << 60)
#define SATP_ASID_SHIFT	44
#define SATP_ASID_MASK	(0xFFul << SATP_ASID_SHIFT)
#define SATP_PPN_MASK	((1ul << 44) - 1)

#define SIE_SSIE	(1ul << 1)
#define SIE_STIE	(1ul << 5)
#define SIE_SEIE	(1ul << 9)

#endif	/* !_ASM */
