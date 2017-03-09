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
 * Use is subject to license terms.
 */

#ifndef _ASM_PAL_H
#define _ASM_PAL_H

#if !defined(_ASM)
#include <sys/types.h>

static inline void pal_clrfen(void)
{
	asm volatile ("call_pal %0"
	    :
	    : "i"(PAL_clrfen)
	    // t0 and t8...t11
	    : "$1", "$22", "$23", "$24", "$25");
}

static inline void pal_imb(void)
{
	asm volatile ("call_pal %0"
	    :
	    : "i"(PAL_imb)
	    : "memory");
}

static inline void pal_bpt(void)
{
	asm volatile ("call_pal %0"
	    :
	    : "i"(PAL_bpt)
	    : "memory");
}

static inline void pal_draina(void)
{
	asm volatile ("call_pal %0"
	    :
	    : "i"(PAL_draina)
	    : "memory");
}

static inline uint64_t pal_rdunique(void)
{
	register uint64_t v0 asm("$0");
	asm volatile("call_pal %1"
		: "=r" (v0)
		: "i" (PAL_rdunique));
	return v0;
}

static inline void pal_wrunique(uint64_t unique)
{
	register uint64_t a0 asm("$16") = unique;
	asm volatile("call_pal %1"
		: "=r" (a0)
		: "i" (PAL_wrunique), "0"(a0));
}

static inline uint64_t pal_rdps(void)
{
	register uint64_t v0 asm("$0");
	asm volatile("call_pal %1"
		: "=r" (v0)
		: "i" (PAL_rdps)
		// t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25");
	return v0;
}

static inline uint64_t pal_rdusp(void)
{
	register uint64_t v0 asm("$0");
	asm volatile("call_pal %1"
		: "=r" (v0)
		: "i" (PAL_rdusp)
		// t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25");
	return v0;
}

static inline uint64_t pal_rdval(void)
{
	register uint64_t v0 asm("$0");
	asm volatile("call_pal %1"
		: "=r" (v0)
		: "i" (PAL_rdval)
		// t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25");
	return v0;
}

static inline uint64_t pal_swpipl(uint64_t ipl)
{
	register uint64_t v0 asm("$0");
	register uint64_t a0 asm("$16") = ipl;
	asm volatile("call_pal %2"
		: "=r" (a0), "=r"(v0)
		: "i" (PAL_swpipl), "0"(a0)
		// a0, t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25", "memory");
	return v0;
}

static inline void __tbis(uint64_t op, uint64_t va)
{
	register uint64_t a0 asm("$16") = op;
	register uint64_t a1 asm("$17") = va;
	asm volatile("call_pal %2"
		: "=r" (a0), "=r"(a1)
		: "i" (PAL_tbi), "0"(a0), "1"(a1)
		// a0, a1, t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25", "memory");
}

static inline void __tbia(uint64_t op)
{
	register uint64_t a0 asm("$16") = op;
	asm volatile("call_pal %1"
		: "=r" (a0)
		: "i" (PAL_tbi), "0"(a0)
		// a0, a1, t0 and t8...t11
		: "$1", "$17", "$22", "$23", "$24", "$25", "memory");
}

static inline uint64_t pal_whami(void)
{
	register uint64_t v0 asm("$0");
	asm volatile("call_pal %1"
		: "=r" (v0)
		: "i" (PAL_whami)
		// t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25");
	return v0;
}

static inline void pal_wrusp(uint64_t usp)
{
	register uint64_t a0 asm("$16") = usp;
	asm volatile("call_pal %1"
		: "=r" (a0)
		: "i" (PAL_wrusp), "0"(a0)
		// a0, t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25");
}

static inline void pal_wrval(uint64_t val)
{
	register uint64_t a0 asm("$16") = val;
	asm volatile("call_pal %1"
		: "=r" (a0)
		: "i" (PAL_wrval), "0"(a0)
		// a0, t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25");
}

static inline void pal_wrfen(uint64_t val)
{
	register uint64_t a0 asm("$16") = val;
	asm volatile("call_pal %1"
		: "=r" (a0)
		: "i" (PAL_wrfen), "0"(a0)
		// a0, t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25");
}

static inline void pal_wrasn(uint64_t asn)
{
	register uint64_t a0 asm("$16") = asn;
	asm volatile("call_pal %1"
		: "=r" (a0)
		: "i" (PAL_wrfen), "0"(a0)
		// a0, t0 and t8...t11
		: "$1", "$22", "$23", "$24", "$25", "memory");
}

#define pal_tbisi(va)	__tbis(1, va)
#define pal_tbisd(va)	__tbis(2, va)
#define pal_tbis(va)	__tbis(3, va)
#define pal_tbiap()	__tbia(-1)
#define pal_tbia()	__tbia(-2)

extern void pal_halt(void);
extern uint64_t pal_rdmces(void);
extern uint64_t pal_swpctx(uint64_t ctx);
extern void pal_wripir(uint64_t cpu_id);
extern void pal_wrmces(uint64_t mces);
extern void pal_wrvptptr(uint64_t vptptr);
extern void pal_cflush(uint64_t pfn);
extern void pal_wrent(uint64_t ent, uint64_t op);
extern void pal_cserve(uint64_t arg0, uint64_t arg1);
#endif	/* !_ASM */

#endif /* _ASM_PAL_H */
