/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Trevor Blackwell.  Support for use as memcpy() and memmove()
 *	   added by Chris Demetriou.
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include <sys/errno.h>
#include "assym.h"
/*
 * LEAF
 *      Declare a global leaf function.
 *      A leaf function does not call other functions AND does not
 *      use any register that is callee-saved AND does not modify
 *      the stack pointer.
 */
#define LEAF(_name_,_n_args_)                                   \
        .globl  _name_;                                         \
        .ent    _name_ 0;                                       \
_name_:;                                                        \
        .frame  sp,0,ra

/*
 * END
 *      Function delimiter
 */
#define END(_name_)                                             \
	.end    _name_

/*
 * RET
 *	Return from function
 */
#define	RET							\
	ret	zero,(ra),1

#if defined(MEMCOPY) || defined(MEMMOVE)
#ifdef MEMCOPY
#define	FUNCTION	memcpy
#else
#define FUNCTION	memmove
#endif
#define	SRCREG		a1
#define	DSTREG		a0
#else /* !(defined(MEMCOPY) || defined(MEMMOVE)) */
#define	FUNCTION	bcopy
#define	SRCREG		a0
#define	DSTREG		a1
#endif /* !(defined(MEMCOPY) || defined(MEMMOVE)) */

#define	SIZEREG		a2

/*
 * Copy bytes.
 *
 * void bcopy(char *from, char *to, size_t len);
 * char *memcpy(void *to, const void *from, size_t len);
 * char *memmove(void *to, const void *from, size_t len);
 *
 * No matter how invoked, the source and destination registers
 * for calculation.  There's no point in copying them to "working"
 * registers, since the code uses their values "in place," and
 * copying them would be slower.
 */

LEAF(FUNCTION,3)

#if defined(MEMCOPY) || defined(MEMMOVE)
	/* set up return value, while we still can */
	mov	DSTREG,v0
#endif

	/* Check for negative length */
	ble	SIZEREG,bcopy_done

	/* Check for overlap */
	subq	DSTREG,SRCREG,t5
	cmpult	t5,SIZEREG,t5
	bne	t5,bcopy_overlap

	/* a3 = end address */
	addq	SRCREG,SIZEREG,a3

	/* Get the first word */
	ldq_u	t2,0(SRCREG)

	/* Do they have the same alignment? */
	xor	SRCREG,DSTREG,t0
	and	t0,7,t0
	and	DSTREG,7,t1
	bne	t0,bcopy_different_alignment

	/* src & dst have same alignment */
	beq	t1,bcopy_all_aligned

	ldq_u	t3,0(DSTREG)
	addq	SIZEREG,t1,SIZEREG
	mskqh	t2,SRCREG,t2
	mskql	t3,SRCREG,t3
	or	t2,t3,t2

	/* Dst is 8-byte aligned */

bcopy_all_aligned:
	/* If less than 8 bytes,skip loop */
	subq	SIZEREG,1,t0
	and	SIZEREG,7,SIZEREG
	bic	t0,7,t0
	beq	t0,bcopy_samealign_lp_end

bcopy_samealign_lp:
	stq_u	t2,0(DSTREG)
	addq	DSTREG,8,DSTREG
	ldq_u	t2,8(SRCREG)
	subq	t0,8,t0
	addq	SRCREG,8,SRCREG
	bne	t0,bcopy_samealign_lp

bcopy_samealign_lp_end:
	/* If we're done, exit */
	bne	SIZEREG,bcopy_small_left
	stq_u	t2,0(DSTREG)
	RET

bcopy_small_left:
	mskql	t2,SIZEREG,t4
	ldq_u	t3,0(DSTREG)
	mskqh	t3,SIZEREG,t3
	or	t4,t3,t4
	stq_u	t4,0(DSTREG)
	RET

bcopy_different_alignment:
	/*
	 * this is the fun part
	 */
	addq	SRCREG,SIZEREG,a3
	cmpule	SIZEREG,8,t0
	bne	t0,bcopy_da_finish

	beq	t1,bcopy_da_noentry

	/* Do the initial partial word */
	subq	zero,DSTREG,t0
	and	t0,7,t0
	ldq_u	t3,7(SRCREG)
	extql	t2,SRCREG,t2
	extqh	t3,SRCREG,t3
	or	t2,t3,t5
	insql	t5,DSTREG,t5
	ldq_u	t6,0(DSTREG)
	mskql	t6,DSTREG,t6
	or	t5,t6,t5
	stq_u	t5,0(DSTREG)
	addq	SRCREG,t0,SRCREG
	addq	DSTREG,t0,DSTREG
	subq	SIZEREG,t0,SIZEREG
	ldq_u	t2,0(SRCREG)

