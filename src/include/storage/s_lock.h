/*-------------------------------------------------------------------------
 *
 * s_lock.h
 *	   This file contains the implementation (if any) for spinlocks.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/include/storage/s_lock.h,v 1.75 2000-12-03 14:41:42 thomas Exp $
 *
 *-------------------------------------------------------------------------
 */

/*
 *	 DESCRIPTION
 *		The public macros that must be provided are:
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
 *		The S_LOCK() macro implements a primitive but still useful random
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
 *		There are default implementations for all these macros at the bottom
 *		of this file. Check if your platform can use these or needs to
 *		override them.
 *
 *	NOTES
 *		If none of this can be done, POSTGRES will default to using
 *		System V semaphores (and take a large performance hit -- around 40%
 *		of its time on a DS5000/240 is spent in semop(3)...).
 *
 *		AIX has a test-and-set but the recommended interface is the cs(3)
 *		system call.  This provides an 8-instruction (plus system call
 *		overhead) uninterruptible compare-and-set operation.  True
 *		spinlocks might be faster but using cs(3) still speeds up the
 *		regression test suite by about 25%.  I don't have an assembler
 *		manual for POWER in any case.
 *
 */
#ifndef S_LOCK_H
#define S_LOCK_H

#include "storage/ipc.h"

extern void s_lock_sleep(unsigned spin);

#if defined(HAS_TEST_AND_SET)


#if defined(__GNUC__)
/*************************************************************************
 * All the gcc inlines
 */


#if defined(__i386__)
#define TAS(lock) tas(lock)

static __inline__ int
tas(volatile slock_t *lock)
{
	register slock_t _res = 1;

__asm__("lock; xchgb %0,%1": "=q"(_res), "=m"(*lock):"0"(_res));
	return (int) _res;
}

#endif	 /* __i386__ */


#ifdef __ia64__
#define TAS(lock) tas(lock)

static __inline__ int
tas (volatile slock_t *lock)
{
  long int ret;

  __asm__ __volatile__(
       "xchg4 %0=%1,%2"
       : "=r"(ret), "=m"(*lock)
       : "r"(1), "1"(*lock)
       : "memory");

  return (int) ret;
}
#endif /* __ia64__ */


#if defined(__arm__) || defined(__arm__)
#define TAS(lock) tas(lock)

static __inline__ int
tas(volatile slock_t *lock)
{
        register slock_t _res = 1;

__asm__("swpb %0, %0, [%3]": "=r"(_res), "=m"(*lock):"0"(_res), "r" (lock));
        return (int) _res;
}

#endif   /* __arm__ */

#if defined(__s390__)
/*
 * S/390 Linux
 */
#define TAS(lock)      tas(lock)

static inline int
tas(volatile slock_t *lock)
{
 int _res;

        __asm__ __volatile("    la    1,1\n"
                           "    l     2,%2\n"
                           "    slr   0,0\n"
                           "    cs    0,1,0(2)\n"
                           "    lr    %1,0"
                           : "=m" (lock), "=d" (_res)
                           : "m" (lock)
                           : "0", "1", "2");

       return (_res);
}
#endif  /* __s390__ */


#if defined(__sparc__)
#define TAS(lock) tas(lock)

static __inline__ int
tas(volatile slock_t *lock)
{
	register slock_t _res = 1;

	__asm__("ldstub [%2], %0" \
:			"=r"(_res), "=m"(*lock) \
:			"r"(lock));
	return (int) _res;
}

#endif	 /* __sparc__ */


#if defined(__mc68000__) && defined(__linux__)
#define TAS(lock) tas(lock)

static __inline__ int
tas(volatile slock_t *lock)
{
	register int rv;
	
	__asm__ __volatile__ (
		"tas %1; sne %0"
		: "=d" (rv), "=m"(*lock)
		: "1" (*lock)
		: "cc" );
	return rv;
}

#endif /* defined(__mc68000__) && defined(__linux__) */


#if defined(NEED_VAX_TAS_ASM)
/*
 * VAXen -- even multiprocessor ones
 * (thanks to Tom Ivar Helbekkmo)
 */
#define TAS(lock) tas(lock)

typedef unsigned char slock_t;

