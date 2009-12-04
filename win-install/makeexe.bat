rem Quick build script to create the installer for release.
@echo on
setlocal
set NAME=nifskope
set VERSION=1.0.22
set REVISION=

rem get revision via git - needs to exist in path since it could be installed (almost) anywhere
for %%i in (git.exe) do IF EXIST "%%~$PATH:i" set GIT=%%~$PATH:i
IF EXIST "%GIT%" for /f "delims=| usebackq" %%i in (`"%GIT% log -1 --pretty=format:%%h"`) do set REVISION=%%i

IF NOT "%REVISION%" == "" set VERSION=%VERSION%.%REVISION%

del %NAME%-%VERSION%-windows.exe > nul

echo !define VERSION "%VERSION%" > nifversion.nsh

cd ..\docsys
del doc\*.html

for %%i in (python.exe) do IF EXIST "%%~$PATH:i" set PYTHON=%%~$PATH:i
IF NOT EXIST "%PYTHON%" set PYTHON=\Python25\python.exe
IF NOT EXIST "%PYTHON%" set PYTHON=\Python26\python.exe
"%PYTHON%" nifxml_doc.py

if EXIST "%QTDIR%\bin\lrelease.exe" (
    pushd ..\lang
    for %%i in (*.ts) do call "%QTDIR%\bin\lrelease.exe" %%i
    popd
)

cd ..\win-install

if exist "%PROGRAMFILES%\NSIS\makensis.exe" "%PROGRAMFILES%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi
if exist "%PROGRAMFILES(x86)%\NSIS\makensis.exe" "%PROGRAMFILES(x86)%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi

REM pause
endlocal
