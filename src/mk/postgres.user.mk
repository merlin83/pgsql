#-------------------------------------------------------------------------
#
# postgres.user.mk--
#    rules for building object/shared libraries used in dynamic loading. 
#    To use the rules, set the following variables:
#	DLOBJS     - objects to be linked in dynamically
#    This makefile adds the files you need to build to CREATEFILES.
#
#    For building user modules (user functions to be loaded in dynamically).
#    Make sure the following variables are set properly (You can either
#    define them manually or include postgres.mk which defines them.):
#	MKDIR	    - where postgres makefiles are
#	includedir  - where header files are installed
#	PORTNAME    - your platform (alpha, sparc, sparc_solaris, etc.)
#	objdir	    - where to put the generated files
#
#    An SQL script foo.sql or a shell script foo.sh generated from foo.source.
#    Occurrence of the following strings will be replaced with the respective
#    values. This is a feeble attempt to provide "portable" scripts.
#	_CWD_	    - current working directory
#	_OBJWD_     - where the generated files (eg. object files) are
#	_SLSUFF_    - suffix of the shared library or object for
#	              dynamic loading
#	_USER_	    - the login of the user
#
# Copyright (c) 1994-5, Regents of the University of California
#
#
# IDENTIFICATION
#    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/mk/Attic/postgres.user.mk,v 1.1.1.1 1996-07-09 06:22:19 scrappy Exp $
#
#-------------------------------------------------------------------------

-include $(MKDIR)/port/postgres.mk.$(PORTNAME)
CFLAGS+=  -I$(includedir) $(CFLAGS_SL)

%.sql: %.source
	if [ -z "$$USER" ]; then USER=$$LOGNAME; fi; \
	if [ -z "$$USER" ]; then USER=`whoami`; fi; \
	if [ -z "$$USER" ]; then echo 'Cannot deduce $$USER.'; exit 1; fi; \
	rm -f $(objdir)/$*.sql; \
	C=`pwd`; \
	sed -e "s:_CWD_:$$C:g" \
	    -e "s:_OBJWD_:$$C/$(objdir):g" \
	    -e "s:_SLSUFF_:$(SLSUFF):g" \
	    -e "s/_USER_/$$USER/g" < $*.source > $(objdir)/$*.sql

#How to create a dynamic lib
%.so.1:	%.so
	@rm -f $(objdir)/$(@F)
	$(CC) -shared $< -o $(objdir)/$(@F)

%.sh: %.source
	if [ -z "$$USER" ]; then USER=$$LOGNAME; fi; \
	if [ -z "$$USER" ]; then USER=`whoami`; fi; \
	if [ -z "$$USER" ]; then echo 'Cannot deduce $USER.'; exit 1; fi; \
	rm -f $(objdir)/$*.sh; \
	C="`pwd`/"; \
	sed -e "s:_CWD_:$$C:g" \
	    -e "s:_OBJWD_:$$C/$(objdir):g" \
	    -e "s:_SLSUFF_:$(SLSUFF):g" \
	    -e "s/_USER_/$$USER/g" < $*.source > $(objdir)/$*.sh

#
# plus exports files
#
ifdef EXPSUFF
CREATEFILES+= $(DLOBJS:.o=$(EXPSUFF))
endif

#
# plus shared libraries
#
ifdef SLSUFF
ifneq ($(SLSUFF), '.o')
CREATEFILES+= $(DLOBJS:.so=$(SLSUFF))
endif
endif

