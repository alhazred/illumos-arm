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
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sys/pal.h>
#include <sys/lockstat.h>
#include "assym.h"

	.text

#define BR_LOCKSTAT_WRAPPER(lp, lsid) \
	mov	lp, a1; \
	mov	lsid, a0; \
	call_pal PAL_rdval; \
	ldq	a2, CPU_THREAD(v0); \
	br	lockstat_wrapper

#define BR_LOCKSTAT_WRAPPER_ARG0(lp, lsid, arg0) \
	mov	lp, a1; \
	mov	lsid, a0; \
	mov	arg0, a2; \
	call_pal PAL_rdval; \
	ldq	a3, CPU_THREAD(v0); \
	br	lockstat_wrapper_arg0

/*
 * int
 * lock_try(lock_t *lp)
 *	Try to acquire lock.
 *	0xFF in lock_t means the lock is busy.
 *
 * Calling/Exit State:
 *	Upon successful completion, lock_try() returns non-zero value.
 *	lock_try() doesn't block interrupts so don't use this to spin
 *	on a lock.
 */
	ENTRY(lock_try)
	mov	LOCK_HELD_VALUE, a1
	insbl	a1, a0, a1
	andnot	a0, 0x7, t0
.lock_try_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	extbl	t2, a0, v0
	xor	v0, LOCK_HELD_VALUE, v0
	mb
.lock_try_lockstat_patch_point:
	ret
	beq	v0, .lock_try_lockstat_fail
	BR_LOCKSTAT_WRAPPER(a0, LS_LOCK_TRY_ACQUIRE)
.lock_try_lockstat_fail:
	ret
1:	br	zero, .lock_try_retry
	SET_SIZE(lock_try)

/*
 * int
 * lock_spin_try(lock_t *lp)
 *	Same as lock_try(), but it has no patch point for dtrace.
 */
	ENTRY(ulock_try)
	mov	1, a1
	insbl	a1, a0, a1
	andnot	a0, 0x7, t0
.ulock_try_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	extbl	t2, a0, v0
	xor	v0, 1, v0
	mb
	ret
1:	br	zero, .ulock_try_retry
	SET_SIZE(ulock_try)

/*
 * void
 * ulock_clear(ulock_t *lp)
 *	Release lock without changing interrupt priority level.
 */
	ENTRY(ulock_clear)
	mb
	andnot	a0, 0x7, t0
.ulock_clear_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	ret
1:	br	zero, .ulock_clear_retry
	SET_SIZE(ulock_clear)

/*
 * int
 * lock_spin_try(lock_t *lp)
 *	Same as lock_try(), but it has no patch point for dtrace.
 */
	ENTRY(lock_spin_try)
	mov	LOCK_HELD_VALUE, a1
	insbl	a1, a0, a1
	andnot	a0, 0x7, t0
.lock_spin_try_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	extbl	t2, a0, v0
	xor	v0, LOCK_HELD_VALUE, v0
	mb
	ret
1:	br	zero, .lock_spin_try_retry
	SET_SIZE(lock_spin_try)

/*
 * void
 * lock_clear(lock_t *lp)
 *	Release lock without changing interrupt priority level.
 */
	ENTRY(lock_clear)
	mb
	andnot	a0, 0x7, t0
.lock_clear_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
.lock_clear_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER(a0, LS_LOCK_CLEAR_RELEASE)
1:	br	zero, .lock_clear_retry
	SET_SIZE(lock_clear)


/*
 * void
 * lock_set_spl(lock_t *lp, int new_pil, u_short *old_pil_addr)
 *	Acquire lock, and set interrupt priority to new_pil.
 *	Old priority is stored in *old_pil.
 */
	ENTRY(lock_set_spl)
	LDGP(pv)
	lda	sp, -4*8(sp)
	stq	ra, 0*8(sp)
	stq	a0, 1*8(sp)
	stq	a1, 2*8(sp)
	stq	a2, 3*8(sp)
	mov	a1, a0
	CALL(splr)

	ldq	a0, 1*8(sp)
	mov	LOCK_HELD_VALUE, a1
	insbl	a1, a0, a1
	andnot	a0, 0x7, t0
