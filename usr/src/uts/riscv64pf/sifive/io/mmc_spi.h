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

#include <sys/types.h>
#include <stdbool.h>
#include <sys/platform.h>
#include <sys/ddi_impldefs.h>
#include <sys/blkdev.h>

struct spi_sc {
	dev_info_t *dip;
	ddi_acc_handle_t handle;
	uint32_t *addr;
	int cs;
};

struct mmc_sc {
	dev_info_t *dip;
	struct spi_sc spi;
	bd_handle_t bdh;
	uint32_t ocr;
	uint8_t csd[16];
	uint8_t cid[16];

	kmutex_t intrlock;
};

