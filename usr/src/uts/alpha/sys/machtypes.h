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
 * Copyright 2017 Hayashi Naoyuki
 */

#pragma once
#include <sys/feature_tests.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Machine dependent types:
 *
 *	Alpha AXP Version
 */
#define	LABEL_REG_PC	0
#define	LABEL_REG_SP	1
#define	LABEL_REG_S0	2
#define	LABEL_REG_S1	3
#define	LABEL_REG_S2	4
#define	LABEL_REG_S3	5
#define	LABEL_REG_S4	6
#define	LABEL_REG_S5	7
#define	LABEL_REG_S6	8
#define	LABEL_T_SIZE	10

#if (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)) || \
	defined(__EXTENSIONS__)

typedef	struct	_label_t { long val[LABEL_T_SIZE]; } label_t;

#endif /* !defined(_POSIX_C_SOURCE)... */

typedef	unsigned char	lock_t;		/* lock work for busy wait */

#ifdef	__cplusplus
}
#endif
