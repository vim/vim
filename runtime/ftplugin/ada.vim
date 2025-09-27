"------------------------------------------------------------------------------
"  Description: Perform Ada specific completion & tagging.
"     Language: Ada (2022)
"    Copyright: Copyright (C) 2006 … 2022 Martin Krischik
"   Maintainer:	Doug Kearns <dougkearns@gmail.com> (Vim)
"		Martin Krischik <krischik@users.sourceforge.net> (Upstream)
"		Taylor Venable <taylor@metasyntax.net>
"		Neil Bird <neil@fnxweb.com>
"               Bartek Jasicki <thindil@laeran.pl>
" Contributors: Doug Kearns <dougkearns@gmail.com>
"	   URL: https://github.com/krischik/vim-ada
"      Version: 5.5.0
"      History: 24.05.2006 MK Unified Headers
"		26.05.2006 MK ' should not be in iskeyword.
"		16.07.2006 MK Ada-Mode as vim-ball
"		02.10.2006 MK Better folding.
"		15.10.2006 MK Bram's suggestion for runtime integration
"               05.11.2006 MK Bram suggested not to use include protection for
"                             autoload
"		05.11.2006 MK Bram suggested to save on spaces
"		08.07.2007 TV fix default compiler problems.
"		28.08.2022 MK Merge Ada 2012 changes from thindil
"		01.09.2022 MK Use GitHub and dein to publish new versions
"		12.09.2022 MK Rainbow Parenthesis have been updated and
"			      modernised so they are a viable light weight
"			      alternative to rainbow-improved.
"		25.10.2022 MK Add Alire compiler support
"		25.10.2022 MK Toggle Rainbow Colour was missing parameters.
"		28.10.2022 MK Issue #13 Fix key and menu mappings.
"		04.11.2022 DK Improve matchit config
"		04.11.2022 DK Define iabbrevs as buffer-local
"		19.11.2022 MK Hotfix for comment setting. Messed up the ':'
"               21.08.2023 MK Release 5.5.0
"	 Usage: Use dein to install
"    Help Page: ft-ada-plugin
"------------------------------------------------------------------------------
" Provides mapping overrides for tag jumping that figure out the current
" Ada object and tag jump to that, not the 'simple' vim word.
" Similarly allows <Ctrl-N> matching of full-length ada entities from tags.
"------------------------------------------------------------------------------

" Only do this when not done yet for this buffer
if exists ("b:did_ftplugin") || version < 700
   finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 45

"
" Temporarily set cpoptions to ensure the script loads OK
"
let s:cpoptions = &cpoptions
set cpoptions-=C

" Section: Keyword characters {{{1
"
" Valid character for keywords and identifiers. '_' is not a keyword character
" but is included otherwise the syntax highlighter will detect keywords inside
" identifier.
"
setlocal iskeyword=@,48-57,_
setlocal isident=@,48-57,_

" Section: Comments  {{{1
"
" GNAT prefers comments with two spaces after the double dash. First space is
" defined with `\ ` the second with the b: option.
"
setlocal comments=b:--\ ,O:--
setlocal commentstring=--\ \ %s
setlocal complete=.,w,b,u,t,i

let b:undo_ftplugin = "setlocal comments< commentstring< complete<"

" Section: case	     {{{1
"
" Disabled global options (for Vim redistribution)
" setlocal nosmartcase
" setlocal ignorecase

" let b:undo_ftplugin .= " | setlocal smartcase< ignorecase<"

" Section: formatoptions {{{1
"
setlocal formatoptions+=ron

let b:undo_ftplugin .= " | setlocal formatoptions<"

" Section: Completion {{{1
"
setlocal completefunc=ada#User_Complete
setlocal omnifunc=adacomplete#Complete

let b:undo_ftplugin .= " | setlocal completefunc< omnifunc<"

if exists ("g:ada_extended_completion")
   if mapcheck ('<C-N>','i') == ''
      inoremap <unique> <buffer> <C-N> <C-R>=ada#Completion("\<lt>C-N>")<cr>
   endif
   if mapcheck ('<C-P>','i') == ''
      inoremap <unique> <buffer> <C-P> <C-R>=ada#Completion("\<lt>C-P>")<cr>
   endif
   if mapcheck ('<C-X><C-]>','i') == ''
      inoremap <unique> <buffer> <C-X><C-]> <C-R>=<SID>ada#Completion("\<lt>C-X>\<lt>C-]>")<cr>
   endif
   if mapcheck ('<bs>','i') == ''
      inoremap <silent> <unique> <buffer> <bs> <C-R>=ada#Insert_Backspace ()<cr>
   endif
   let b:undo_ftplugin .= " | silent! execute 'iunmap <buffer> <C-N>'" .
	    \             " | silent! execute 'iunmap <buffer> <C-P>'" .
	    \             " | silent! execute 'iunmap <buffer> <C-X><C-]>'" .
	    \             " | silent! execute 'iunmap <buffer> <bs>'"
