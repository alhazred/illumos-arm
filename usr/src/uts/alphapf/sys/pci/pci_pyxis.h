/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2018 Hayashi Naoyuki
 */

#pragma once

#ifdef	__cplusplus
extern "C" {
#endif

#define	PYXIS_PCI_DIRECT_MAP_INDEX	1
#define	PYXIS_PCI_DIRECT_MAP_BASE	0x40000000
#define	PYXIS_PCI_DIRECT_MAP_SIZE	0x40000000
#define	PYXIS_PCI_DIRECT_MAP_TRANS_BASE	0
#define	PYXIS_PCI_WnBASE(n)	(*(volatile uint32_t *)(0xfffffc8760000400UL + 0x100 * (n)))
#define	PYXIS_PCI_WnMASK(n)	(*(volatile uint32_t *)(0xfffffc8760000440UL + 0x100 * (n)))
#define	PYXIS_PCI_TnBASE(n)	(*(volatile uint32_t *)(0xfffffc8760000480UL + 0x100 * (n)))

#ifdef	__cplusplus
}
#endif

