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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/stack.h>
#include <sys/regset.h>
#include <sys/frame.h>
#include <sys/sysmacros.h>
#include <sys/trap.h>
#include <sys/machelf.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "Pcontrol.h"
#include "Pstack.h"

const char *
Ppltdest(struct ps_prochandle *P, uintptr_t pltaddr)
{
	map_info_t *mp = Paddr2mptr(P, pltaddr);
	file_info_t *fp;
	size_t i;
	uintptr_t r_addr;

	if (mp == NULL || (fp = mp->map_file) == NULL ||
	    fp->file_plt_base == 0 ||
	    pltaddr - fp->file_plt_base >= fp->file_plt_size) {
		errno = EINVAL;
		return (NULL);
	}

	i = (pltaddr - (fp->file_plt_base + M_PLT_RESERVSZ)) / M_PLT_ENTSIZE;

	Elf64_Rela r;

	r_addr = fp->file_jmp_rel + i * sizeof (r);

	if (Pread(P, &r, sizeof (r), r_addr) == sizeof (r) &&
	    (i = ELF64_R_SYM(r.r_info)) < fp->file_dynsym.sym_symn) {
		Elf_Data *data = fp->file_dynsym.sym_data_pri;
		Elf64_Sym *symp = &(((Elf64_Sym *)data->d_buf)[i]);

		return (fp->file_dynsym.sym_strs + symp->st_name);
	}

	return (NULL);
}

int
Pissyscall(struct ps_prochandle *P, uintptr_t addr)
{
	uint32_t instr;

	if (Pread(P, &instr, sizeof (instr), addr) != sizeof (instr))
		return (0);

	if (instr == 0x83)
		return (1);

	return (0);
}

int
Pissyscall_prev(struct ps_prochandle *P, uintptr_t addr, uintptr_t *dst)
{
	int ret;

	if (ret = Pissyscall(P, addr - sizeof (uint32_t))) {
		if (dst)
			*dst = addr - sizeof (uint32_t);
		return (ret);
	}

	return (0);
}

/* ARGSUSED */
int
Pissyscall_text(struct ps_prochandle *P, const void *buf, size_t buflen)
{
	if (buflen < sizeof (uint32_t))
		return (0);

	const char *inst = buf;

	if (inst[0]==0x83 && inst[1]==0 && inst[2]==0 && inst[3]==0)
		return (1);

	return (0);
}

int
Pstack_iter(struct ps_prochandle *P, const prgregset_t regs,
	proc_stack_f *func, void *arg)
{
	return -1;
}

uintptr_t
Psyscall_setup(struct ps_prochandle *P, int nargs, int sysindex, uintptr_t sp)
{
	int pusharg = (nargs > 6) ? nargs - 6: 0;

	sp -= sizeof (int64_t) * (pusharg);
	P->status.pr_lwp.pr_reg[REG_V0] = sysindex;
	P->status.pr_lwp.pr_reg[REG_SP] = sp;
	P->status.pr_lwp.pr_reg[REG_PC] = P->sysaddr;
	return (sp);
}

int
Psyscall_copyinargs(struct ps_prochandle *P, int nargs, argdes_t *argp,
    uintptr_t ap)
{
	int64_t arglist[MAXARGS];
	int i;
	argdes_t *adp;
	int pusharg = (nargs > 6) ? nargs - 6: 0;

	for (i = 0, adp = argp; i < nargs; i++, adp++) {
		switch (i) {
		case 0:
			(void) Pputareg(P, REG_A0, adp->arg_value);
			break;
		case 1:
			(void) Pputareg(P, REG_A1, adp->arg_value);
			break;
		case 2:
			(void) Pputareg(P, REG_A2, adp->arg_value);
			break;
		case 3:
			(void) Pputareg(P, REG_A3, adp->arg_value);
			break;
		case 4:
			(void) Pputareg(P, REG_A4, adp->arg_value);
			break;
		case 5:
			(void) Pputareg(P, REG_A5, adp->arg_value);
			break;
		default:
			arglist[i - 6] = (uint64_t)adp->arg_value;
			break;
		}
	}

	if (Pwrite(P, &arglist[0], sizeof (int64_t) * (pusharg), ap) !=
	    sizeof (int64_t) * (pusharg))
		return (-1);

	return (0);
}

int
Psyscall_copyoutargs(struct ps_prochandle *P, int nargs, argdes_t *argp,
    uintptr_t ap)
{
	int pusharg = (nargs > 6) ? nargs - 6: 0;
	int64_t arglist[MAXARGS];
	int i;
	argdes_t *adp;

	if (pusharg  > 0 &&
	    Pread(P, &arglist[0], sizeof (int64_t) * (pusharg),
		    ap) != sizeof (int64_t) * (pusharg))
		return (-1);

	for (i = 0, adp = argp; i < nargs; i++, adp++) {
		switch (i) {
		case 0:
			adp->arg_value =
			    P->status.pr_lwp.pr_reg[REG_A0];
			break;
		case 1:
			adp->arg_value =
			    P->status.pr_lwp.pr_reg[REG_A1];
			break;
		case 2:
			adp->arg_value =
			    P->status.pr_lwp.pr_reg[REG_A2];
			break;
		case 3:
			adp->arg_value =
			    P->status.pr_lwp.pr_reg[REG_A3];
			break;
		case 4:
			adp->arg_value =
			    P->status.pr_lwp.pr_reg[REG_A4];
			break;
		case 5:
			adp->arg_value =
			    P->status.pr_lwp.pr_reg[REG_A5];
			break;
		default:
			adp->arg_value = arglist[i - 6];
			break;
		}
	}

	return (0);
}
