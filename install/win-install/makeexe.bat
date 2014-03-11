@echo off
rem Quick build script to create the installer for release.
rem Run this batch file from buildenv, see https://github.com/amorilia/buildenv
rem Call it as "buildenv.bat C:\Python27 mingw 32 workspace"

setlocal

set NAME=nifskope
set VERSION=1.1.1
if not "%1" == "" set VERSION=%VERSION%-%1
set REVISION=

rem Check that required tools are in PATH
for %%i in (git.exe, python.exe, lrelease.exe, makensis.exe) do if not exist "%%~$PATH:i" (
  echo.%%i not found in path
  exit /B 1
)


for /f "delims=| usebackq" %%i in (`"git.exe log -1 --pretty=format:%%h"`) do set REVISION=%%i

del %NAME%-%VERSION%.%REVISION%-windows.exe > nul

echo !define VERSION "%VERSION%.%REVISION%" > nifversion.nsh
echo !define BUILD_RELEASE_FOLDER "..\..\NifSkope-build-desktop\release" >> nifversion.nsh
echo !define DLL_RELEASE_FOLDER "%QTDIR%\bin" >> nifversion.nsh

cd ..\docsys
del doc\*.html

python.exe nifxml_doc.py

pushd ..\lang
for %%i in (*.ts) do call lrelease.exe %%i
popd

rem copy qhull's COPYING.TXT
copy ..\qhull\COPYING.TXT ..\Qhull_COPYING.TXT

cd ..\win-install

makensis.exe /v3 %NAME%-mingw-dynamic.nsi

endlocal
