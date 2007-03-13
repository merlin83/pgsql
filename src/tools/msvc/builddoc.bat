@echo off
REM Adjust path for your docbook installation in buildenv.bat

SETLOCAL
SET STARTDIR=%CD%
SET OPENJADE=openjade-1.3.1
SET DSSSL=docbook-dsssl-1.79

IF EXIST ..\msvc IF EXIST ..\..\..\src cd ..\..\..
IF NOT EXIST doc\src\sgml\version.sgml goto noversion
IF EXIST src\tools\msvc\buildenv.bat CALL src\tools\msvc\buildenv.bat

IF NOT EXIST %DOCROOT%\%OPENJADE% SET NF=OpenJade
IF NOT EXIST %DOCROOT%\docbook SET NF=docbook
IF NOT EXIST %DOCROOT%\%DSSSL% set NF=docbook-dssl

IF NOT "%NF%" == "" GOTO notfound

IF "%1" == "renamefiles" GOTO renamefiles

cmd /v /c src\tools\msvc\builddoc renamefiles
cd doc\src\sgml

SET SGML_CATALOG_FILES=%DOCROOT%\%OPENJADE%\dsssl\catalog;%DOCROOT%\docbook\docbook.cat
perl %DOCROOT%\%DSSSL%\bin\collateindex.pl -f -g -o bookindex.sgml -N
perl mk_feature_tables.pl YES ..\..\..\src\backend\catalog\sql_feature_packages.txt ..\..\..\src\backend\catalog\sql_features.txt > features-supported.sgml
perl mk_feature_tables.pl NO ..\..\..\src\backend\catalog\sql_feature_packages.txt ..\..\..\src\backend\catalog\sql_features.txt > features-unsupported.sgml

%DOCROOT%\%OPENJADE%\bin\openjade -V draft-mode -wall -wno-unused-param -wno-empty -D . -c %DOCROOT%\%DSSSL%\catalog -d stylesheet.dsl -i output-html -t sgml postgres.sgml
perl %DOCROOT%\%DSSSL%\bin\collateindex.pl -f -g -i 'bookindex' -o bookindex.sgml HTML.index
%DOCROOT%\%OPENJADE%\bin\openjade -V draft-mode -wall -wno-unused-param -wno-empty -D . -c %DOCROOT%\%DSSSL%\catalog -d stylesheet.dsl -i output-html -t sgml postgres.sgml

cd %STARTDIR%
echo Docs build complete.
exit /b


:renamefiles
REM Rename ISO entity files
CD %DOCROOT%\docbook
FOR %%f in (ISO*) do (
   set foo=%%f
   IF NOT "!foo:~-4!" == ".gml" ren !foo! !foo:~0,3!-!foo:~3!.gml
)
exit /b

:notfound
echo Could not find directory for %NF%.
cd %STARTDIR%
goto :eof

:noversion
echo Could not find version.sgml. Please run mkvcbuild.pl first!
cd %STARTDIR%
goto :eof
