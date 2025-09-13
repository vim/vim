"------------------------------------------------------------------------------
"  Description: Vim Ada/GNAT compiler file
"     Language: Ada (GNAT)
"    Copyright: Copyright (C) 2006 … 2022 Martin Krischik
"   Maintainer:	Doug Kearns <dougkearns@gmail.com> (Vim)
"		Martin Krischik <krischik@users.sourceforge.net> (Upstream)
"		Ned Okie <nokie@radford.edu>
"               Bartek Jasicki <thindil@laeran.pl>
"	   URL: https://github.com/krischik/vim-ada
"      Version: 5.5.0
"      History: 24.05.2006 MK Unified Headers
"		16.07.2006 MK Ada-Mode as vim-ball
"		05.08.2006 MK Add session support
"               15.10.2006 MK Bram's suggestion for runtime integration
"               05.11.2006 MK Bram suggested not to use include protection for
"                             autoload
"		05.11.2006 MK Bram suggested to save on spaces
"		19.09.2007 NO use project file only when there is a project
"		28.08.2022 MK Merge Ada 2012 changes from thindil
"		01.09.2022 MK Use GitHub und dein to publish new versions
"		25.10.2022 MK Add Alire compiler support
"               21.08.2023 MK Release 5.5.0
"	 Usage: Use dein to install
"    Help Page: compiler-gnat
"------------------------------------------------------------------------------

if version < 700
    finish
endif

function gnat#Make () dict					     " {{{1
   let &l:makeprg     = self.Get_Command('Make') . ' ' . self.Make_Options
   let &l:errorformat = self.Error_Format
   wall
   make
endfunction gnat#Make						     " }}}1

function gnat#Pretty () dict					     " {{{1
   execute "!" . self.Get_Command('Pretty')
endfunction gnat#Make						     " }}}1

function gnat#Set_Project_File (...) dict			     " {{{1
   if a:0 > 0
      let self.Project_File = a:1

      if ! filereadable (self.Project_File)
	 let self.Project_File = findfile (
	    \ fnamemodify (self.Project_File, ':r'),
	    \ $ADA_PROJECT_PATH,
	    \ 1)
      endif
   elseif strlen (self.Project_File) > 0
      let self.Project_File = browse (0, 'GNAT Project File?', '', self.Project_File)
   elseif expand ("%:e") == 'gpr'
      let self.Project_File = browse (0, 'GNAT Project File?', '', expand ("%:e"))
   else
      let self.Project_File = browse (0, 'GNAT Project File?', '', 'default.gpr')
   endif

   if self.Project_File[strlen(self.Project_File) - 4:] == ".gpr"
      if exists('g:ale_enabled')
	 let g:ale_ada_gnatmake_options = "-P " . self.Project_File . " -gnatwa -gnatq"
	 let g:ale_ada_adals_project = self.Project_File
	 let g:ale_lsp_root = {'adals': fnamemodify(self.Project_File, ':p:h') }
	 let g:ale_ada_gnatpp_options = "-P " . self.Project_File
	 call ale#lsp_linter#SendRequest('%',
		  \ 'adals',
		  \ ale#lsp#message#DidChangeConfiguration('%',
			\ {'ada' : {"projectFile" : self.Project_File}}))
      endif
      let self.Make_Command = '"gnatmake -P " . self.Project_File . "  -F -gnatef"'
      let self.Pretty_Command = '"gnatpp -P " . self.Project_File'
      let &l:makeprg  = "gnatmake -P " . self.Project_File . "  -F -gnatef"
      if exists("g:ada_create_session")
	 call ada#Switch_Session(self.Project_File . '.vim')
      endif
   endif

endfunction gnat#Set_Project_File				     " }}}1

function gnat#Get_Command (Command) dict			     " {{{1
   let l:Command = eval ('self.' . a:Command . '_Command')
   return eval (l:Command)
endfunction gnat#Get_Command					     " }}}1

function gnat#Set_Session (...) dict				     " {{{1
   if argc() == 1 && fnamemodify (argv(0), ':e') == 'gpr'
      call self.Set_Project_File (argv(0))
   elseif  strlen (v:servername) > 0
      call self.Set_Project_File (v:servername . '.gpr')
   endif
endfunction gnat#Set_Session					     " }}}1

function gnat#Set_Options (Options) dict			     " {{{1
   let self.Make_Options = a:Options
   let &l:makeprg = self.Get_Command('Make') . ' ' . self.Make_Options
endfunction gnat#Set_Options					     " }}}1

function gnat#New ()						     " {{{1
   let l:Retval = {
      \ 'Make'		   : function ('gnat#Make'),
      \ 'Pretty'	   : function ('gnat#Pretty'),
      \ 'Set_Project_File' : function ('gnat#Set_Project_File'),
      \ 'Set_Session'      : function ('gnat#Set_Session'),
      \ 'Get_Command'      : function ('gnat#Get_Command'),
      \ 'Set_Options'	   : function ('gnat#Set_Options'),
      \ 'Project_File'     : '',
      \ 'Make_Command'     : '"gnatmake -F -gnatef " . expand("%:p")',
      \ 'Make_Options'	   : '',
      \ 'Pretty_Command'   : '"gnatpp " . expand("%:p")' ,
      \ 'Error_Format'     : '%f:%l:%c: %trror: %m,'   .
			   \ '%f:%l:%c: %tarning: %m,' .
			   \ '%f:%l:%c: %tnfo: %m,'    .
			   \ '%f:%l:%c: %tow: %m,'     .
			   \ '%f:%l:%c: %tedium: %m,'  .
			   \ '%f:%l:%c: %tigh: %m,'    .
			   \ '%f:%l:%c: %theck: %m,'   .
			   \ '%f:%l:%c: (%ttyle) %m,'  .
			   \ '%f:%l:%c: %m'}

   return l:Retval
endfunction gnat#New						  " }}}1

finish " }}}1

"------------------------------------------------------------------------------
"   Vim is Charityware - see ":help license" or uganda.txt for licence details.
"------------------------------------------------------------------------------
" vim: set textwidth=0 wrap tabstop=8 shiftwidth=3 softtabstop=3 noexpandtab :
" vim: set filetype=vim fileencoding=utf-8 fileformat=unix foldmethod=marker :
" vim: set spell spelllang=en_gb :
