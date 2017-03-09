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
	return csr_read_frm();
}

void
__get_ieee_flags(__ieee_flags_type *b)
{
	b->mode = csr_read_frm();
	b->status = csr_read_set_fflags(0);
}

void
__set_ieee_flags(__ieee_flags_type *b)
{
	csr_write_frm(b->mode);
	csr_write_fflags(b->status);
}

double
__dabs(double *d)
{
	return ((*d < 0.0) ? -*d : *d);
}

double
__mul_set(double x, double y, int *pe)
{
	uint32_t fflags = csr_read_set_fflags(0);

	double z = x * y;

	uint32_t fflags_new = csr_read_set_fflags(fflags);

	if ((fflags_new & FE_ALL_EXCEPT) == 0) {
		*pe = 0;
	} else {
		*pe = 1;
	}
	return (z);
}

double
__div_set(double x, double y, int *pe)
{
	uint32_t fflags = csr_read_set_fflags(0);

	double z = x / y;

	uint32_t fflags_new = csr_read_set_fflags(fflags);

	if ((fflags_new & FE_ALL_EXCEPT) == 0) {
		*pe = 0;
	} else {
		*pe = 1;
	}
	return (z);
}
