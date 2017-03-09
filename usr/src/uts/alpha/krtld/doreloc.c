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
 * Copyright (c) 2004, 2010, Oracle and/or its affiliates. All rights reserved.
 */

#if	defined(_KERNEL)
#include	<sys/types.h>
#include	"reloc.h"
#else
#define	ELF_TARGET_ALPHA
#if defined(DO_RELOC_LIBLD)
#undef DO_RELOC_LIBLD
#define	DO_RELOC_LIBLD_ALPHA
#endif
#include	<stdio.h>
#include	"sgs.h"
#include	"machdep.h"
#include	"libld.h"
#include	"reloc.h"
#include	"conv.h"
#include	"msg.h"
#endif

/*
 * We need to build this code differently when it is used for
 * cross linking:
 *	- Data alignment requirements can differ from those
 *		of the running system, so we can't access data
 *		in units larger than a byte
 *	- We have to include code to do byte swapping when the
 *		target and linker host use different byte ordering,
 *		but such code is a waste when running natively.
 */
#if !defined(DO_RELOC_LIBLD) || defined(__alpha)
#define	DORELOC_NATIVE
#endif

/*
 * This table represents the current relocations that do_reloc() is able to
 * process.  The relocations below that are marked SPECIAL are relocations that
 * take special processing and shouldn't actually ever be passed to do_reloc().
 */
const Rel_entry	reloc_table[R_ALPHA_NUM] = {
/* R_ALPHA_NONE */	{0, FLG_RE_NOTREL, 0, 0, 0},
/* R_ALPHA_REFLONG */	{0, FLG_RE_NOTREL, 4, 0, 0},
/* R_ALPHA_REFQUAD */	{0, FLG_RE_NOTREL, 8, 0, 0},
/* R_ALPHA_GPREL32 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_LITERAL */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_LITUSE */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_GPDISP */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_BRADDR */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_HINT */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_SREL16 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_SREL32 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_SREL64 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_OP_PUSH */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_OP_STORE */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_OP_PSUB */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_OP_PRSHIFT */{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_GPVALUE */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_GPRELHIGH */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_GPRELLOF */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_IMMED */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TLS_LITERAL*/{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TLS_HIGH */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TLS_LOW */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* 23 */		{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_COPY */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_GLOB_DAT */	{0, FLG_RE_NOTREL, 8, 0, 0},
/* R_ALPHA_JMP_SLOT */	{0, FLG_RE_NOTREL, 8, 0, 0},
/* R_ALPHA_RELATIVE */	{0, FLG_RE_NOTREL, 8, 0, 0},
/* R_ALPHA_BRSGP */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TLSGD */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TLSLDM */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_DTPMOD64 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_GOTDTPREL */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_DTPREL64 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_DTPRELHI */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_DTPRELLO */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_DTPREL16 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_GOTTPREL */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TPREL64 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TPRELHI */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TPRELLO */	{0, FLG_RE_NOTSUP, 0, 0, 0},
/* R_ALPHA_TPREL16 */	{0, FLG_RE_NOTSUP, 0, 0, 0},
};


#if defined(_KERNEL)
#define	lml	0		/* Needed by arglist of REL_ERR_* macros */
int
do_reloc_krtld(Word rtype, uchar_t *off, Xword *value, const char *sym,
    const char *file)
#elif defined(DO_RELOC_LIBLD)
/*ARGSUSED5*/
int
do_reloc_ld(Rel_desc *rdesc, uchar_t *off, Xword *value,
    rel_desc_sname_func_t rel_desc_sname_func,
    const char *file, int bswap, void *lml)
#else
int
do_reloc_rtld(Word rtype, uchar_t *off, Xword *value, const char *sym,
    const char *file, void *lml)
#endif
{
#ifdef DO_RELOC_LIBLD
#define	sym (* rel_desc_sname_func)(rdesc)
	uchar_t	rtype = rdesc->rel_rtype;
#endif
	Xword	uvalue = 0;
	Xword	basevalue;
	Xword	corevalue = *value;
	const	Rel_entry	*rep;

	rep = &reloc_table[rtype];

	switch (rep->re_fsize) {
	case 1:
		basevalue = *((uchar_t *)off);
		break;
	case 2:
		basevalue = *((Half *)off);
		break;
	case 4:
		basevalue = *((Word *)off);
		break;
	case 8:
		basevalue = *((Xword *)off);
		break;
	default:
		/*
		 * To keep chkmsg() happy: MSG_INTL(MSG_REL_UNSUPSZ)
		 */
		REL_ERR_UNSUPSZ(lml, file, sym, rtype, rep->re_fsize);
		return (0);
	}

	switch (rtype) {
	case R_ALPHA_REFQUAD:
	case R_ALPHA_GLOB_DAT:
	case R_ALPHA_JMP_SLOT:
	case R_ALPHA_RELATIVE:
		uvalue = *value;
		break;
	}

	switch (rep->re_fsize) {
	case 1:
		*((uchar_t *)off) = (uchar_t)uvalue;
		break;
	case 2:
		*((Half *)off) = (Half)uvalue;
		break;
	case 4:
		*((Word *)off) = (Word)uvalue;
		break;
	case 8:
		*((Xword *)off) = uvalue;
		break;
	}
	return (1);

#ifdef DO_RELOC_LIBLD
#undef sym
#endif
}
