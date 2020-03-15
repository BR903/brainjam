; installer.nsi: script for building the Brain Jam windows installer.

Unicode True
RequestExecutionLevel user

!include LogicLib.nsh
!include nsDialogs.nsh
!include WinMessages.nsh

; The name of the installer
Name "Brain Jam"
OutFile "installer.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Brain Jam"
InstallDirRegKey HKCU "Software\Brain Jam" ""

Page custom MakeDialog RecordChoices
Page InstFiles

UninstPage UninstConfirm
UninstPage InstFiles

; Variables for the user's choices.
var DirSelect
var MenuItems
var Shortcut

; Installing
;
Section "Install"

  ; Installing the program
  ;
  StrCpy $INSTDIR $DirSelect
  SetOutPath "$DirSelect"
  File "brainjam.exe"
  File "README.txt"
  File "libfreetype-6.dll"
  File "libpng16-16.dll"
  File "SDL2.dll"
  File "SDL2_ttf.dll"
  File "zlib1.dll"
  WriteRegStr HKCU "Software\Brain Jam" "" "$DirSelect"
  WriteUninstaller "$DirSelect\uninstall.exe"

  ; Modifying the Start menu
  ;
  ${If} $MenuItems != 0
    CreateDirectory "$SMPROGRAMS\Brain Jam"
    CreateShortcut "$SMPROGRAMS\Brain Jam\Brain Jam.lnk" \
                   "$INSTDIR\brainjam.exe"
    CreateShortcut "$SMPROGRAMS\Brain Jam\Uninstall.lnk" \
                   "$INSTDIR\uninstall.exe"
  ${EndIf}

  ; Adding a desktop shortcut
  ;
  ${If} $Shortcut != 0
    CreateShortcut "$DESKTOP\Brain Jam.lnk" "$INSTDIR\brainjam.exe"
  ${EndIf}

SectionEnd

; Uninstalling
;
Section "Uninstall"
  Delete "$DESKTOP\Brain Jam.lnk"
  Delete "$SMPROGRAMS\Brain Jam\*.*"
  RMDir "$SMPROGRAMS\Brain Jam"
  Delete "$INSTDIR\brainjam.exe"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\libfreetype-6.dll"
  Delete "$INSTDIR\libpng16-16.dll"
  Delete "$INSTDIR\SDL2.dll"
  Delete "$INSTDIR\SDL2_ttf.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey /ifempty HKCU "Software\Brain Jam"
SectionEnd

;
; The dialog box.
;

; The dialog controls that hold the user's choices.
;
var CtrlDirSelect
var CtrlMenuItems
var CtrlShortcut

; Lay out the contents of the dialog box, one element at a time.
;
Function MakeDialog

  nsDialogs::Create 1018

  ${NSD_CreateLabel} 0 0 224u 8u \
      "This program will install Brain Jam on your computer."

  ${NSD_CreateLabel} -40u 0 40u 8u "(v${VER}) "
  Pop $0
  ${NSD_AddStyle} $0 ${SS_RIGHT}

  ${NSD_CreateGroupBox} 0 12u 100% 80u "Options"

  ${NSD_CreateLabel} 8u 24u -16u 8u \
      "Brain Jam will be installed in the folder shown below."
  ${NSD_CreateLabel} 8u 32u -16u 8u \
      "To use a different folder, enter it here (or use the button to browse)."

  ${NSD_CreateDirRequest} 12u 44u 200u 12u "Select a folder"
  Pop $CtrlDirSelect
  ${NSD_SetText} $CtrlDirSelect "C:\Program Files\Brain Jam"
  ${NSD_CreateBrowseButton} 216u 44u 40u 12u "Browse"
  Pop $0
  ${NSD_OnClick} $0 BrowseForDir

  ${NSD_CreateCheckbox} 12u 64u 80% 8u "Add an entry to the Start &Menu"
  Pop $CtrlMenuItems
  ${NSD_Check} $CtrlMenuItems

  ${NSD_CreateCheckbox} 12u 76u 80% 8u "Create a shortcut on the &desktop"
  Pop $CtrlShortcut
  ${NSD_Check} $CtrlShortcut

  ${NSD_CreateLabel} 0 96u 100% 16u \
      "Brain Jam is free software. See the README.txt included with\
 this program for more information, or visit this program's web page."

  ${NSD_CreateLink} 16u 116u -32u 8u \
      "http://www.muppetlabs.com/~breadbox/software/brainjam.html    "
  Pop $0
  ${NSD_OnClick} $0 LinkClick

  nsDialogs::Show

FunctionEnd

; Callback for the browse button: Open a folder-selection dialog, and
; then copy the selected folder to the DirRequest control.
;
Function BrowseForDir
  nsDialogs::SelectFolderDialog /NOUNLOAD "Select a folder"
  Pop $0
  ${If} $0 == error
  ${Else}
    ${NSD_SetText} $CtrlDirSelect $0
  ${EndIf}
FunctionEnd

; Callback for a link: Hand the link text off to the Shell.
;
Function LinkClick
  Pop $0
  ${NSD_GetText} $0 $1
  ExecShell "" $1
FunctionEnd

; Record the states of the dialog controls.
;
Function RecordChoices
  ${NSD_GetText} $CtrlDirSelect $DirSelect
  ${NSD_GetState} $CtrlMenuItems $MenuItems
  ${NSD_GetState} $CtrlShortcut $Shortcut
FunctionEnd
