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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sys/errno.h>
#include "assym.h"

#define THREADP(reg)			\
	   mrs reg, tpidr_el1

#define	SUWORD(NAME, INSTR, COPYOP, SIZE)	\
	ENTRY(NAME);				\
	li	t6, SSR_SUM;			\
	csrs	sstatus, t6;			\
	la	t0, kernelbase;			\
	ld	t0, (t0);			\
	bgeu	a0, t0, 2f;			\
	addi	t1, a0, SIZE - 1;		\
	bgeu	t1, t0, 2f;			\
	ld	t3, T_LOFAULT(tp);		\
	la	t4, 1f;				\
	sd	t4, T_LOFAULT(tp);		\
	INSTR	a1, (a0);			\
	sd	t3, T_LOFAULT(tp);		\
	li	a0, 0;				\
	csrc	sstatus, t6;			\
	ret;					\
1:	sd	t3, T_LOFAULT(tp);		\
	ld	t1, T_COPYOPS(tp);		\
	beqz	t1, 2f;				\
	ld	t2, COPYOP(t1);			\
	csrc	sstatus, t6;			\
	jr	t2;				\
2:	li	a0, -1;				\
	csrc	sstatus, t6;			\
	ret;					\
	SET_SIZE(NAME)

	SUWORD(suword64, sd, CP_SUWORD64, 8)
	SUWORD(suword32, sw, CP_SUWORD32, 4)
	SUWORD(suword16, sh, CP_SUWORD16, 2)
	SUWORD(suword8,  sb, CP_SUWORD8,  1)

#define	FUWORD(NAME, INSTR, INSTR2, COPYOP, SIZE)	\
	ENTRY(NAME);				\
	li	t6, SSR_SUM;			\
	csrs	sstatus, t6;			\
	la	t0, kernelbase;			\
	ld	t0, (t0);			\
	bgeu	a0, t0, 2f;			\
	addi	t1, a0, SIZE - 1;		\
	bgeu	t1, t0, 2f;			\
	ld	t3, T_LOFAULT(tp);		\
	la	t4, 1f;				\
	sd	t4, T_LOFAULT(tp);		\
	INSTR	a3, (a0);			\
	sd	t3, T_LOFAULT(tp);		\
	INSTR2	a3, (a1);			\
	li	a0, 0;				\
	csrc	sstatus, t6;			\
	ret;					\
1:	sd	t3, T_LOFAULT(tp);		\
	ld	t1, T_COPYOPS(tp);		\
	beqz	t1, 2f;				\
	ld	t2, COPYOP(t1);			\
	csrc	sstatus, t6;			\
	jr	t2;				\
2:	li	a0, -1;				\
	csrc	sstatus, t6;			\
	ret;					\
	SET_SIZE(NAME)

	FUWORD(fuword64, ld, sd, CP_FUWORD64, 8)
	FUWORD(fuword32, lw, sw, CP_FUWORD32, 4)
	FUWORD(fuword16, lh, sh, CP_FUWORD16, 2)
	FUWORD(fuword8,  lb, sb, CP_FUWORD8,  1)


#define	SUWORD_NOERR(NAME, INSTR, SIZE)		\
	ENTRY(NAME);				\
	li	t6, SSR_SUM;			\
	csrs	sstatus, t6;			\
	la	t0, kernelbase;			\
	ld	t0, (t0);			\
	bgeu	a0, t0, 1f;			\
	addi	t1, a0, SIZE - 1;		\
	bgeu	t1, t0, 1f;			\
	INSTR	a1, (a0);			\
1:	csrc	sstatus, t6;			\
	ret;					\
	SET_SIZE(NAME)

	SUWORD_NOERR(suword64_noerr, sd, 8)
	SUWORD_NOERR(suword32_noerr, sw, 4)
	SUWORD_NOERR(suword16_noerr, sh, 2)
	SUWORD_NOERR(suword8_noerr,  sb, 1)

