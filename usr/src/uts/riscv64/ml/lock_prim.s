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
#include <sys/lockstat.h>
#include "assym.h"

/************************************************************************
 *		MINIMUM LOCKS
 */

#if defined(lint)

/*
 * lock_try(lp), ulock_try(lp)
 *	- returns non-zero on success.
 *	- doesn't block interrupts so don't use this to spin on a lock.
 *	- uses "0xFF is busy, anything else is free" model.
 *
 *      ulock_try() is for a lock in the user address space.
 *      For all V7/V8 sparc systems they are same since the kernel and
 *      user are mapped in a user' context.
 *      For V9 platforms the lock_try and ulock_try are different impl.
 */

int
lock_try(lock_t *lp)
{
	return (0xFF ^ ldstub(lp));
}

int
lock_spin_try(lock_t *lp)
{
	return (0xFF ^ ldstub(lp));
}

void
lock_set(lock_t *lp)
{
	extern void lock_set_spin(lock_t *);

	if (!lock_try(lp))
		lock_set_spin(lp);
	membar_enter();
}

void
lock_clear(lock_t *lp)
{
	membar_exit();
	*lp = 0;
}

int
ulock_try(lock_t *lp)
{
	return (0xFF ^ ldstub(lp));
}

void
ulock_clear(lock_t *lp)
{
	membar_exit();
	*lp = 0;
}

#else	/* lint */

	// Pre-conditions: LOCK_HELD_VALUE == 0xFF
	ENTRY(lock_try)
	ALTENTRY(lock_spin_try)
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, LOCK_HELD_VALUE
	sllw	t1, t1, t0	// t1 = LOCK_HELD_VALUE << (8 * ((uintptr_t)lp & 3));

	andi	a0, a0, -4
	amoor.w.aq t1, t1, (a0)

	srlw	t1, t1, t0
	andi	t1, t1, 0xFF
	xori	a0, t1, LOCK_HELD_VALUE
	ret
	SET_SIZE(lock_spin_try)
	SET_SIZE(lock_try)

	// Pre-conditions: *lp == 0 or *lp == 1
	ENTRY(ulock_try)
	li	t6, SSR_SUM
	csrs	sstatus, t6

	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 1
	sllw	t1, t1, t0	// t1 = 1 << (8 * ((uintptr_t)lp & 3));

	andi	a0, a0, -4
	amoor.w.aq t1, t1, (a0)

	srlw	t1, t1, t0
	andi	t1, t1, 1
	xori	a0, t1, 1
	csrc	sstatus, t6
	ret
	SET_SIZE(ulock_try)

	ENTRY(lock_clear)
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xff
	sllw	t1, t1, t0	// t1 = 0xff << (8 * ((uintptr_t)lp & 3));
	not	t1, t1

	andi	a0, a0, -4
	amoand.w.rl t1, t1, (a0)

	ret
	SET_SIZE(lock_clear)

	ENTRY(ulock_clear)
	li	t6, SSR_SUM
	csrs	sstatus, t6

	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xff
	sllw	t1, t1, t0	// t1 = 0xFF << (8 * ((uintptr_t)lp & 3));
	not	t1, t1

	andi	a0, a0, -4
	amoand.w.rl t1, t1, (a0)

	csrc	sstatus, t6
	ret
	SET_SIZE(ulock_clear)

	ENTRY(lock_set)
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, LOCK_HELD_VALUE
	sllw	t1, t1, t0	// t1 = LOCK_HELD_VALUE << (8 * ((uintptr_t)lp & 3));

	andi	t2, a0, -4
	amoor.w.aq t1, t1, (t2)

	srlw	t1, t1, t0
	andi	t1, t1, 0xFF
	bnez	t1, .lock_set_spin
	ret

.lock_set_spin:
	tail	lock_set_spin
	SET_SIZE(lock_set)

	ENTRY(lock_init)
	sb	zero, (a0)
	fence	rw, rw
	ret
	SET_SIZE(lock_init)
#endif

/*
 * lock_set_spl(lp, new_pil, *old_pil_addr)
 * 	Sets pil to new_pil, grabs lp, stores old pil in *old_pil_addr.
 */

#if defined(lint)
void
lock_set_spl(lock_t *lp, int new_pil, u_short *old_pil_addr)
{
	extern int splr(int);
	extern void lock_set_spl_spin(lock_t *, int, u_short *, int);
	int old_pil;

	old_pil = splr(new_pil);
	if (!lock_try(lp)) {
		lock_set_spl_spin(lp, new_pil, old_pil_addr, old_pil);
	} else {
		*old_pil_addr = (u_short)old_pil;
		membar_enter();
	}
}
#else
	ENTRY(lock_set_spl)
	addi	sp, sp, -32
	sd	a0, 0(sp)
	sd	a2, 8(sp)
	sw	a1, 16(sp)
	sd	ra, 24(sp)

	mv	a0, a1
	call	splr
	mv	a3, a0

	ld	a0, 0(sp)
	ld	a2, 8(sp)
	lw	a1, 16(sp)
	ld	ra, 24(sp)

	addi	sp, sp, 32

	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, LOCK_HELD_VALUE
	sllw	t1, t1, t0	// t1 = LOCK_HELD_VALUE << (8 * ((uintptr_t)lp & 3));

	andi	t2, a0, -4
	amoor.w.aq t1, t1, (t2)

	srlw	t1, t1, t0
	andi	t1, t1, 0xFF
	bnez	t1, .lock_set_spl_spin

	sh	a3, (a2)
	ret
