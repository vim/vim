" Vim compiler file
" Compiler:	Checkstyle
" Maintainer:	Doug Kearns <djkea2@gus.gscit.monash.edu.au>
" URL:		http://gus.gscit.monash.edu.au/~djkea2/vim/compiler/checkstyle.vim
" Last Change:	2004 Nov 27

if exists("current_compiler")
  finish
endif
let current_compiler = "checkstyle"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet makeprg=java\ com.puppycrawl.tools.checkstyle.Main\ -f\ plain

" sample error: WebTable.java:282: '+=' is not preceeded with whitespace.
"		WebTable.java:201:1: '{' should be on the previous line.
CompilerSet errorformat=%f:%l:\ %m,%f:%l:%v:\ %m,%-G%.%#