#define	FUWORD_NOERR(NAME, INSTR, INSTR2, SIZE)	\
	ENTRY(NAME);				\
	li	t6, SSR_SUM;			\
	csrs	sstatus, t6;			\
	la	t0, kernelbase;			\
	ld	t0, (t0);			\
	bgeu	a0, t0, 1f;			\
	addi	t1, a0, SIZE - 1;		\
	bgeu	t1, t0, 1f;			\
	INSTR	t0, (a0);			\
	INSTR2	t0, (a1);			\
1:	csrc	sstatus, t6;			\
	ret;					\
	SET_SIZE(NAME)

	FUWORD_NOERR(fuword64_noerr, ld, sd, 8)
	FUWORD_NOERR(fuword32_noerr, lw, sw, 4)
	FUWORD_NOERR(fuword16_noerr, lh, sh, 2)
	FUWORD_NOERR(fuword8_noerr,  lb, sb, 1)

	.weak	subyte
	.weak	subyte_noerr
	.weak	fulword
	.weak	fulword_noerr
	.weak	sulword
	.weak	sulword_noerr
subyte=suword8
subyte_noerr=suword8_noerr
fulword=fuword64
fulword_noerr=fuword64_noerr
sulword=suword64
sulword_noerr=suword64_noerr


/*
   void uzero(void *addr, size_t count)
 */
	ENTRY(uzero)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 2f
	beqz	a1, 2f
	sb	zero, (a0)
	add	a0, a0, 1
	add	a1, a1, -1
	j	1b
2:	csrc	sstatus, t6
	ret
	SET_SIZE(uzero)

/*
   void ucopy(const void *ufrom, void *uto, size_t ulength)
 */
	ENTRY(ucopy)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 2f
	bgeu	a1, t0, 2f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	csrc	sstatus, t6
	ret
	SET_SIZE(ucopy)

/*
   void ucopystr(const char *ufrom, char *uto, size_t umaxlength, size_t *ulencopied)
 */
	ENTRY(ucopystr)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	li	t2, 0
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 2f
	bgeu	a1, t0, 2f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	add	t2, t2, 1
	beqz	t1, 2f
	j	1b
2:	beqz	a3, 10f
	sd	t2, (a3)
10:	csrc	sstatus, t6
	ret
	SET_SIZE(ucopystr)

/*
   int copyoutstr(const char *kaddr, char *uaddr, size_t maxlength, size_t *lencopied)
   int copyoutstr_noerr(const char *kaddr, char *uaddr, size_t maxlength, size_t *lencopied)
 */
	ENTRY(copyoutstr)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	mv	a5, a0
	mv	a6, a1
	mv	a7, a2
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_copyoutstr
	sd	t4, T_LOFAULT(tp)
	li	t2, 0
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a1, t0, 3f
	beqz	a2, 4f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	add	t2, t2, 1
	beqz	t1, 2f
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	beqz	a3, 10f
	sd	t2, (a3)
10:	li	a0, 0
	csrc	sstatus, t6
	ret
3:	sd	t3, T_LOFAULT(tp)
	li	a0, EFAULT
	csrc	sstatus, t6
	ret
4:	sd	t3, T_LOFAULT(tp)
	li	a0, ENAMETOOLONG
	csrc	sstatus, t6
	ret
.L_flt_copyoutstr:
	sd	t3, T_LOFAULT(tp)
	ld	t0, T_COPYOPS(tp)
	beqz	t0, 3b
	ld	t0, CP_COPYOUTSTR(t0)
	mv	a0, a5
	mv	a1, a6
	mv	a2, a7
	csrc	sstatus, t6
	jr	t0
	SET_SIZE(copyoutstr)

	ENTRY(copyoutstr_noerr)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	li	t2, 0
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a1, t0, 3f
	beqz	a2, 4f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	add	t2, t2, 1
	beqz	t1, 2f
	j	1b
2:	beqz	a3, 10f
	sd	t2, (a3)
10:	li	a0, 0
	csrc	sstatus, t6
	ret
3:	li	a0, EFAULT
	csrc	sstatus, t6
	ret
