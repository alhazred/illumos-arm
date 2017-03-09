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

#include <sys/ieeefp.h>
#include <base_conversion.h>
#include "fp.h"

enum fp_direction_type _QgetRD(void)
{
	unsigned long fpcr;
	__get_fpcr(&fpcr);

	return (fpcr >> 58) & 3;
}

void
__get_ieee_flags(__ieee_flags_type *b)
{
	unsigned long fpcr;
	__get_fpcr(&fpcr);
	b->status = (fpcr & FPCR_TRAP_STATUS) >> 32;
	b->mode = (fpcr & ~FPCR_TRAP_STATUS) >> 32;
	fpcr = FPCR_TRAP_DISABLE | (fpcr & FPCR_DYN_MASK);
	__set_fpcr(&fpcr);
}

void
__set_ieee_flags(__ieee_flags_type *b)
{
	unsigned long fpcr;
	fpcr = (unsigned long)b->status << 32;
	fpcr |= (unsigned long)b->mode << 32;
	__set_fpcr(&fpcr);
}

double
__mul_set(double x, double y, int *pe)
{
	unsigned long fpcr;
	unsigned long fpcr_old;
	double z;

	__get_fpcr(&fpcr_old);
	fpcr = FPCR_TRAP_DISABLE | (fpcr_old & FPCR_DYN_MASK);
	__set_fpcr(&fpcr);

	z = x * y;

	__get_fpcr(&fpcr);
	__set_fpcr(&fpcr_old);

	if ((fpcr & FPCR_SUM) == 0) {
		*pe = 0;
	} else {
		*pe = 1;
	}
	return (z);
}

double
__div_set(double x, double y, int *pe)
{
	unsigned long fpcr;
	unsigned long fpcr_old;
	double z;

	__get_fpcr(&fpcr_old);
	fpcr = FPCR_TRAP_DISABLE | (fpcr_old & FPCR_DYN_MASK);
	__set_fpcr(&fpcr);

	z = x / y;

	__get_fpcr(&fpcr);
	__set_fpcr(&fpcr_old);

	if ((fpcr & FPCR_SUM) == 0) {
		*pe = 0;
	} else {
		*pe = 1;
	}
	return (z);
}
