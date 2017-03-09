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
 * Copyright (c) 1992, 2010, Oracle and/or its affiliates. All rights reserved.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/vmparam.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/machlock.h>
#include <sys/pal.h>
#include <sys/panic.h>
#include <sys/privregs.h>
#include <sys/regset.h>
#include <sys/pcb.h>
#include <sys/psw.h>
#include <sys/frame.h>
#include <sys/archsystm.h>
#include <sys/dtrace.h>
#include <sys/cmn_err.h>
#include <sys/cpu.h>
#include <sys/archsystm.h>
#include <sys/hrt.h>
#include <sys/spl.h>
#include <sys/fp.h>

void
sync_icache(caddr_t addr, uint_t len)
{
	pal_imb();
}

void
sync_data_memory(caddr_t va, size_t len)
{
	__asm__ __volatile__("mb" ::: "memory");
}
void
traceregs(struct regs *rp)
{
	printf("not support traceback\n");
}

void
exec_set_sp(size_t stksize)
{
#if 1
	klwp_t *lwp = ttolwp(curthread);
	pcb_t *pcb = &lwp->lwp_pcb;
	pcb->pcb_hw.usp = (uintptr_t)curproc->p_usrstack - stksize;
	pal_wrusp((uint64_t)curproc->p_usrstack - stksize);
#else
	klwp_t *lwp = ttolwp(curthread);
	lwptoregs(lwp)->r_sp = (uintptr_t)curproc->p_usrstack - stksize;
#endif
}

hrtime_t
gethrtime_waitfree(void)
{
	return (dtrace_gethrtime());
}

hrtime_t
gethrtime(void)
{
	return (pcc_gethrtime());
}

hrtime_t
gethrtime_unscaled(void)
{
	return (pcc_gethrtimeunscaled());
}

void
scalehrtime(hrtime_t *hrt)
{
	pcc_scalehrtime(hrt);
}

void
gethrestime(timespec_t *tp)
{
	pc_gethrestime(tp);
}

extern int one_sec;
extern int max_hres_adj;

void
__adj_hrestime(void)
{
	long long adj;

	if (hrestime_adj == 0)
		adj = 0;
	else if (hrestime_adj > 0) {
		if (hrestime_adj < max_hres_adj)
			adj = hrestime_adj;
		else
			adj = max_hres_adj;
	} else {
		if (hrestime_adj < -max_hres_adj)
			adj = -max_hres_adj;
		else
			adj = hrestime_adj;
	}

	timedelta -= adj;
	hrestime_adj = timedelta;
	hrestime.tv_nsec += adj;

	while (hrestime.tv_nsec >= NANOSEC) {
		one_sec++;
		hrestime.tv_sec++;
		hrestime.tv_nsec -= NANOSEC;
	}
}

/*
 * The panic code invokes panic_saveregs() to record the contents of a
 * regs structure into the specified panic_data structure for debuggers.
 */
void
panic_saveregs(panic_data_t *pdp, struct regs *rp)
{
	panic_nv_t *pnv = PANICNVGET(pdp);

	PANICNVADD(pnv, "a0", rp->r_a0);
	PANICNVADD(pnv, "a1", rp->r_a1);
	PANICNVADD(pnv, "a2", rp->r_a2);
	PANICNVADD(pnv, "a3", rp->r_a3);
	PANICNVADD(pnv, "a4", rp->r_a4);
	PANICNVADD(pnv, "a5", rp->r_a5);
	PANICNVADD(pnv, "t0", rp->r_t0);
	PANICNVADD(pnv, "t1", rp->r_t1);
	PANICNVADD(pnv, "t2", rp->r_t2);
	PANICNVADD(pnv, "t3", rp->r_t3);
	PANICNVADD(pnv, "t4", rp->r_t4);
	PANICNVADD(pnv, "t5", rp->r_t5);
	PANICNVADD(pnv, "t6", rp->r_t6);
	PANICNVADD(pnv, "t7", rp->r_t7);
	PANICNVADD(pnv, "t8", rp->r_t8);
	PANICNVADD(pnv, "t9", rp->r_t9);
	PANICNVADD(pnv, "t10", rp->r_t10);
	PANICNVADD(pnv, "t11", rp->r_t11);
	PANICNVADD(pnv, "pv", rp->r_pv);
	PANICNVADD(pnv, "at", rp->r_at);
	PANICNVADD(pnv, "v0", rp->r_v0);
	PANICNVADD(pnv, "s0", rp->r_s0);
	PANICNVADD(pnv, "s1", rp->r_s1);
	PANICNVADD(pnv, "s2", rp->r_s2);
	PANICNVADD(pnv, "s3", rp->r_s3);
	PANICNVADD(pnv, "s4", rp->r_s4);
	PANICNVADD(pnv, "s5", rp->r_s5);
	PANICNVADD(pnv, "s6", rp->r_s6);

	PANICNVADD(pnv, "pc", rp->r_pc);
	PANICNVADD(pnv, "ps", rp->r_ps);
	PANICNVADD(pnv, "gp", rp->r_gp);
	PANICNVADD(pnv, "usp", pal_rdusp());

	PANICNVSET(pdp, pnv);
}

