" Vim compiler file
" Compiler:	HTML Tidy
" Maintainer:	Doug Kearns <djkea2@gus.gscit.monash.edu.au>
" URL:		http://gus.gscit.monash.edu.au/~djkea2/vim/compiler/tidy.vim
" Last Change:	2004 Nov 27

" NOTE: set 'tidy_compiler_040800' if you are using the 4th August 2000 release
"       of HTML Tidy.

if exists("current_compiler")
  finish
endif
let current_compiler = "tidy"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

" this is needed to work around a bug in the 04/08/00 release of tidy which
" failed to set the filename if the -quiet option was used
if exists("tidy_compiler_040800")
  CompilerSet makeprg=tidy\ -errors\ --gnu-emacs\ yes\ %
else
  CompilerSet makeprg=tidy\ -quiet\ -errors\ --gnu-emacs\ yes\ %
endif

" sample warning: foo.html:8:1: Warning: inserting missing 'foobar' element
" sample error:   foo.html:9:2: Error: <foobar> is not recognized!
CompilerSet errorformat=%f:%l:%c:\ Error:%m,%f:%l:%c:\ Warning:%m,%-G%.%#
