;
; NifSkope Self-Installer for Windows
; (NifTools - http://niftools.sourceforge.net) 
; (NSIS - http://nsis.sourceforge.net)
;
; Copyright (c) 2005, NIF File Format Library and Tools
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

!define VERSION "0.7"

Name "NifSkope ${VERSION}"

; define installer pages
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE

!define MUI_WELCOMEPAGE_TEXT  "This wizard will guide you through the installation of NifSkope ${VERSION}.\r\n\r\nIt is recommended that you close all other applications.\r\n\r\nNote to Win2k/XP users: you require administrator privileges to install NifSkope successfully."
!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_LICENSE Copyright.txt

!define MUI_DIRECTORYPAGE_TEXT_TOP "Use the field below to specify the folder where you want NifSkope to be copied to. To specify a different folder, type a new name or use the Browse button to select an existing folder."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "NifSkope Folder"
!define MUI_DIRECTORYPAGE_VARIABLE $INSTDIR
!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.TXT"
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

OutFile "nifskope_${VERSION}-windows.exe"
InstallDir "$PROGRAMFILES\NifTools\NifSkope"
BrandingText "http://niftools.sourceforge.net/"
Icon "../icon.ico"
UninstallIcon "../icon.ico" ; TODO create uninstall icon
ShowInstDetails show
ShowUninstDetails show

;--------------------------------
; Functions

Section
  SectionIn RO

  SetShellVarContext all

  ; Cleanup
  ; Nothing here yet...

  ; associate NIF files with NifSkope
  ReadRegStr $1 HKCR ".nif" ""
  StrCmp $1 "" NoBackup ; not yet defined, no need to backup
  StrCmp $1 "NetImmerseFile" NoBackup ; our definition, no need to backup

    WriteRegStr HKCR ".nif" "backup_val" $1

NoBackup:
  WriteRegStr HKCR ".nif" "" "NetImmerseFile"
  ReadRegStr $0 HKCR "NetImmerseFile" ""
  StrCmp $0 "" 0 "Skip" ; if our association is already defined, skip it
  
    WriteRegStr HKCR "NetImmerseFile" "" "NetImmerse/Gamebryo File"
    WriteRegStr HKCR "NetImmerseFile\shell" "" "open"
    WriteRegStr HKCR "NetImmerseFile\DefaultIcon" "" "$INSTDIR\nif.ico"

Skip: ; make sure we write the correct install path to NifSkope, so we must write these
  WriteRegStr HKCR "NetImmerseFile\shell\open\command" "" '$INSTDIR\NifSkope.exe "%1"'
  WriteRegStr HKCR "NetImmerseFile\shell\edit" "" "Edit NIF File"
  WriteRegStr HKCR "NetImmerseFile\shell\edit\command" "" '$INSTDIR\NifSkope.exe "%1"'

  ; Install NifSkope
  SetOutPath $INSTDIR
  File ..\NifSkope.exe
  File ..\mingwm10.dll
  File ..\README.TXT
  File Copyright.txt
  File nif.ico

  ; Install shortcuts
  CreateDirectory "$SMPROGRAMS\NifTools\NifSkope\"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\NifSkope.lnk" "$INSTDIR\NifSkope.exe"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Readme.lnk" "$INSTDIR\README.TXT"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Support.lnk" "http://niftools.sourceforge.net/forum/viewforum.php?f=6"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Development.lnk" "http://niftools.sourceforge.net/forum/viewforum.php?f=4"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Copyright.lnk" "$INSTDIR\Copyright.txt"
  CreateShortCut "$SMPROGRAMS\NifTools\NifSkope\Uninstall.lnk" "$INSTDIR\uninstall.exe"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NifSkope "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys & uninstaller for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NifSkope" "DisplayName" "NifSkope (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NifSkope" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  SetAutoClose false

  ; restore file association
  ReadRegStr $1 HKCR ".nif" ""
  StrCmp $1 "NetImmerseFile" 0 NoOwn ; only do this if we own it

    ReadRegStr $1 HKCR ".nif" "backup_val"
    StrCmp $1 "" 0 Restore ; if backup="" then delete the whole key

      DeleteRegKey HKCR ".nif"

    Goto NoOwn

Restore:
      WriteRegStr HKCR ".nif" "" $1
      DeleteRegValue HKCR ".nif" "backup_val"
      DeleteRegKey HKCR "NetImmerseFile" ;Delete key with association settings

NoOwn:

  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NifSkope"
  DeleteRegKey HKLM "SOFTWARE\NifSkope"

  ; remove program files and program directory
  Delete "$INSTDIR\*.*"
  RMDir "$INSTDIR"

  ; remove links in start menu
  Delete "$SMPROGRAMS\NifTools\NifSkope\*.*"
  RMDir "$SMPROGRAMS\NifTools\NifSkope"
  RMDir "$SMPROGRAMS\NifTools" ; this will only delete if the directory is empty
SectionEnd
