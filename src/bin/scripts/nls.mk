# $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/scripts/nls.mk,v 1.9 2003-10-06 16:31:16 petere Exp $
CATALOG_NAME    := pgscripts
AVAIL_LANGUAGES := cs de es ru sl sv zh_CN
GETTEXT_FILES   := createdb.c createlang.c createuser.c \
                   dropdb.c droplang.c dropuser.c \
                   clusterdb.c vacuumdb.c \
                   common.c
GETTEXT_TRIGGERS:= _ simple_prompt
