" =============================================================================
"
"   Program:   CMake - Cross-Platform Makefile Generator
"   Module:    $RCSfile: cmake-syntax.vim,v $
"   Language:  VIM
"   Date:      $Date: 2006/09/23 21:09:08 $
"   Version:   $Revision: 1.6 $
"
" =============================================================================

" Vim syntax file
" Language:     CMake
" Author:       Andy Cedilnik <andy.cedilnik@kitware.com>
" Maintainer:   Andy Cedilnik <andy.cedilnik@kitware.com>
" Last Change:  $Date: 2006/09/23 21:09:08 $
" Version:      $Revision: 1.6 $
"
" Licence:      The CMake license applies to this file. See
"               http://www.cmake.org/HTML/Copyright.html
"               This implies that distribution with Vim is allowed

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case ignore
syn match cmakeComment /#.*$/
syn region cmakeRegistry start=/\[/ end=/\]/ skip=/\\[\[\]]/
            \ contained
syn match cmakeArgument /[^()"]+/
            \ contained
syn match cmakeVariableValue /\${[^}]*}/
            \ contained oneline
syn match cmakeEnvironment /\$ENV{.*}/
            \ contained
syn keyword cmakeSystemVariables
            \ WIN32 UNIX APPLE CYGWIN BORLAND MINGW MSVC MSVC_IDE MSVC60 MSVC70 MSVC71 MSVC80
syn keyword cmakeOperators
            \ AND BOOL CACHE COMMAND DEFINED DOC EQUAL EXISTS FALSE GREATER INTERNAL LESS MATCHES NAME NAMES NAME_WE NOT OFF ON OR PATH PATHS PROGRAM STREQUAL STRGREATER STRING STRLESS TRUE
"            \ contained
syn region cmakeString start=/"/ end=/"/ skip=/\\"/
            \ contains=ALLBUT,cmakeString
syn region cmakeArguments start=/\s*(/ end=/)/
           \ contains=ALLBUT,cmakeArguments
syn keyword cmakeDeprecated ABSTRACT_FILES BUILD_NAME SOURCE_FILES SOURCE_FILES_REMOVE VTK_MAKE_INSTANTIATOR VTK_WRAP_JAVA VTK_WRAP_PYTHON VTK_WRAP_TCL WRAP_EXCLUDE_FILES
           \ nextgroup=cmakeArgument
syn keyword cmakeStatement
           \ ADD_CUSTOM_COMMAND ADD_CUSTOM_TARGET ADD_DEFINITIONS ADD_DEPENDENCIES ADD_EXECUTABLE ADD_LIBRARY ADD_SUBDIRECTORY ADD_TEST AUX_SOURCE_DIRECTORY BUILD_COMMAND BUILD_NAME CMAKE_MINIMUM_REQUIRED CONFIGURE_FILE CREATE_TEST_SOURCELIST ELSE ELSEIF ENABLE_LANGUAGE ENABLE_TESTING ENDFOREACH ENDIF ENDWHILE EXEC_PROGRAM EXECUTE_PROCESS EXPORT_LIBRARY_DEPENDENCIES FILE FIND_FILE FIND_LIBRARY FIND_PACKAGE FIND_PATH FIND_PROGRAM FLTK_WRAP_UI FOREACH GET_CMAKE_PROPERTY GET_DIRECTORY_PROPERTY GET_FILENAME_COMPONENT GET_SOURCE_FILE_PROPERTY GET_TARGET_PROPERTY GET_TEST_PROPERTY IF INCLUDE INCLUDE_DIRECTORIES INCLUDE_EXTERNAL_MSPROJECT INCLUDE_REGULAR_EXPRESSION INSTALL INSTALL_FILES INSTALL_PROGRAMS INSTALL_TARGETS LINK_DIRECTORIES LINK_LIBRARIES LIST LOAD_CACHE LOAD_COMMAND MACRO MAKE_DIRECTORY MARK_AS_ADVANCED MATH MESSAGE OPTION OUTPUT_REQUIRED_FILES PROJECT QT_WRAP_CPP QT_WRAP_UI REMOVE REMOVE_DEFINITIONS SEPARATE_ARGUMENTS SET SET_DIRECTORY_PROPERTIES SET_SOURCE_FILES_PROPERTIES SET_TARGET_PROPERTIES SET_TESTS_PROPERTIES SITE_NAME SOURCE_GROUP STRING SUBDIR_DEPENDS SUBDIRS TARGET_LINK_LIBRARIES TRY_COMPILE TRY_RUN USE_MANGLED_MESA UTILITY_SOURCE VARIABLE_REQUIRES VTK_MAKE_INSTANTIATOR VTK_WRAP_JAVA VTK_WRAP_PYTHON VTK_WRAP_TCL WHILE WRITE_FILE ENDMACRO
           \ nextgroup=cmakeArgumnts

"syn match cmakeMacro /^\s*[A-Z_]\+/ nextgroup=cmakeArgumnts

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_cmake_syntax_inits")
  if version < 508
    let did_cmake_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink cmakeStatement Statement
  HiLink cmakeComment Comment
  HiLink cmakeString String
  HiLink cmakeVariableValue Type
  HiLink cmakeRegistry Underlined
  HiLink cmakeArguments Identifier
  HiLink cmakeArgument Constant
  HiLink cmakeEnvironment Special
  HiLink cmakeOperators Operator
  HiLink cmakeMacro PreProc
  HiLink cmakeError	Error

  delcommand HiLink
endif

let b:current_syntax = "cmake"

"EOF"
