rem Quick build script to create the installer for release.

set NAME=nifskope
set VERSION=1.0.13

del %NAME%-%VERSION%-windows.exe

cd ..\..\docsys
del doc\*.html
dir doc\
\Python25\python nifxml_doc.py

pause

cd ..\nifskope\win-install

if exist "%PROGRAMFILES%\NSIS\makensis.exe" "%PROGRAMFILES%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi
if exist "%PROGRAMFILES(x86)%\NSIS\makensis.exe" "%PROGRAMFILES(x86)%\NSIS\makensis.exe" /v3 %NAME%-mingw-dynamic.nsi

 pause
