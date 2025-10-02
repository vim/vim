# -*- coding: utf-8 -*-
# NSIS file to create a self-installing exe for Vim.
# It requires NSIS version 3.0 or later.
# Last Change:	2025-09-30
#

Unicode true  ; !include defaults to UTF-8 after Unicode True since 3.0 Alpha 2

# WARNING: if you make changes to this script, look out for $0 to be valid,
# because uninstall deletes most files in $0.

# Location of gvim_ole.exe, vimw32.exe, GvimExt/*, etc.
!ifndef VIMSRC
  !define VIMSRC "..\src"
!endif

# Location of runtime files
!ifndef VIMRT
  !define VIMRT "..\runtime"
!endif

# Location of extra tools: diff.exe, winpty{32|64}.dll, winpty-agent.exe, etc.
!ifndef VIMTOOLS
  !define VIMTOOLS "..\.."
!endif

# Location of gettext.
# It must contain two directories: gettext32 and gettext64.
# See README.txt for detail.
!ifndef GETTEXT
  !define GETTEXT ${VIMTOOLS}
!endif

# If you have UPX, use the switch /DHAVE_UPX=1 on the command line makensis.exe.
# This property will be set to 1. Get it at https://upx.github.io/
!ifndef HAVE_UPX
  !define HAVE_UPX 0
!endif

# If you do not want to add Native Language Support, use the switch /DHAVE_NLS=0
# in the command line makensis.exe. This property will be set to 0.
!ifndef HAVE_NLS
  !define HAVE_NLS 1
!endif

# To create an English-only the installer, use the switch /DHAVE_MULTI_LANG=0 on
# the command line makensis.exe. This property will be set to 0.
!ifndef HAVE_MULTI_LANG
  !define HAVE_MULTI_LANG 1
!endif

# if you want to create a 64-bit the installer, use the switch /DWIN64=1 on
# the command line makensis.exe. This property will be set to 1.
!ifndef WIN64
  !define WIN64 0
!endif

# if you want to create the installer for ARM64, use the /DARM64=1 on
# the command line makensis.exe. This property will be set to 1.
!ifndef ARM64
  !define ARM64 0
!else
  !if ${ARM64} > 0
    !if ${WIN64} < 1
      !define /redef WIN64 1
    !endif
  !endif
!endif

# if you don't want to include libgcc_s_sjlj-1.dll in the package, use the
# switch /DINCLUDE_LIBGCC=0 on the command line makensis.exe.
!ifndef INCLUDE_LIBGCC
  !define INCLUDE_LIBGCC 1
!endif

# Get version numbers
!getdllversion "${VIMSRC}\gvim_ole.exe" VimVer_
!echo "Vim version MAJOR=${VimVer_1} MINOR=${VimVer_2} PATCHLEVEL=${VimVer_3}"

!ifndef VER_MAJOR
  !define VER_MAJOR  ${VimVer_1}
!endif
!ifndef VER_MINOR
  !define VER_MINOR  ${VimVer_2}
!endif
!ifndef PATCHLEVEL
  !define PATCHLEVEL ${VimVer_3}
!endif

# ----------- No configurable settings below this line -----------

##########################################################
# Installer Attributes, Including headers, Plugins and etc. 

CRCCheck force

SetCompressor /SOLID lzma
SetCompressorDictSize 64
SetDatablockOptimize on

!if ${HAVE_UPX}
  !packhdr temp.dat "upx.exe --best --compress-icons=1 temp.dat"
!endif

RequestExecutionLevel highest
ManifestDPIAware true
# https://github.com/NSIS-Dev/nsis/blob/691211035c2aaaebe8fbca48ee02d4de93594a52/Docs/src/attributes.but#L292
ManifestDPIAwareness "PerMonitorV2,System"
ManifestSupportedOS \
    {35138b9a-5d96-4fbd-8e2d-a2440225f93a} /* WinNT 6.1 */ \
    {4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38} /* WinNT 6.2 */ \
    {1f676c76-80e1-4239-95bb-83d0f6d0da78} /* WinNT 6.3 */ \
    {8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a} /* WinNT 10/11 */

!define PRODUCT         "Vim ${VER_MAJOR}.${VER_MINOR}"
!define UNINST_REG_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall"
!define UNINST_REG_KEY_VIM  "${UNINST_REG_KEY}\${PRODUCT}"

!if ${WIN64}
  !define BIT 64
# This adds '\Vim' to the user choice automagically.  The actual value is
# obtained below with CheckOldVim.
  !define DEFAULT_INSTDIR "$PROGRAMFILES64\Vim"
  !if ${ARM64}
    Name "${PRODUCT} (ARM64)"
  !else
    Name "${PRODUCT} (x64)"
  !endif
!else
  !define BIT 32
  !define DEFAULT_INSTDIR "$PROGRAMFILES\Vim"
  Name "${PRODUCT}"
!endif

OutFile gvim${VER_MAJOR}${VER_MINOR}.exe
InstallDir ${DEFAULT_INSTDIR}
BrandingText "Vim - the text editor"

# Types of installs we can perform:
InstType $(str_type_typical)
InstType $(str_type_minimal)
InstType $(str_type_full)

SilentInstall normal

##########################################################
# Version resources

