" Vim compiler file
" Compiler:	se (SmartEiffel Compiler)
" Maintainer:	Doug Kearns <djkea2@mugca.its.monash.edu.au>
" URL:		http://mugca.its.monash.edu.au/~djkea2/vim/compiler/se.vim
" Last Change:	2004 May 16

if exists("current_compiler")
  finish
endif
let current_compiler = "se"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

let s:cpo_save = &cpo
set cpo-=C

CompilerSet makeprg=compile\ %

CompilerSet errorformat=%W******\ Warning:\ %m,
		    \%E******\ Fatal\ Error:\ %m,
		    \%E******\ Error:\ %m,
		    \%CLine\ %l\ column\ %c\ in\ %\\w%\\+\ (%f)\ :,
		    \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