.lock_set_spl_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	extbl	t2, a0, t2
	bne	t2, .lock_set_spl_miss

	// success
	ldq	a2, 3*8(sp)
	stw	v0, 0(a2)	// store old pil
	mb
	// restore stack
	ldq	ra, 0*8(sp)
	lda	sp, 4*8(sp)
.lock_set_spl_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER(a0, LS_LOCK_SET_SPL_ACQUIRE)
.lock_set_spl_miss:
	ldq	ra, 0*8(sp)
	ldq	a0, 1*8(sp)
	ldq	a1, 2*8(sp)
	ldq	a2, 3*8(sp)
	lda	sp, 4*8(sp)
	mov	v0, a3
	lda	pv, lock_set_spl_spin
	jsr	zero, (pv)
1:	br	zero, .lock_set_spl_retry
	SET_SIZE(lock_set_spl)


/*
 * void
 * lock_init(lock_t *lp)
 *	Initialize lock.
 */
	ENTRY(lock_init)
	stb	zero, 0(a0)	/* ev56 or later */
	ret
	SET_SIZE(lock_init)


/*
 * void
 * lock_set(lp)
 *	Acquire spin lock.
 */
	ENTRY(lock_set)
	LDGP(pv)
	mov	LOCK_HELD_VALUE, a1
	insbl	a1, a0, a1
	andnot	a0, 0x7, t0
.lock_set_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	or	t1, a1, t1
	stq_c	t1, 0(t0)
	beq	t1, .lock_set_retry
	extbl	t2, a0, t2
	bne	t2, .lock_set_miss
	mb
.lock_set_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER(a0, LS_LOCK_SET_ACQUIRE)
.lock_set_miss:
	lda	pv, lock_set_spin
	jsr	zero, (pv)
	SET_SIZE(lock_set)


/*
 * void
 * lock_clear_splx(lock_t *lp, int s)
 *	Release lock and restore PIL.
 */
	ENTRY(lock_clear_splx)
	LDGP(pv)
	mb
	andnot	a0, 0x7, t0
.lock_clear_splx_retry:
	ldq_l	t2, 0(t0)
	mskbl	t2, a0, t1
	stq_c	t1, 0(t0)
	beq	t1, 1f
	lda	sp, -2*8(sp)
	stq	ra, 0*8(sp)
	stq	a0, 1*8(sp)
	mov	a1, a0
	CALL(splx)
	ldq	a0, 1*8(sp)
	ldq	ra, 0*8(sp)
	lda	sp, 2*8(sp)
.lock_clear_splx_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER(a0, LS_LOCK_CLEAR_SPLX_RELEASE)
1:	br	zero, .lock_clear_splx_retry
	SET_SIZE(lock_clear_splx)


	ENTRY(membar_enter)
	ALTENTRY(membar_sync)
	ALTENTRY(membar_exit)
	ALTENTRY(membar_producer)
	ALTENTRY(membar_consumer)
	mb
	ret
	SET_SIZE(membar_consumer)
	SET_SIZE(membar_producer)
	SET_SIZE(membar_exit)
	SET_SIZE(membar_sync)
	SET_SIZE(membar_enter)

/*
 * void mutex_enter(kmutex_t *lp)
 */
	ENTRY(mutex_enter)
	LDGP(pv)
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)
.mutex_enter_retry:
	mov	v0, t1
	ldq_l	t0, 0(a0)
	bne	t0, .mutex_enter_fail
	stq_c	t1, 0(a0)		/* attempt to store */
	beq	t1, 1f			/* if it failed, spin */
	mb
.mutex_enter_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER(a0, LS_MUTEX_ENTER_ACQUIRE)
.mutex_enter_fail:
	lda	pv, mutex_vector_enter
	jsr	zero, (pv)
1:	br	zero, .mutex_enter_retry
	SET_SIZE(mutex_enter)


