# $PostgreSQL: pgsql/src/bin/scripts/nls.mk,v 1.14 2004/08/01 17:54:21 petere Exp $
CATALOG_NAME    := pgscripts
AVAIL_LANGUAGES := cs de es fr it pt_BR ru sl sv zh_CN zh_TW
GETTEXT_FILES   := createdb.c createlang.c createuser.c \
                   dropdb.c droplang.c dropuser.c \
                   clusterdb.c vacuumdb.c \
                   common.c
GETTEXT_TRIGGERS:= _ simple_prompt
