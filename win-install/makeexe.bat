rem Quick build script to create the installer for release.
@echo on
setlocal
set NAME=nifskope
set VERSION=1.0.17

for %%i in (svnversion.exe) do IF EXIST "%%~$PATH:i" set SVNVERSION=%%~$PATH:i)
IF NOT EXIST "%SVNVERSION%" set SVNVERSION=%SystemDrive%\svn\bin\svnversion.exe
IF NOT EXIST "%SVNVERSION%" set SVNVERSION=%PROGRAMFILES%\TortoiseSVN\bin\svnversion.exe
IF NOT EXIST "%SVNVERSION%" set SVNVERSION=%PROGRAMFILES%\Subversion\bin\svnversion.exe
IF EXIST "%SVNVERSION%" for /f "delims=| usebackq" %%j in (`"%SVNVERSION%" ..`) do set VERSION=%VERSION%.%%j 

del %NAME%-%VERSION%-windows.exe > nul

cd ..\..\docsys
del doc\*.html
\Python25\python nifxml_doc.py

if EXIST "%QTDIR%\bin\lrelease.exe" (
    pushd ..\nifskope\lang
    for %%i in (*.ts) do call "%QTDIR%\bin\lrelease.exe" %%i
    popd
)

cd ..\nifskope\win-install

if exist "%PROGRAMFILES%\NSIS\makensis.exe" "%PROGRAMFILES%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi
if exist "%PROGRAMFILES(x86)%\NSIS\makensis.exe" "%PROGRAMFILES(x86)%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi

pause
:TheEnd
endlocal