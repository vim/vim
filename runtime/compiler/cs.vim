" Vim compiler file
" Compiler:	ms C#
" Maintainer:	Joseph H. Yao (hyao@sina.com)
" Last Change:	2004 Mar 27

if exists("current_compiler")
  finish
endif
let current_compiler = "cs"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

" default errorformat
CompilerSet errorformat&

" default make
CompilerSet makeprg=csc\ %