VIFileVersion ${VER_MAJOR}.${VER_MINOR}.${PATCHLEVEL}.0
VIProductVersion ${VER_MAJOR}.${VER_MINOR}.${PATCHLEVEL}.0
VIAddVersionKey /LANG=0 "ProductName" "Vim"
VIAddVersionKey /LANG=0 "CompanyName" "The Vim Project"
VIAddVersionKey /LANG=0 "LegalTrademarks" "Vim"
VIAddVersionKey /LANG=0 "LegalCopyright" "Copyright (C) 1996"
VIAddVersionKey /LANG=0 "FileDescription" \
    "Vi Improved - A Text Editor"
VIAddVersionKey /LANG=0 "ProductVersion" \
    "${VER_MAJOR}.${VER_MINOR}.${PATCHLEVEL}.0"
VIAddVersionKey /LANG=0 "FileVersion" \
    "${VER_MAJOR}.${VER_MINOR}.${PATCHLEVEL}.0"

##########################################################
# including headers

!include "Library.nsh"		; for DLL install
!include "LogicLib.nsh"
!include "MUI2.nsh"		; new user interface
!include "nsDialogs.nsh"
!include "Sections.nsh"		; for section control
!include "x64.nsh"

!include .\auxiliary.nsh	; helper file

##########################################################
# MUI2 settings

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

!define MUI_ICON   "icons\in_vim_32bpp.ico"
!define MUI_UNICON "icons\un_vim_32bpp.ico"

# Show all languages, despite user's codepage:
!define MUI_LANGDLL_ALLLANGUAGES
# Always show dialog choice language
#!define MUI_LANGDLL_ALWAYSSHOW
!define MUI_LANGDLL_REGISTRY_ROOT	"HKCU"
!define MUI_LANGDLL_REGISTRY_KEY	"Software\Vim"
!define MUI_LANGDLL_REGISTRY_VALUENAME  "Installer Language"

!define MUI_WELCOMEFINISHPAGE_BITMAP	"icons\in_welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP	"icons\un_welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP		"icons\in_header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP	"icons\un_header.bmp"

!define MUI_WELCOMEFINISHPAGE_BITMAP_STRETCH	"AspectFitHeight"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP_STRETCH  "AspectFitHeight"
!define MUI_HEADERIMAGE_BITMAP_STRETCH		"AspectFitHeight"
!define MUI_HEADERIMAGE_UNBITMAP_STRETCH	"AspectFitHeight"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_LICENSEPAGE_CHECKBOX
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_TEXT		$(str_show_readme)
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION	LaunchApplication

# Installer pages:
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE $(page_lic_file)
!insertmacro MUI_PAGE_COMPONENTS
Page custom SetCustom ValidateCustom
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!insertmacro MUI_PAGE_FINISH

# Uninstaller pages:
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!insertmacro MUI_UNPAGE_FINISH

##########################################################
# Languages Files

!insertmacro MUI_RESERVEFILE_LANGDLL
!include "lang\english.nsi"

# Include support for other languages:
!if ${HAVE_MULTI_LANG}
  !include "lang\danish.nsi"
  !include "lang\dutch.nsi"
  !include "lang\german.nsi"
  !include "lang\greek.nsi"
  !include "lang\italian.nsi"
  !include "lang\japanese.nsi"
  !include "lang\portuguesebr.nsi"
  !include "lang\russian.nsi"
  !include "lang\serbian.nsi"
  !include "lang\simpchinese.nsi"
  !include "lang\tradchinese.nsi"
  !include "lang\turkish.nsi"
!endif

##########################################################
# Global variables
Var vim_dialog
Var vim_nsd_compat
Var vim_nsd_keymap
Var vim_nsd_mouse
Var vim_compat_stat
Var vim_keymap_stat
Var vim_mouse_stat

##########################################################
# Reserve files
ReserveFile ${VIMSRC}\installw32.exe

##########################################################
# Functions

# Check if Vim is already installed.
# return: Installed directory. If not found, it will be empty.
Function CheckOldVim
  Push $0
  Push $R0
  Push $R1
  Push $R2

  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}

  ClearErrors
  StrCpy $0 ""   ; Installed directory
  StrCpy $R0 0   ; Sub-key index
  StrCpy $R1 ""  ; Sub-key
  ${Do}
    # Enumerate the sub-key:
    EnumRegKey $R1 HKLM ${UNINST_REG_KEY} $R0

    # Stop if no more sub-key:
    ${If} ${Errors}
    ${OrIf} $R1 == ""
      ${ExitDo}
    ${EndIf}

    # Move to the next sub-key:
    IntOp $R0 $R0 + 1

    # Check if the key is Vim uninstall key or not:
    StrCpy $R2 $R1 4
    ${If} $R2 S!= "Vim "
      ${Continue}
    ${EndIf}

    # Verifies required sub-keys:
    ReadRegStr $R2 HKLM "${UNINST_REG_KEY}\$R1" "DisplayName"
    ${If} ${Errors}
    ${OrIf} $R2 == ""
      ${Continue}
    ${EndIf}

    ReadRegStr $R2 HKLM "${UNINST_REG_KEY}\$R1" "UninstallString"
    ${If} ${Errors}
    ${OrIf} $R2 == ""
      ${Continue}
    ${EndIf}

    # Found
    Push $R2
    call GetParent
    call GetParent
    Pop $0  ; Vim directory
    ${ExitDo}

  ${Loop}

  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}

  Pop $R2
  Pop $R1
  Pop $R0
  Exch $0  ; put $0 on top of stack, restore $0 to original value