/*
 * Set floating-point registers from a native fpregset_t.
 */
void
setfpregs(klwp_t *lwp, fpregset_t *fp)
{
	struct fpu_ctx *fpu = &lwp->lwp_pcb.pcb_fpu;
	pcb_t *pcb = &lwp->lwp_pcb;

	kpreempt_disable();
	fpu->fpu_regs.kfpu_cr = fp->fp_cr;
	bcopy(fp->d_fpregs, fpu->fpu_regs.kfpu_regs, sizeof(fpu->fpu_regs.kfpu_regs));
	if (ttolwp(curthread) == lwp) {
		fp_restore(pcb);
	}
	kpreempt_enable();
}

/*
 * Get floating-point registers into a native fpregset_t.
 */
void
getfpregs(klwp_t *lwp, fpregset_t *fp)
{
	struct fpu_ctx *fpu = &lwp->lwp_pcb.pcb_fpu;
	pcb_t *pcb = &lwp->lwp_pcb;

	kpreempt_disable();
	if (ttolwp(curthread) == lwp) {
		fp_save(pcb);
	}

	fp->fp_cr = fpu->fpu_regs.kfpu_cr;
	bcopy(fpu->fpu_regs.kfpu_regs, fp->d_fpregs, sizeof(fp->d_fpregs));

	kpreempt_enable();
}

/*
 * Return the user-level PC.
 * If in a system call, return the address of the syscall trap.
 */
greg_t
getuserpc()
{
	greg_t upc = lwptoregs(ttolwp(curthread))->r_pc;

	if (curthread->t_sysnum != 0)
		upc -= 4;

	return upc;
}

/*
 * Get a pc-only stacktrace.  Used for kmem_alloc() buffer ownership tracking.
 * Returns MIN(current stack depth, pcstack_limit).
 */
int
getpcstack(pc_t *pcstack, int pcstack_limit)
{
	return 0;
}


/*
 * Return the general registers
 */
void
getgregs(klwp_t *lwp, gregset_t grp)
{
	struct regs *rp = lwptoregs(lwp);
	grp[REG_V0] = rp->r_v0;
	grp[REG_T0] = rp->r_t0;
	grp[REG_T1] = rp->r_t1;
	grp[REG_T2] = rp->r_t2;
	grp[REG_T3] = rp->r_t3;
	grp[REG_T4] = rp->r_t4;
	grp[REG_T5] = rp->r_t5;
	grp[REG_T6] = rp->r_t6;
	grp[REG_T7] = rp->r_t7;
	grp[REG_T8] = rp->r_t8;
	grp[REG_T9] = rp->r_t9;
	grp[REG_T10] = rp->r_t10;
	grp[REG_T11] = rp->r_t11;
	grp[REG_T12] = rp->r_pv;
	grp[REG_S0] = rp->r_s0;
	grp[REG_S1] = rp->r_s1;
	grp[REG_S2] = rp->r_s2;
	grp[REG_S3] = rp->r_s3;
	grp[REG_S4] = rp->r_s4;
	grp[REG_S5] = rp->r_s5;
	grp[REG_S6] = rp->r_s6;
	grp[REG_A0] = rp->r_a0;
	grp[REG_A1] = rp->r_a1;
	grp[REG_A2] = rp->r_a2;
	grp[REG_A3] = rp->r_a3;
	grp[REG_A4] = rp->r_a4;
	grp[REG_A5] = rp->r_a5;
	grp[REG_RA] = rp->r_ra;
	grp[REG_AT] = rp->r_at;
	grp[REG_GP] = rp->r_gp;
	grp[REG_PC] = rp->r_pc;

	ASSERT(lwptoregs(lwp)->r_ps & PSL_USER);
	if (ttolwp(curthread) == lwp) {
		grp[REG_SP] = pal_rdusp();
		grp[REG_UQ] = pal_rdunique();
	} else {
		grp[REG_SP] = lwp->lwp_pcb.pcb_hw.usp;
		grp[REG_UQ] = lwp->lwp_pcb.pcb_hw.uniq;
	}
}

