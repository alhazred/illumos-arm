#include <sys/asm_linkage.h>
#include <sys/pal.h>

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL3LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */
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
 * M4 Parameters
 * __divl		name of function to generate
 * div		div=div: t10 / t11 -> t12; div=rem: t10 % t11 -> t12
 * true		true=true: signed; true=false: unsigned
 * 32	total number of bits
 */

LEAF(__divl, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)
	stq	t4, 32(sp)
	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */


	/* Compute sign of result.  If either is negative, this is easy.  */
	or	t10, t11, t4			/* not the sign, but... */
	srl	t4, 32 - 1, t4		/* rather, or of high bits */
	blbc	t4, L3doit			/* neither negative? do it! */

	xor	t10, t11, t4			/* THIS is the sign! */

	srl	t4, 32 - 1, t4		/* make negation the low bit. */

	srl	t10, 32 - 1, t1		/* is t10 negative? */
	blbc	t1, L3negB			/* no. */
	/* t10 is negative; flip it. */

	/* top 32 bits may be random junk */
	zap	t10, 0xf0, t10

	subq	zero, t10, t10
	srl	t11, 32 - 1, t1		/* is t11 negative? */
	blbc	t1, L3doit			/* no. */
L3negB:
	/* t11 is definitely negative, no matter how we got here. */

	/* top 32 bits may be random junk */
	zap	t11, 0xf0, t11

	subq	zero, t11, t11
L3doit:


	/*
	 * Clear the top 32 bits of each operand, as they may
	 * sign extension (if negated above), or random junk.
	 */
	zap	t10, 0xf0, t10
	zap	t11, 0xf0, t11


	/* kill the special cases. */
	beq	t11, L3dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L3ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L3ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L3Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<32-1 */
	mov	zero, t1
	sll	t3, 32-1, t0
L3Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L3Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 32-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L3Bloop

L3Abits:
	beq	t1, L3dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<32-1 */
	sll	t3, 32-1, t0

L3Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L3dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L3Aloop			/* If t1 != 0, loop again */

L3dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L3divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L3ret_result
	bne	t0, L3divloop

L3ret_result:


	/* Check to see if we should negate it. */
	subq	zero, t12, t3
	cmovlbs	t4, t3, t12


	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)
	ldq	t4, 32(sp)
	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L3dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap

	br	zero, L3ret_result

END(__divl)


/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL2LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */

/*
 * M4 Parameters
 * __divlu		name of function to generate
 * div		div=div: t10 / t11 -> t12; div=rem: t10 % t11 -> t12
 * false		false=true: signed; false=false: unsigned
 * 32	total number of bits
 */

LEAF(__divlu, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)

	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */



	/*
	 * Clear the top 32 bits of each operand, as they may
	 * sign extension (if negated above), or random junk.
	 */
	zap	t10, 0xf0, t10
	zap	t11, 0xf0, t11


	/* kill the special cases. */
	beq	t11, L2dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L2ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L2ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L2Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<32-1 */
	mov	zero, t1
	sll	t3, 32-1, t0
L2Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L2Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 32-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L2Bloop

L2Abits:
	beq	t1, L2dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<32-1 */
	sll	t3, 32-1, t0

L2Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L2dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L2Aloop			/* If t1 != 0, loop again */

L2dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L2divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L2ret_result
	bne	t0, L2divloop

L2ret_result:



	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)

	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L2dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap

	br	zero, L2ret_result

END(__divlu)


/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL1LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */

/*
 * M4 Parameters
 * __divq		name of function to generate
 * div		div=div: t10 / t11 -> t12; div=rem: t10 % t11 -> t12
 * true		true=true: signed; true=false: unsigned
 * 64	total number of bits
 */

LEAF(__divq, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)
	stq	t4, 32(sp)
	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */


	/* Compute sign of result.  If either is negative, this is easy.  */
	or	t10, t11, t4			/* not the sign, but... */
	srl	t4, 64 - 1, t4		/* rather, or of high bits */
	blbc	t4, L1doit			/* neither negative? do it! */

	xor	t10, t11, t4			/* THIS is the sign! */

	srl	t4, 64 - 1, t4		/* make negation the low bit. */

	srl	t10, 64 - 1, t1		/* is t10 negative? */
	blbc	t1, L1negB			/* no. */
	/* t10 is negative; flip it. */

	subq	zero, t10, t10
	srl	t11, 64 - 1, t1		/* is t11 negative? */
	blbc	t1, L1doit			/* no. */
L1negB:
	/* t11 is definitely negative, no matter how we got here. */

	subq	zero, t11, t11
L1doit:



	/* kill the special cases. */
	beq	t11, L1dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L1ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L1ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L1Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<64-1 */
	mov	zero, t1
	sll	t3, 64-1, t0
L1Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L1Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 64-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L1Bloop

L1Abits:
	beq	t1, L1dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<64-1 */
	sll	t3, 64-1, t0

L1Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L1dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L1Aloop			/* If t1 != 0, loop again */

L1dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L1divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L1ret_result
	bne	t0, L1divloop