.lock_set_spl_spin:
	tail	lock_set_spl_spin
	SET_SIZE(lock_set_spl)
#endif


/*
 * lock_clear_splx(lp, s)
 */

#if defined(lint)

void
lock_clear_splx(lock_t *lp, int s)
{
	extern void splx(int);

	lock_clear(lp);
	splx(s);
}

#else	/* lint */

	ENTRY(lock_clear_splx)
	andi	t0, a0, 3
	slli	t0, t0, 3	// t0 <- (8 * ((uintptr_t)lp & 3))
	li	t1, 0xff
	sllw	t1, t1, t0	// t1 = 0xff << (8 * ((uintptr_t)lp & 3));
	not	t1, t1

	andi	t2, a0, -4
	amoand.w.rl t1, t1, (t2)
	mv	a0, a1
	tail	splx
	SET_SIZE(lock_clear_splx)
#endif	/* lint */

/*
 * mutex_enter() and mutex_exit().
 * 
 * These routines handle the simple cases of mutex_enter() (adaptive
 * lock, not held) and mutex_exit() (adaptive lock, held, no waiters).
 * If anything complicated is going on we punt to mutex_vector_enter().
 *
 * mutex_tryenter() is similar to mutex_enter() but returns zero if
 * the lock cannot be acquired, nonzero on success.
 *
 * If mutex_exit() gets preempted in the window between checking waiters
 * and clearing the lock, we can miss wakeups.  Disabling preemption
 * in the mutex code is prohibitively expensive, so instead we detect
 * mutex preemption by examining the trapped PC in the interrupt path.
 * If we interrupt a thread in mutex_exit() that has not yet cleared
 * the lock, pil_interrupt() resets its PC back to the beginning of
 * mutex_exit() so it will check again for waiters when it resumes.
 *
 * The lockstat code below is activated when the lockstat driver
 * calls lockstat_hot_patch() to hot-patch the kernel mutex code.
 * Note that we don't need to test lockstat_event_mask here -- we won't
 * patch this code in unless we're gathering ADAPTIVE_HOLD lockstats.
 */

#if defined (lint)

/* ARGSUSED */
void
mutex_enter(kmutex_t *lp)
{}

/* ARGSUSED */
int
mutex_tryenter(kmutex_t *lp)
{ return (0); }

/* ARGSUSED */
void
mutex_exit(kmutex_t *lp)
{}

/* ARGSUSED */
void *
mutex_owner_running(mutex_impl_t *lp)
{ return (NULL); }

#else
	ENTRY(mutex_enter)
1:	lr.d.aq	t0, (a0)
	bnez	t0, .mutex_vector_enter
	sc.d	t2, tp, (a0)
	bnez	t2, 1b
	ret
.mutex_vector_enter:
	tail	mutex_vector_enter
	SET_SIZE(mutex_enter)

	ENTRY(mutex_tryenter)
1:	lr.d.aq	t0, (a0)
	bnez	t0, .mutex_vector_tryenter
	sc.d	t2, tp, (a0)
	bnez	t2, 1b
	li	a0, 1
	ret
.mutex_vector_tryenter:
	tail	mutex_vector_tryenter
	SET_SIZE(mutex_tryenter)

	ENTRY(mutex_adaptive_tryenter)
1:	lr.d.aq	t0, (a0)
	bnez	t0, 2f
	sc.d	t2, tp, (a0)
	bnez	t2, 1b
	li	a0, 1
	ret
2:	li	a0, 0
	ret
	SET_SIZE(mutex_adaptive_tryenter)

	ENTRY(mutex_exit)
1:	lr.d	t0, (a0)
	bne	t0, tp, .mutex_vector_exit
	sc.d.rl	t2, zero, (a0)
	bnez	t2, 1b
	ret
.mutex_vector_exit:
	tail	mutex_vector_exit
	SET_SIZE(mutex_exit)

	.globl	mutex_owner_running_critical_start
	ENTRY(mutex_owner_running)
mutex_owner_running_critical_start:
	ld	t3, (a0)
	fence
	andi	t3, t3, MUTEX_THREAD		/* t3 = lp->m_owner */
	beqz	t3, 1f				/* if (owner == NULL) return NULL */
	ld	t1, T_CPU(t3)			/* t1 = owner->t_cpu */
	ld	t2, CPU_THREAD(t1)		/* t2 = owner->t_cpu->cpu_thread */
.mutex_owner_running_critical_end:
	bne	t2, t3, 1f			/* owner == running thread ? */
	mv	a0, t1				/* yes: return cpu */
	ret
