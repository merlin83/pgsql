/* the purpose of this file is to reduce the use of #ifdef's through
 * the code base by those porting the software, an dto facilitate the
 * eventual use of autoconf to build the server 
 */

#ifndef CONFIG_H
#define CONFIG_H

#define BLCKSZ	8192

#if defined(sequent) 
#  define NEED_UNION_SEMUN 
#endif

#if defined(alpha)
#  define USE_POSIX_TIME 
#  define DISABLE_XOPEN_NLS 
#  define NEED_ISINF 
#  define HAS_LONG_LONG
#  define NEED_UNION_SEMUN 
#endif

#if defined(dgux)
#  define LINUX_ELF
#  define NEED_UNION_SEMUN 
#endif

#if defined(ultrix4)
#  define NEED_ISINF 
#  define USE_POSIX_TIME
#  define NEED_UNION_SEMUN 
#endif

#if defined(linux)
#  define NEED_CBRT
#endif

#if defined(hpux)
#  define USE_POSIX_TIME
#  define HAVE_TZSET
#  define NEED_CBRT
#  define NEED_RINT
#  define NEED_UNION_SEMUN 
#endif

#if defined(i386_solaris) 
#  define USE_POSIX_TIME 
#  define NEED_ISINF 
#  define NEED_RUSAGE 
#  define NO_EMPTY_STMTS
#  define HAVE_TZSET
#  define NEED_UNION_SEMUN 
#endif

#if defined(sparc)
#  define USE_POSIX_TIME
#endif

#if defined(sparc_solaris)
#  define USE_POSIX_TIME 
#  define NEED_ISINF 
#  define NEED_RUSAGE 
#  define NO_EMPTY_STMTS
#  define USE_POSIX_TIME
#  define HAVE_TZSET
#  define NEED_UNION_SEMUN 
#endif

#if defined(svr4) 
#  define USE_POSIX_TIME 
#  define NEED_ISINF 
#  define NEED_RUSAGE 
#  define NO_EMPTY_STMTS
#  define HAVE_TZSET
#  define NEED_UNION_SEMUN 
#endif

#if defined(win32)
#  define WIN32
#  define NO_UNISTD_H
#  define USES_WINSOCK 
#  define NOFILE	100
#  define NEED_UNION_SEMUN
#  define HAVE_TZSET
#  define NEED_CBRT
#  define NEED_ISINF
#endif /* WIN32 */

#if defined(BSD44_derived) || \
    defined(bsdi_2_1)
#  define USE_LIMITS_H
#  define USE_POSIX_TIME
#  define NEED_CBRT
#endif

#if defined(bsdi)
#  define NEED_UNION_SEMUN 
#  define USE_LIMITS_H
#  define USE_POSIX_TIME
#  define NEED_CBRT
#endif

#if defined(aix)
#  define CLASS_CONFLICT 
#  define DISABLE_XOPEN_NLS 
#  define NEED_ISINF
#  define NEED_UNION_SEMUN 
#  define NEED_SYS_SELECT_H
#  define HAVE_TZSET
#endif

#if defined(irix5)
#  define USE_POSIX_TIME 
#  define NEED_ISINF 
#  define NO_EMPTY_STMTS
#  define NO_VFORK
#  define HAVE_TZSET
#endif

/*
 * The following is used as the arg list for signal handlers.  Any ports
 * that take something other than an int argument should change this in
 * the port specific makefile.  Note that variable names are required
 * because it is used in both the prototypes as well as the definitions.
 * Note also the long name.  We expect that this won't collide with
 * other names causing compiler warnings.
 */ 

#ifndef       SIGNAL_ARGS
#  define SIGNAL_ARGS int postgres_signal_arg
#endif

/* NAMEDATALEN is the max length for system identifiers (e.g. table names,
 * attribute names, function names, etc.)
 *
 * These MUST be set here.  DO NOT COMMENT THESE OUT
 * Setting these too high will result in excess space usage for system catalogs
 * Setting them too low will make the system unusable.
 * values between 16 and 64 that are multiples of four are recommended.
 *
 * NOTE also that databases with different NAMEDATALEN's cannot interoperate!
 */ 
#define NAMEDATALEN 32
/* OIDNAMELEN should be set to NAMEDATALEN + sizeof(Oid) */
#define OIDNAMELEN  36

/* turn this on if you prefer European style dates instead of American
 * style dates
 */
#define EUROPEAN_DATES FALSE

/*
 * If you do not plan to use Host based authentication,
 * comment out the following line
 */
#define HBA

/*
 * On architectures for which we have not implemented spinlocks (or
 * cannot do so), we use System V semaphores.  We also use them for
 * long locks.  For some reason union semun is never defined in the
 * System V header files so we must do it ourselves.
 */

/*  Debug and various "defines" that should be documented */

/* found in function aclparse() in src/backend/utils/adt/acl.c */
/* #define ACLDEBUG */

/* found in src/backend/utils/adt/arrayfuncs.c */
/* #define LOARRAY */
#define ESCAPE_PATCH
#define ARRAY_PATCH
#define NULL_PATCH
#define OPENLINK_PATCHES

/* This is the time, in seconds, at which a given backend server
 * will wait on a lock before deciding to abort the transaction
 * (this is what we do in lieu of deadlock detection).
 *
 * Low numbers are not recommended as they will tend to cause
 * false aborts if many transactions are long-lived.
 */
#define DEADLOCK_TIMEOUT 60


/* Fixes use of indexes infunctions */
#define INDEXSCAN_PATCH 

/* found in src/backend/utils/adt/date.c */
/* #define DATEDEBUG */

/* found in src/backend/utils/adt/datetimes.c */
/* #define USE_SHORT_YEAR */
/* #define AMERICAN_STYLE */

/*----------------------------------------*/
/* found in src/backend/utils/adt/float.c */
/*------------------------------------------------------*/
/* defining unsafe floats's will make float4 and float8 */
/* ops faster at the cost of safety, of course!         */
/*------------------------------------------------------*/
/* #define UNSAFE_FLOATS */

/*

There is a bug in the function executor. The backend crashes while trying to
execute an sql function containing an utility command (create, notify, ...).
The bug is part in the planner, which returns a number of plans different
than the number of commands if there are utility commands in the query, and
in part in the function executor which assumes that all commands are normal
query commands and causes a SIGSEGV trying to execute commands without plan.

*/
#define FUNC_UTIL_PATCH

/*

Async notifies received while a backend is in the middle of a begin/end
transaction block are lost by libpq when the final end command is issued.

The bug is in the routine PQexec of libpq. The routine throws away any
message from the backend when a message of type 'C' is received. This
type of message is sent when the result of a portal query command with
no tuples is returned. Unfortunately this is the case of the end command.
As all async notification are sent only when the transaction is finished,
if they are received in the middle of a transaction they are lost in the
libpq library. I added some tracing code to PQexec and this is the output:

*/
#define PQ_NOTIFY_PATCH
#endif /* CONFIG_H */

