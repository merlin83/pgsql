# Makefile for Borland C++ 5.5
# Borland C++ base install directory goes here
# BCB=d:\Borland\Bcc55

!MESSAGE Building PSQL.EXE ...
!MESSAGE
!IF "$(CFG)" == ""
CFG=Release
!MESSAGE No configuration specified. Defaulting to Release with STATIC libraries.
!MESSAGE To use dynamic link libraries add -DDLL_LIBS to make command line.
!MESSAGE
!ELSE
!MESSAGE Configuration "$(CFG)"
!MESSAGE
!ENDIF

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running MAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE make  -DCFG=[Release | Debug] /f bcc32.mak
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "Release" (Win32 Release EXE)
!MESSAGE "Debug" (Win32 Debug EXE)
!MESSAGE
!ERROR An invalid configuration was specified.
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=bcc32.exe
PERL=perl.exe
FLEX=flex.exe

!IF "$(CFG)" == "Debug"
DEBUG=1
OUTDIR=.\Debug
INTDIR=.\Debug
!else
OUTDIR=.\Release
INTDIR=.\Release
!endif
REFDOCDIR=../../../doc/src/sgml/ref

.path.obj = $(INTDIR)

.c.obj:
	$(CPP) -o"$(INTDIR)\$&" $(CPP_PROJ) $<

ALL : sql_help.h psqlscan.c "..\..\port\pg_config_paths.h" "$(OUTDIR)\psql.exe"

CLEAN :
	-@erase "$(INTDIR)\command.obj"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\copy.obj"
	-@erase "$(INTDIR)\describe.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\input.obj"
	-@erase "$(INTDIR)\large_obj.obj"
	-@erase "$(INTDIR)\mainloop.obj"
	-@erase "$(INTDIR)\mbprint.obj"
	-@erase "$(INTDIR)\print.obj"
	-@erase "$(INTDIR)\prompt.obj"
	-@erase "$(INTDIR)\startup.obj"
	-@erase "$(INTDIR)\stringutils.obj"
	-@erase "$(INTDIR)\tab-complete.obj"
	-@erase "$(INTDIR)\variables.obj"
	-@erase "$(INTDIR)\exec.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt_long.obj"
	-@erase "$(INTDIR)\path.obj"
	-@erase "$(INTDIR)\pgstrcasecmp.obj"
	-@erase "$(INTDIR)\sprompt.obj"
	-@erase "$(INTDIR)\psql.ilc"
	-@erase "$(INTDIR)\psql.ild"
	-@erase "$(INTDIR)\psql.tds"
	-@erase "$(INTDIR)\psql.ils"
	-@erase "$(INTDIR)\psql.ilf"
	-@erase "$(OUTDIR)\psql.exe"
	-@erase "$(INTDIR)\..\..\port\pg_config_paths.h"

"..\..\port\pg_config_paths.h": win32.mak
	echo #define PGBINDIR "" >$@
	echo #define PGSHAREDIR "" >>$@
	echo #define SYSCONFDIR "" >>$@
	echo #define INCLUDEDIR "" >>$@
	echo #define PKGINCLUDEDIR "" >>$@
	echo #define INCLUDEDIRSERVER "" >>$@
	echo #define LIBDIR "" >>$@
	echo #define PKGLIBDIR "" >>$@
	echo #define LOCALEDIR "" >>$@

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

USERDEFINES = WIN32;_CONSOLE;_MBCS;HAVE_STRDUP

# ---------------------------------------------------------------------------
CPP_PROJ = -I$(BCB)\include;..\..\include;..\..\interfaces\libpq;..\..\include\port\win32 \
           -c -D$(USERDEFINES) -DFRONTEND -tWM -tWC -q -5 -a8 -pc -X -w-use -w-par -w-pia \
	   -w-csu -w-aus -w-ccc

!IFDEF DEBUG
CPP_PROJ  	= $(CPP_PROJ) -Od -r- -k -v -y -vi- -D_DEBUG
LIBPG_DIR 	= Debug
!ELSE
CPP_PROJ	= $(CPP_PROJ) -O -Oi -OS -DNDEBUG
LIBPG_DIR 	= Release
!ENDIF

!IFDEF DLL_LIBS
CPP_PROJ	= $(CPP_PROJ) -D_RTLDLL
LIBRARIES	= cw32mti.lib ..\..\interfaces\libpq\$(LIBPG_DIR)\blibpqdll.lib
!ELSE
CPP_PROJ	= $(CPP_PROJ) -DBCC32_STATIC
LIBRARIES	= cw32mt.lib ..\..\interfaces\libpq\$(LIBPG_DIR)\blibpq.lib
!ENDIF

LINK32=ilink32.exe
LINK32_FLAGS=-L$(BCB)\lib;.\$(LIBPG_DIR) -x -v 
LINK32_OBJS= \
	command.obj \
	common.obj \
	copy.obj \
	describe.obj \
	help.obj \
	input.obj \
	large_obj.obj \
	mainloop.obj \
	mbprint.obj
	print.obj \
	prompt.obj \
	startup.obj \
	stringutils.obj \
	tab-complete.obj \
	variables.obj \
	exec.obj \
	getopt.obj \
	getopt_long.obj \
	path.obj \
	pgstrcasecmp.obj \
	sprompt.obj \
	

"$(OUTDIR)\psql.exe" : "$(OUTDIR)" $(LINK32_OBJS)
	$(LINK32) @&&!
	$(LINK32_FLAGS) +
	c0x32.obj $(LINK32_OBJS), +
	$@,, +
	import32.lib $(LIBRARIES),,
!

exec.obj : "$(OUTDIR)" ..\..\port\exec.c
getopt.obj : "$(OUTDIR)" ..\..\port\getopt.c
getopt_long.obj : "$(OUTDIR)" ..\..\port\getopt_long.c
path.obj : "$(OUTDIR)" ..\..\port\path.c
pgstrcasecmp.obj : "$(OUTDIR)" ..\..\port\pgstrcasecmp.c
sprompt.obj : "$(OUTDIR)" ..\..\port\sprompt.c

"sql_help.h": create_help.pl 
       $(PERL) create_help.pl $(REFDOCDIR) $@

psqlscan.c : psqlscan.l
	$(FLEX) -Cfe -opsqlscan.c psqlscan.l
