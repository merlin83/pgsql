# $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/config/tcl.m4,v 1.2 2000-09-30 10:45:17 petere Exp $

# Autoconf macros to check for Tcl related things


AC_DEFUN([PGAC_PATH_TCLSH],
         [AC_PATH_PROGS(TCLSH, [tclsh tcl])])


# PGAC_PATH_TCLCONFIGSH([SEARCH-PATH])
# ------------------------------------
AC_DEFUN([PGAC_PATH_TCLCONFIGSH],
[AC_REQUIRE([PGAC_PATH_TCLSH])[]dnl
AC_BEFORE([$0], [PGAC_PATH_TKCONFIGSH])[]dnl
AC_MSG_CHECKING([for tclConfig.sh])
# Let user override test
if test -z "$TCL_CONFIG_SH"; then
    pgac_test_dirs="$1"

    set X $pgac_test_dirs; shift
    if test $[#] -eq 0; then
        test -z "$TCLSH" && AC_MSG_ERROR([unable to locate tclConfig.sh because no Tcl shell was found])
        set X `echo 'puts $auto_path' | $TCLSH`; shift
    fi

    for pgac_dir do
        if test -r "$pgac_dir/tclConfig.sh"; then
            TCL_CONFIG_SH=$pgac_dir/tclConfig.sh
            break
        fi
    done
fi

if test -z "$TCL_CONFIG_SH"; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([file \`tclConfig.sh' is required for Tcl])
else
    AC_MSG_RESULT([$TCL_CONFIG_SH])
fi

AC_SUBST([TCL_CONFIG_SH])
])# PGAC_PATH_TCLCONFIGSH


# PGAC_PATH_TKCONFIGSH([SEARCH-PATH])
# ------------------------------------
AC_DEFUN([PGAC_PATH_TKCONFIGSH],
[AC_REQUIRE([PGAC_PATH_TCLSH])[]dnl
AC_MSG_CHECKING([for tkConfig.sh])
# Let user override test
if test -z "$TK_CONFIG_SH"; then
    pgac_test_dirs="$1"

    set X $pgac_test_dirs; shift
    if test $[#] -eq 0; then
        test -z "$TCLSH" && AC_MSG_ERROR([unable to locate tkConfig.sh because no Tcl shell was found])
        set X `echo 'puts $auto_path' | $TCLSH`; shift
    fi

    for pgac_dir do
        if test -r "$pgac_dir/tkConfig.sh"; then
            TK_CONFIG_SH=$pgac_dir/tkConfig.sh
            break
        fi
    done
fi

if test -z "$TK_CONFIG_SH"; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([file \`tkConfig.sh' is required for Tk])
else
    AC_MSG_RESULT([$TK_CONFIG_SH])
fi

AC_SUBST([TK_CONFIG_SH])
])# PGAC_PATH_TKCONFIGSH
