" Vim filetype plugin file
" Language:     Perl
" Maintainer:   Andy Lester <andy@petdance.com>
" URL:          http://github.com/petdance/vim-perl
" Last Change:  2012 Mar 11

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Make sure the continuation lines below do not cause problems in
" compatibility mode.
let s:save_cpo = &cpo
set cpo-=C

setlocal formatoptions+=crq
setlocal keywordprg=perldoc\ -f

setlocal comments=:#
setlocal commentstring=#%s

" Change the browse dialog on Win32 to show mainly Perl-related files
if has("gui_win32")
    let b:browsefilter = "Perl Source Files (*.pl)\t*.pl\n" .
		       \ "Perl Modules (*.pm)\t*.pm\n" .
		       \ "Perl Documentation Files (*.pod)\t*.pod\n" .
		       \ "All Files (*.*)\t*.*\n"
endif

" Provided by Ned Konz <ned at bike-nomad dot com>
"---------------------------------------------
setlocal include=\\<\\(use\\\|require\\)\\>
setlocal includeexpr=substitute(substitute(v:fname,'::','/','g'),'$','.pm','')
setlocal define=[^A-Za-z_]

" The following line changes a global variable but is necessary to make
" gf and similar commands work.  The change to iskeyword was incorrect.
" Thanks to Andrew Pimlott for pointing out the problem. If this causes a
" problem for you, add an after/ftplugin/perl.vim file that contains
"       set isfname-=:
set isfname+=:
"setlocal iskeyword=48-57,_,A-Z,a-z,:

" Set this once, globally.
if !exists("perlpath")
    if executable("perl")
      try
	if &shellxquote != '"'
	    let perlpath = system('perl -e "print join(q/,/,@INC)"')
	else
	    let perlpath = system("perl -e 'print join(q/,/,@INC)'")
	endif
	let perlpath = substitute(perlpath,',.$',',,','')
      catch /E145:/
	let perlpath = ".,,"
      endtry
    else
	" If we can't call perl to get its path, just default to using the
	" current directory and the directory of the current file.
	let perlpath = ".,,"
    endif
endif

let &l:path=perlpath
"---------------------------------------------

" Undo the stuff we changed.
let b:undo_ftplugin = "setlocal fo< com< cms< inc< inex< def< isf< kp<" .
	    \	      " | unlet! b:browsefilter"

" Restore the saved compatibility options.
let &cpo = s:save_cpo
unlet s:save_cpo
