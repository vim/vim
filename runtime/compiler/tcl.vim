" Vim compiler file
" Compiler:	tcl
" Maintainer:	Doug Kearns <djkea2@mugca.its.monash.edu.au>
" URL:		http://mugca.its.monash.edu.au/~djkea2/vim/compiler/tcl.vim
" Last Change:	2004 Mar 27

if exists("current_compiler")
  finish
endif
let current_compiler = "tcl"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet makeprg=tcl

CompilerSet errorformat=%EError:\ %m,%+Z\ %\\{4}(file\ \"%f\"\ line\ %l),%-G%.%#
