# $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/psql/nls.mk,v 1.13 2003-10-06 17:37:39 petere Exp $
CATALOG_NAME	:= psql
AVAIL_LANGUAGES	:= cs de es fr hu nb pt_BR ru sl sv zh_CN zh_TW
GETTEXT_FILES	:= command.c common.c copy.c help.c input.c large_obj.c \
                   mainloop.c print.c startup.c describe.c sql_help.h
GETTEXT_TRIGGERS:= _ N_ psql_error simple_prompt
