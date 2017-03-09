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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include	<sys/types.h>
#include	"reloc.h"

static const char	*rels[R_ALPHA_NUM] = {
	"R_ALPHA_NONE",		"R_ALPHA_REFLONG",
	"R_ALPHA_REFQUAD",	"R_ALPHA_GPREL32",
	"R_ALPHA_LITERAL",	"R_ALPHA_LITUSE",
	"R_ALPHA_GPDISP",	"R_ALPHA_BRADDR",
	"R_ALPHA_HINT",		"R_ALPHA_SREL16",
	"R_ALPHA_SREL32",	"R_ALPHA_SREL64",
	"R_ALPHA_OP_PUSH",	"R_ALPHA_OP_STORE",
	"R_ALPHA_OP_PSUB",	"R_ALPHA_OP_PRSHIFT",
	"R_ALPHA_GPVALUE",	"R_ALPHA_GPRELHIGH",
	"R_ALPHA_GPRELLOF",	"R_ALPHA_IMMED",
	"R_ALPHA_TLS_LITERAL",	"R_ALPHA_TLS_HIGH",
	"R_ALPHA_TLS_LOW",	"R_ALPHA_UNKNOWN23",
	"R_ALPHA_COPY",		"R_ALPHA_GLOB_DAT",
	"R_ALPHA_JMP_SLOT",	"R_ALPHA_RELATIVE",
	"R_ALPHA_BRSGP",	"R_ALPHA_TLSGD",
	"R_ALPHA_TLSLDM",	"R_ALPHA_DTPMOD64",
	"R_ALPHA_GOTDTPREL",	"R_ALPHA_DTPREL64",
	"R_ALPHA_DTPRELHI",	"R_ALPHA_DTPRELLO",
	"R_ALPHA_DTPREL16",	"R_ALPHA_GOTTPREL",
	"R_ALPHA_TPREL64",	"R_ALPHA_TPRELHI",
	"R_ALPHA_TPRELLO",	"R_ALPHA_TPREL16",
};

/*
 * This is a 'stub' of the orignal version defined in liblddbg.so.  This stub
 * returns the 'int string' of the relocation in question instead of converting
 * the relocation to it's full syntax.
 */
const char *
conv_reloc_ALPHA_type(Word type)
{
	static char 	strbuf[32];
	int		ndx = 31;

	if (type < R_ALPHA_NUM)
		return (rels[type]);

	strbuf[ndx--] = '\0';
	do {
		strbuf[ndx--] = '0' + (type % 10);
		type = type / 10;
	} while ((ndx >= (int)0) && (type > (Word)0));

	return (&strbuf[ndx + 1]);
}