/*
 * int mutex_tryenter(kmutex_t *lp)
 */
	ENTRY(mutex_tryenter)
	LDGP(pv)
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)
.mutex_tryenter_retry:
	mov	v0, t1
	ldq_l	t0, 0(a0)
	bne	t0, .mutex_tryenter_fail
	stq_c	t1, 0(a0)		/* attempt to store */
	beq	t1, 1f			/* if it failed, spin */
	mov	1, v0
	mb
.mutex_tryenter_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER(a0, LS_MUTEX_ENTER_ACQUIRE)
.mutex_tryenter_fail:
	lda	pv, mutex_vector_tryenter
	jsr	zero, (pv)
1:	br	zero, .mutex_tryenter_retry
	SET_SIZE(mutex_tryenter)


/*
 * int mutex_adaptive_tryenter(kmutex_t *lp)
 */
	ENTRY(mutex_adaptive_tryenter)
	LDGP(pv)
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)
.mutex_adaptive_tryenter_retry:
	mov	v0, t1
	ldq_l	t0, 0(a0)
	bne	t0, .mutex_adaptive_tryenter_fail
	stq_c	t1, 0(a0)		/* attempt to store */
	beq	t1, 1f			/* if it failed, spin */
	mov	1, v0
	mb
	ret
.mutex_adaptive_tryenter_fail:
	mov	zero, v0
	ret
1:	br	zero, .mutex_adaptive_tryenter_retry
	SET_SIZE(mutex_adaptive_tryenter)


	.globl	mutex_owner_running_critical_start
/*
 * void *mutex_owner_running(mutex_impl_t *);
 */
	ENTRY(mutex_owner_running)
	LDGP(pv)
mutex_owner_running_critical_start:
	ldq	v0, 0(a0)
	lda	t0, MUTEX_THREAD(zero)
	and	v0, t0, v0
	beq	v0, .mutex_owner_running_no_running
	ldq	t0, T_CPU(v0)		/* get owner->t_cpu */
	ldq	t1, CPU_THREAD(t0)	/* get t_cpu->cpu_thread */
.mutex_owner_running_critical_end:
	xor	t1, v0, t0		/* owner == running thread? */
	beq	t0, .mutex_owner_running_running	/* yes, go return cpu */
	mov	zero, v0
.mutex_owner_running_no_running:
.mutex_owner_running_running:
	ret
	SET_SIZE(mutex_owner_running)

	.globl	mutex_owner_running_critical_size
	.type	mutex_owner_running_critical_size, @object
mutex_owner_running_critical_size:
	.quad	.mutex_owner_running_critical_end - mutex_owner_running_critical_start
	SET_SIZE(mutex_owner_running_critical_size)

	.globl	mutex_exit_critical_start
/*
 * void mutex_exit(kmutex_t *lp)
 */
	ENTRY(mutex_exit)
	LDGP(pv)
mutex_exit_critical_start:		/* If interrupted, restart here */
	mb
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)
	ldq	t0, 0(a0)
	cmpeq	v0, t0, t0
	beq	t0, .mutex_exit_fail	/* if t0 != v0 then jmp */
	stq	zero, 0(a0)		/* attempt to store */
.mutex_exit_critical_end:
.mutex_exit_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER(a0, LS_MUTEX_EXIT_RELEASE)
.mutex_exit_fail:
	lda	pv, mutex_vector_exit
	jsr	zero, (pv)
	SET_SIZE(mutex_exit)

	.globl	mutex_exit_critical_size
	.type	mutex_exit_critical_size, @object
mutex_exit_critical_size:
	.quad	.mutex_exit_critical_end - mutex_exit_critical_start
	SET_SIZE(mutex_exit_critical_size)

/*
 * void
 * rw_enter(krwlock_t *lp, krw_t rw)
 *	Acquire read/write lock as the specified mode.
 */
	ENTRY(rw_enter)
	LDGP(pv)
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)
	xor	a1, RW_WRITER, t0
	beq	t0, .rw_write_enter
.rw_read_enter_retry:
	ldq_l	t0, 0(a0)		/* r2 = lp->rw_wwwh */
	and	t0, RW_WRITE_LOCKED|RW_WRITE_WANTED, t1
	bne	t1, .rw_enter_sleep
	addq	t0, RW_READ_LOCK, t0	/* Increment hold count */
	stq_c	t0, 0(a0)		/* Install new rw_wwwh */
	beq	t0, .rw_read_enter_retry
	mb
