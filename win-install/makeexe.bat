rem Quick build script to create the installer for release.
rem Run this batch file from buildenv, see https://github.com/amorilia/buildenv
rem Call it as "buildenv.bat C:\Python27 mingw 32 workspace"
@echo on
setlocal
set NAME=nifskope
set VERSION=1.1.1
set REVISION=

rem get revision via git - needs to exist in path since it could be installed (almost) anywhere
for %%i in (git.exe) do IF EXIST "%%~$PATH:i" set GIT=%%~$PATH:i
IF EXIST "%GIT%" for /f "delims=| usebackq" %%i in (`"%GIT% log -1 --pretty=format:%%h"`) do set REVISION=%%i

IF NOT "%REVISION%" == "" set VERSION=%VERSION%.%REVISION%

del %NAME%-%VERSION%-windows.exe > nul

echo !define VERSION "%VERSION%" > nifversion.nsh
echo !define BUILD_RELEASE_FOLDER "..\..\NifSkope-build-desktop\release" >> nifversion.nsh
echo !define DLL_RELEASE_FOLDER "%QTDIR%\bin" >> nifversion.nsh

cd ..\docsys
del doc\*.html

python nifxml_doc.py

pushd ..\lang
for %%i in (*.ts) do call lrelease.exe %%i
popd

rem copy qhull's COPYING.TXT
copy ..\qhull\COPYING.TXT ..\Qhull_COPYING.TXT

cd ..\win-install

makensis.exe /v3 %NAME%-mingw-dynamic.nsi

REM pause
endlocal