bcopy_da_noentry:
	subq	SIZEREG,1,t0
	bic	t0,7,t0
	and	SIZEREG,7,SIZEREG
	beq	t0,bcopy_da_finish2

bcopy_da_lp:
	ldq_u	t3,7(SRCREG)
	addq	SRCREG,8,SRCREG
	extql	t2,SRCREG,t4
	extqh	t3,SRCREG,t5
	subq	t0,8,t0
	or	t4,t5,t5
	stq	t5,0(DSTREG)
	addq	DSTREG,8,DSTREG
	beq	t0,bcopy_da_finish1
	ldq_u	t2,7(SRCREG)
	addq	SRCREG,8,SRCREG
	extql	t3,SRCREG,t4
	extqh	t2,SRCREG,t5
	subq	t0,8,t0
	or	t4,t5,t5
	stq	t5,0(DSTREG)
	addq	DSTREG,8,DSTREG
	bne	t0,bcopy_da_lp

bcopy_da_finish2:
	/* Do the last new word */
	mov	t2,t3

bcopy_da_finish1:
	/* Do the last partial word */
	ldq_u	t2,-1(a3)
	extql	t3,SRCREG,t3
	extqh	t2,SRCREG,t2
	or	t2,t3,t2
	br	zero,bcopy_samealign_lp_end

bcopy_da_finish:
	/* Do the last word in the next source word */
	ldq_u	t3,-1(a3)
	extql	t2,SRCREG,t2
	extqh	t3,SRCREG,t3
	or	t2,t3,t2
	insqh	t2,DSTREG,t3
	insql	t2,DSTREG,t2
	lda	t4,-1(zero)
	mskql	t4,SIZEREG,t5
	cmovne	t5,t5,t4
	insqh	t4,DSTREG,t5
	insql	t4,DSTREG,t4
	addq	DSTREG,SIZEREG,a4
	ldq_u	t6,0(DSTREG)
	ldq_u	t7,-1(a4)
	bic	t6,t4,t6
	bic	t7,t5,t7
	and	t2,t4,t2
	and	t3,t5,t3
	or	t2,t6,t2
	or	t3,t7,t3
	stq_u	t3,-1(a4)
	stq_u	t2,0(DSTREG)
	RET

bcopy_overlap:
	/*
	 * Basically equivalent to previous case, only backwards.
	 * Not quite as highly optimized
	 */
	addq	SRCREG,SIZEREG,a3
	addq	DSTREG,SIZEREG,a4

	/* less than 8 bytes - don't worry about overlap */
	cmpule	SIZEREG,8,t0
	bne	t0,bcopy_ov_short

	/* Possibly do a partial first word */
	and	a4,7,t4
	beq	t4,bcopy_ov_nostart2
	subq	a3,t4,a3
	subq	a4,t4,a4
	ldq_u	t1,0(a3)
	subq	SIZEREG,t4,SIZEREG
	ldq_u	t2,7(a3)
	ldq	t3,0(a4)
	extql	t1,a3,t1
	extqh	t2,a3,t2
	or	t1,t2,t1
	mskqh	t3,t4,t3
	mskql	t1,t4,t1
	or	t1,t3,t1
	stq	t1,0(a4)

bcopy_ov_nostart2:
	bic	SIZEREG,7,t4
	and	SIZEREG,7,SIZEREG
	beq	t4,bcopy_ov_lp_end

bcopy_ov_lp:
	/* This could be more pipelined, but it doesn't seem worth it */
	ldq_u	t0,-8(a3)
	subq	a4,8,a4
	ldq_u	t1,-1(a3)
	subq	a3,8,a3
	extql	t0,a3,t0
	extqh	t1,a3,t1
	subq	t4,8,t4
	or	t0,t1,t0
	stq	t0,0(a4)
	bne	t4,bcopy_ov_lp