.rw_read_enter_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER_ARG0(a0, LS_RW_ENTER_ACQUIRE, RW_READER)
.rw_write_enter:
.rw_write_enter_retry:
	or	v0, RW_WRITE_LOCKED, t1
	ldq_l	t0, 0(a0)		/* r2 = lp->rw_wwwh */
	bne	t0, .rw_enter_sleep
	stq_c	t1, 0(a0)		/* Install new rw_wwwh */
	beq	t1, .rw_write_enter_retry
	mb
.rw_write_enter_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER_ARG0(a0, LS_RW_ENTER_ACQUIRE, RW_WRITER)
.rw_enter_sleep:
	lda	pv, rw_enter_sleep
	jsr	zero, (pv)
	SET_SIZE(rw_enter)


/*
 * void
 * rw_exit(krwlock_t *lp)
 *	Release read/write lock.
 */
	ENTRY(rw_exit)
	LDGP(pv)
	mb
	ldq	t0, 0(a0)
	subq	t0, RW_READ_LOCK, t1
	bne	t1, .rw_not_single_reader
.rw_read_exit_retry:
	mov	t1, t2
	ldq_l	t3, 0(a0)
	xor	t3, t0, t3
	bne	t3, .rw_exit_wakeup
	stq_c	t2, 0(a0)
	beq	t2, .rw_read_exit_retry
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)
.rw_read_exit_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER_ARG0(a0, LS_RW_EXIT_RELEASE, RW_READER)
.rw_not_single_reader:
	and	t0, RW_WRITE_LOCKED, t2
	bne	t2, .rw_write_exit
	cmplt	t1, RW_READ_LOCK, t2
	beq	t2, .rw_read_exit_retry
	br	.rw_exit_wakeup
.rw_write_exit:
	call_pal PAL_rdval
	ldq	v0, CPU_THREAD(v0)
	or	v0, RW_WRITE_LOCKED, t1
.rw_write_exit_retry:
	ldq_l	t0, 0(a0)		/* r2 = lp->rw_wwwh */
	xor	t0, t1, t0
	bne	t0, .rw_exit_wakeup
	stq_c	t0, 0(a0)		/* r2 = lp->rw_wwwh */
	beq	t0, .rw_write_exit_retry
.rw_write_exit_lockstat_patch_point:
	ret
	BR_LOCKSTAT_WRAPPER_ARG0(a0, LS_RW_EXIT_RELEASE, RW_WRITER)
.rw_exit_wakeup:
	lda	pv, rw_exit_wakeup
	jsr	zero, (pv)
	SET_SIZE(rw_exit)


/*
 * expects a0=lockstat event, a1=lock, a2=curthread()
 */
	ENTRY(lockstat_wrapper)
	lda	sp, -2*8(sp)
	stq	ra, 0*8(sp)
	ldbu	t0, T_LOCKSTAT(a2)		/* curthread->t_lockstat++ */
	addq	t0, 1, t0
	stb	t0, T_LOCKSTAT(a2)

	lda	t0, lockstat_probemap	/* t0 = lockstat_probemap */
	s4addq	a0, t0, t0		/* t0 += 4 * a0 */
	ldl	t0, 0(t0)
	beq	t0, .lockstat_wrapper_skip

	stq	a2, 1*8(sp)
	lda	t0, lockstat_probe
	ldq	pv, 0(t0)
	jsr	ra, (pv)
	ldq	a2, 1*8(sp)

.lockstat_wrapper_skip:
	ldbu	t0, T_LOCKSTAT(a2)		/* curthread->t_lockstat-- */
	subq	t0, 1, t0
	stb	t0, T_LOCKSTAT(a2)
	ldq	ra, 0*8(sp)
	lda	sp, 2*8(sp)
	ret
	SET_SIZE(lockstat_wrapper)

