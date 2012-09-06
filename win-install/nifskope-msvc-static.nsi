; *** REQUIRES MOREINFO PLUGIN ***
; Download the MoreInfo zip file from  http://nsis.sourceforge.net/MoreInfo_plug-in
; and copy the Plugins\MoreInfo.dll file to your NSIS Plugins folder.

; NifSkope Self-Installer for Windows (MSVC build)
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

SetCompressor /SOLID lzma

!define INSTALLERPOSTFIX "windows"

!include "FileFunc.nsh"
!include "WordFunc.nsh"

!insertmacro Locate
!insertmacro VersionCompare

;--------------------------------
; Functions

Var DLL_found

# Uses $0
Function openLinkNewWindow
  Push $3 
  Push $2
  Push $1
  Push $0
  ReadRegStr $0 HKCR "http\shell\open\command" ""
# Get browser path
    DetailPrint $0
  StrCpy $2 '"'
  StrCpy $1 $0 1
  StrCmp $1 $2 +2 # if path is not enclosed in " look for space as final char
    StrCpy $2 ' '
  StrCpy $3 1
  loop:
    StrCpy $1 $0 1 $3
    DetailPrint $1
    StrCmp $1 $2 found
    StrCmp $1 "" found
    IntOp $3 $3 + 1
    Goto loop
 
  found:
    StrCpy $1 $0 $3
    StrCmp $2 " " +2
      StrCpy $1 '$1"'
 
  Pop $0
  Exec '$1 $0'
  Pop $1
  Pop $2
  Pop $3
FunctionEnd

!define DLL_VER "9.00.21022.8"

Function LocateCallback

	MoreInfo::GetProductVersion "$R9"
	Pop $0

        ${VersionCompare} "$0" "${DLL_VER}" $R1

        StrCmp $R1 0 0 new
      new:
        StrCmp $R1 1 0 old
      old:
        StrCmp $R1 2 0 end
	; Found DLL is older
        Call DownloadDLL

     end:
	StrCpy "$0" StopLocate
	StrCpy $DLL_found "true"
	Push "$0"

FunctionEnd

Function DownloadDLL
    MessageBox MB_OK "You will need to download the Microsoft Visual C++ 2008 Redistributable Package in order to run NifSkope. Pressing OK will take you to the download page, please follow the instructions on the page that appears."
    StrCpy $0 "http://www.microsoft.com/downloads/details.aspx?familyid=9b2da534-3e03-4391-8a4d-074b9f2bc1bf&displaylang=en"
    Call openLinkNewWindow
FunctionEnd

!macro InstallHook
  ; Check for msvcr90.dll - give notice to download if not found
  MessageBox MB_OK "The installer will now check your system for the required system dlls."
  StrCpy $1 $WINDIR
  StrCpy $DLL_found "false"
  ${Locate} "$1" "/L=F /M=MSVCR90.DLL /S=0B" "LocateCallback"
  StrCmp $DLL_found "false" 0 +2
    Call DownloadDLL
!macroend

!include "nifskope.nsh"