FunctionEnd

Function LaunchApplication
  SetOutPath $0
  !if ${HAVE_NLS}
    ShellExecAsUser::ShellExecAsUser "" "$0\gvim.exe" \
	'-R "$0\$(vim_readme_file)"'
  !else
    ShellExecAsUser::ShellExecAsUser "" "$0\gvim.exe" '-R "$0\README.txt"'
  !endif
FunctionEnd

##########################################################
# Installer Functions and Sections

Section "$(str_section_old_ver)" id_section_old_ver
  SectionIn 1 2 3 RO

  # run the install program to check for already installed versions
  SetOutPath $TEMP
  File /oname=install.exe ${VIMSRC}\installw32.exe
  DetailPrint "$(str_msg_uninstalling)"
  ${Do}
    nsExec::Exec "$TEMP\install.exe -uninstall-check"
    Pop $3

    call CheckOldVim
    Pop $3
    ${If} $3 == ""
      ${ExitDo}
    ${Else}
      # It seems that the old version is still remaining.
      # TODO: Should we show a warning and run the uninstaller again?

      ${ExitDo}  ; Just ignore for now.
    ${EndIf}
  ${Loop}
  Delete $TEMP\install.exe
  Delete $TEMP\vimini.ini   ; install.exe creates this, but we don't need it.

  # We may have been put to the background when uninstall did something.
  BringToFront
SectionEnd

##########################################################
Section "$(str_section_exe)" id_section_exe
  SectionIn 1 2 3 RO

  # we need also this here if the user changes the instdir
  StrCpy $0 "$INSTDIR\vim${VER_MAJOR}${VER_MINOR}"

  SetOutPath $0
  File /oname=gvim.exe ${VIMSRC}\gvim_ole.exe
  !if /FileExists "${VIMSRC}\vim${BIT}.dll"
    File ${VIMSRC}\vim${BIT}.dll
  !endif
  File /oname=install.exe ${VIMSRC}\installw32.exe
  File /oname=uninstall.exe ${VIMSRC}\uninstallw32.exe
  File ${VIMSRC}\vimrun.exe
  File /oname=tee.exe ${VIMSRC}\teew32.exe
  File /oname=xxd.exe ${VIMSRC}\xxdw32.exe
  File ..\vimtutor.bat
  File ..\README.txt
  File /oname=LICENSE.txt ..\LICENSE
  File ..\uninstall.txt
  File ${VIMRT}\*.vim

  !if /FileExists "${VIMTOOLS}\diff.exe"
    File ${VIMTOOLS}\diff.exe
  !endif
  !if /FileExists "${VIMTOOLS}\winpty${BIT}.dll"
    File ${VIMTOOLS}\winpty${BIT}.dll
  !endif
  !if /FileExists "${VIMTOOLS}\winpty-agent.exe"
    File ${VIMTOOLS}\winpty-agent.exe
  !endif
  !if /FileExists "${VIMTOOLS}\libsodium.dll"
    File ${VIMTOOLS}\libsodium.dll
  !endif

  SetOutPath $0\colors
  File /r ${VIMRT}\colors\*.*

  SetOutPath $0\compiler
  File ${VIMRT}\compiler\*.*

  SetOutPath $0\doc
  File /x uganda.nsis.txt ${VIMRT}\doc\*.txt
  File ${VIMRT}\doc\tags

  SetOutPath $0\ftplugin
  File ${VIMRT}\ftplugin\*.*

  SetOutPath $0\indent
  File ${VIMRT}\indent\README.txt
  File ${VIMRT}\indent\*.vim

  SetOutPath $0\keymap
  File ${VIMRT}\keymap\README.txt
  File ${VIMRT}\keymap\*.vim

  SetOutPath $0\macros
  File /r /x *.info ${VIMRT}\macros\*.*

  SetOutPath $0\pack
  File /r ${VIMRT}\pack\*.*

  SetOutPath $0\plugin
  File ${VIMRT}\plugin\*.*

  SetOutPath $0\autoload
  File /r ${VIMRT}\autoload\*.*

  SetOutPath $0\import\dist
  File ${VIMRT}\import\dist\*.*

  SetOutPath $0\bitmaps
  File ${VIMSRC}\vim.ico

  SetOutPath $0\syntax
  File /r /x testdir /x generator /x Makefile ${VIMRT}\syntax\*.*

  SetOutPath $0\spell
  File ${VIMRT}\spell\*.txt
  File ${VIMRT}\spell\*.vim
  File ${VIMRT}\spell\*.spl
  File ${VIMRT}\spell\*.sug

  SetOutPath $0\tools
  File ${VIMRT}\tools\*.*

  SetOutPath $0\tutor
  File /r /x *.info ${VIMRT}\tutor\*.*
SectionEnd

##########################################################
Section "$(str_section_console)" id_section_console
  SectionIn 1 3

  SetOutPath $0
  File /oname=vim.exe ${VIMSRC}\vimw32.exe
  StrCpy $2 "$2 vim view vimdiff"
