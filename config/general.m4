# $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/config/general.m4,v 1.1 2000-09-21 20:17:42 petere Exp $

# This file defines new macros to process configure command line
# arguments, to replace the brain-dead AC_ARG_WITH and AC_ARG_ENABLE.
# The flaw in these is particularly that they only differentiate
# between "given" and "not given" and do not provide enough help to
# process arguments that only accept "yes/no", that require an
# argument (other than "yes/no"), etc.
#
# The point of this implementation is to reduce code size and
# redundancy in configure.in and to improve robustness and consistency
# in the option evaluation code.


# print an error message and exit (while running `autoconf')
define([pgac_error],
       [errprint(__file__:__line__[: error: $1
])
        m4exit(10)])


# Convert type and name to shell variable name (e.g., "enable_long_strings")
define([pgac_arg_to_variable],
       [$1[]_[]patsubst($2, -, _)])


# PGAC_ARG(TYPE, NAME, HELP-STRING,
#          [ACTION-IF-YES], [ACTION-IF-NO], [ACTION-IF-ARG],
#          [ACTION-IF-OMITTED])
# ----------------------------------------------------------
# This is the base layer. TYPE is either "with" or "enable", depending
# on what you like. NAME is the rest of the option name, HELP-STRING
# as usual. ACTION-IF-YES is executed if the option is given without
# and argument (or "yes", which is the same); similar for ACTION-IF-NO.

AC_DEFUN([PGAC_ARG],
[dnl Check arguments
ifelse([$1], enable, ,
        [$1], with, ,
        [pgac_error([first argument of $0 must be `enable' or `with', not `$1'])])[]dnl
define([pgac_variable], [pgac_arg_to_variable([$1], [$2])])[]dnl
dnl Register help string, only if there is one
ifelse([$3], [], [],
[AC_DIVERT_PUSH(AC_DIVERSION_NOTICE)dnl
ac_help="$ac_help
[$3]"
AC_DIVERT_POP()])dnl
[#] Check whether --$1-$2 was given
if test x"[$]{pgac_variable+set}" = xset; then
  case [$]pgac_variable in
    yes)
      ifelse([$4], [], :, [$4])
      ;;
    no)
      ifelse([$5], [], :, [$5])
      ;;
dnl Action if called with argument
ifelse([$6], [], [],
[    *)
dnl Set up $withval or $enableval
      $1[]val=[$]pgac_variable
      $6
      ;;
])[]dnl
  esac [#] [$]pgac_variable
dnl Action if omitted
ifelse([$7], [], [],
[else
  $7
])[]dnl
fi[]dnl
undefine([pgac_variable])dnl
])# PGAC_ARG


# PGAC_ARG_BOOL(TYPE, NAME, DEFAULT, HELP-STRING, 
#               [ACTION-IF-YES], [ACTION-IF-NO])
# -----------------------------------------------
# Accept a boolean option, that is, one that only takes yes or no.
# ("no" is equivalent to "disable" or "without"). DEFAULT is what
# should be done if the option is omitted; it should be "yes" or "no".
# (Consequently, one of ACTION-IF-YES and ACTION-IF-NO will always
# execute.)

AC_DEFUN([PGAC_ARG_BOOL],
[PGAC_ARG([$1], [$2], [$4], [$5], [$6],
          [AC_MSG_ERROR([no argument expected for --$1-$2 option])],
          [ifelse([$3], yes, [pgac_arg_to_variable([$1], [$2])=yes
$5],
                  [$3], no,  [pgac_arg_to_variable([$1], [$2])=no
$6],
                  [pgac_error([third argument of $0 must be `yes' or `no', not `$3'])])])[]dnl
])# PGAC_ARG_BOOL


# PGAC_ARG_REQ(TYPE, NAME, HELP-STRING, [ACTION-IF-GIVEN], [ACTION-IF-NOT-GIVEN])
# -------------------------------------------------------------------------------
# This option will require an argument; "yes" or "no" will not be
# accepted.

AC_DEFUN([PGAC_ARG_REQ],
[PGAC_ARG([$1], [$2], [$3],
          [AC_MSG_ERROR([argument required for --$1-$2 option])],
          [AC_MSG_ERROR([argument required for --$1-$2 option])],
          [$4],
          [$5])])# PGAC_ARG_REQ


# PGAC_ARG_OPTARG(TYPE, NAME, HELP-STRING, [DEFAULT-ACTION], [ARG-ACTION]
#                 [ACTION-ENABLED], [ACTION-DISABLED])
# -----------------------------------------------------------------------
# This will create an option that behaves as follows: If omitted, or
# called with "no", then set the enable_variable to "no" and do
# nothing else. If called with "yes", then execute DEFAULT-ACTION. If
# called with argument, set enable_variable to "yes" and execute
# ARG-ACTION. Additionally, execute ACTION-ENABLED if we ended up with
# "yes" either way, else ACTION-DISABLED.
#
# The intent is to allow enabling a feature, and optionally pass an
# additional piece of information.

AC_DEFUN([PGAC_ARG_OPTARG],
[PGAC_ARG([$1], [$2], [$3], [$4], [],
          [pgac_arg_to_variable([$1], [$2])=yes
$5],
          [pgac_arg_to_variable([$1], [$2])=no])
dnl Add this code only if there's a ACTION-ENABLED or ACTION-DISABLED.
ifelse([$6[]$7], [], [],
[
if test "[$]pgac_arg_to_variable([$1], [$2])" = yes; then
  ifelse([$6], [], :, [$6])
ifelse([$7], [], [],
[else
  $7
])[]dnl
fi
])[]dnl
])# PGAC_ARG_OPTARG
