" Vim compiler file
" Compiler:	tcl
" Maintainer:	Doug Kearns <djkea2@gus.gscit.monash.edu.au>
" URL:		http://gus.gscit.monash.edu.au/~djkea2/vim/compiler/tcl.vim
" Last Change:	2004 Nov 27

if exists("current_compiler")
  finish
endif
let current_compiler = "tcl"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet makeprg=tcl

CompilerSet errorformat=%EError:\ %m,%+Z\ %\\{4}(file\ \"%f\"\ line\ %l),%-G%.%#
