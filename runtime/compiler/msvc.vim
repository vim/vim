" Vim compiler file
" Compiler:	Miscrosoft Visual C
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2004 Mar 19

if exists("current_compiler")
  finish
endif
let current_compiler = "msvc"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

" The errorformat for MSVC is the default.
CompilerSet errorformat&
CompilerSet makeprg=nmake
