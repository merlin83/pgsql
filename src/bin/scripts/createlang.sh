#!/bin/sh
#-------------------------------------------------------------------------
#
# createlang.sh--
#    Install a procedural language in a database
#
# Copyright (c) 1994, Regents of the University of California
#
#
# IDENTIFICATION
#    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/scripts/Attic/createlang.sh,v 1.6 2000-01-19 20:08:35 petere Exp $
#
#-------------------------------------------------------------------------

CMDNAME=`basename $0`
PATHNAME=`echo $0 | sed "s,$CMDNAME\$,,"`

PSQLOPT=
dbname=
langname=
list=

# Check for echo -n vs echo \c

if echo '\c' | grep -s c >/dev/null 2>&1
then
    ECHO_N="echo -n"
    ECHO_C=""
else
    ECHO_N="echo"
    ECHO_C='\c'
fi



# ----------
# Get options, language name and dbname
# ----------
while [ $# -gt 0 ]
do
    case "$1" in 
	--help|-\?)
		usage=t
                break
		;;
        --list|-l)
                list=t
                ;;
# options passed on to psql
	--host|-h)
		PSQLOPT="$PSQLOPT -h $2"
		shift;;
        -h*)
                PSQLOPT="$PSQLOPT $1"
                ;;
        --host=*)
                PSQLOPT="$PSQLOPT -h "`echo $1 | sed 's/^--host=//'`
                ;;
	--port|-p)
		PSQLOPT="$PSQLOPT -p $2"
		shift;;
        -p*)
                PSQLOPT="$PSQLOPT $1"
                ;;
        --port=*)
                PSQLOPT="$PSQLOPT -p "`echo $1 | sed 's/^--port=//'`
                ;;
	--username|-U)
		PSQLOPT="$PSQLOPT -U $2"
		shift;;
        -U*)
                PSQLOPT="$PSQLOPT $1"
                ;;
        --username=*)
                PSQLOPT="$PSQLOPT -U "`echo $1 | sed 's/^--username=//'`
                ;;
	--password|-W)
		PSQLOPT="$PSQLOPT -W"
		;;
	--dbname|-d)
		dbname="$2"
		shift;;
        -d*)
                dbname=`echo $1 | sed 's/^-d//'`
                ;;
        --dbname=*)
                dbname=`echo $1 | sed 's/^--dbname=//'`
                ;;
# misc options
	--pglib|-L)
                PGLIB="$2"
                shift;;
        -L*)
                PGLIB=`echo $1 | sed 's/^-L//'`
                ;;
        --pglib=*)
                PGLIB=`echo $1 | sed 's/^--pglib=//'`
                ;;

	-*)
		echo "$CMDNAME: invalid option: $1"
                echo "Try -? for help."
		exit 1
		;;
	 *)
 		langname="$1"
                if [ "$2" ]; then
                        shift
			dbname="$1"
		fi
                ;;
    esac
    shift
done

if [ "$usage" ]; then	
        echo "$CMDNAME installs a procedural language into a PostgreSQL database."
	echo
	echo "Usage:"
        echo "  $CMDNAME [options] [langname [dbname]]"
        echo
	echo "Options:"
	echo "  -h, --host=HOSTNAME             Database server host"
	echo "  -p, --port=PORT                 Database server port"
	echo "  -U, --username=USERNAME         Username to connect as"
	echo "  -W, --password                  Prompt for password"
	echo "  -d, --dbname=DBNAME             Database to install language in"
	echo "  -L, --pglib=PGLIB               Find language interpreter in directory PGLIB"
	echo "  -l, --list                      Show a list of currently installed languages"
        echo
	echo "Report bugs to <pgsql-bugs@postgresql.org>."
	exit 0
fi


# ----------
# Check that we have a database
# ----------
if [ -z "$dbname" ]; then
	echo "$CMDNAME: missing required argument database name"
        echo "Try -? for help."
	exit 1
fi


# ----------
# List option
# ----------
if [ "$list" ]; then
        ${PATHNAME}psql $PSQLOPT -d "$dbname" -P 'title=Procedural languages' -c "SELECT lanname as \"Name\", lanpltrusted as \"Trusted?\", lancompiler as \"Compiler\" FROM pg_language WHERE lanispl = 't'"
        exit $?
fi


# ----------
# Check that we have PGLIB
# ----------
if [ -z "$PGLIB" ]; then
	echo "$CMDNAME: missing required argument PGLIB directory"
        echo "(This is the directory where the interpreter for the procedural"
        echo "language is stored. Traditionally, these are installed in whatever"
        echo "'lib' directory was specified at configure time.)"
	exit 1
fi

# ----------
# If not given on the command line, ask for the language
# ----------
if [ -z "$langname" ]; then
	$ECHO_N "Language to install in database $dbname: "$ECHO_C
	read langname
fi

# ----------
# Check if supported and set related values
# ----------
case "$langname" in
	plpgsql)
                lancomp="PL/pgSQL"
		trusted="TRUSTED "
		handler="plpgsql_call_handler"
                ;;
	pltcl)
		lancomp="PL/Tcl"
		trusted="TRUSTED "
		handler="pltcl_call_handler";;
	*)
		echo "$CMDNAME: unsupported language '$langname'"
		echo "Supported languages are 'plpgsql' and 'pltcl'."
		exit 1
        ;;
esac


# ----------
# Check that the shared object for the call handler is installed
# in PGLIB
# ----------
if [ ! -f $PGLIB/${langname}__DLSUFFIX__ ]; then
	echo "$CMDNAME: cannot find the file $PGLIB/${langname}__DLSUFFIX__"
        echo ""
	echo "This file contains the call handler for $lancomp. By default,"
        echo "only PL/pgSQL is built and installed; other languages must be"
        echo "explicitly enabled at configure time."
	echo ""
	echo "To install PL/Tcl, make sure the option --with-tcl is given to"
        echo "configure, then recompile and install."
	exit 1
fi


PSQL="${PATHNAME}psql -A -t -q $PSQLOPT -d $dbname -c"

# ----------
# Make sure the language isn't already installed
# ----------
res=`$PSQL "SELECT oid FROM pg_language WHERE lanname = '$langname'"`
if [ $? -ne 0 ]; then
	echo "$CMDNAME: external error"
	exit 1
fi
if [ "$res" ]; then
	echo "$CMDNAME: '$langname' is already installed in database $dbname"
	exit 1
fi

# ----------
# Check that there is no function named as the call handler
# ----------
res=`$PSQL "SELECT oid FROM pg_proc WHERE proname = '$handler'"`
if [ ! -z "$res" ]; then
	echo "$CMDNAME: A function named '$handler' already exists. Installation aborted."
	exit 1
fi

# ----------
# Create the call handler and the language
# ----------
$PSQL "CREATE FUNCTION $handler () RETURNS OPAQUE AS '$PGLIB/${langname}__DLSUFFIX__' LANGUAGE 'C'"
if [ $? -ne 0 ]; then
	echo "$CMDNAME: language installation failed"
	exit 1
fi
$PSQL "CREATE ${trusted}PROCEDURAL LANGUAGE '$langname' HANDLER $handler LANCOMPILER '$lancomp'"
if [ $? -ne 0 ]; then
	echo "$CMDNAME: language installation failed"
	exit 1
fi

echo "Ok"
exit 0
