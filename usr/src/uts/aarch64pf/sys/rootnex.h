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
 * Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.
 */

#ifndef	_SYS_ROOTNEX_H
#define	_SYS_ROOTNEX_H

/*
 * x86 root nexus implementation specific state
 */

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/sunddi.h>
#include <sys/sdt.h>

#ifdef	__cplusplus
extern "C" {
#endif


/* size of buffer used for ctlop reportdev */
#define	REPORTDEV_BUFSIZE	1024

/*
 * integer or boolean property name and value. A few static rootnex properties
 * are created during rootnex attach from an array of rootnex_intprop_t..
 */
typedef struct rootnex_intprop_s {
	char	*prop_name;
	int	prop_value;
} rootnex_intprop_t;


#define	MAKE_DMA_COOKIE(cp, address, size)	\
	{					\
		(cp)->dmac_notused = 0;		\
		(cp)->dmac_type = 0;		\
		(cp)->dmac_laddress = (address);	\
		(cp)->dmac_size = (size);	\
	}

typedef struct rootnex_dma_hdl {
	ddi_dma_impl_t		pdh_ddi_hdl;
	ddi_dma_attr_t		pdh_attr_dev;
	uint_t			max_cookies;
	uint_t			ncookies;
	ddi_dma_cookie_t	cookies[0];
} rootnex_dma_hdl_t;

typedef struct rootnex_state_s {
	kmutex_t		r_peekpoke_mutex;
	dev_info_t		*r_dip;
	ddi_iblock_cookie_t	r_err_ibc;
} rootnex_state_t;


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ROOTNEX_H */
