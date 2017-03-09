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
 * Copyright (c) 1992, 2010, Oracle and/or its affiliates. All rights reserved.
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

#pragma once

#if defined(_ASM)
#define	ADDRESS_C(c)	(c)
#else
#include <sys/types.h>
#include <sys/int_const.h>
#define	ADDRESS_C(c)	UINT64_C(c)
#endif

/*
 * Maximum cpuid value that we support.  NCPU can be defined in a platform's
 * makefile.
 */
/* NCPU_P2 is NCPU rounded to a power of 2 */
#define	NCPU		64
#define	NCPU_LOG2	6
#define	NCPU_P2		(1 << NCPU_LOG2)

/*
 * MMU_PAGES* describes the physical page size used by the mapping hardware.
 * PAGES* describes the logical page size used by the system.
 */
/* mmu-supported page sizes */
#define	MMU_PAGE_SIZES	1
#define	MMU_PAGE_LEVELS	3

/*
 * XXX make sure the MMU_PAGESHIFT definition here is
 * consistent with the one in param.h
 */
#define	MMU_PAGESHIFT		13
#define	MMU_PAGESIZE		(ADDRESS_C(1) << MMU_PAGESHIFT)
#define	MMU_PAGEOFFSET		(MMU_PAGESIZE - 1)
#define	MMU_PAGEMASK		(~MMU_PAGEOFFSET)

#define	MMU_PAGESHIFT64K	16
#define	MMU_PAGESIZE64K		(ADDRESS_C(1) << MMU_PAGESHIFT64K)
#define	MMU_PAGEOFFSET64K	(MMU_PAGESIZE64K - 1)
#define	MMU_PAGEMASK64K		(~MMU_PAGEOFFSET64K)

#define	MMU_PAGESHIFT512K	19
#define	MMU_PAGESIZE512K	(ADDRESS_C(1) << MMU_PAGESHIFT512K)
#define	MMU_PAGEOFFSET512K	(MMU_PAGESIZE512K - 1)
#define	MMU_PAGEMASK512K	(~MMU_PAGEOFFSET512K)

#define	MMU_PAGESHIFT4M		22
#define	MMU_PAGESIZE4M		(ADDRESS_C(1) << MMU_PAGESHIFT4M)
#define	MMU_PAGEOFFSET4M	(MMU_PAGESIZE4M - 1)
#define	MMU_PAGEMASK4M		(~MMU_PAGEOFFSET4M)

#define	PAGESHIFT		MMU_PAGESHIFT
#define	PAGESIZE		MMU_PAGESIZE
#define	PAGEOFFSET		MMU_PAGEOFFSET
#define	PAGEMASK		MMU_PAGEMASK

/*
 * DATA_ALIGN is used to define the alignment of the Unix data segment.
 */
#define	DATA_ALIGN	PAGESIZE

/*
 * DEFAULT KERNEL THREAD stack size.
 */
#define	DEFAULTSTKSZ	(5*PAGESIZE)

/*
 * DEFAULT initial thread stack size.
 */
#define	T0STKSZ		(2 * DEFAULTSTKSZ)

/*
 * Virtual Address Spaces
 */
#define	PA_BITS		41
#define	VA_BITS		43
#define	PTE_BITS	3
#define	SEG0_BASE	ADDRESS_C(0)
#define	SEG0_SIZE	(ADDRESS_C(2)<<PA_BITS)
#define	KSEG_SIZE	(ADDRESS_C(1)<<PA_BITS)
#define	KSEG_BASE	(0 - (ADDRESS_C(2)<<PA_BITS))
#define	SEG1_SIZE	(ADDRESS_C(1)<<PA_BITS)
#define	SEG1_BASE	(0 - (ADDRESS_C(1)<<PA_BITS))

#define	HOLE_START	(SEG0_BASE + SEG0_SIZE)
#define	HOLE_END	KSEG_BASE

/*
 * KERNELBASE is the virtual address at which the kernel segments start in
 * all contexts.
 */
#define	CONSOLE_SIZE	(8L * 1024L * 1024L)
#define	ARGSSIZE	(4L * 1024L * 1024L)
#define	SEGDEBUGSIZE	(4L * 1024L * 1024L)
#define	VPT_SIZE	(ADDRESS_C(1)<<(VA_BITS-(MMU_PAGESHIFT-PTE_BITS)))
#define	MISC_VA_SIZE	(1L * 1024L * 1024L * 1024L)
#define	SEGKPM_SIZE	KSEG_SIZE

#define	VPT_BASE	(ADDRESS_C(0) - VPT_SIZE)	// 0xfffffffe_00000000
#define	CONSOLE_BASE	(VPT_BASE - CONSOLE_SIZE)	// 0xfffffffd_ff800000
#define	ARGSBASE	(CONSOLE_BASE - ARGSSIZE)	// 0xfffffffd_ff400000
#define	SEGDEBUGBASE	(ARGSBASE - SEGDEBUGSIZE)	// 0xfffffffd_ff000000
#define	KERNEL_TEXT	ADDRESS_C(0xfffffffdfb000000)	// 0xfffffffd_fb000000
							// ...
#define	VALLOC_BASE	(MISC_VA_BASE + MISC_VA_SIZE)	// 0xfffffe00_40000000
#define	MISC_VA_BASE	SEG1_BASE			// 0xfffffe00_00000000
#define	SEGKPM_BASE	KSEG_BASE			// 0xfffffc00_00000000
#define	KERNELBASE	KSEG_BASE

#define	VPT_IDX(va)	(((uintptr_t)(va) << (64 - (MMU_PAGESHIFT * 4 - 9))) >> \
    (64 - (MMU_PAGESHIFT * 4 - 9) + MMU_PAGESHIFT))
/*
 * default and boundary sizes for segkp
 */
#define	SEGKPDEFSIZE	(2L * 1024L * 1024L * 1024L)		/*   2G */
#define	SEGKPMAXSIZE	(8L * 1024L * 1024L * 1024L)		/*   8G */
#define	SEGKPMINSIZE	(200L * 1024 * 1024L)			/* 200M */

/*
 * minimum size for segzio
 */
#define	SEGZIOMINSIZE	(400L * 1024 * 1024L)			/* 400M */
#define	SEGZIOMAXSIZE	(512L * 1024L * 1024L * 1024L)		/* 512G */

/*
 * Define upper limit on user address space
 */
#define	USERLIMIT	ADDRESS_C(0x0000040000000000)
#define	USERLIMIT32	ADDRESS_C(0x00000000fffff000)

/*
 * reserve space for modules
 */
#define	MODTEXT	(MMU_PAGESIZE * 256)	// 2MB
#define	MODDATA	(MMU_PAGESIZE * 64)	// 512KB

/*
 * The heap has a region allocated from it of HEAPTEXT_SIZE bytes specifically
 * for module text.
 */
#define	HEAPTEXT_SIZE	(64 * 1024 * 1024)	/* bytes */

/*
 * Bus types
 */
#define	BTISA		1
#define	BTEISA		2

#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(_ASM) && !defined(_KMDB)
extern uintptr_t kernelbase, segmap_start, segmapsize;
#endif

#ifdef	__cplusplus
}
#endif