1:	li	a0, 0
	ret
	SET_SIZE(mutex_owner_running)

	.globl	mutex_owner_running_critical_size
	.type	mutex_owner_running_critical_size, @object
	.align	CPTRSHIFT
mutex_owner_running_critical_size:
	.quad	.mutex_owner_running_critical_end - mutex_owner_running_critical_start
	SET_SIZE(mutex_owner_running_critical_size)
#endif	/* lint */

/*
 * rw_enter() and rw_exit().
 * 
 * These routines handle the simple cases of rw_enter (write-locking an unheld
 * lock or read-locking a lock that's neither write-locked nor write-wanted)
 * and rw_exit (no waiters or not the last reader).  If anything complicated
 * is going on we punt to rw_enter_sleep() and rw_exit_wakeup(), respectively.
 */
#if defined(lint)

/* ARGSUSED */
void
rw_enter(krwlock_t *lp, krw_t rw)
{}

/* ARGSUSED */
void
rw_exit(krwlock_t *lp)
{}

#else

	ENTRY(rw_enter)
	li	t0, RW_WRITER
	beq	a1, t0, .rw_write_enter

	/* Acquire reader lock */
1:	lr.d.aq	t0, (a0)			/* t0 = lp->rw_wwwh */
	andi	t1, t0, RW_WRITE_CLAIMED	/* if (t0 & RW_WRITE_CLAIMED) goto .rw_enter_sleep */
	bnez	t1, .rw_enter_sleep
	addi	t0, t0, RW_READ_LOCK		/* Increment hold count */
	sc.d	t2, t0, (a0)			/* Install new rw_wwwh */
	bnez	t2, 1b
	ret

	/* Acquire writer lock */
.rw_write_enter:
1:	lr.d.aq	t0, (a0)			/* t0 = lp->rw_wwwh */
	bnez	t0, .rw_enter_sleep		/* if (t0 != 0) goto rw_enter_sleep */
	ori	t0, tp, RW_WRITE_LOCKED		/* Install writer lock */
	sc.d	t2, t0, (a0)
	bnez	t2, 1b
	ret
.rw_enter_sleep:
	tail	rw_enter_sleep
	SET_SIZE(rw_enter)


	ENTRY(rw_exit)
	ld	t3, (a0)			/* Read lock value */
	addi	t1, t3, -RW_READ_LOCK		/* r1 = new lock value for reader */
	bnez	t1, .rw_not_single_reader	/* if !(single-reader && no waiter) goto 10 */

	/* Try to release read lock here. */
.Lrw_read_release:
1:	lr.d	t2, (a0)
	bne	t2, t3, .rw_exit_wakeup		/* if the lock has been changed */
						/*   rw_exit_wakeup, return */
	sc.d.rl	t2, t1, (a0)			/* Install new lock value */
	bnez	t2, 1b
	ret

.rw_exit_wakeup:
	tail	rw_exit_wakeup

.rw_not_single_reader:
	/* Check whether we own this lock as writer mode. */
	andi	t3, t3, RW_WRITE_LOCKED
	bnez	t3, .Lrw_write_release
	li	t0, RW_READ_LOCK	/* if the lock would still be held */
	bge	t1, t0, .Lrw_read_release
	tail	rw_exit_wakeup		/* Let rw_exit_wakeup() do the rest */

.Lrw_write_release:
	/* Try to release write lock here. */
	ori	t3, t3, RW_WRITE_LOCKED
1:	lr.d	t2, (a0)
	bne	t2, t3, .rw_exit_wakeup	/* if the lock has been changed */
					/*   rw_exit_wakeup, return */
	sc.d.rl	t2, zero, (a0)		/* Install new lock value */
	bnez	t2, 1b
	ret
	SET_SIZE(rw_exit)
#endif

#if defined(lint)

void
lockstat_hot_patch(void)
{}

#endif	/* lint */

/*
 * thread_onproc()
 * Set thread in onproc state for the specified CPU.
 * Also set the thread lock pointer to the CPU's onproc lock.
 * Since the new lock isn't held, the store ordering is important.
 * If not done in assembler, the compiler could reorder the stores.
 */
#if defined(lint)

void
thread_onproc(kthread_id_t t, cpu_t *cp)
{
	t->t_state = TS_ONPROC;
	t->t_lockp = &cp->cpu_thread_lock;
}

#else	/* lint */

	ENTRY(thread_onproc)
	li	t2, TS_ONPROC
	sw	t2, T_STATE(a0)
	addi	t2, a1, CPU_THREAD_LOCK
	sd	t2, T_LOCKP(a0)
	ret
	SET_SIZE(thread_onproc)

#endif	/* lint */

/*
 * mutex_delay_default(void)
 * Spins for approx a few hundred processor cycles and returns to caller.
 */
#if defined(lint)

void
mutex_delay_default(void)
{}

#else	/* lint */

	ENTRY(mutex_delay_default)
	li	t0, 72
1:	add	t0, t0, -1
	bnez	t0, 1b
	ret
	SET_SIZE(mutex_delay_default)

#endif  /* lint */