SectionEnd

##########################################################
Section "$(str_section_batch)" id_section_batch
  SectionIn 3

  StrCpy $1 "$1 -create-batfiles $2"
SectionEnd

##########################################################
SectionGroup $(str_group_icons) id_group_icons
  Section "$(str_section_desktop)" id_section_desktop
    SectionIn 1 3

    StrCpy $1 "$1 -install-icons"
  SectionEnd

  Section "$(str_section_start_menu)" id_section_startmenu
    SectionIn 1 3

    StrCpy $1 "$1 -add-start-menu"
  SectionEnd
SectionGroupEnd

##########################################################
Section "$(str_section_edit_with)" id_section_editwith
  SectionIn 1 3

  SetOutPath $0

  ${If} ${RunningX64}
    # Install 64-bit gvimext.dll into the GvimExt64 directory.
    SetOutPath $0\GvimExt64
    ClearErrors
    !define LIBRARY_SHELL_EXTENSION
    !define LIBRARY_X64
    !insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"${VIMSRC}\GvimExt\gvimext64.dll" "$0\GvimExt64\gvimext.dll" "$0"
    !undef LIBRARY_X64
    !undef LIBRARY_SHELL_EXTENSION
  ${EndIf}

  # Install 32-bit gvimext.dll into the GvimExt32 directory.
  SetOutPath $0\GvimExt32
  ClearErrors
  !define LIBRARY_SHELL_EXTENSION
  !insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
      "${VIMSRC}\GvimExt\gvimext.dll" "$0\GvimExt32\gvimext.dll" "$0"
  !undef LIBRARY_SHELL_EXTENSION

  # We don't have a separate entry for the "Open With..." menu, assume
  # the user wants either both or none.
  StrCpy $1 "$1 -install-popup -install-openwith"
SectionEnd

##########################################################
Section "$(str_section_vim_rc)" id_section_vimrc
  SectionIn 1 3

  StrCpy $1 "$1 -create-vimrc"

  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  WriteRegStr HKLM "${UNINST_REG_KEY_VIM}" "vim_compat"   "$vim_compat_stat"
  WriteRegStr HKLM "${UNINST_REG_KEY_VIM}" "vim_keyremap" "$vim_keymap_stat"
  WriteRegStr HKLM "${UNINST_REG_KEY_VIM}" "vim_mouse"    "$vim_mouse_stat"
  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}

  ${If} $vim_compat_stat == "vi"
    StrCpy $1 "$1 -vimrc-compat vi"
  ${ElseIf} $vim_compat_stat == "vim"
    StrCpy $1 "$1 -vimrc-compat vim"
  ${ElseIf} $vim_compat_stat == "defaults"
    StrCpy $1 "$1 -vimrc-compat defaults"
  ${Else}
    StrCpy $1 "$1 -vimrc-compat all"
  ${EndIf}

  ${If} $vim_keymap_stat == "default"
    StrCpy $1 "$1 -vimrc-remap no"
  ${Else}
    StrCpy $1 "$1 -vimrc-remap win"
  ${EndIf}

  ${If} $vim_mouse_stat == "default"
    StrCpy $1 "$1 -vimrc-behave default"
  ${ElseIf} $vim_mouse_stat == "windows"
    StrCpy $1 "$1 -vimrc-behave mswin"
  ${Else}
    StrCpy $1 "$1 -vimrc-behave unix"
  ${EndIf}
SectionEnd

##########################################################
SectionGroup $(str_group_plugin) id_group_plugin
  Section "$(str_section_plugin_home)" id_section_pluginhome
    SectionIn 1 3
    # use ShellExecAsUser below instead
    # StrCpy $1 "$1 -create-directories home"
  SectionEnd

  Section "$(str_section_plugin_vim)" id_section_pluginvim
    SectionIn 3
    StrCpy $1 "$1 -create-directories vim"
  SectionEnd
SectionGroupEnd