endif

" Section: Matchit {{{1
"
" Only do this when not done yet for this buffer & matchit is used
"
if !exists ("b:match_words")  &&
  \ exists ("loaded_matchit")
   "
   " The following lines enable the macros/matchit.vim plugin for
   " Ada-specific extended matching with the % key.
   "
   let s:notend      = '\%(\<end\s\+\)\@<!'
   let b:match_words =
      \ s:notend . '\<if\>:\<elsif\>:\<\%(or\s\)\@3<!else\>:\<end\s\+if\>,' .
      \ s:notend . '\<case\>:\<when\>:\<end\s\+case\>,' .
      \ '\%(\<while\>.*\|\<for\>.*\|'.s:notend.'\)\<loop\>:\<end\s\+loop\>,' .
      \ '\%(\<do\>\|\<begin\>\):\<exception\>:\<end\%(\s*\%($\|;\)\|\s\+\%(\%(if\|case\|loop\|record\)\>\)\@!\a\)\@=,' .
      \ s:notend . '\<record\>:\<end\s\+record\>'
   let b:undo_ftplugin .= " | unlet! b:match_skip b:match_words"
   let b:match_skip = 's:Comment\|String\|Operator'
endif


" Section: Compiler {{{1
"
if ! exists("g:ada_default_compiler")
   let g:ada_default_compiler = 'alire'
endif

if ! exists("current_compiler")			||
   \ current_compiler != g:ada_default_compiler
   execute "compiler " . g:ada_default_compiler
endif

" Section: Folding {{{1
"
if exists("g:ada_folding")
   if g:ada_folding[0] == 'i'
      setlocal foldmethod=indent
      setlocal foldignore=--
      setlocal foldnestmax=5
      let b:undo_ftplugin .= " | setlocal foldmethod< foldignore< foldnestmax<"
   elseif g:ada_folding[0] == 'g'
      setlocal foldmethod=expr
      setlocal foldexpr=ada#Pretty_Print_Folding(v:lnum)
      let b:undo_ftplugin .= " | setlocal foldmethod< foldexpr<"
   elseif g:ada_folding[0] == 's'
      setlocal foldmethod=syntax
      let b:undo_ftplugin .= " | setlocal foldmethod<"
   endif
   setlocal tabstop=8
   setlocal softtabstop=3
   setlocal shiftwidth=3
   let b:undo_ftplugin .= " | setlocal tabstop< softtabstop< shiftwidth<"
endif

" Section: Abbrev {{{1
"
if exists("g:ada_abbrev")
   iabbrev <buffer> ret  return
   iabbrev <buffer> proc procedure
   iabbrev <buffer> pack package
   iabbrev <buffer> func function
   let b:undo_ftplugin .= " | iunabbrev <buffer> ret" .
	    \		  " | iunabbrev <buffer> proc" .
	    \		  " | iunabbrev <buffer> pack" .
	    \		  " | iunabbrev <buffer> func"
endif

" Section: Commands, Mapping, Menus {{{1
"
execute "50amenu &Ada.-sep- :"

" Map_Menu parameter:
"  Text:       Menu text to display
"  Keys:       Key short cut to define (used only when g:mapleader is used)
"  Command:    Command short cut to define
"  Function:   Function to call
"  Args:       Additional parameter.

if !exists ("g:did_adamapping")

let g:did_adamapping = 521

call ada#Map_Menu (
   \ 'Toggle Space Errors',
   \ 'as',
   \ 'AdaSpaces',
   \ 'call ada#Switch_Syntax_Option',
   \ '''space_errors''')
call ada#Map_Menu (
   \ 'Toggle Lines Errors',
   \ 'al',
   \ 'AdaLines',
   \ 'call ada#Switch_Syntax_Option',
   \ '''line_errors''')
call ada#Map_Menu (
   \ 'Toggle Rainbow Colour',
   \ 'rp',
   \ 'AdaRainbow',
   \ 'call ada#Switch_Syntax_Option',
   \ '''rainbow_color''')
call ada#Map_Menu (
   \'Toggle Standard Types',
   \ 'at',
   \ 'AdaTypes',
   \ 'call ada#Switch_Syntax_Option',
   \ '''standard_types''')

endif

" }}}1

" Reset cpoptions
let &cpoptions = s:cpoptions
unlet s:cpoptions

finish " }}}1

"------------------------------------------------------------------------------
"   Vim is Charityware - see ":help license" or uganda.txt for licence details.
"------------------------------------------------------------------------------
" vim: set textwidth=78 nowrap tabstop=8 shiftwidth=3 softtabstop=3 noexpandtab :
" vim: set filetype=vim fileencoding=utf-8 fileformat=unix foldmethod=marker :
" vim: set spell spelllang=en_gb :