/*
 * Set general registers.
 */
void
setgregs(klwp_t *lwp, gregset_t grp)
{
	struct regs *rp = lwptoregs(lwp);
	rp->r_v0 = grp[REG_V0];
	rp->r_t0 = grp[REG_T0];
	rp->r_t1 = grp[REG_T1];
	rp->r_t2 = grp[REG_T2];
	rp->r_t3 = grp[REG_T3];
	rp->r_t4 = grp[REG_T4];
	rp->r_t5 = grp[REG_T5];
	rp->r_t6 = grp[REG_T6];
	rp->r_t7 = grp[REG_T7];
	rp->r_t8 = grp[REG_T8];
	rp->r_t9 = grp[REG_T9];
	rp->r_t10 = grp[REG_T10];
	rp->r_t11 = grp[REG_T11];
	rp->r_pv = grp[REG_T12];
	rp->r_s0 = grp[REG_S0];
	rp->r_s1 = grp[REG_S1];
	rp->r_s2 = grp[REG_S2];
	rp->r_s3 = grp[REG_S3];
	rp->r_s4 = grp[REG_S4];
	rp->r_s5 = grp[REG_S5];
	rp->r_s6 = grp[REG_S6];
	rp->update = 1;
	rp->r_a0 = grp[REG_A0];
	rp->r_a1 = grp[REG_A1];
	rp->r_a2 = grp[REG_A2];
	rp->r_a3 = grp[REG_A3];
	rp->r_a4 = grp[REG_A4];
	rp->r_a5 = grp[REG_A5];
	rp->r_ra = grp[REG_RA];
	rp->r_at = grp[REG_AT];
	rp->r_gp = grp[REG_GP];
	rp->r_pc = grp[REG_PC];

	ASSERT(lwptoregs(lwp)->r_ps & PSL_USER);
	lwp->lwp_pcb.pcb_hw.usp = grp[REG_SP];
	lwp->lwp_pcb.pcb_hw.uniq = grp[REG_UQ];
	if (ttolwp(curthread) == lwp) {
		pal_wrusp(grp[REG_SP]);
		pal_wrunique(grp[REG_UQ]);
	}
}

/*
 * The following ELF header fields are defined as processor-specific
 * in the V8 ABI:
 *
 *	e_ident[EI_DATA]	encoding of the processor-specific
 *				data in the object file
 *	e_machine		processor identification
 *	e_flags			processor-specific flags associated
 *				with the file
 */

/*
 * The value of at_flags reflects a platform's cpu module support.
 * at_flags is used to check for allowing a binary to execute and
 * is passed as the value of the AT_FLAGS auxiliary vector.
 */
int at_flags = 0;

/*
 * Check the processor-specific fields of an ELF header.
 *
 * returns 1 if the fields are valid, 0 otherwise
 */
int
elfheadcheck(
	unsigned char e_data,
	Elf32_Half e_machine,
	Elf32_Word e_flags)
{
	if (e_data != ELFDATA2LSB)
		return (0);
	return (e_machine == EM_ALPHA);
}
uint_t auxv_hwcap_include = 0;	/* patch to enable unrecognized features */
uint_t auxv_hwcap_exclude = 0;	/* patch for broken cpus, debugging */

int
scanc(size_t length, u_char *string, u_char table[], u_char mask)
{
	const u_char *end = &string[length];

	while (string < end && (table[*string] & mask) == 0)
		string++;
	return (end - string);
}

int
__ipltospl(int ipl)
{
	return (ipltospl(ipl));
}
