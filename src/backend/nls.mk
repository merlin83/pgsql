# $PostgreSQL: pgsql/src/backend/nls.mk,v 1.18 2004/12/16 11:31:50 petere Exp $
CATALOG_NAME	:= postgres
AVAIL_LANGUAGES	:= af cs de es fr hr hu it ko nb pt_BR ru sk sl sv tr zh_CN zh_TW
GETTEXT_FILES	:= + gettext-files
# you can add "elog:2" and "errmsg_internal" to this list if you want to
# include internal messages in the translation list.
GETTEXT_TRIGGERS:= errmsg errdetail errhint errcontext write_stderr yyerror

gettext-files: distprep
	find $(srcdir)/ $(srcdir)/../port/ -name '*.c' -print >$@

my-maintainer-clean:
	rm -f gettext-files

.PHONY: my-maintainer-clean
maintainer-clean: my-maintainer-clean
