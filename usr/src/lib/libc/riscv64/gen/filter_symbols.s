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

#define FILTER_SYM(x)		\
	.global	x;		\
	.type	x, %function;	\
x:;				\
	.size	x, [.-x]

	.struct	0

	FILTER_SYM(dlopen)
	FILTER_SYM(_dlopen)
	FILTER_SYM(dlsym)
	FILTER_SYM(_dlsym)
	FILTER_SYM(dlclose)
	FILTER_SYM(_dlclose)
	FILTER_SYM(dlerror)
	FILTER_SYM(_dlerror)
	FILTER_SYM(dladdr)
	FILTER_SYM(_dladdr)
	FILTER_SYM(dladdr1)
	FILTER_SYM(_dladdr1)
	FILTER_SYM(dldump)
	FILTER_SYM(_dldump)
	FILTER_SYM(dlinfo)
	FILTER_SYM(_dlinfo)
	FILTER_SYM(dlmopen)
	FILTER_SYM(_dlmopen)
	FILTER_SYM(_ld_libc)
