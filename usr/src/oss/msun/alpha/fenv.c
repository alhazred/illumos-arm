/*-
 * Copyright (c) 2004 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <fenv.h>
#include <stdint.h>

#define	_ROUND_MASK	(FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO)
#define	_ROUND_SHIFT	58
#define	_FPUSW_SHIFT	51

#define	__excb()	__asm __volatile("excb")
#define	__mf_fpcr(__cw)	__asm __volatile("mf_fpcr %0" : "=f" (*(__cw)))
#define	__mt_fpcr(__cw)	__asm __volatile("mt_fpcr %0" : : "f" (__cw))

const fenv_t __fe_dfl_env = {0};

union __fpcr {
	double __d;
	uint64_t __bits;
};

int
feclearexcept(int __excepts)
{
	union __fpcr __r;

	__excb();
	__mf_fpcr(&__r.__d);
	__r.__bits &= ~((uint64_t)__excepts << _FPUSW_SHIFT);
	__mt_fpcr(__r.__d);
	__excb();
	return (0);
}

int
fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
	union __fpcr __r;

	__excb();
	__mf_fpcr(&__r.__d);
	__excb();
	*__flagp = (__r.__bits >> _FPUSW_SHIFT) & __excepts;
	return (0);
}

int
fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
	union __fpcr __r;
	uint64_t __xflag, __xexcepts;

	__xflag = (uint64_t)*__flagp << _FPUSW_SHIFT;
	__xexcepts = (uint64_t)__excepts << _FPUSW_SHIFT;
	__excb();
	__mf_fpcr(&__r.__d);
	__r.__bits &= ~__xexcepts;
	__r.__bits |= __xflag & __xexcepts;
	__mt_fpcr(__r.__d);
	__excb();
	return (0);
}

int
feraiseexcept(int __excepts)
{
	fexcept_t __ex = __excepts;
	fesetexceptflag(&__ex, __excepts);
	return (0);
}

int
fetestexcept(int __excepts)
{
	union __fpcr __r;

	__excb();
	__mf_fpcr(&__r.__d);
	__excb();
	return ((__r.__bits >> _FPUSW_SHIFT) & __excepts);
}

int
fegetround(void)
{
	union __fpcr __r;

	__mf_fpcr(&__r.__d);
	return ((int)(__r.__bits >> _ROUND_SHIFT) & _ROUND_MASK);
}

inline int
fesetround(int __round)
{
	union __fpcr __r;

	if (__round & ~_ROUND_MASK)
		return (-1);
	__excb();
	__mf_fpcr(&__r.__d);
	__r.__bits &= ~((uint64_t)_ROUND_MASK << _ROUND_SHIFT);
	__r.__bits |= (uint64_t)__round << _ROUND_SHIFT;
	__mt_fpcr(__r.__d);
	__excb();
	return (0);
}

int
fegetenv(fenv_t *__envp)
{
	union __fpcr __r;

	__mf_fpcr(&__r.__d);
	__envp->__fsr = __r.__bits;
	return (0);
}

int
feholdexcept(fenv_t *__envp)
{
	union __fpcr __r;

	__excb();
	__mf_fpcr(&__r.__d);
	__envp->__fsr = __r.__bits;
	__r.__bits &= ~((uint64_t)FE_ALL_EXCEPT << _FPUSW_SHIFT);
	__mt_fpcr(__r.__d);
	__excb();
	return (0);
}

int
fesetenv(const fenv_t *__envp)
{
	union __fpcr __r;
	__r.__bits = __envp->__fsr;

	__excb();
	__mt_fpcr(__r.__d);
	__excb();
	return (0);
}

int
feupdateenv(const fenv_t *__envp)
{
	union __fpcr __r;

	__excb();
	__mf_fpcr(&__r.__d);
	__mt_fpcr(__envp->__fsr);
	__excb();

	feraiseexcept(__r.__bits >> _FPUSW_SHIFT);
	return (0);
}

