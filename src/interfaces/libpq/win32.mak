# Makefile for Microsoft Visual C++ 5.0 (or compat)

# Will build a Win32 static library (non-debug) libpq.lib
#        and a Win32 dynamic library (non-debug) libpq.dll with import library libpqdll.lib


!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\libpq.lib" "$(OUTDIR)\libpq.dll" 

CLEAN :
	-@erase "$(INTDIR)\dllist.obj"
	-@erase "$(INTDIR)\fe-auth.obj"
	-@erase "$(INTDIR)\fe-connect.obj"
	-@erase "$(INTDIR)\fe-exec.obj"
	-@erase "$(INTDIR)\fe-lobj.obj"
	-@erase "$(INTDIR)\fe-misc.obj"
	-@erase "$(INTDIR)\fe-print.obj"
	-@erase "$(OUTDIR)\libpqdll.obj"
	-@erase "$(OUTDIR)\libpq.lib"
	-@erase "$(OUTDIR)\libpq.dll"
	-@erase "$(OUTDIR)\libpq.res"
	-@erase "$(OUTDIR)\vc*.*"
	-@erase "$(OUTDIR)\libpq.pch"
	-@erase "$(OUTDIR)\libpqdll.exp"
	-@erase "$(OUTDIR)\libpqdll.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\..\include" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\libpq.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libpq.lib" 
LIB32_OBJS= \
	"$(INTDIR)\dllist.obj" \
	"$(INTDIR)\fe-auth.obj" \
	"$(INTDIR)\fe-connect.obj" \
	"$(INTDIR)\fe-exec.obj" \
	"$(INTDIR)\fe-lobj.obj" \
	"$(INTDIR)\fe-misc.obj" \
	"$(INTDIR)\fe-print.obj"

RSC_PROJ=/l 0x409 /fo"$(INTDIR)\libpq.res"

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib wsock32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\libpqdll.pdb" /machine:I386 /out:"$(OUTDIR)\libpq.dll"\
 /implib:"$(OUTDIR)\libpqdll.lib"  /def:libpqdll.def
LINK32_OBJS= \
	"$(INTDIR)\libpqdll.obj" \
	"$(OUTDIR)\libpq.lib" \
	"$(OUTDIR)\libpq.res"


"$(OUTDIR)\libpq.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

"$(INTDIR)\libpq.res" : "$(INTDIR)" libpq.rc
    $(RSC) $(RSC_PROJ) libpq.rc


"$(OUTDIR)\libpq.dll" : "$(OUTDIR)" "$(OUTDIR)\libpqdll.obj" "$(INTDIR)\libpqdll.obj" "$(INTDIR)\libpq.res"
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<


"$(OUTDIR)\dllist.obj" : ..\..\backend\lib\dllist.c
    $(CPP) @<<
    $(CPP_PROJ) ..\..\backend\lib\dllist.c
<<
    

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<