##########################################################
!if ${HAVE_NLS}
  Section "$(str_section_nls)" id_section_nls
    SectionIn 1 3

    SetOutPath $INSTDIR
    !if /FileExists "..\lang\README.*.txt"
      File ..\lang\README.*.txt
      CopyFiles /SILENT /FILESONLY $INSTDIR\README.$lng_usr.txt \
	  $INSTDIR\vim${VER_MAJOR}${VER_MINOR}\README.$lng_usr.txt
      Delete $INSTDIR\README.*.txt
    !endif
    StrCpy $R7 0
    !if /FileExists "..\lang\LICENSE.??.txt"
      File ..\lang\LICENSE.??.txt
      IntOp $R7 $R7 + 1
    !endif
    !if /FileExists "..\lang\LICENSE.??_??.txt"
      File ..\lang\LICENSE.??_??.txt
      IntOp $R7 $R7 + 1
    !endif
    IntCmp $R7 0 notcpy notcpy cpy
    cpy:
    CopyFiles /SILENT /FILESONLY $INSTDIR\LICENSE.$lng_usr.txt \
	$INSTDIR\vim${VER_MAJOR}${VER_MINOR}\LICENSE.$lng_usr.txt
    Delete $INSTDIR\LICENSE.*.txt
    notcpy:

    SetOutPath $0\lang
    File /r /x Makefile ${VIMRT}\lang\*.*
    SetOutPath $0
    !insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"${GETTEXT}\gettext${BIT}\libintl-8.dll" "$0\libintl-8.dll" "$0"
    !insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"${GETTEXT}\gettext${BIT}\libiconv-2.dll" "$0\libiconv-2.dll" "$0"
    # Install libgcc_s_sjlj-1.dll only if it is needed.
    !if ${INCLUDE_LIBGCC}
      !if /FileExists "${GETTEXT}\gettext${BIT}\libgcc_s_sjlj-1.dll"
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	    "${GETTEXT}\gettext${BIT}\libgcc_s_sjlj-1.dll" \
	    "$0\libgcc_s_sjlj-1.dll" "$0"
      !endif
    !endif

    ${If} ${SectionIsSelected} ${id_section_editwith}
      ${If} ${RunningX64}
	# Install DLLs for 64-bit gvimext.dll into the GvimExt64 directory.
	SetOutPath $0\GvimExt64
	ClearErrors
	!define LIBRARY_X64
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	    "${GETTEXT}\gettext64\libintl-8.dll" \
	    "$0\GvimExt64\libintl-8.dll" "$0\GvimExt64"
	!insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	    "${GETTEXT}\gettext64\libiconv-2.dll" \
	    "$0\GvimExt64\libiconv-2.dll" "$0\GvimExt64"
	!undef LIBRARY_X64
      ${EndIf}

      # Install DLLs for 32-bit gvimext.dll into the GvimExt32 directory.
      SetOutPath $0\GvimExt32
      ClearErrors
      !insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	  "${GETTEXT}\gettext32\libintl-8.dll" \
	  "$0\GvimExt32\libintl-8.dll" "$0\GvimExt32"
      !insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	  "${GETTEXT}\gettext32\libiconv-2.dll" \
	  "$0\GvimExt32\libiconv-2.dll" "$0\GvimExt32"
      # Install libgcc_s_sjlj-1.dll only if it is needed.
      !if ${INCLUDE_LIBGCC}
	!if /FileExists "${GETTEXT}\gettext32\libgcc_s_sjlj-1.dll"
	    !insertmacro InstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
		"${GETTEXT}\gettext32\libgcc_s_sjlj-1.dll" \
		"$0\GvimExt32\libgcc_s_sjlj-1.dll" "$0\GvimExt32"
	!endif
      !endif
    ${EndIf}
  SectionEnd
!endif

##########################################################
Section -call_install_exe
  SetOutPath $0
  DetailPrint "$(str_msg_registering)"
  nsExec::Exec "$0\install.exe $1"
  Pop $3

  ${If} ${SectionIsSelected} ${id_section_pluginhome}
    ReadEnvStr $3 "COMSPEC"
    Call GetHomeDir
    Pop $4
    ShellExecAsUser::ShellExecAsUser "" "$3" '/c "cd /d "$4" & mkdir vimfiles \
	& cd vimfiles & mkdir colors compiler doc ftdetect ftplugin indent \
	keymap plugin syntax"' SW_HIDE
  ${EndIf}
SectionEnd

##########################################################
Section -post
  # Get estimated install size
  SectionGetSize ${id_section_exe} $3
  ${If} ${SectionIsSelected} ${id_section_console}
    SectionGetSize ${id_section_console} $4
    IntOp $3 $3 + $4
  ${EndIf}
  ${If} ${SectionIsSelected} ${id_section_editwith}
    SectionGetSize ${id_section_editwith} $4
    IntOp $3 $3 + $4
  ${EndIf}
  !if ${HAVE_NLS}
    ${If} ${SectionIsSelected} ${id_section_nls}
      SectionGetSize ${id_section_nls} $4
      IntOp $3 $3 + $4
    ${EndIf}
  !endif

  # Register EstimatedSize and AllowSilent.
  # Other information will be set by the install.exe (dosinst.c).
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  WriteRegDWORD HKLM "${UNINST_REG_KEY_VIM}" "EstimatedSize" $3
  WriteRegDWORD HKLM "${UNINST_REG_KEY_VIM}" "AllowSilent" 1
  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}

  # Store the selections to the registry.
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  !insertmacro SaveSectionSelection ${id_section_console}    "select_console"
  !insertmacro SaveSectionSelection ${id_section_batch}      "select_batch"
  !insertmacro SaveSectionSelection ${id_section_desktop}    "select_desktop"
  !insertmacro SaveSectionSelection ${id_section_startmenu}  "select_startmenu"
  !insertmacro SaveSectionSelection ${id_section_editwith}   "select_editwith"
  !insertmacro SaveSectionSelection ${id_section_vimrc}      "select_vimrc"
  !insertmacro SaveSectionSelection ${id_section_pluginhome} \
      "select_pluginhome"
  !insertmacro SaveSectionSelection ${id_section_pluginvim}  "select_pluginvim"
  !if ${HAVE_NLS}
    !insertmacro SaveSectionSelection ${id_section_nls}      "select_nls"
  !endif
  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}

  BringToFront
SectionEnd