bcopy_ov_lp_end:
	beq	SIZEREG,bcopy_done

	ldq_u	t0,0(SRCREG)
	ldq_u	t1,7(SRCREG)
	ldq_u	t2,0(DSTREG)
	extql	t0,SRCREG,t0
	extqh	t1,SRCREG,t1
	or	t0,t1,t0
	insql	t0,DSTREG,t0
	mskql	t2,DSTREG,t2
	or	t2,t0,t2
	stq_u	t2,0(DSTREG)

bcopy_done:
	RET

bcopy_ov_short:
	ldq_u	t2,0(SRCREG)
	br	zero,bcopy_da_finish

	END(FUNCTION)

#if !defined(MEMCOPY) && !defined(MEMMOVE)

/*
 * int kcopy(const void *from, void *to, size_t count)
 */
	ENTRY(kcopy)
	ALTENTRY(kcopy_nta)
	LDGP(pv)
	lda	sp, -4*8(sp)
	stq	ra, 0*8(sp)
	stq	s0, 1*8(sp)
	stq	s1, 2*8(sp)

	call_pal PAL_rdval
	ldq	s0, CPU_THREAD(v0)	// s0 <- curthread()
	ldq	s1, T_LOFAULT(s0)

	br	t0, Ldo_kcopy

Lkcopy_err:
	ldiq	v0, EFAULT
	stq	s1, T_LOFAULT(s0)
	br	zero, Ldone_kcopy

Ldo_kcopy:
	stq	t0, T_LOFAULT(s0)
	br	ra, bcopy
	stq	s1, T_LOFAULT(s0)
	lda	v0, 0(zero)

Ldone_kcopy:
	ldq	s1, 2*8(sp)
	ldq	s0, 1*8(sp)
	ldq	ra, 0*8(sp)
	lda	sp, 4*8(sp)
	ret
	SET_SIZE(kcopy_nta)
	SET_SIZE(kcopy)

/*
 * int copyin(const void *uaddr, void *kaddr, size_t count)
 * int copyout(const void *kaddr, void *uaddr, size_t count)
 */
#define COPYINOUT(name, COPYOP, err)				\
	ENTRY(name);						\
	LDGP(pv);						\
	lda	sp, -6*8(sp);					\
	stq	ra, 0*8(sp);					\
	stq	s0, 1*8(sp);					\
	stq	s1, 2*8(sp);					\
	stq	a0, 3*8(sp);					\
	stq	a1, 4*8(sp);					\
	stq	a2, 5*8(sp);					\
	call_pal PAL_rdval;					\
	ldq	s0, CPU_THREAD(v0);	/* s0 <- curthread() */	\
	ldq	s1, T_LOFAULT(s0);				\
	br	t0, 2f;						\
	stq	s1, T_LOFAULT(s0);	/* <- error handler */	\
	ldiq	v0, err;					\
	ldq	t0, T_COPYOPS(s0);				\
	beq	t0, 3f;						\
	ldq	a2, 5*8(sp);					\
	ldq	a1, 4*8(sp);					\
	ldq	a0, 3*8(sp);					\
	ldq	s1, 2*8(sp);					\
	ldq	s0, 1*8(sp);					\
	ldq	ra, 0*8(sp);					\
	lda	sp, 6*8(sp);					\
	ldq	pv, COPYOP(t0);					\
	jsr	zero, (pv);					\
2:	stq	t0, T_LOFAULT(s0);				\
	br	ra, bcopy;					\
	stq	s1, T_LOFAULT(s0);				\
	lda	v0, 0(zero);					\
3:	ldq	s1, 2*8(sp);					\
	ldq	s0, 1*8(sp);					\
	ldq	ra, 0*8(sp);					\
	lda	sp, 6*8(sp);					\
	ret;							\
	SET_SIZE(name)
COPYINOUT(copyin, CP_COPYIN, -1);
COPYINOUT(copyout, CP_COPYOUT, -1);
COPYINOUT(xcopyin, CP_XCOPYIN, EFAULT);
COPYINOUT(xcopyout, CP_XCOPYOUT, EFAULT);
	.weak xcopyin_nta
	.weak xcopyout_nta
xcopyin_nta = xcopyin
xcopyout_nta = xcopyout

