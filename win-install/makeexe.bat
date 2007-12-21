set NAME=nifskope
set VERSION=1.0.4

del %NAME%-%VERSION%-windows.exe

cd ..\..\docsys
del doc\*.html
dir doc\
\Python25\python nifxml_doc.py

pause

cd ..\nifskope\win-install

"%PROGRAMFILES%\NSIS\makensis.exe" /v3 %NAME%.nsi

pause
