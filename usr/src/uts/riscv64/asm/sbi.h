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
 * Copyright 2019 Hayashi Naoyuki
 */

#pragma once

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define SBI_SUCCESS		0
#define SBI_ERR_FAILURE		-1
#define SBI_ERR_NOT_SUPPORTED	-2
#define SBI_ERR_INVALID_PARAM	-3
#define SBI_ERR_DENIED		-4
#define SBI_ERR_INVALID_ADDRESS	-5

#define	SBI_SET_TIMER			0
#define	SBI_CONSOLE_PUTCHAR		1
#define	SBI_CONSOLE_GETCHAR		2
#define	SBI_CLEAR_IPI			3
#define	SBI_SEND_IPI			4
#define	SBI_REMOTE_FENCE_I		5
#define	SBI_REMOTE_SFENCE_VMA		6
#define	SBI_REMOTE_SFENCE_VMA_ASID	7
#define	SBI_SHUTDOWN			8


static inline
void sbi_set_timer(uint64_t stime_value)
{
	register uint64_t a0 asm ("a0") = stime_value;
	register uint64_t a7 asm ("a7") = SBI_SET_TIMER;
	asm volatile ("ecall" :: "r"(a0), "r"(a7) :"memory");
}

static inline
void sbi_send_ipi(const unsigned long *hart_mask)
{
	register uint64_t a0 asm ("a0") = (uint64_t)hart_mask;
	register uint64_t a7 asm ("a7") = SBI_SEND_IPI;
	asm volatile ("ecall" :: "r"(a0), "r"(a7) :"memory");
}

static inline
void sbi_clear_ipi(void)
{
	register uint64_t a7 asm ("a7") = SBI_CLEAR_IPI;
	asm volatile ("ecall" :: "r"(a7) :"memory");
}

static inline
void sbi_remote_fence_i(const unsigned long *hart_mask)
{
	register uint64_t a0 asm ("a0") = (uint64_t)hart_mask;
	register uint64_t a7 asm ("a7") = SBI_REMOTE_FENCE_I;
	asm volatile ("ecall" :: "r"(a0), "r"(a7) :"memory");
}

static inline
void sbi_remote_sfence_vma(
    const unsigned long *hart_mask,
    unsigned long start,
    unsigned long size)
{
	register uint64_t a0 asm ("a0") = (uint64_t)hart_mask;
	register uint64_t a1 asm ("a1") = start;
	register uint64_t a2 asm ("a2") = size;
	register uint64_t a7 asm ("a7") = SBI_REMOTE_SFENCE_VMA;
	asm volatile ("ecall" :: "r"(a0), "r"(a1), "r"(a2), "r"(a7) :"memory");
}

static inline
void sbi_remote_sfence_vma_asid(
    const unsigned long *hart_mask,
    unsigned long start,
    unsigned long size,
    unsigned long asid)
{
	register uint64_t a0 asm ("a0") = (uint64_t)hart_mask;
	register uint64_t a1 asm ("a1") = start;
	register uint64_t a2 asm ("a2") = size;
	register uint64_t a3 asm ("a3") = asid;
	register uint64_t a7 asm ("a7") = SBI_REMOTE_SFENCE_VMA_ASID;
	asm volatile ("ecall" :: "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a7) :"memory");
}

static inline
int sbi_console_getchar(void)
{
	register uint64_t a0 asm ("a0");
	register uint64_t a7 asm ("a7") = SBI_CONSOLE_GETCHAR;
	asm volatile ("ecall" : "=r"(a0) :"r"(a7) :"memory");
	return (int)a0;
}

static inline
void sbi_console_putchar(int ch)
{
	register uint64_t a0 asm ("a0") = ch;
	register uint64_t a7 asm ("a7") = SBI_CONSOLE_PUTCHAR;
	asm volatile ("ecall" :: "r"(a0),"r"(a7) :"memory");
}

static inline
void sbi_shutdown(void)
{
	register uint64_t a7 asm ("a7") = SBI_SHUTDOWN;
	asm volatile ("ecall" :: "r"(a7) :"memory");
}

#ifdef __cplusplus
}
#endif
