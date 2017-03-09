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

/*
 *	Copyright (c) 1988 AT&T
 *	  All Rights Reserved
 *
 */

#ifndef	_MACHDEP_ALPHA_H
#define	_MACHDEP_ALPHA_H

#include <link.h>
#include <sys/machelf.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Elf header information.
 */
#define	M_MACH			EM_ALPHA
#define	M_MACH_64		EM_ALPHA
#define	M_CLASS			ELFCLASS64
#define	M_DATA			ELFDATA2LSB

/*
 * Page boundary Macros: truncate to previous page boundary and round to
 * next page boundary (refer to generic macros in ../sgs.h also).
 */
#define	M_PTRUNC(X)	((X) & ~(syspagsz - 1))
#define	M_PROUND(X)	(((X) + syspagsz - 1) & ~(syspagsz - 1))

/*
 * Make common relocation types transparent to the common code
 */
#define	M_R_NONE	R_ALPHA_NONE
#define	M_R_GLOB_DAT	R_ALPHA_GLOB_DAT
#define	M_R_COPY	R_ALPHA_COPY
#define	M_R_RELATIVE	R_ALPHA_RELATIVE
#define	M_R_JMP_SLOT	R_ALPHA_JMP_SLOT
#define	M_R_REGISTER	R_ALPHA_REGISTER
#define	M_R_FPTR	R_ALPHA_NONE
#define	M_R_NUM		R_ALPHA_NUM

#define	M_BIND_ADJ	4

/*
 * Relocation type macro.
 */
#define	M_RELOC		Rela

/*
 * TLS static segments must be rounded to the following requirements,
 * due to libthread stack allocation.
 */
#define	M_TLSSTATALIGN	0x10

/*
 * Make register symbols transparent to common code
 */
#define	M_DT_REGISTER	0xffffffff 

/*
 * Make plt section information transparent to the common code.
 */
#define	M_PLT_SHF_FLAGS	(SHF_ALLOC | SHF_WRITE | SHF_EXECINSTR)

/*
 * Make default data segment and stack flags transparent to the common code.
 */
#define	M_DATASEG_PERM	(PF_R | PF_W | PF_X)
#define	M_STACK_PERM	(PF_R | PF_W)

/*
 * Make common relocation information transparent to the common code
 */
#define	M_REL_DT_TYPE	DT_RELA		/* .dynamic entry */
#define	M_REL_DT_SIZE	DT_RELASZ	/* .dynamic entry */
#define	M_REL_DT_ENT	DT_RELAENT	/* .dynamic entry */
#define	M_REL_DT_COUNT	DT_RELACOUNT	/* .dynamic entry */
#define	M_REL_SHT_TYPE	SHT_RELA	/* section header type */
#define	M_REL_ELF_TYPE	ELF_T_RELA	/* data buffer type */


#ifdef	__cplusplus
}
#endif

#endif /* _MACHDEP_ALPHA_H */
