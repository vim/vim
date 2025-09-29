# -*- coding: utf-8 -*-
# NSIS helper file for creating a self-installing exe for Vim.
# Contains service macros and functions. 
# Last Change:	2025-09-03
#

!ifndef __AUXILIARY__NSH__
  !define __AUXILIARY__NSH__

# See https://nsis.sourceforge.io/LogicLib
;FileExists is already part of LogicLib, but returns true for directories
;as well as files
  !macro _FileExists2 _a _b _t _f
    !insertmacro _LOGICLIB_TEMP
    StrCpy $_LOGICLIB_TEMP "0"
;if path is not blank, continue to next check
    StrCmp `${_b}` `` +4 0
;if path exists, continue to next check (IfFileExists returns true if this
;is a directory)
    IfFileExists `${_b}` `0` +3
;if path is not a directory, continue to confirm exists
    IfFileExists `${_b}\*.*` +2 0
    StrCpy $_LOGICLIB_TEMP "1" ;file exists
;now we have a definitive value - the file exists or it does not
    StrCmp $_LOGICLIB_TEMP "1" `${_t}` `${_f}`
  !macroend
  !undef FileExists
  !define FileExists `"" FileExists2`
  !macro _DirExists _a _b _t _f
    !insertmacro _LOGICLIB_TEMP
    StrCpy $_LOGICLIB_TEMP "0"
;if path is not blank, continue to next check
    StrCmp `${_b}` `` +3 0
;if directory exists, continue to confirm exists
    IfFileExists `${_b}\*.*` 0 +2
    StrCpy $_LOGICLIB_TEMP "1"
    StrCmp $_LOGICLIB_TEMP "1" `${_t}` `${_f}`
  !macroend
  !define DirExists `"" DirExists`

# Get parent directory
# Share this function both on installer and uninstaller
  !macro GetParent un
    Function ${un}GetParent
      Exch $0  ; old $0 is on top of stack
      Push $1
      Push $2
      StrCpy $1 -1
      ${Do}
	StrCpy $2 $0 1 $1
	${If} $2 == ""
	${OrIf} $2 == "\"
	  ${ExitDo}
	${EndIf}
	IntOp $1 $1 - 1
      ${Loop}
      StrCpy $0 $0 $1
      Pop $2
      Pop $1
      Exch $0  ; put $0 on top of stack, restore $0 to original value
    FunctionEnd
  !macroend

  !insertmacro GetParent ""
  !insertmacro GetParent "un."

# Get home directory
  !macro GetHomeDir un
    Function ${un}GetHomeDir
      Push $0
      Push $1
      ReadEnvStr $0 "HOME"
      ${If} $0 == ""
	ReadEnvStr $0 "HOMEDRIVE"
	ReadEnvStr $1 "HOMEPATH"
	StrCpy $0 "$0$1"
	${If} $0 == ""
	  ReadEnvStr $0 "USERPROFILE"
	${EndIf}
      ${EndIf}
      Pop $1
      Exch $0  ; put $0 on top of stack, restore $0 to original value
    FunctionEnd
  !macroend

  !insertmacro GetHomeDir ""
  !insertmacro GetHomeDir "un."

# Saving the status of sections of the current installation in the registry
  !macro SaveSectionSelection section_id reg_value
    ${If} ${SectionIsSelected} ${section_id}
      WriteRegDWORD HKLM "${UNINST_REG_KEY_VIM}" ${reg_value} 1
    ${Else}
      WriteRegDWORD HKLM "${UNINST_REG_KEY_VIM}" ${reg_value} 0
    ${EndIf}
  !macroend

# Reading the status of sections from the registry of the previous installation 
  !macro LoadSectionSelection section_id reg_value
    ClearErrors
    ReadRegDWORD $3 HKLM "${UNINST_REG_KEY_VIM}" ${reg_value}
    ${IfNot} ${Errors}
      ${If} $3 = 1
	!insertmacro SelectSection ${section_id}
      ${Else}
	!insertmacro UnselectSection ${section_id}
      ${EndIf}
    ${EndIf}
  !macroend

# Reading the settings for _vimrc from the registry of a previous installation
  !macro LoadDefaultVimrc out_var reg_value default_value
    ClearErrors
    ReadRegStr ${out_var} HKLM "${UNINST_REG_KEY_VIM}" ${reg_value}
    ${If} ${Errors}
    ${OrIf} ${out_var} == ""
      StrCpy ${out_var} ${default_value}
    ${EndIf}
  !macroend

# Get user locale
  !if ${HAVE_NLS}
    Var lng_usr  ; variable containing the locale of the current user

    !include "StrFunc.nsh"
    ${StrRep}

    Function GetUserLocale
      ClearErrors
      System::Call \
	  'kernel32::GetUserDefaultLocaleName(t.r19, *i${NSIS_MAX_STRLEN})'
      StrCmp $R9 "zh-cn" coincide 0
      StrCmp $R9 "zh-tw" coincide 0
      StrCmp $R9 "pt-br" 0 part
      coincide:
      System::Call 'User32::CharLower(t r19 r19)*i${NSIS_MAX_STRLEN}'
      ${StrRep} $lng_usr "$R9" "-" "_"
      Goto done
      part:
      StrCpy $lng_usr $R9 2
      done:
    FunctionEnd
  !endif



!endif # __AUXILIARY__NSH__
# vi:set ts=8 sw=2 sts=2 tw=79 wm=0 ft=nsis:
