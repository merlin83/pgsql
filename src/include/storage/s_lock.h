/*-------------------------------------------------------------------------
 *
 * s_lock.h--
 *	   This file contains the implementation (if any) for spinlocks.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/include/storage/s_lock.h,v 1.33 1998-05-04 23:49:17 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *	 DESCRIPTION
 *		The public functions that must be provided are:
 *
 *		void S_INIT_LOCK(slock_t *lock)
 *
 *		void S_LOCK(slock_t *lock)
 *
 *		void S_UNLOCK(slock_t *lock)
 *
 *		void S_LOCK_FREE(slock_t *lock)
 *			Tests if the lock is free. Returns non-zero if free, 0 if locked.
 *
 *		The S_LOCK() macro	implements a primitive but still useful random
 *		backoff to avoid hordes of busywaiting lockers chewing CPU.
 *
 *		Effectively:
 *		void
 *		S_LOCK(slock_t *lock)
 *		{
 *			while (TAS(lock))
 *			{
 *			// back off the cpu for a semi-random short time
 *			}
 *		}
 *
 *		This implementation takes advantage of a tas function written
 *		(in assembly language) on machines that have a native test-and-set
 *		instruction. Alternative mutex implementations may also be used.
 *		This function is hidden under the TAS macro to allow substitutions.
 *
 *		#define TAS(lock) tas(lock)
 *		int tas(slock_t *lock)		// True if lock already set
 *
 *		If none of this can be done, POSTGRES will default to using
 *		System V semaphores (and take a large performance hit -- around 40%
 *		of its time on a DS5000/240 is spent in semop(3)...).
 *
 *	NOTES
 *		AIX has a test-and-set but the recommended interface is the cs(3)
 *		system call.  This provides an 8-instruction (plus system call
 *		overhead) uninterruptible compare-and-set operation.  True
 *		spinlocks might be faster but using cs(3) still speeds up the
 *		regression test suite by about 25%.  I don't have an assembler
 *		manual for POWER in any case.
 *
 *		There are default implementations for all these macros at the bottom
 *		of this file. Check if your platform can use these or needs to
 *		override them.
 *
 */
#if !defined(S_LOCK_H)
#define S_LOCK_H

#include "storage/ipc.h"

#if defined(HAS_TEST_AND_SET)

#if defined(linux)
/***************************************************************************
 * All Linux
 */

#if defined(__alpha)

#define S_UNLOCK(lock) { __asm__("mb"); *(lock) = 0; }

#endif /* __alpha */




#else /* linux */
/***************************************************************************
 * All non Linux
 */

#if defined (nextstep)
/*
 * NEXTSTEP (mach)
 * slock_t is defined as a struct mutex.
 */

#define S_LOCK(lock)	mutex_lock(lock)

#define S_UNLOCK(lock)	mutex_unlock(lock)

#define S_INIT_LOCK(lock)	mutex_init(lock)

/* For Mach, we have to delve inside the entrails of `struct mutex'.  Ick! */
#define S_LOCK_FREE(alock)	((alock)->lock == 0)

#endif /* nextstep */



#if defined(__sgi)
/*
 * SGI IRIX 5
 * slock_t is defined as a struct abilock_t, which has a single unsigned long
 * member.
 *
 * This stuff may be supplemented in the future with Masato Kataoka's MIPS-II
 * assembly from his NECEWS SVR4 port, but we probably ought to retain this
 * for the R3000 chips out there.
 */
#define TAS(lock)	(!acquire_lock(lock))

#define S_UNLOCK(lock)	release_lock(lock)

#define S_INIT_LOCK(lock)	init_lock(lock)

#define S_LOCK_FREE(lock)	(stat_lock(lock) == UNLOCKED)

#endif /* __sgi */



#if (defined(__alpha)
/*
 * OSF/1 (Alpha AXP)
 *
 * Note that slock_t on the Alpha AXP is msemaphore instead of char
 * (see storage/ipc.h).
 */
#define TAS(lock)	(msem_lock((lock), MSEM_IF_NOWAIT) < 0)

#define S_UNLOCK(lock)	msem_unlock((lock), 0)

#define S_INIT_LOCK(lock)	msem_init((lock), MSEM_UNLOCKED)

#define S_LOCK_FREE(lock)	(!(lock)->msem_state)

#endif /* __alpha */



#if defined(_AIX)
/*
 * AIX (POWER)
 *
 * Note that slock_t on POWER/POWER2/PowerPC is int instead of char
 * (see storage/ipc.h).
 */
#define TAS(lock)	cs((int *) (lock), 0, 1)

#endif /* _AIX */



#if defined(__hpux)
/*
 * HP-UX (PA-RISC)
 *
 * Note that slock_t on PA-RISC is a structure instead of char
 * (see storage/ipc.h).
 *
 * a "set" slock_t has a single word cleared.  a "clear" slock_t has
 * all words set to non-zero. tas() in tas.s
 */
static slock_t clear_lock =
{-1, -1, -1, -1};

#define S_UNLOCK(lock)	(*(lock) = clear_lock)	/* struct assignment */

#define S_LOCK_FREE(lock)	( *(int *) (((long) (lock) + 15) & ~15) != 0)

#endif /* __hpux */



#endif /* else defined(linux) */




/****************************************************************************
 * Default Definitions - override these above as needed.
 */

#if !defined(S_LOCK)

#include <sys/time.h>

#define S_NSPINCYCLE	16
#define S_MAX_BUSY		1000 * S_NSPINCYCLE

extern int	s_spincycle[];
extern void s_lock_stuck(slock_t *lock, char *file, int line);

#if defined(S_LOCK_DEBUG)

extern void s_lock(slock_t *lock);

#define S_LOCK(lock) s_lock(lock, __FILE__, __LINE__)

#else /* S_LOCK_DEBUG */

#define S_LOCK(lock) if (1) { \
	int spins = 0; \
	while (TAS(lock)) { \
		struct timeval	delay; \
		delay.tv_sec = 0; \
		delay.tv_usec = s_spincycle[spins++ % S_NSPINCYCLE]; \
		(void) select(0, NULL, NULL, NULL, &delay); \
		if (spins > S_MAX_BUSY) { \
			/* It's been well over a minute...  */ \
			s_lock_stuck(lock, __FILE__, __LINE__); \
		} \
	} \
} else

#endif /* S_LOCK_DEBUG */
#endif /* S_LOCK */



#if !defined(S_LOCK_FREE)
#define S_LOCK_FREE(lock)	((*lock) == 0)
#endif /* S_LOCK_FREE */

#if !defined(S_UNLOCK)
#define S_UNLOCK(lock)		(*(lock) = 0)
#endif /* S_UNLOCK */

#if !defined(S_INIT_LOCK)
#define S_INIT_LOCK(lock)	S_UNLOCK(lock)
#endif /* S_INIT_LOCK */

#if !defined(TAS)
int			tas(slock_t *lock); /* port/.../tas.s, or s_lock.c */

#define TAS(lock)		tas(lock)
#endif /* TAS */


#endif /* HAS_TEST_AND_SET */

#endif /* S_LOCK_H */
