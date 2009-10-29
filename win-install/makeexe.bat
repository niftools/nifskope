rem Quick build script to create the installer for release.
@echo on
setlocal
set NAME=nifskope
set VERSION=1.0.22
set REVISION=

rem get revision via svnversion
for %%i in (svnversion.exe) do IF EXIST "%%~$PATH:i" set SVNVERSION=%%~$PATH:i
IF NOT EXIST "%SVNVERSION%" set SVNVERSION=%SystemDrive%\svn\bin\svnversion.exe
IF NOT EXIST "%SVNVERSION%" set SVNVERSION=%PROGRAMFILES%\TortoiseSVN\bin\svnversion.exe
IF NOT EXIST "%SVNVERSION%" set SVNVERSION=%PROGRAMFILES%\Subversion\bin\svnversion.exe
IF EXIST "%SVNVERSION%" for /f "delims=| usebackq" %%j in (`"%SVNVERSION%" ..`) do set REVISION=%%j

for %%i in (sed.exe) do IF EXIST "%%~$PATH:i" set SED=%%~$PATH:i
for %%i in (SubWCRev.exe) do IF EXIST "%%~$PATH:i" set SUBWCREV=%%~$PATH:i
IF NOT EXIST "%SUBWCREV%" set SUBWCREV=%PROGRAMFILES%\TortoiseSVN\bin\SubWCRev.exe
IF NOT EXIST "%SUBWCREV%" set SUBWCREV=%PROGRAMFILES%\SubWCRev\SubWCRev.exe
rem get revision via sed + SubWCRev
IF NOT "%SED%" == "" (
    IF NOT "%SUBWCREV%" == "" (
        "%SUBWCREV%" .. -f | %SED% "s#Last committed at revision ##pg" -n > %TEMP%\nifskope.svnrev
        for /f %%j in (%TEMP%\nifskope.svnrev) do set REVISION=%%j
        del /q %TEMP%\nifskope.svnrev
    )
)
rem get revision via SubWCRev
IF "%SED%" == "" (
    IF NOT "%SUBWCREV%" == "" (
        "%SUBWCREV%" .. nifskope.svnrev.in %TEMP%\nifskope.svnrev
        for /f %%j in (%TEMP%\nifskope.svnrev) do set REVISION=%%j
        del /q %TEMP%\nifskope.svnrev
    )
)
IF NOT "%REVISION%" == "" set VERSION=%VERSION%.%REVISION%

del %NAME%-%VERSION%-windows.exe > nul

echo !define VERSION "%VERSION%" > nifversion.nsh

cd ..\..\docsys
del doc\*.html

for %%i in (python.exe) do IF EXIST "%%~$PATH:i" set PYTHON=%%~$PATH:i
IF NOT EXIST "%PYTHON%" set PYTHON=\Python25\python.exe
IF NOT EXIST "%PYTHON%" set PYTHON=\Python26\python.exe
"%PYTHON%" nifxml_doc.py

if EXIST "%QTDIR%\bin\lrelease.exe" (
    pushd ..\nifskope\lang
    for %%i in (*.ts) do call "%QTDIR%\bin\lrelease.exe" %%i
    popd
)

cd ..\nifskope\win-install

if exist "%PROGRAMFILES%\NSIS\makensis.exe" "%PROGRAMFILES%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi
if exist "%PROGRAMFILES(x86)%\NSIS\makensis.exe" "%PROGRAMFILES(x86)%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi

REM pause
endlocal
