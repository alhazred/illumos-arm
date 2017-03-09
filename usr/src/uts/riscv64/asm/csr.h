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

#pragma once

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

static inline uint64_t csr_read_time(void)
{
	uint64_t _v;
	asm volatile ("rdtime %0":"=r"(_v));
	return _v;
}

#define CSR_GEN_FUNCS(reg) \
static inline uint64_t						\
csr_read_##reg(void)						\
{								\
	uint64_t _v;						\
	asm volatile ("csrr %0," #reg :"=r"(_v):: "memory");	\
	return _v;						\
}								\
								\
static inline void						\
csr_write_##reg(uint64_t val)					\
{								\
	asm volatile ("csrw " #reg ", %0"::"r"(val): "memory");	\
}								\
								\
static inline void						\
csr_set_##reg(uint64_t val)					\
{								\
	asm volatile ("csrs " #reg ", %0"::"r"(val): "memory");	\
}								\
								\
static inline void						\
csr_clear_##reg(uint64_t val)					\
{								\
	asm volatile ("csrc " #reg ", %0"::"r"(val): "memory");	\
}								\
								\
static inline uint64_t						\
csr_read_set_##reg(uint64_t val)				\
{									\
	uint64_t _v;							\
	asm volatile ("csrrs %0," #reg ", %1":"=r"(_v): "r"(val): "memory");	\
	return _v;							\
}									\
									\
static inline uint64_t							\
csr_read_clear_##reg(uint64_t val)					\
{									\
	uint64_t _v;							\
	asm volatile ("csrrc %0," #reg ", %1":"=r"(_v): "r"(val): "memory");	\
	return _v;							\
}									\

CSR_GEN_FUNCS(sstatus)
CSR_GEN_FUNCS(satp)
CSR_GEN_FUNCS(sip)
CSR_GEN_FUNCS(sie)
CSR_GEN_FUNCS(sscratch)
CSR_GEN_FUNCS(stvec)
CSR_GEN_FUNCS(fcsr)
CSR_GEN_FUNCS(fflags)
CSR_GEN_FUNCS(frm)

#ifdef __cplusplus
}
#endif
