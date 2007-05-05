"------------------------------------------------------------------------------
"  Description: Vim Ada/Dec Ada compiler file
"     Language: Ada (Dec Ada)
"          $Id$
"    Copyright: Copyright (C) 2006 Martin Krischik
"   Maintainer:	Martin Krischik
"      $Author$
"        $Date$
"      Version: 4.2
"    $Revision$
"     $HeadURL: https://svn.sourceforge.net/svnroot/gnuada/trunk/tools/vim/compiler/decada.vim $
"      History: 21.07.2006 MK New Dec Ada
"               15.10.2006 MK Bram's suggestion for runtime integration
"    Help Page: compiler-decada
"------------------------------------------------------------------------------

if (exists("current_compiler")	    &&
   \ current_compiler == "decada")  ||
   \ version < 700
   finish
endif

let current_compiler = "decada"

if !exists("g:decada")
   let g:decada = decada#New ()
endif

if exists(":CompilerSet") != 2
   "
   " plugin loaded by other means then the "compiler" command
   "
   command -nargs=* CompilerSet setlocal <args>
endif

call g:decada.Set_Session ()

execute "CompilerSet makeprg="     . escape (g:decada.Make_Command, ' ')
execute "CompilerSet errorformat=" . escape (g:decada.Error_Format, ' ')

call ada#Map_Menu (
  \'Dec Ada.Build',
  \'<F7>',
  \'call decada.Make ()')

finish " 1}}}

"------------------------------------------------------------------------------
"   Copyright (C) 2006  Martin Krischik
"
"   Vim is Charityware - see ":help license" or uganda.txt for licence details.
"------------------------------------------------------------------------------
" vim: textwidth=78 wrap tabstop=8 shiftwidth=3 softtabstop=3 noexpandtab
" vim: foldmethod=marker