4:	li	a0, ENAMETOOLONG
	csrc	sstatus, t6
	ret
	SET_SIZE(copyoutstr_noerr)

/*
   int copyinstr(const char *uaddr, char *kaddr, size_t maxlength, size_t *lencopied)
   int copyinstr_noerr(const char *uaddr, char *kaddr, size_t maxlength, size_t *lencopied)
 */
	ENTRY(copyinstr)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	mv	a5, a0
	mv	a6, a1
	mv	a7, a2
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_copyinstr
	sd	t4, T_LOFAULT(tp)
	li	t2, 0
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 3f
	beqz	a2, 4f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	add	t2, t2, 1
	beqz	t1, 2f
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	beqz	a3, 10f
	sd	t2, (a3)
10:	li	a0, 0
	csrc	sstatus, t6
	ret
3:	sd	t3, T_LOFAULT(tp)
	li	a0, EFAULT
	csrc	sstatus, t6
	ret
4:	sd	t3, T_LOFAULT(tp)
	li	a0, ENAMETOOLONG
	csrc	sstatus, t6
	ret
.L_flt_copyinstr:
	sd	t3, T_LOFAULT(tp)
	ld	t0, T_COPYOPS(tp)
	beqz	t0, 3b
	ld	t0, CP_COPYINSTR(t0)
	mv	a0, a5
	mv	a1, a6
	mv	a2, a7
	csrc	sstatus, t6
	jr	t0
	SET_SIZE(copyinstr)

	ENTRY(copyinstr_noerr)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	li	t2, 0
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 3f
	beqz	a2, 4f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	add	t2, t2, 1
	beqz	t1, 2f
	j	1b
2:	beqz	a3, 10f
	sd	t2, (a3)
10:	li	a0, 0
	csrc	sstatus, t6
	ret
3:	li	a0, EFAULT
	csrc	sstatus, t6
	ret
4:	li	a0, ENAMETOOLONG
	csrc	sstatus, t6
	ret
	SET_SIZE(copyinstr_noerr)

/*
   int copyin(const void *uaddr, void *kaddr, size_t count)
   void copyin_noerr(const void *ufrom, void *kto, size_t count)
 */
	ENTRY(copyin)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	mv	a5, a0
	mv	a6, a1
	mv	a7, a2
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_copyin
	sd	t4, T_LOFAULT(tp)
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 3f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	li	a0, 0
	csrc	sstatus, t6
	ret
3:	sd	t3, T_LOFAULT(tp)
	li	a0, -1
	csrc	sstatus, t6
	ret
.L_flt_copyin:
	sd	t3, T_LOFAULT(tp)
	ld	t0, T_COPYOPS(tp)
	beqz	t0, 3b
	ld	t0, CP_COPYIN(t0);
	mv	a0, a5
	mv	a1, a6
	mv	a2, a7
	csrc	sstatus, t6
	jr	t0
	SET_SIZE(copyin)

	ENTRY(xcopyin)
	ALTENTRY(xcopyin_nta)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	mv	a5, a0
	mv	a6, a1
	mv	a7, a2
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_xcopyin
	sd	t4, T_LOFAULT(tp)
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 3f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	li	a0, 0
	csrc	sstatus, t6
	ret
3:	sd	t3, T_LOFAULT(tp)
	li	a0, EFAULT
	csrc	sstatus, t6
	ret
.L_flt_xcopyin:
	sd	t3, T_LOFAULT(tp)
	ld	t0, T_COPYOPS(tp)
	beqz	t0, 3b
	ld	t0, CP_XCOPYIN(t0);
	mv	a0, a5
	mv	a1, a6
	mv	a2, a7
	csrc	sstatus, t6
	jr	t0
	SET_SIZE(xcopyin_nta)
	SET_SIZE(xcopyin)

	ENTRY(copyin_noerr)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a0, t0, 2f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	csrc	sstatus, t6
	ret
	SET_SIZE(copyin_noerr)