static __inline__ int
tas(volatile slock_t *lock)
{
	register	_res;

	__asm__("	movl $1, r0 \
			bbssi $0, (%1), 1 f \
			clrl r0 \
1:			movl r0, %0 "
:			"=r"(_res)			/* return value, in register */
:			"r"(lock)			/* argument, 'lock pointer', in register */
:			"r0");				/* inline code uses this register */
	return (int) _res;
}

#endif	 /* NEED_VAX_TAS_ASM */



#if defined(NEED_NS32K_TAS_ASM)
#define TAS(lock) tas(lock)

static __inline__ int
tas(volatile slock_t *lock)
{
  register _res;
  __asm__("sbitb 0, %0 \n\
	sfsd %1"
	: "=m"(*lock), "=r"(_res));
  return (int) _res; 
}

#endif  /* NEED_NS32K_TAS_ASM */



#else							/* __GNUC__ */
/***************************************************************************
 * All non gcc
 */

#if defined(__QNX__)
/*
 * QNX 4
 *
 * Note that slock_t under QNX is sem_t instead of char
 */
#define TAS(lock)       (sem_trywait((lock)) < 0)
#define S_UNLOCK(lock)  sem_post((lock))
#define S_INIT_LOCK(lock)       sem_init((lock), 1, 1)
#define S_LOCK_FREE(lock)       (lock)->value
#endif   /* __QNX__ */


#if defined(NEED_I386_TAS_ASM)
/* non gcc i386 based things */

#if defined(USE_UNIVEL_CC)
#define TAS(lock)	tas(lock)

asm int
tas(volatile slock_t *s_lock)
{
/* UNIVEL wants %mem in column 1, so we don't pg_indent this file */
%mem s_lock
	pushl %ebx
	movl s_lock, %ebx
	movl $255, %eax
	lock
	xchgb %al, (%ebx)
	popl %ebx
}

#endif	 /* USE_UNIVEL_CC */

#endif	 /* NEED_I386_TAS_ASM */

#endif	 /* defined(__GNUC__) */



/*************************************************************************
 * These are the platforms that have common code for gcc and non-gcc
 */


#if defined(__alpha)

#if defined(__osf__)
/*
 * OSF/1 (Alpha AXP)
 *
 * Note that slock_t on the Alpha AXP is msemaphore instead of char
 * (see storage/ipc.h).
 */
#include <alpha/builtins.h>
#if 0
#define TAS(lock)	  (msem_lock((lock), MSEM_IF_NOWAIT) < 0)
#define S_UNLOCK(lock) msem_unlock((lock), 0)
#define S_INIT_LOCK(lock)		msem_init((lock), MSEM_UNLOCKED)
#define S_LOCK_FREE(lock)	  (!(lock)->msem_state)
#else
#define TAS(lock)         (__INTERLOCKED_TESTBITSS_QUAD((lock),0))
#endif

#else /* i.e. not __osf__ */

#define TAS(lock) tas(lock)
#define S_UNLOCK(lock) do { __asm__("mb"); *(lock) = 0; } while (0)

static __inline__ int
tas(volatile slock_t *lock)
{
 register slock_t _res;

__asm__("	 ldq   $0, %0			   \n\
				 bne   $0, 3f		   \n\
				 ldq_l $0, %0			 \n\
				 bne   $0, 3f		   \n\
				 or    $31, 1, $0		   \n\
				 stq_c $0, %0				   \n\
				 beq   $0, 2f			   \n\
				 bis   $31, $31, %1 	   \n\
				 mb 							   \n\
				 jmp   $31, 4f			   \n\
			  2: or    $31, 1, $0			   \n\
			  3: bis   $0, $0, %1		   \n\
			  4: nop	  ": "=m"(*lock), "=r"(_res): :"0");

	return (int) _res;
}
#endif /* __osf__ */

#endif /* __alpha */


#if defined(__hpux)
/*
 * HP-UX (PA-RISC)
 *
 * Note that slock_t on PA-RISC is a structure instead of char
 * (see include/port/hpux.h).
 *
 * a "set" slock_t has a single word cleared.  a "clear" slock_t has
 * all words set to non-zero. tas() in tas.s
 */

#define S_UNLOCK(lock) \
do { \
	volatile slock_t *lock_ = (volatile slock_t *) (lock); \
	lock_->sema[0] = lock_->sema[1] = lock_->sema[2] = lock_->sema[3] = -1; \
} while (0)

