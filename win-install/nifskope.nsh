; NifSkope Self-Installer Header File for Windows
; (NifTools - http://niftools.sourceforge.net) 
; (NSIS - http://nsis.sourceforge.net)
;
; Copyright (c) 2005-2012, NIF File Format Library and Tools
; All rights reserved.
; 
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are
; met:
; 
;     * Redistributions of source code must retain the above copyright
;       notice, this list of conditions and the following disclaimer.
;     * Redistributions in binary form must reproduce the above copyright
;       notice, this list of conditions and the following disclaimer in the
;       documentation ; and/or other materials provided with the
;       distribution.
;     * Neither the name of the NIF File Format Library and Tools project
;       nor the names of its contributors may be used to endorse or promote
;       products derived from this software without specific prior written
;       permission.
; 
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
; IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
; THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

!include "MUI.nsh"

!include "nifversion.nsh"

Name "NifSkope ${VERSION}"

; define installer pages
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE

!define MUI_WELCOMEPAGE_TEXT  "This wizard will guide you through the installation of NifSkope ${VERSION}.\r\n\r\nIt is recommended that you close all other applications.\r\n\r\nNote to Win2k/XP users: you require administrator privileges to install NifSkope successfully."
!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_LICENSE ..\LICENSE.TXT

!define MUI_DIRECTORYPAGE_TEXT_TOP "Use the field below to specify the folder where you want NifSkope to be copied to. To specify a different folder, type a new name or use the Browse button to select an existing folder."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "NifSkope Folder"
!define MUI_DIRECTORYPAGE_VARIABLE $INSTDIR
!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show Readme and Changelog"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION finishShowReadmeChangelog
!define MUI_FINISHPAGE_RUN "$INSTDIR\NifSkope.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run NifSkope"
!define MUI_FINISHPAGE_LINK "Visit us at http://niftools.sourceforge.net/"
!define MUI_FINISHPAGE_LINK_LOCATION "http://niftools.sourceforge.net/"
!insertmacro MUI_PAGE_FINISH

!define MUI_WELCOMEPAGE_TEXT  "This wizard will guide you through the uninstallation of NifSkope ${VERSION}.\r\n\r\nBefore starting the uninstallation, make sure NifSkope is not running.\r\n\r\nClick Next to continue."
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages
 
!insertmacro MUI_LANGUAGE "English"
    
;--------------------------------
;Language Strings

;Description
LangString DESC_SecCopyUI ${LANG_ENGLISH} "Copy all required files to the application folder."

;--------------------------------
; Data

OutFile "nifskope-${VERSION}-${INSTALLERPOSTFIX}.exe"
InstallDir "$PROGRAMFILES\NifTools\NifSkope"
InstallDirRegKey HKLM SOFTWARE\NifSkope Install_Dir
BrandingText "http://www.niftools.org/"
Icon "inst.ico"
UninstallIcon "inst.ico"
ShowInstDetails show
ShowUninstDetails show

;--------------------------------
; Functions

Function finishShowReadmeChangelog
	ExecShell "open" "$INSTDIR\README.TXT"
	ExecShell "open" "$INSTDIR\CHANGELOG.TXT"
FunctionEnd

Section
  SectionIn RO

  SetShellVarContext all

  ; Cleanup: silently uninstall old MSI versions of NifSkope
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\{7C67EDD6-1CAB-469E-9B64-EA03099D68BD}" "Contact"
  StrCmp $0 "NifTools" 0 +3
  DetailPrint "Uninstalling NifSkope 0.3.2"
  ExecWait '"$SYSDIR\msiexec.exe" /x {7C67EDD6-1CAB-469E-9B64-EA03099D68BD} /q'

  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\{58AFAC5C-972B-41D3-909F-EF9278BC6F60}" "Contact"
  StrCmp $0 "NifTools" 0 +3
  DetailPrint "Uninstalling NifSkope 0.4"
  ExecWait '"$SYSDIR\msiexec.exe" /x {58AFAC5C-972B-41D3-909F-EF9278BC6F60} /q'
  
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\{6ADE1E2F-E0A8-4717-B114-6BBD94766221}" "Contact"
  StrCmp $0 "NifTools" 0 +3
  DetailPrint "Uninstalling NifSkope 0.4.1"
  ExecWait '"$SYSDIR\msiexec.exe" /x {6ADE1E2F-E0A8-4717-B114-6BBD94766221} /q'
  
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\{D7FC69CC-3835-466B-AB29-110A3554368B}" "Publisher"
  StrCmp $0 "NifTools" 0 +3
  DetailPrint "Uninstalling NifSkope 0.5"
  ExecWait '"$SYSDIR\msiexec.exe" /x {D7FC69CC-3835-466B-AB29-110A3554368B} /q'

  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\{7F33BC54-4874-4C61-9AA1-C642E9622035}" "Publisher"
  StrCmp $0 "NifTools" 0 +3
  DetailPrint "Uninstalling NifSkope 0.6"
  ExecWait '"$SYSDIR\msiexec.exe" /x {7F33BC54-4874-4C61-9AA1-C642E9622035} /q'
  
  ; associate NIF files with NifSkope
  ReadRegStr $1 HKCR ".nif" ""
  StrCmp $1 "" NifAssocNoBackup ; not yet defined, no need to backup
  StrCmp $1 "NetImmerseFile" NifAssocNoBackup ; our definition, no need to backup

    WriteRegStr HKCR ".nif" "backup_val" $1

NifAssocNoBackup:

  ; associate Kf files with NifSkope
  ReadRegStr $1 HKCR ".kf" ""
  StrCmp $1 "" KfAssocNoBackup ; not yet defined, no need to backup
  StrCmp $1 "NetImmerseFile" KfAssocNoBackup ; our definition, no need to backup

    WriteRegStr HKCR ".kf" "backup_val" $1

KfAssocNoBackup:

  ; associate Kfa files with NifSkope
  ReadRegStr $1 HKCR ".kfa" ""
  StrCmp $1 "" KfaAssocNoBackup ; not yet defined, no need to backup
  StrCmp $1 "NetImmerseFile" KfaAssocNoBackup ; our definition, no need to backup

    WriteRegStr HKCR ".kfa" "backup_val" $1

KfaAssocNoBackup:

  ; write out the file association details for NetImmerseFile
  WriteRegStr HKCR ".nif" "" "NetImmerseFile"
  WriteRegStr HKCR ".nifcache" "" "NetImmerseFile"
  WriteRegStr HKCR ".pcpatch" "" "NetImmerseFile"
  WriteRegStr HKCR ".texcache" "" "NetImmerseFile"
  WriteRegStr HKCR ".jmi" "" "NetImmerseFile"
  WriteRegStr HKCR ".kf" "" "NetImmerseAnim"
  WriteRegStr HKCR ".kfa" "" "NetImmerseAnim"
  WriteRegStr HKCR ".kfm" "" "NetImmerseAnimMgr"
  
  ReadRegStr $0 HKCR "NetImmerseFile" ""
  StrCmp $0 "" 0 NifModelSkip ; if our association is already defined, skip it
  WriteRegStr HKCR "NetImmerseFile" "" "NetImmerse/Gamebryo Model File"
  WriteRegStr HKCR "NetImmerseFile\shell" "" "open"
  WriteRegStr HKCR "NetImmerseFile\DefaultIcon" "" "$INSTDIR\nif_file.ico"

NifModelSkip: ; make sure we write the correct install path to NifSkope, so we must write these
  WriteRegStr HKCR "NetImmerseFile\shell\open\command" "" '$INSTDIR\NifSkope.exe "%1"'
  WriteRegStr HKCR "NetImmerseFile\shell\edit" "" "Edit NIF File"
  WriteRegStr HKCR "NetImmerseFile\shell\edit\command" "" '$INSTDIR\NifSkope.exe "%1"'
  ReadRegStr $0 HKCR "NetImmerseAnim" ""
  StrCmp $0 "" 0 NifAnimSkip ; if our association is already defined, skip it
  WriteRegStr HKCR "NetImmerseAnim" "" "NetImmerse/Gamebryo Animation File"
  WriteRegStr HKCR "NetImmerseAnim\shell" "" "open"
  WriteRegStr HKCR "NetImmerseAnim\DefaultIcon" "" "$INSTDIR\nif_file.ico"

NifAnimSkip: ; make sure we write the correct install path to NifSkope, so we must write these
  WriteRegStr HKCR "NetImmerseAnim\shell\open\command" "" '$INSTDIR\NifSkope.exe "%1"'
  WriteRegStr HKCR "NetImmerseAnim\shell\edit" "" "Edit KF File"
  WriteRegStr HKCR "NetImmerseAnim\shell\edit\command" "" '$INSTDIR\NifSkope.exe "%1"'
  ReadRegStr $0 HKCR "NetImmerseAnimMgr" ""
  StrCmp $0 "" 0 NifAssocSkip ; if our association is already defined, skip it
  WriteRegStr HKCR "NetImmerseAnimMgr" "" "NetImmerse/Gamebryo Animation Manager File"
  WriteRegStr HKCR "NetImmerseAnimMgr\shell" "" "open"
  WriteRegStr HKCR "NetImmerseAnimMgr\DefaultIcon" "" "$INSTDIR\nif_file.ico"
  
NifAssocSkip: ; make sure we write the correct install path to NifSkope, so we must write these
  WriteRegStr HKCR "NetImmerseAnimMgr\shell\open\command" "" '$INSTDIR\NifSkope.exe "%1"'
  WriteRegStr HKCR "NetImmerseAnimMgr\shell\edit" "" "Edit KFM File"
  WriteRegStr HKCR "NetImmerseAnimMgr\shell\edit\command" "" '$INSTDIR\NifSkope.exe "%1"'

  ; Cleanup old dll files
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\imageformats\*.dll
  Delete $INSTDIR\Copyright.txt
  
  ; Install NifSkope
  SetOutPath $INSTDIR
  File ${BUILD_RELEASE_FOLDER}\NifSkope.exe
  File ..\README.TXT
  File ..\CHANGELOG.TXT
  File ..\LICENSE.TXT
  ; copied by makeexe.bat
  File ..\Qhull_COPYING.TXT
  File ..\docsys\nifxml\nif.xml
  File ..\docsys\kfmxml\kfm.xml
  File nif_file.ico
  File ..\style.qss
  File ..\NifMopp.dll
  
  ; Install shaders
  SetOutPath $INSTDIR\shaders
  File ..\shaders\*.prog
  File ..\shaders\*.vert
  File ..\shaders\*.frag

  ; Install languages
  SetOutPath $INSTDIR\lang
  File ..\lang\*.ts
  File ..\lang\*.qm

  ; Install reference manual
  ; run ..\docsys\nifxml_doc.py to update the HTML files before compiling the install script
  SetOutPath $INSTDIR\doc
  File ..\docsys\doc\*.html
  File ..\docsys\doc\docsys.css
  File ..\docsys\doc\favicon.ico

  ; Install shortcuts
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\NifTools\NifSkope\"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\NifSkope.lnk" "$INSTDIR\NifSkope.exe"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Readme.lnk" "$INSTDIR\README.TXT"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Changelog.lnk" "$INSTDIR\CHANGELOG.TXT"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Support.lnk" "http://www.niftools.org/forum/viewforum.php?f=24"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Development.lnk" "http://www.niftools.org/forum/viewforum.php?f=4"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Copyright.lnk" "$INSTDIR\LICENSE.TXT"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Uninstall.lnk" "$INSTDIR\uninstall.exe"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NifSkope "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys & uninstaller for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NifSkope" "DisplayName" "NifSkope (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NifSkope" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteUninstaller "uninstall.exe"

  ; call installer hook for extra install stuff
  !insertmacro InstallHook
  
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  SetAutoClose false

  ; restore file association for .nif
  ReadRegStr $1 HKCR ".nif" ""
  StrCmp $1 "NetImmerseFile" 0 NifAssocNoOwn ; only do this if we own it

    ReadRegStr $1 HKCR ".nif" "backup_val"
    StrCmp $1 "" 0 NifAssocRestore ; if backup="" then delete the whole key

      DeleteRegKey HKCR ".nif"

    Goto NifAssocNoOwn

NifAssocRestore:
      WriteRegStr HKCR ".nif" "" $1
      DeleteRegValue HKCR ".nif" "backup_val"
      DeleteRegKey HKCR "NetImmerseFile" ;Delete key with association settings

NifAssocNoOwn:

  ; restore file association for .kf
  ReadRegStr $1 HKCR ".kf" ""
  StrCmp $1 "NetImmerseFile" 0 KfAssocNoOwn ; only do this if we own it

    ReadRegStr $1 HKCR ".kf" "backup_val"
    StrCmp $1 "" 0 KfAssocRestore ; if backup="" then delete the whole key

      DeleteRegKey HKCR ".kf"

    Goto KfAssocNoOwn

KfAssocRestore:
      WriteRegStr HKCR ".kf" "" $1
      DeleteRegValue HKCR ".kf" "backup_val"
      DeleteRegKey HKCR "NetImmerseFile" ;Delete key with association settings

KfAssocNoOwn:

  ; restore file association for .kfa
  ReadRegStr $1 HKCR ".kfa" ""
  StrCmp $1 "NetImmerseFile" 0 KfaAssocNoOwn ; only do this if we own it

    ReadRegStr $1 HKCR ".kfa" "backup_val"
    StrCmp $1 "" 0 KfaAssocRestore ; if backup="" then delete the whole key

      DeleteRegKey HKCR ".kfa"

    Goto KfaAssocNoOwn

KfaAssocRestore:
      WriteRegStr HKCR ".kfa" "" $1
      DeleteRegValue HKCR ".kfa" "backup_val"
      DeleteRegKey HKCR "NetImmerseFile" ;Delete key with association settings

KfaAssocNoOwn:

  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NifSkope"
  DeleteRegKey HKLM "SOFTWARE\NifSkope"

  ; remove program files and program directory
  Delete "$INSTDIR\doc\*.*"
  Delete "$INSTDIR\shaders\*.*"
  Delete "$INSTDIR\lang\*.*"
  Delete "$INSTDIR\imageformats\*.*"
  Delete "$INSTDIR\*.*"
  RMDir "$INSTDIR\doc"
  RMDir "$INSTDIR\shaders"
  RMDir "$INSTDIR\lang"
  RMDir "$INSTDIR\imageformats"
  RMDir "$INSTDIR"

  ; remove links in start menu
  Delete "$SMPROGRAMS\NifTools\NifSkope\*.*"
  RMDir "$SMPROGRAMS\NifTools\NifSkope"
  RMDir "$SMPROGRAMS\NifTools" ; this will only delete if the directory is empty
SectionEnd