/*
 * void
 * copyin_noerr(const void *uaddr, void *kaddr, size_t count)
 * void
 * copyout_noerr(const void *kaddr, void *uaddr, size_t count)
 */
	.weak copyin_noerr
	.weak copyout_noerr
	.weak ucopy
	.weak ovbcopy
copyin_noerr = bcopy
copyout_noerr = bcopy
ucopy = bcopy
ovbcopy = bcopy

/*
 * int
 * copystr(const char *from, char *to, size_t maxlength, size_t *lencopied)
 */
	.align 4
_copystr:
	mov	a2, t0			/* t0 = i = len */
	bne	a2, 1f			/* if (len != 0), proceed */
	ldiq	t1, 1			/* else bail */
	br	zero, 2f
1:	ldq_u	t1, 0(a0)		/* t1 = *from */
	extbl	t1, a0, t1
	ldq_u	t3, 0(a1)		/* set up t2 with quad around *to */
	insbl	t1, a1, t2
	mskbl	t3, a1, t3
	or	t3, t2, t3		/* add *from to quad around *to */
	stq_u	t3, 0(a1)		/* write out that quad */
	subl	a2, 1, a2		/* len-- */
	beq	t1, 2f			/* if (*from == 0), bail out */
	addq	a1, 1, a1		/* to++ */
	addq	a0, 1, a0		/* from++ */
	bne	a2, 1b			/* if (len != 0) copy more */
2:	beq	a3, 3f			/* if (lenp != NULL) */
	subl	t0, a2, t0		/* *lenp = (i - len) */
	stq	t0, 0(a3)
3:	beq	t1, 4f			/* *from == '\0'; leave quietly */
	ldiq	v0, ENAMETOOLONG	/* *from != '\0'; error. */
	ret
4:	mov	zero, v0		/* return 0. */
	ret

	ENTRY(copystr)
	LDGP(pv)
	lda	sp, -4*8(sp)
	stq	ra, 0*8(sp)
	stq	s0, 1*8(sp)
	stq	s1, 2*8(sp)

	call_pal PAL_rdval
	ldq	s0, CPU_THREAD(v0)	// s0 <- curthread()
	ldq	s1, T_LOFAULT(s0)

	br	t0, Ldo_copystr

Lcopystr_err:
	stq	s1, T_LOFAULT(s0)
	ldiq	v0, EFAULT
	br	zero, Ldone_copystr

Ldo_copystr:
	stq	t0, T_LOFAULT(s0)
	br	ra, _copystr
	stq	s1, T_LOFAULT(s0)

Ldone_copystr:
	ldq	s1, 2*8(sp)
	ldq	s0, 1*8(sp)
	ldq	ra, 0*8(sp)
	lda	sp, 4*8(sp)
	ret
	SET_SIZE(copystr)

#define COPYINOUTSTR(name, COPYOP)				\
	ENTRY(name);						\
	LDGP(pv);						\
	lda	sp, -8*8(sp);					\
	stq	ra, 0*8(sp);					\
	stq	s0, 1*8(sp);					\
	stq	s1, 2*8(sp);					\
	stq	a0, 3*8(sp);					\
	stq	a1, 4*8(sp);					\
	stq	a2, 5*8(sp);					\
	stq	a3, 6*8(sp);					\
	call_pal PAL_rdval;					\
	ldq	s0, CPU_THREAD(v0);	/* s0 <- curthread() */	\
	ldq	s1, T_LOFAULT(s0);				\
	br	t0, 2f;						\
	stq	s1, T_LOFAULT(s0);	/* <- error handler */	\
	ldiq	v0, EFAULT;					\
	ldq	t0, T_COPYOPS(s0);				\
	beq	t0, 3f;						\
	ldq	a3, 6*8(sp);					\
	ldq	a2, 5*8(sp);					\
	ldq	a1, 4*8(sp);					\
	ldq	a0, 3*8(sp);					\
	ldq	s1, 2*8(sp);					\
	ldq	s0, 1*8(sp);					\
	ldq	ra, 0*8(sp);					\
	lda	sp, 8*8(sp);					\
	ldq	pv, COPYOP(t0);					\
	jsr	zero, (pv);					\