L1ret_result:


	/* Check to see if we should negate it. */
	subq	zero, t12, t3
	cmovlbs	t4, t3, t12


	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)
	ldq	t4, 32(sp)
	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L1dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap

	br	zero, L1ret_result

END(__divq)


/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL0LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */

/*
 * M4 Parameters
 * __divqu		name of function to generate
 * div		div=div: t10 / t11 -> t12; div=rem: t10 % t11 -> t12
 * false		false=true: signed; false=false: unsigned
 * 64	total number of bits
 */

LEAF(__divqu, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)

	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */




	/* kill the special cases. */
	beq	t11, L0dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L0ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L0ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L0Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<64-1 */
	mov	zero, t1
	sll	t3, 64-1, t0
L0Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L0Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 64-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L0Bloop

L0Abits:
	beq	t1, L0dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<64-1 */
	sll	t3, 64-1, t0

L0Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L0dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L0Aloop			/* If t1 != 0, loop again */

L0dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L0divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L0ret_result
	bne	t0, L0divloop

L0ret_result:



	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)

	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L0dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap

	br	zero, L0ret_result

END(__divqu)


/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL7LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */

/*
 * M4 Parameters
 * __reml		name of function to generate
 * rem		rem=div: t10 / t11 -> t12; rem=rem: t10 % t11 -> t12
 * true		true=true: signed; true=false: unsigned
 * 32	total number of bits
 */

LEAF(__reml, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)
	stq	t4, 32(sp)
	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */


	/* Compute sign of result.  If either is negative, this is easy.  */
	or	t10, t11, t4			/* not the sign, but... */
	srl	t4, 32 - 1, t4		/* rather, or of high bits */
	blbc	t4, L7doit			/* neither negative? do it! */

	mov	t10, t4				/* sign follows t10. */

	srl	t4, 32 - 1, t4		/* make negation the low bit. */

	srl	t10, 32 - 1, t1		/* is t10 negative? */
	blbc	t1, L7negB			/* no. */
	/* t10 is negative; flip it. */

	/* top 32 bits may be random junk */
	zap	t10, 0xf0, t10

	subq	zero, t10, t10
	srl	t11, 32 - 1, t1		/* is t11 negative? */
	blbc	t1, L7doit			/* no. */
L7negB:
	/* t11 is definitely negative, no matter how we got here. */

	/* top 32 bits may be random junk */
	zap	t11, 0xf0, t11

	subq	zero, t11, t11
L7doit:


	/*
	 * Clear the top 32 bits of each operand, as they may
	 * sign extension (if negated above), or random junk.
	 */
	zap	t10, 0xf0, t10
	zap	t11, 0xf0, t11


	/* kill the special cases. */
	beq	t11, L7dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L7ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L7ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L7Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<32-1 */
	mov	zero, t1
	sll	t3, 32-1, t0
L7Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L7Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 32-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L7Bloop

L7Abits:
	beq	t1, L7dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<32-1 */
	sll	t3, 32-1, t0

L7Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L7dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L7Aloop			/* If t1 != 0, loop again */

L7dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L7divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L7ret_result
	bne	t0, L7divloop

L7ret_result:
	mov	t10, t12


	/* Check to see if we should negate it. */
	subq	zero, t12, t3
	cmovlbs	t4, t3, t12


	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)
	ldq	t4, 32(sp)
	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L7dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap
	mov	zero, t10			/* so that zero will be returned */

	br	zero, L7ret_result

END(__reml)


/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL6LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */

/*
 * M4 Parameters
 * __remlu		name of function to generate
 * rem		rem=div: t10 / t11 -> t12; rem=rem: t10 % t11 -> t12
 * false		false=true: signed; false=false: unsigned
 * 32	total number of bits
 */

LEAF(__remlu, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)

	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */



	/*
	 * Clear the top 32 bits of each operand, as they may
	 * sign extension (if negated above), or random junk.
	 */
	zap	t10, 0xf0, t10
	zap	t11, 0xf0, t11


	/* kill the special cases. */
	beq	t11, L6dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L6ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L6ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L6Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<32-1 */
	mov	zero, t1
	sll	t3, 32-1, t0
L6Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L6Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 32-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L6Bloop

L6Abits:
	beq	t1, L6dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<32-1 */
	sll	t3, 32-1, t0

L6Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L6dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L6Aloop			/* If t1 != 0, loop again */

L6dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L6divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L6ret_result
	bne	t0, L6divloop

L6ret_result:
	mov	t10, t12



	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)

	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L6dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap
	mov	zero, t10			/* so that zero will be returned */

	br	zero, L6ret_result

END(__remlu)


/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL5LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */

/*
 * M4 Parameters
 * __remq		name of function to generate
 * rem		rem=div: t10 / t11 -> t12; rem=rem: t10 % t11 -> t12
 * true		true=true: signed; true=false: unsigned
 * 64	total number of bits
 */