/*
   int copyout(const void *kaddr, void *uaddr, size_t count)
   void copyout_noerr(const void *kaddr, void *uaddr, size_t count)
 */
	ENTRY(copyout)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	mv	a5, a0
	mv	a6, a1
	mv	a7, a2
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_copyout
	sd	t4, T_LOFAULT(tp)
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a1, t0, 3f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	li	a0, 0
	csrc	sstatus, t6
	ret
3:	sd	t3, T_LOFAULT(tp)
	li	a0, -1
	csrc	sstatus, t6
	ret
.L_flt_copyout:
	sd	t3, T_LOFAULT(tp)
	ld	t0, T_COPYOPS(tp)
	beqz	t0, 3b
	ld	t0, CP_COPYOUT(t0);
	mv	a0, a5
	mv	a1, a6
	mv	a2, a7
	csrc	sstatus, t6
	jr	t0
	SET_SIZE(copyout)

	ENTRY(xcopyout)
	ALTENTRY(xcopyout_nta)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	mv	a5, a0
	mv	a6, a1
	mv	a7, a2
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_xcopyout
	sd	t4, T_LOFAULT(tp)
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a1, t0, 3f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	li	a0, 0
	csrc	sstatus, t6
	ret
3:	sd	t3, T_LOFAULT(tp)
	li	a0, EFAULT
	csrc	sstatus, t6
	ret
.L_flt_xcopyout:
	sd	t3, T_LOFAULT(tp)
	ld	t0, T_COPYOPS(tp)
	beqz	t0, 3b
	ld	t0, CP_XCOPYOUT(t0);
	mv	a0, a5
	mv	a1, a6
	mv	a2, a7
	csrc	sstatus, t6
	jr	t0
	SET_SIZE(xcopyout_nta)
	SET_SIZE(xcopyout)

	ENTRY(copyout_noerr)
	li	t6, SSR_SUM
	csrs	sstatus, t6
	la	t0, kernelbase
	ld	t0, (t0)
1:	bgeu	a1, t0, 2f
	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	csrc	sstatus, t6
	ret
	SET_SIZE(copyout_noerr)

/*
   int kcopy(const void *from, void *to, size_t count)
 */
	ENTRY(kcopy)
	ALTENTRY(kcopy_nta)
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_kcopy
	sd	t4, T_LOFAULT(tp)
1:	beqz	a2, 2f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	li	a0, 0
	ret
.L_flt_kcopy:
	sd	t3, T_LOFAULT(tp)
	li	a0, EFAULT
	ret
	SET_SIZE(kcopy_nta)
	SET_SIZE(kcopy)

/*
   int kzero(void *addr, size_t count)
 */
	ENTRY(kzero)
	ld	t3, T_LOFAULT(tp)
	la	t4, .L_flt_kzero
	sd	t4, T_LOFAULT(tp)
1:	beqz	a1, 2f
	sb	zero, (a0)
	add	a0, a0, 1
	add	a1, a1, -1
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	li	a0, 0
	ret
.L_flt_kzero:
	sd	t3, T_LOFAULT(tp)
	li	a0, EFAULT
	ret
	SET_SIZE(kzero)


/*
   int copystr(const char *from, char *to, size_t maxlength, size_t *lencopied)
 */
	ENTRY(copystr)
	ld	t3, T_LOFAULT(tp)
	la	t4, 3f
	sd	t4, T_LOFAULT(tp)
	li	t2, 0
1:	beqz	a2, 4f
	lb	t1, (a0)
	sb	t1, (a1)
	add	a0, a0, 1
	add	a1, a1, 1
	add	a2, a2, -1
	add	t2, t2, 1
	beqz	t1, 2f
	j	1b
2:	sd	t3, T_LOFAULT(tp)
	li	a0, 0
10:	beqz	a3, 11f
	sd	t2, (a3)
11:	ret
3:	sd	t3, T_LOFAULT(tp)
	li	a0, EFAULT
	li	t2, 0
	j	10b
4:	sd	t3, T_LOFAULT(tp)
	li	a0, ENAMETOOLONG
	li	t2, 0
	j	10b
	SET_SIZE(copystr)

