#!/bin/sh
#-------------------------------------------------------------------------
#
# destroydb.sh--
#    destroy a postgres database
#
#    this program runs the monitor with the ? option to destroy
#    the requested database.
#
# Copyright (c) 1994, Regents of the University of California
#
#
# IDENTIFICATION
#    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/destroydb/Attic/destroydb.sh,v 1.9 1999-11-18 21:47:37 momjian Exp $
#
#-------------------------------------------------------------------------

CMDNAME=`basename $0`

if [ -z "$USER" ]; then
    if [ -z "$LOGNAME" ]; then
	if [ -z "`whoami`" ]; then
	    echo "$CMDNAME: cannot determine user name"
	    exit 1
	fi
    else
	USER=$LOGNAME
	export USER
    fi
fi

dbname=$USER
forcedel=t
while [ -n "$1" ]
do
	case $1 in 
	        -i) forcedel=f;;
		-a) AUTHSYS=$2; shift;;
		-h) PGHOST=$2; shift;;
		-p) PGPORT=$2; shift;;
		 *) dbname=$1;;
	esac
	shift;
done
if [ -z "$AUTHSYS" ]; then
  AUTHOPT=""
else
  AUTHOPT="-a $AUTHSYS"
fi

if [ -z "$PGHOST" ]; then
  PGHOSTOPT=""
else
  PGHOSTOPT="-h $PGHOST"
fi

if [ -z "$PGPORT" ]; then
  PGPORTOPT=""
else
  PGPORTOPT="-p $PGPORT"
fi

answer=y
if [ "$forcedel" = f ]
   then
   answer=f

   while [ "$answer" != y -a "$answer" != n ]
   do
       echo "Database '$dbname' will be permanently deleted."
       echo -n "Are you sure? (y/n) "
       read answer
   done
fi

if [ "$answer" = y ]
then
  psql -tq $AUTHOPT $PGHOSTOPT $PGPORTOPT -c "drop database \"$dbname\"" template1
    if [ $? -ne 0 ]
       then echo "$CMDNAME: database destroy failed on $dbname."
       exit 1
    fi
fi

exit 0
