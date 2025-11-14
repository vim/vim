"------------------------------------------------------------------------------
"  Description: Vim Ada/GNAT compiler file
"     Language: Ada (GNAT)
"    Copyright: Copyright (C) 2006 … 2022 Martin Krischik
"   Maintainer:	Martin Krischi <krischik@users.sourceforge.net>k
"               Bartek Jasicki <thindil@laeran.pl>
"		Ned Okie <nokie@radford.edu>
"      Version: 5.5.0
"      History: 24.05.2006 MK Unified Headers
"		16.07.2006 MK Ada-Mode as vim-ball
"               15.10.2006 MK Bram's suggestion for runtime integration
"		19.09.2007 NO use project file only when there is a project
"		28.08.2022 MK Merge Ada 2012 changes from thindil
"		01.09.2022 MK Use GitHub and dein to publish new versions
"		25.10.2022 MK Add Alire compiler support
"		26.10.2022 MK Fix mapping conflict
"		28.10.2022 MK Bug #43 Duplicated mappings in Gnat compiler
"			      plug in
"               21.08.2023 MK Release 5.5.0
"	 Usage: Use dein to install
"    Help Page: compiler-gnat
"------------------------------------------------------------------------------

if (exists("current_compiler")	    &&
   \ current_compiler == "gnat")    ||
   \ version < 700
   finish
endif
let s:keepcpo= &cpo
set cpo&vim

let current_compiler = "gnat"

if !exists("g:gnat")
   let g:gnat = gnat#New ()

   " Map_Menu parameter:
   "  Text:	Menu text to display
   "  Keys:	Key shortcut to define (used only when g:mapleader is used)
   "  Command:  Command shortcut to define
   "  Function: Function to call
   "  Args:	Additional parameter.

   call ada#Map_Menu (
      \ 'Pretty Print',
      \ 'gp',
      \ 'GnatPretty',
      \ 'call gnat.Pretty',
      \ '')
   call ada#Map_Menu (
      \ 'Set Project file\.\.\.',
      \ 'gP',
      \ 'SetProject',
      \ 'call gnat.Set_Project_File',
      \ '')
   call ada#Map_Menu (
      \ 'Set Project options\.\.\.',
      \ 'go',
      \ 'SetOptions',
      \ 'call gnat.Set_Options',
      \ '')

   call g:gnat.Set_Session ()
endif

if exists(":CompilerSet") != 2
   "
   " plugin loaded by other means then the "compiler" command
   "
   command -nargs=* CompilerSet setlocal <args>
endif

execute "CompilerSet makeprg="     . escape (g:gnat.Get_Command('Make'), ' ')
execute "CompilerSet errorformat=" . escape (g:gnat.Error_Format, ' ')

let &cpo = s:keepcpo
unlet s:keepcpo

finish " }}}1

"------------------------------------------------------------------------------
"   Vim is Charityware - see ":help license" or uganda.txt for licence details.
"------------------------------------------------------------------------------
" vim: set textwidth=0 wrap tabstop=8 shiftwidth=3 softtabstop=3 noexpandtab :
" vim: set filetype=vim fileencoding=utf-8 fileformat=unix foldmethod=marker :
" vim: set spell spelllang=en_gb :