##########################################################
Function .onInit
  !if ${HAVE_MULTI_LANG}
    # Select a language (or read from the registry).
    !insertmacro MUI_LANGDLL_DISPLAY
  !endif

  !if ${HAVE_NLS}
    call GetUserLocale
  !endif

  ${If} $INSTDIR == ${DEFAULT_INSTDIR}
    # Check $VIM
    ReadEnvStr $3 "VIM"
    ${If} $3 != ""
      StrCpy $INSTDIR $3
    ${EndIf}
  ${EndIf}

  call CheckOldVim
  Pop $3
  ${If} $3 == ""
    # No old versions of Vim found. Unselect and hide the section.
    !insertmacro UnselectSection ${id_section_old_ver}
    SectionSetInstTypes ${id_section_old_ver} 0
    SectionSetText ${id_section_old_ver} ""
  ${Else}
    ${If} $INSTDIR == ${DEFAULT_INSTDIR}
      StrCpy $INSTDIR $3
    ${EndIf}
  ${EndIf}

  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  # Load the selections from the registry (if any).
  !insertmacro LoadSectionSelection ${id_section_console}    "select_console"
  !insertmacro LoadSectionSelection ${id_section_batch}      "select_batch"
  !insertmacro LoadSectionSelection ${id_section_desktop}    "select_desktop"
  !insertmacro LoadSectionSelection ${id_section_startmenu}  "select_startmenu"
  !insertmacro LoadSectionSelection ${id_section_editwith}   "select_editwith"
  !insertmacro LoadSectionSelection ${id_section_vimrc}      "select_vimrc"
  !insertmacro LoadSectionSelection ${id_section_pluginhome} \
      "select_pluginhome"
  !insertmacro LoadSectionSelection ${id_section_pluginvim}  "select_pluginvim"
  !if ${HAVE_NLS}
    !insertmacro LoadSectionSelection ${id_section_nls}      "select_nls"
  !endif
  # Load the default _vimrc settings from the registry (if any).
  !insertmacro LoadDefaultVimrc $vim_compat_stat "vim_compat"   "all"
  !insertmacro LoadDefaultVimrc $vim_keymap_stat "vim_keyremap" "default"
  !insertmacro LoadDefaultVimrc $vim_mouse_stat  "vim_mouse"    "default"
  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}

  # User variables:
  # $0 - holds the directory the executables are installed to
  # $1 - holds the parameters to be passed to install.exe.  Starts with OLE
  #      registration (since a non-OLE gvim will not complain, and we want to
  #      always register an OLE gvim).
  # $2 - holds the names to create batch files for
  StrCpy $0 "$INSTDIR\vim${VER_MAJOR}${VER_MINOR}"
  StrCpy $1 "-register-OLE"
  StrCpy $2 "gvim evim gview gvimdiff vimtutor"
FunctionEnd

Function .onInstSuccess
  WriteUninstaller vim${VER_MAJOR}${VER_MINOR}\uninstall-gui.exe
FunctionEnd

Function .onInstFailed
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(str_msg_install_fail)" /SD IDOK
FunctionEnd

##########################################################
Function SetCustom
  # Display the _vimrc setting dialog using nsDialogs.

  # Check if a _vimrc should be created
  ${IfNot} ${SectionIsSelected} ${id_section_vimrc}
    Abort
  ${EndIf}

  !insertmacro MUI_HEADER_TEXT \
      $(str_vimrc_page_title) $(str_vimrc_page_subtitle)

  nsDialogs::Create 1018
  Pop $vim_dialog

  ${If} $vim_dialog == error
    Abort
  ${EndIf}

  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}

  GetFunctionAddress $3 ValidateCustom
  nsDialogs::OnBack $3

  # 1st group - Compatibility
  ${NSD_CreateGroupBox} 0u 0u 296u 44u $(str_msg_compat_title)
  Pop $3

  ${NSD_CreateLabel} 16u 14u 269u 10u $(str_msg_compat_desc)
  Pop $3
  ${NSD_CreateDropList} 42u 26u 237u 13u ""
  Pop $vim_nsd_compat
  ${NSD_CB_AddString} $vim_nsd_compat $(str_msg_compat_vi)
  ${NSD_CB_AddString} $vim_nsd_compat $(str_msg_compat_vim)
  ${NSD_CB_AddString} $vim_nsd_compat $(str_msg_compat_defaults)
  ${NSD_CB_AddString} $vim_nsd_compat $(str_msg_compat_all)

  ${If} $vim_compat_stat == "defaults"
    StrCpy $4 2
  ${ElseIf} $vim_compat_stat == "vim"
    StrCpy $4 1
  ${ElseIf} $vim_compat_stat == "vi"
    StrCpy $4 0
  ${Else} ; default
    StrCpy $4 3
  ${EndIf}
  ${NSD_CB_SetSelectionIndex} $vim_nsd_compat $4

  # 2nd group - Key remapping
  ${NSD_CreateGroupBox} 0u 48u 296u 44u $(str_msg_keymap_title)
  Pop $3

  ${NSD_CreateLabel} 16u 62u 269u 10u $(str_msg_keymap_desc)
  Pop $3
  ${NSD_CreateDropList} 42u 74u 236u 13u ""
  Pop $vim_nsd_keymap
  ${NSD_CB_AddString} $vim_nsd_keymap $(str_msg_keymap_default)
  ${NSD_CB_AddString} $vim_nsd_keymap $(str_msg_keymap_windows)

  ${If} $vim_keymap_stat == "windows"
    StrCpy $4 1
  ${Else} ; default
    StrCpy $4 0
  ${EndIf}
  ${NSD_CB_SetSelectionIndex} $vim_nsd_keymap $4

  # 3rd group - Mouse behavior
  ${NSD_CreateGroupBox} 0u 95u 296u 44u $(str_msg_mouse_title)
  Pop $3

  ${NSD_CreateLabel} 16u 108u 269u 10u $(str_msg_mouse_desc)
  Pop $3
  ${NSD_CreateDropList} 42u 121u 237u 13u ""
  Pop $vim_nsd_mouse
  ${NSD_CB_AddString} $vim_nsd_mouse $(str_msg_mouse_default)
  ${NSD_CB_AddString} $vim_nsd_mouse $(str_msg_mouse_windows)
  ${NSD_CB_AddString} $vim_nsd_mouse $(str_msg_mouse_unix)

  ${If} $vim_mouse_stat == "xterm"
    StrCpy $4 2
  ${ElseIf} $vim_mouse_stat == "windows"
    StrCpy $4 1
  ${Else} ; default
    StrCpy $4 0
  ${EndIf}
  ${NSD_CB_SetSelectionIndex} $vim_nsd_mouse $4

  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}

  nsDialogs::Show