#define S_LOCK_FREE(lock)	( *(int *) (((long) (lock) + 15) & ~15) != 0)

#endif	 /* __hpux */


#if defined(__sgi)
/*
 * SGI IRIX 5
 * slock_t is defined as a unsigned long. We use the standard SGI
 * mutex API. 
 *
 * The following comment is left for historical reasons, but is probably
 * not a good idea since the mutex ABI is supported.
 *
 * This stuff may be supplemented in the future with Masato Kataoka's MIPS-II
 * assembly from his NECEWS SVR4 port, but we probably ought to retain this
 * for the R3000 chips out there.
 */
#include "mutex.h"
#define TAS(lock)	(test_and_set(lock,1))
#define S_UNLOCK(lock)	(test_then_and(lock,0))
#define S_INIT_LOCK(lock)	(test_then_and(lock,0))
#define S_LOCK_FREE(lock)	(test_then_add(lock,0) == 0)
#endif	 /* __sgi */

#if defined(sinix)
/*
 * SINIX / Reliant UNIX 
 * slock_t is defined as a struct abilock_t, which has a single unsigned long
 * member. (Basically same as SGI)
 *
 */
#define TAS(lock)	(!acquire_lock(lock))
#define S_UNLOCK(lock)	release_lock(lock)
#define S_INIT_LOCK(lock)	init_lock(lock)
#define S_LOCK_FREE(lock)	(stat_lock(lock) == UNLOCKED)
#endif	 /* sinix */
 

#if defined(_AIX)
/*
 * AIX (POWER)
 *
 * Note that slock_t on POWER/POWER2/PowerPC is int instead of char
 * (see storage/ipc.h).
 */
#define TAS(lock)	cs((int *) (lock), 0, 1)
#endif	 /* _AIX */


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
#endif	 /* nextstep */




/****************************************************************************
 * Default Definitions - override these above as needed.
 */

#if !defined(S_LOCK)
extern void s_lock(volatile slock_t *lock, const char *file, const int line);

#define S_LOCK(lock) \
	do { \
		if (TAS((volatile slock_t *) (lock))) \
			s_lock((volatile slock_t *) (lock), __FILE__, __LINE__); \
	} while (0)
#endif	 /* S_LOCK */

#if !defined(S_LOCK_FREE)
#define S_LOCK_FREE(lock)	(*(lock) == 0)
#endif	 /* S_LOCK_FREE */

#if !defined(S_UNLOCK)
#define S_UNLOCK(lock)		(*(lock) = 0)
#endif	 /* S_UNLOCK */

#if !defined(S_INIT_LOCK)
#define S_INIT_LOCK(lock)	S_UNLOCK(lock)
#endif	 /* S_INIT_LOCK */

#if !defined(TAS)
extern int	tas(volatile slock_t *lock);		/* port/.../tas.s, or
												 * s_lock.c */

#define TAS(lock)		tas((volatile slock_t *) (lock))
#endif	 /* TAS */


#else	 /* !HAS_TEST_AND_SET */

/*
 * Fake spinlock implementation using SysV semaphores --- slow and prone
 * to fall foul of kernel limits on number of semaphores, so don't use this
 * unless you must!
 */

typedef struct
{
	/* reference to semaphore used to implement this spinlock */
	IpcSemaphoreId	semId;
	int				sem;
} slock_t;

extern bool s_lock_free_sema(volatile slock_t *lock);
extern void s_unlock_sema(volatile slock_t *lock);
extern void s_init_lock_sema(volatile slock_t *lock);
extern int tas_sema(volatile slock_t *lock);

extern void s_lock(volatile slock_t *lock, const char *file, const int line);

#define S_LOCK(lock) \
	do { \
		if (TAS((volatile slock_t *) (lock))) \
			s_lock((volatile slock_t *) (lock), __FILE__, __LINE__); \
	} while (0)

#define S_LOCK_FREE(lock)   s_lock_free_sema(lock)
#define S_UNLOCK(lock)   s_unlock_sema(lock)
#define S_INIT_LOCK(lock)   s_init_lock_sema(lock)
#define TAS(lock)   tas_sema(lock)

#endif	 /* HAS_TEST_AND_SET */

#endif	 /* S_LOCK_H */