2:	stq	t0, T_LOFAULT(s0);				\
	br	ra, _copystr;					\
	stq	s1, T_LOFAULT(s0);				\
3:	ldq	s1, 2*8(sp);					\
	ldq	s0, 1*8(sp);					\
	ldq	ra, 0*8(sp);					\
	lda	sp, 8*8(sp);					\
	ret;							\
	SET_SIZE(name)
COPYINOUTSTR(copyinstr, CP_COPYINSTR);
COPYINOUTSTR(copyoutstr, CP_COPYOUTSTR);

	ENTRY(copyinstr_noerr)
	br	_copystr
	SET_SIZE(copyinstr_noerr)

	ENTRY(copyoutstr_noerr)
	br	_copystr
	SET_SIZE(copyoutstr_noerr)

	ENTRY(ucopystr)
	br	_copystr
	SET_SIZE(ucopystr)

#define FUWORD(name,ld,st,COPYOP) \
	ENTRY(name);							\
	call_pal PAL_rdval;						\
	ldq	v0, CPU_THREAD(v0);	/* v0 <- curthread() */		\
	ldq	t0, T_LOFAULT(v0);	/* Save existing handler */	\
	bsr	t1, 2f;							\
	stq	t0, T_LOFAULT(v0);	/* Restore lofault handler */	\
	ldq	t0, T_COPYOPS(v0);					\
	beq	t0, 1f;							\
	ldq	pv, COPYOP(t0);						\
	jsr	zero, (pv);						\
1:	lda	v0, -1;							\
	ret;								\
2:	stq	t1, T_LOFAULT(v0);	/* Put error in t_lofault */	\
	ld	t2, 0(a0);						\
	st	t2, 0(a1);						\
	stq	t0, T_LOFAULT(v0);	/* Restore lofault handler */	\
	lda	v0, 0;							\
	ret;								\
	SET_SIZE(name);

#define FUWORD_NOERROR(name,ld,st) \
	ENTRY(name);							\
	ld	t2, 0(a0);						\
	st	t2, 0(a1);						\
	ret;								\
	SET_SIZE(name);

#define SUWORD(name,st,COPYOP) \
	ENTRY(name);							\
	call_pal PAL_rdval;						\
	ldq	v0, CPU_THREAD(v0);	/* v0 <- curthread() */		\
	ldq	t0, T_LOFAULT(v0);	/* Save existing handler */	\
	bsr	t1, 2f;							\
	stq	t0, T_LOFAULT(v0);	/* Restore lofault handler */	\
	ldq	t0, T_COPYOPS(v0);					\
	beq	t0, 1f;							\
	ldq	pv, COPYOP(t0);						\
	jsr	zero, (pv);						\
1:	lda	v0, -1;							\
	ret;								\
2:	stq	t1, T_LOFAULT(v0);	/* Put error in t_lofault */	\
	st	a1, 0(a0);						\
	stq	t0, T_LOFAULT(v0);	/* Restore lofault handler */	\
	lda	v0, 0;							\
	ret;								\
	SET_SIZE(name);

#define SUWORD_NOERROR(name,st) \
	ENTRY(name);							\
	st	a1, 0(a0);						\
	ret;								\
	SET_SIZE(name);

FUWORD(fuword64,ldq,stq,CP_FUWORD64)
FUWORD(fuword32,ldl,stl,CP_FUWORD32)
FUWORD(fuword16,ldwu,stw,CP_FUWORD16)
FUWORD(fuword8,ldbu,stb,CP_FUWORD8)
FUWORD_NOERROR(fuword64_noerr,ldq,stq)
FUWORD_NOERROR(fuword32_noerr,ldl,stl)
FUWORD_NOERROR(fuword16_noerr,ldwu,stw)
FUWORD_NOERROR(fuword8_noerr,ldbu,stb)

SUWORD(suword64,stq,CP_SUWORD64)
SUWORD(suword32,stl,CP_SUWORD32)
SUWORD(suword16,stw,CP_SUWORD16)
SUWORD(suword8,stb,CP_SUWORD8)
SUWORD_NOERROR(suword64_noerr,stq)
SUWORD_NOERROR(suword32_noerr,stl)
SUWORD_NOERROR(suword16_noerr,stw)
SUWORD_NOERROR(suword8_noerr,stb)

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



	#endif