/*
 * expects a0=lockstat event, a1=lock, a2=curthread()
 */
	ENTRY(lockstat_wrapper_arg0)
	lda	sp, -2*8(sp)
	stq	ra, 0*8(sp)
	ldbu	t0, T_LOCKSTAT(a3)		/* curthread->t_lockstat++ */
	addq	t0, 1, t0
	stb	t0, T_LOCKSTAT(a3)

	lda	t0, lockstat_probemap	/* t0 = lockstat_probemap */
	s4addq	a0, t0, t0		/* t0 += 4 * a0 */
	ldl	t0, 0(t0)
	beq	t0, .lockstat_wrapper_arg0_skip

	stq	a3, 1*8(sp)
	lda	t0, lockstat_probe
	ldq	pv, 0(t0)
	jsr	ra, (pv)
	ldq	a3, 1*8(sp)

.lockstat_wrapper_arg0_skip:
	ldbu	t0, T_LOCKSTAT(a3)		/* curthread->t_lockstat-- */
	subq	t0, 1, t0
	stb	t0, T_LOCKSTAT(a3)
	ldq	ra, 0*8(sp)
	lda	sp, 2*8(sp)
	ret
	SET_SIZE(lockstat_wrapper_arg0)

.inst_retrun:
	ret
.inst_nop:
	nop

#define	HOT_PATCH(label, event)		\
	lda	a0, label;		\
	lda	t0, lockstat_probemap;	\
	ldiq	t1, event;		\
	s4addq	t1, t0, t0;		\
	ldl	t0, 0(t0);		\
	beq	t0, 1f;			\
	lda	t1, .inst_nop;		\
	br	2f;			\
1:	lda	t1, .inst_retrun;	\
2:	ldl	a1, 0(t1);		\
	ldiq	a2, 4;			\
	CALL(hot_patch_kernel_text)


	ENTRY(lockstat_hot_patch)
	LDGP(pv)
	lda	sp, -8*2(sp)
	stq	ra, 0(sp)
	HOT_PATCH(.lock_try_lockstat_patch_point, LS_LOCK_TRY_ACQUIRE)
	HOT_PATCH(.lock_clear_lockstat_patch_point, LS_LOCK_CLEAR_RELEASE)
	HOT_PATCH(.lock_set_lockstat_patch_point, LS_LOCK_SET_ACQUIRE)
	HOT_PATCH(.lock_set_spl_lockstat_patch_point, LS_LOCK_SET_SPL_ACQUIRE)
	HOT_PATCH(.lock_clear_splx_lockstat_patch_point, LS_LOCK_CLEAR_SPLX_RELEASE)
	HOT_PATCH(.mutex_enter_lockstat_patch_point, LS_MUTEX_ENTER_ACQUIRE)
	HOT_PATCH(.mutex_tryenter_lockstat_patch_point, LS_MUTEX_ENTER_ACQUIRE)
	HOT_PATCH(.mutex_exit_lockstat_patch_point, LS_MUTEX_EXIT_RELEASE)
	HOT_PATCH(.rw_read_enter_lockstat_patch_point, LS_RW_ENTER_ACQUIRE)
	HOT_PATCH(.rw_write_enter_lockstat_patch_point, LS_RW_ENTER_ACQUIRE)
	HOT_PATCH(.rw_read_exit_lockstat_patch_point, LS_RW_EXIT_RELEASE)
	HOT_PATCH(.rw_write_exit_lockstat_patch_point, LS_RW_EXIT_RELEASE)
	ldq	ra, 0(sp)
	lda	sp, 8*2(sp)
	ret
	SET_SIZE(lockstat_hot_patch)

	ENTRY(mutex_delay_default)
	mov	92, t0
1:	subq	t0, 1, t0
	bne	t0, 1b
	ret
	SET_SIZE(mutex_delay_default)

	ENTRY(thread_onproc)
	LDGP(pv)
	lda	t0, TS_ONPROC
	stl	t0, T_STATE(a0)
	addq	a1, CPU_THREAD_LOCK, t0
	stq	t0, T_LOCKP(a0)
	ret
	SET_SIZE(thread_onproc)