LEAF(__remq, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)
	stq	t4, 32(sp)
	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */


	/* Compute sign of result.  If either is negative, this is easy.  */
	or	t10, t11, t4			/* not the sign, but... */
	srl	t4, 64 - 1, t4		/* rather, or of high bits */
	blbc	t4, L5doit			/* neither negative? do it! */

	mov	t10, t4				/* sign follows t10. */

	srl	t4, 64 - 1, t4		/* make negation the low bit. */

	srl	t10, 64 - 1, t1		/* is t10 negative? */
	blbc	t1, L5negB			/* no. */
	/* t10 is negative; flip it. */

	subq	zero, t10, t10
	srl	t11, 64 - 1, t1		/* is t11 negative? */
	blbc	t1, L5doit			/* no. */
L5negB:
	/* t11 is definitely negative, no matter how we got here. */

	subq	zero, t11, t11
L5doit:



	/* kill the special cases. */
	beq	t11, L5dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L5ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L5ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L5Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<64-1 */
	mov	zero, t1
	sll	t3, 64-1, t0
L5Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L5Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 64-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L5Bloop

L5Abits:
	beq	t1, L5dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<64-1 */
	sll	t3, 64-1, t0

L5Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L5dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L5Aloop			/* If t1 != 0, loop again */

L5dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L5divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L5ret_result
	bne	t0, L5divloop

L5ret_result:
	mov	t10, t12


	/* Check to see if we should negate it. */
	subq	zero, t12, t3
	cmovlbs	t4, t3, t12


	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)
	ldq	t4, 32(sp)
	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L5dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap
	mov	zero, t10			/* so that zero will be returned */

	br	zero, L5ret_result

END(__remq)


/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MEL4LON DISCLAIMS ANY LIABILITY OF ANY KIND
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

/*
 * Division and remainder.
 *
 * The use of m4 is modeled after the sparc code, but the algorithm is
 * simple binary long division.
 *
 * Note that the loops could probably benefit from unrolling.
 */

/*
 * M4 Parameters
 * __remqu		name of function to generate
 * rem		rem=div: t10 / t11 -> t12; rem=rem: t10 % t11 -> t12
 * false		false=true: signed; false=false: unsigned
 * 64	total number of bits
 */

LEAF(__remqu, 0)					/* XXX */
	lda	sp, -64(sp)
	stq	t0, 0(sp)
	stq	t1, 8(sp)
	stq	t2, 16(sp)
	stq	t3, 24(sp)

	stq	t10, 40(sp)
	stq	t11, 48(sp)
	mov	zero, t12			/* Initialize result to zero */




	/* kill the special cases. */
	beq	t11, L4dotrap			/* division by zero! */

	cmpult	t10, t11, t2			/* t10 < t11? */
	/* t12 is already zero, from above.  t10 is untouched. */
	bne	t2, L4ret_result

	cmpeq	t10, t11, t2			/* t10 == t11? */
	cmovne	t2, 1, t12
	cmovne	t2, zero, t10
	bne	t2, L4ret_result

	/*
	 * Find out how many bits of zeros are at the beginning of the divisor.
	 */
L4Bbits:
	ldiq	t3, 1				/* t1 = 0; t0 = 1<<64-1 */
	mov	zero, t1
	sll	t3, 64-1, t0
L4Bloop:
	and	t11, t0, t2			/* if bit in t11 is set, done. */
	bne	t2, L4Abits
	addq	t1, 1, t1				/* increment t1,  bit */
	srl	t0, 1, t0
	cmplt	t1, 64-1, t2		/* if t1 leaves one bit, done. */
	bne	t2, L4Bloop

L4Abits:
	beq	t1, L4dodiv			/* If t1 = 0, divide now.  */
	ldiq	t3, 1				/* t0 = 1<<64-1 */
	sll	t3, 64-1, t0

L4Aloop:
	and	t10, t0, t2			/* if bit in t10 is set, done. */
	bne	t2, L4dodiv
	subq	t1, 1, t1				/* decrement t1,  bit */
	srl     t0, 1, t0
	bne	t1, L4Aloop			/* If t1 != 0, loop again */

L4dodiv:
	sll	t11, t1, t11				/* t11 <<= i */
	ldiq	t3, 1
	sll	t3, t1, t0

L4divloop:
	cmpult	t10, t11, t2
	or	t12, t0, t3
	cmoveq	t2, t3, t12
	subq	t10, t11, t3
	cmoveq	t2, t3, t10
	srl	t0, 1, t0
	srl	t11, 1, t11
	beq	t10, L4ret_result
	bne	t0, L4divloop

L4ret_result:
	mov	t10, t12



	ldq	t0, 0(sp)
	ldq	t1, 8(sp)
	ldq	t2, 16(sp)
	ldq	t3, 24(sp)

	ldq	t10, 40(sp)
	ldq	t11, 48(sp)
	lda	sp, 64(sp)
	ret	zero, (t9), 1

L4dotrap:
	ldiq	a0, -2			/* This is the signal to SIGFPE! */
	call_pal PAL_gentrap
	mov	zero, t10			/* so that zero will be returned */

	br	zero, L4ret_result

END(__remqu)
