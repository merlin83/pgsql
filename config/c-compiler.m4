# Macros to detect C compiler features
# $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/config/c-compiler.m4,v 1.5 2002-03-29 17:32:53 petere Exp $


# PGAC_C_SIGNED
# -------------
# Check if the C compiler understands signed types.
AC_DEFUN([PGAC_C_SIGNED],
[AC_CACHE_CHECK(for signed types, pgac_cv_c_signed,
[AC_TRY_COMPILE([],
[signed char c; signed short s; signed int i;],
[pgac_cv_c_signed=yes],
[pgac_cv_c_signed=no])])
if test x"$pgac_cv_c_signed" = xno ; then
  AC_DEFINE(signed,, [Define empty if the C compiler does not understand signed types])
fi])# PGAC_C_SIGNED



# PGAC_TYPE_64BIT_INT(TYPE)
# -------------------------
# Check if TYPE is a working 64 bit integer type. Set HAVE_TYPE_64 to
# yes or no respectively, and define HAVE_TYPE_64 if yes.
AC_DEFUN([PGAC_TYPE_64BIT_INT],
[define([Ac_define], [translit([have_$1_64], [a-z *], [A-Z_P])])dnl
define([Ac_cachevar], [translit([pgac_cv_type_$1_64], [ *], [_p])])dnl
AC_CACHE_CHECK([whether $1 is 64 bits], [Ac_cachevar],
[AC_TRY_RUN(
[typedef $1 int64;

/*
 * These are globals to discourage the compiler from folding all the
 * arithmetic tests down to compile-time constants.
 */
int64 a = 20000001;
int64 b = 40000005;

int does_int64_work()
{
  int64 c,d;

  if (sizeof(int64) != 8)
    return 0;			/* definitely not the right size */

  /* Do perfunctory checks to see if 64-bit arithmetic seems to work */
  c = a * b;
  d = (c + b) / b;
  if (d != a+1)
    return 0;
  return 1;
}
main() {
  exit(! does_int64_work());
}],
[Ac_cachevar=yes],
[Ac_cachevar=no],
[Ac_cachevar=no
dnl We will do better here with Autoconf 2.50
AC_MSG_WARN([64 bit arithmetic disabled when cross-compiling])])])

Ac_define=$Ac_cachevar
if test x"$Ac_cachevar" = xyes ; then
  AC_DEFINE(Ac_define,, [Set to 1 if `]$1[' is 64 bits])
fi
undefine([Ac_define])dnl
undefine([Ac_cachevar])dnl
])# PGAC_TYPE_64BIT_INT



# PGAC_CHECK_ALIGNOF(TYPE, [INCLUDES = DEFAULT-INCLUDES])
# -----------------------------------------------------
# Find the alignment requirement of the given type. Define the result
# as ALIGNOF_TYPE.  This macro works even when cross compiling.
# (Modelled after AC_CHECK_SIZEOF.)

AC_DEFUN([PGAC_CHECK_ALIGNOF],
[AS_LITERAL_IF([$1], [],
               [AC_FATAL([$0: requires literal arguments])])dnl
AC_CHECK_TYPE([$1], [], [], [$2])
AC_CACHE_CHECK([alignment of $1], [AS_TR_SH([pgac_cv_alignof_$1])],
[if test "$AS_TR_SH([ac_cv_type_$1])" = yes; then
  _AC_COMPUTE_INT([((char*) & pgac_struct.field) - ((char*) & pgac_struct)],
                  [AS_TR_SH([pgac_cv_alignof_$1])],
                  [AC_INCLUDES_DEFAULT([$2])
struct { char filler; $1 field; } pgac_struct;],
                  [AC_MSG_ERROR([cannot compute alignment of $1, 77])])
else
  AS_TR_SH([pgac_cv_alignof_$1])=0
fi])dnl
AC_DEFINE_UNQUOTED(AS_TR_CPP(alignof_$1),
                   [$AS_TR_SH([pgac_cv_alignof_$1])],
                   [The alignment requirement of a `$1'])
])# PGAC_CHECK_ALIGNOF
