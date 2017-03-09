/*-
 * Copyright (c) 2004-2005 David Schultz <das@FreeBSD.ORG>
 * Copyright (c) 2015-2016 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Portions of this software were developed by SRI International and the
 * University of Cambridge Computer Laboratory under DARPA/AFRL contract
 * FA8750-10-C-0237 ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Portions of this software were developed by the University of Cambridge
 * Computer Laboratory as part of the CTSRD Project, with support from the
 * UK Higher Education Innovation Fund (HEIF).
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
#include <sys/csr.h>

int
feclearexcept(int __excepts)
{
	csr_clear_fflags(__excepts);
	return (0);
}

int
fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
	*__flagp = csr_read_fflags() & __excepts;

	return (0);
}

int
fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
	csr_clear_fflags(__excepts);
	csr_set_fflags(*__flagp & __excepts);

	return (0);
}

int
feraiseexcept(int __excepts)
{
	csr_set_fflags(__excepts);

	return (0);
}

int
fetestexcept(int __excepts)
{
	return csr_read_fflags() & __excepts;
}

int
fegetround(void)
{
	return csr_read_frm();
}

int
fesetround(int __round)
{
	csr_set_frm(__round);

	return (0);
}

int
fegetenv(fenv_t *__envp)
{
	__envp->__fsr = csr_read_fcsr();

	return (0);
}

int
feholdexcept(fenv_t *__envp)
{

	/* No exception traps. */

	return (-1);
}

int
fesetenv(const fenv_t *__envp)
{
	csr_write_fcsr(__envp->__fsr);

	return (0);
}

int
feupdateenv(const fenv_t *__envp)
{
	uint32_t __fcsr = csr_read_set_fcsr(__envp->__fsr);
	feraiseexcept(__fcsr & FE_ALL_EXCEPT);

	return (0);
}

#if __BSD_VISIBLE

/* We currently provide no external definitions of the functions below. */

int
feenableexcept(int __mask)
{

	/* No exception traps. */

	return (-1);
}

int
fedisableexcept(int __mask)
{

	/* No exception traps. */

	return (0);
}

int
fegetexcept(void)
{

	/* No exception traps. */

	return (0);
}

#endif /* __BSD_VISIBLE */

/*
 * Hopefully the system ID byte is immutable, so it's valid to use
 * this as a default environment.
 */
const fenv_t __fe_dfl_env = {0};

