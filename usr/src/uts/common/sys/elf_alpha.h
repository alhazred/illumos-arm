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
 *	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
 *	  All Rights Reserved
 *
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */

#ifndef _SYS_ELF_ALPHA_H
#define	_SYS_ELF_ALPHA_H

#ifdef	__cplusplus
extern "C" {
#endif

#define EF_ALPHA_32BIT		0x00000001
#define EF_ALPHA_CANRELAX	0x00000002


/*
 * Alpha Relocation Types
 */
#define	R_ALPHA_NONE		0
#define	R_ALPHA_REFLONG		1
#define	R_ALPHA_REFQUAD		2
#define	R_ALPHA_GPREL32		3
#define	R_ALPHA_LITERAL		4
#define	R_ALPHA_LITUSE		5
#define	R_ALPHA_GPDISP		6
#define	R_ALPHA_BRADDR		7
#define	R_ALPHA_HINT		8
#define	R_ALPHA_SREL16		9
#define	R_ALPHA_SREL32		10
#define	R_ALPHA_SREL64		11

#define	R_ALPHA_GPRELHIGH	17
#define	R_ALPHA_GPRELLOW	18
#define	R_ALPHA_GPREL16		19

/* for DSO */
#define	R_ALPHA_COPY		24
#define	R_ALPHA_GLOB_DAT	25
#define	R_ALPHA_JMP_SLOT	26
#define	R_ALPHA_RELATIVE	27
#define	R_ALPHA_BRSGP		28

/* for TLS  */
#define	R_ALPHA_TLSGD		29
#define	R_ALPHA_TLSLDM		30
#define	R_ALPHA_DTPMOD64	31
#define	R_ALPHA_GOTDTPREL	32
#define	R_ALPHA_DTPREL64	33
#define	R_ALPHA_DTPRELHI	34
#define	R_ALPHA_DTPRELLO	35
#define	R_ALPHA_DTPREL16	36
#define	R_ALPHA_GOTTPREL	37
#define	R_ALPHA_TPREL64		38
#define	R_ALPHA_TPRELHI		39
#define	R_ALPHA_TPRELLO		40
#define	R_ALPHA_TPREL16		41
#define	R_ALPHA_NUM		42

/*
 * Relocation aliases
 */
#define	ELF_ALPHA_MAXPGSZ	0x10000		/* maximum page size */

/*
 * Processor specific section types
 */
#define	SHF_ORDERED		0x40000000
#define	SHF_EXCLUDE		0x80000000
#define	SHN_BEFORE		0xff00
#define	SHN_AFTER		0xff01

/*
 * There are consumers of this file that want to include elf defines for
 * all architectures.  This is a problem for the defines below, because
 * while they are architecture specific they have common names.  Hence to
 * prevent attempts to redefine these variables we'll check if any of
 * the other elf architecture header files have been included.  If
 * they have then we'll just stick with the existing definitions.
 */
#if !defined(_SYS_ELF_MACH_COMMON)
#define	_SYS_ELF_MACH_COMMON

/*
 * Plt and Got information; the first few .got and .plt entries are reserved
 *	PLT[0]	jump to dynamic linker
 *	GOT[0]	address of _DYNAMIC
 */
#define	M_PLT_INSSIZE		4	/* single plt instruction size */
#define	M_GOT_XDYNAMIC		0	/* got index for _DYNAMIC */
#define	M_GOT_XNumber		1	/* reserved no. of got entries */

/*
 * ELF64 bit PLT constants
 */
#define	M64_WORD_ALIGN		8
#define	M64_PLT_ENTSIZE		12	/* plt entry size in bytes */
#define	M64_PLT_ALIGN		16	/* alignment of .plt section */
#define	M64_GOT_ENTSIZE		8	/* got entry size in bytes */
#define	M64_PLT_RESERVSZ	32

/*
 * Make common alias for the 64 bit specific defines based on _ELF64
 */
/* architecture common defines */
#define	M_WORD_ALIGN		M64_WORD_ALIGN
#define	M_PLT_ENTSIZE		M64_PLT_ENTSIZE
#define	M_PLT_ALIGN		M64_PLT_ALIGN
#define	M_PLT_RESERVSZ		M64_PLT_RESERVSZ
#define	M_GOT_ENTSIZE		M64_GOT_ENTSIZE

#endif /* !_SYS_ELF_MACH_COMMON */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ELF_SPARC_H */