FunctionEnd

Function ValidateCustom
  ${NSD_CB_GetSelectionIndex} $vim_nsd_compat $3
  ${If} $3 = 0
    StrCpy $vim_compat_stat "vi"
  ${ElseIf} $3 = 1
    StrCpy $vim_compat_stat "vim"
  ${ElseIf} $3 = 2
    StrCpy $vim_compat_stat "defaults"
  ${Else}
    StrCpy $vim_compat_stat "all"
  ${EndIf}

  ${NSD_CB_GetSelectionIndex} $vim_nsd_keymap $3
  ${If} $3 = 0
    StrCpy $vim_keymap_stat "default"
  ${Else}
    StrCpy $vim_keymap_stat "windows"
  ${EndIf}

  ${NSD_CB_GetSelectionIndex} $vim_nsd_mouse $3
  ${If} $3 = 0
    StrCpy $vim_mouse_stat "default"
  ${ElseIf} $3 = 1
    StrCpy $vim_mouse_stat "windows"
  ${Else}
    StrCpy $vim_mouse_stat "xterm"
  ${EndIf}
FunctionEnd

##########################################################
# Description for Installer Sections

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_old_ver}   $(str_desc_old_ver)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_exe}	    $(str_desc_exe)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_console}   $(str_desc_console)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_batch}	    $(str_desc_batch)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_group_icons}	    $(str_desc_icons)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_desktop}   $(str_desc_desktop)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_startmenu} \
      $(str_desc_start_menu)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_editwith}  \
      $(str_desc_edit_with)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_vimrc}	    $(str_desc_vim_rc)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_group_plugin}	    $(str_desc_plugin)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_pluginhome} \
      $(str_desc_plugin_home)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_section_pluginvim} \
      $(str_desc_plugin_vim)
  !if ${HAVE_NLS}
    !insertmacro MUI_DESCRIPTION_TEXT ${id_section_nls}	    $(str_desc_nls)
  !endif
!insertmacro MUI_FUNCTION_DESCRIPTION_END


##########################################################
# Uninstaller Functions and Sections

Function un.onInit
  !if ${HAVE_MULTI_LANG}
    # Get the language from the registry.
    !insertmacro MUI_UNGETLANGUAGE
  !endif
FunctionEnd

Section "un.$(str_unsection_register)" id_unsection_register
  SectionIn RO

  # Apparently $INSTDIR is set to the directory where the uninstaller is
  # created.  Thus the "vim91" directory is included in it.
  StrCpy $0 "$INSTDIR"

  # delete the context menu entry and batch files
  DetailPrint "$(str_msg_unregistering)"
  nsExec::Exec "$0\uninstall.exe -nsis"
  Pop $3

  # We may have been put to the background when uninstall did something.
  BringToFront

  # Delete the installer language setting.
  DeleteRegKey ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY}
SectionEnd

