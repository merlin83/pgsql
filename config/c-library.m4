# Macros that test various C library quirks
# $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/config/c-library.m4,v 1.15 2003-01-28 21:57:12 petere Exp $


# PGAC_VAR_INT_TIMEZONE
# ---------------------
# Check if the global variable `timezone' exists. If so, define
# HAVE_INT_TIMEZONE.
AC_DEFUN([PGAC_VAR_INT_TIMEZONE],
[AC_CACHE_CHECK(for int timezone, pgac_cv_var_int_timezone,
[AC_TRY_LINK([#include <time.h>
int res;],
  [res = timezone / 60;],
  [pgac_cv_var_int_timezone=yes],
  [pgac_cv_var_int_timezone=no])])
if test x"$pgac_cv_var_int_timezone" = xyes ; then
  AC_DEFINE(HAVE_INT_TIMEZONE,, [Set to 1 if you have the global variable timezone])
fi])# PGAC_VAR_INT_TIMEZONE


# PGAC_FUNC_GETTIMEOFDAY_1ARG
# ---------------------------
# Check if gettimeofday() has only one arguments. (Normal is two.)
# If so, define GETTIMEOFDAY_1ARG.
AC_DEFUN([PGAC_FUNC_GETTIMEOFDAY_1ARG],
[AC_CACHE_CHECK(whether gettimeofday takes only one argument,
pgac_cv_func_gettimeofday_1arg,
[AC_TRY_COMPILE([#include <sys/time.h>],
[struct timeval *tp;
struct timezone *tzp;
gettimeofday(tp,tzp);],
[pgac_cv_func_gettimeofday_1arg=no],
[pgac_cv_func_gettimeofday_1arg=yes])])
if test x"$pgac_cv_func_gettimeofday_1arg" = xyes ; then
  AC_DEFINE(GETTIMEOFDAY_1ARG,, [Set to 1 if gettimeofday() takes only 1 argument])
fi])# PGAC_FUNC_GETTIMEOFDAY_1ARG


# PGAC_UNION_SEMUN
# ----------------
# Check if `union semun' exists. Define HAVE_UNION_SEMUN if so.
# If it doesn't then one could define it as
# union semun { int val; struct semid_ds *buf; unsigned short *array; }
AC_DEFUN([PGAC_UNION_SEMUN],
[AC_CHECK_TYPES([union semun], [], [],
[#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>])])# PGAC_UNION_SEMUN


# PGAC_STRUCT_SOCKADDR_UN
# -----------------------
# If `struct sockaddr_un' exists, define HAVE_STRUCT_SOCKADDR_UN. If
# it is missing then one could define it as { short int sun_family;
# char sun_path[108]; }. (Requires test for <sys/un.h>!)
AC_DEFUN([PGAC_STRUCT_SOCKADDR_UN],
[AC_CHECK_TYPES([struct sockaddr_un], [], [],
[#include <sys/types.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
])])# PGAC_STRUCT_SOCKADDR_UN


# PGAC_FUNC_POSIX_SIGNALS
# -----------------------
# Check to see if the machine has the POSIX signal interface. Define
# HAVE_POSIX_SIGNALS if so. Also set the output variable HAVE_POSIX_SIGNALS
# to yes or no.
#
# Note that this test only compiles a test program, it doesn't check
# whether the routines actually work. If that becomes a problem, make
# a fancier check.
AC_DEFUN([PGAC_FUNC_POSIX_SIGNALS],
[AC_CACHE_CHECK(for POSIX signal interface, pgac_cv_func_posix_signals,
[AC_TRY_LINK([#include <signal.h>
],
[struct sigaction act, oact;
sigemptyset(&act.sa_mask);
act.sa_flags = SA_RESTART;
sigaction(0, &act, &oact);],
[pgac_cv_func_posix_signals=yes],
[pgac_cv_func_posix_signals=no])])
if test x"$pgac_cv_func_posix_signals" = xyes ; then
  AC_DEFINE(HAVE_POSIX_SIGNALS,, [Set to 1 if you have the POSIX signal interface])
fi
HAVE_POSIX_SIGNALS=$pgac_cv_func_posix_signals
AC_SUBST(HAVE_POSIX_SIGNALS)])# PGAC_FUNC_POSIX_SIGNALS


# PGAC_FUNC_SNPRINTF_LONG_LONG_INT_FORMAT
# ---------------------------------------
# Determine which format snprintf uses for long long int.  We handle
# %lld, %qd, %I64d.  The result is in shell variable
# LONG_LONG_INT_FORMAT.
AC_DEFUN([PGAC_FUNC_SNPRINTF_LONG_LONG_INT_FORMAT],
[AC_MSG_CHECKING([snprintf format for long long int])
AC_CACHE_VAL(pgac_cv_snprintf_long_long_int_format,
[for pgac_format in '%lld' '%qd' '%I64d'; do
AC_TRY_RUN([#include <stdio.h>
typedef long long int int64;
#define INT64_FORMAT "$pgac_format"

int64 a = 20000001;
int64 b = 40000005;

int does_int64_snprintf_work()
{
  int64 c;
  char buf[100];

  if (sizeof(int64) != 8)
    return 0;			/* doesn't look like the right size */

  c = a * b;
  snprintf(buf, 100, INT64_FORMAT, c);
  if (strcmp(buf, "800000140000005") != 0)
    return 0;			/* either multiply or snprintf is busted */
  return 1;
}
main() {
  exit(! does_int64_snprintf_work());
}],
[pgac_cv_snprintf_long_long_int_format=$pgac_format; break],
[],
[pgac_cv_snprintf_long_long_int_format=cross; break])
done])dnl AC_CACHE_VAL

LONG_LONG_INT_FORMAT=''

case $pgac_cv_snprintf_long_long_int_format in
  cross) AC_MSG_RESULT([cannot test (not on host machine)]);;
  ?*)    AC_MSG_RESULT([$pgac_cv_snprintf_long_long_int_format])
         LONG_LONG_INT_FORMAT=$pgac_cv_snprintf_long_long_int_format;;
  *)     AC_MSG_RESULT(none);;
esac])# PGAC_FUNC_SNPRINTF_LONG_LONG_INT_FORMAT