Section "un.$(str_unsection_exe)" id_unsection_exe
  StrCpy $0 "$INSTDIR"

  # Delete gettext and iconv DLLs
  ${If} ${FileExists} "$0\libiconv-2.dll"
    !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"$0\libiconv-2.dll"
  ${EndIf}
  ${If} ${FileExists} "$0\libintl-8.dll"
    !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"$0\libintl-8.dll"
  ${EndIf}
  ${If} ${FileExists} "$0\libgcc_s_sjlj-1.dll"
    !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"$0\libgcc_s_sjlj-1.dll"
  ${EndIf}

  # Delete other DLLs
  Delete /REBOOTOK $0\*.dll

  # Delete 64-bit GvimExt
  ${If} ${RunningX64}
    !define LIBRARY_X64
    ${If} ${FileExists} "$0\GvimExt64\gvimext.dll"
      !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	  "$0\GvimExt64\gvimext.dll"
    ${EndIf}
    ${If} ${FileExists} "$0\GvimExt64\libiconv-2.dll"
      !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	  "$0\GvimExt64\libiconv-2.dll"
    ${EndIf}
    ${If} ${FileExists} "$0\GvimExt64\libintl-8.dll"
      !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	  "$0\GvimExt64\libintl-8.dll"
    ${EndIf}
    ${If} ${FileExists} "$0\GvimExt64\libwinpthread-1.dll"
      !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	  "$0\GvimExt64\libwinpthread-1.dll"
    ${EndIf}
    !undef LIBRARY_X64
    RMDir /r $0\GvimExt64
  ${EndIf}

  # Delete 32-bit GvimExt
  ${If} ${FileExists} "$0\GvimExt32\gvimext.dll"
    !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"$0\GvimExt32\gvimext.dll"
  ${EndIf}
  ${If} ${FileExists} "$0\GvimExt32\libiconv-2.dll"
    !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"$0\GvimExt32\libiconv-2.dll"
  ${EndIf}
  ${If} ${FileExists} "$0\GvimExt32\libintl-8.dll"
    !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"$0\GvimExt32\libintl-8.dll"
  ${EndIf}
  ${If} ${FileExists} "$0\GvimExt32\libgcc_s_sjlj-1.dll"
    !insertmacro UninstallLib DLL NOTSHARED REBOOT_NOTPROTECTED \
	"$0\GvimExt32\libgcc_s_sjlj-1.dll"
  ${EndIf}
  RMDir /r $0\GvimExt32

  ClearErrors
  # Remove everything but *.dll files.  Avoids that
  # a lot remains when gvimext.dll cannot be deleted.
  RMDir /r $0\autoload
  RMDir /r $0\colors
  RMDir /r $0\compiler
  RMDir /r $0\doc
  RMDir /r $0\ftplugin
  RMDir /r $0\import
  RMDir /r $0\indent
  RMDir /r $0\macros
  RMDir /r $0\pack
  RMDir /r $0\plugin
  RMDir /r $0\spell
  RMDir /r $0\syntax
  RMDir /r $0\tools
  RMDir /r $0\tutor
  RMDir /r $0\lang
  RMDir /r $0\keymap
  RMDir /r $0\bitmaps
  Delete $0\*.exe
  Delete $0\*.bat
  Delete $0\*.vim
  Delete $0\*.txt

  ${If} ${Errors}
    MessageBox MB_OK|MB_ICONEXCLAMATION $(str_msg_rm_exe_fail) /SD IDOK
  ${EndIf}

  # No error message if the "vim91" directory can't be removed, the
  # gvimext.dll may still be there.
  RMDir $0
SectionEnd

# Remove "vimfiles" directory under the specified directory.
!macro RemoveVimfiles dir
  ${If} ${FileExists} ${dir}\_viminfo
    Delete ${dir}\_viminfo
  ${EndIf}
  ${If} ${DirExists} ${dir}\vimfiles
    RMDir ${dir}\vimfiles\colors
    RMDir ${dir}\vimfiles\compiler
    RMDir ${dir}\vimfiles\doc
    RMDir ${dir}\vimfiles\ftdetect
    RMDir ${dir}\vimfiles\ftplugin
    RMDir ${dir}\vimfiles\indent
    RMDir ${dir}\vimfiles\keymap
    RMDir ${dir}\vimfiles\plugin
    RMDir ${dir}\vimfiles\syntax
    ${If} ${FileExists} ${dir}\vimfiles\.netrwhist*
      Delete ${dir}\vimfiles\.netrwhist*
    ${EndIf}
    RMDir ${dir}\vimfiles
  ${EndIf}
!macroend

SectionGroup "un.$(str_ungroup_plugin)" id_ungroup_plugin
  Section /o "un.$(str_unsection_plugin_home)" id_unsection_plugin_home
  # get the home dir
    Call un.GetHomeDir
    Pop $0

    ${If} $0 != ""
      !insertmacro RemoveVimfiles $0
    ${EndIf}
  SectionEnd

  Section "un.$(str_unsection_plugin_vim)" id_unsection_plugin_vim
    # get the parent dir of the installation
    Push $INSTDIR
    Call un.GetParent
    Pop $0

    # if a plugin dir was created at installation remove it
    !insertmacro RemoveVimfiles $0
  SectionEnd
SectionGroupEnd

Section "un.$(str_unsection_rootdir)" id_unsection_rootdir
# get the parent dir of the installation
  Push $INSTDIR
  Call un.GetParent
  Pop $0

  ${IfNot} ${Silent}
    Delete $0\_vimrc
  ${Endif}
  RMDir $0
SectionEnd

##########################################################
# Description for Uninstaller Sections

!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${id_unsection_register}	\
      $(str_desc_unregister)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_unsection_exe}  $(str_desc_rm_exe)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_ungroup_plugin} $(str_desc_rm_plugin)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_unsection_plugin_home} \
      $(str_desc_rm_plugin_home)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_unsection_plugin_vim}	\
      $(str_desc_rm_plugin_vim)
  !insertmacro MUI_DESCRIPTION_TEXT ${id_unsection_rootdir}	\
      $(str_desc_rm_rootdir)
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END

# vi:set ts=8 sw=2 sts=2 tw=79 wm=0 ft=nsis:
