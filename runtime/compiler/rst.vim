" Vim compiler file
" Compiler:	    reStructuredText Documentation Format
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/compiler/pcp/rst/
" Latest Revision:  2004-05-22
" arch-tag:	    ac64a95a-5d45-493d-a9f9-f96fc8568657

if exists("current_compiler")
  finish
endif
let current_compiler = "rst"

if exists(":CompilerSet") != 2          " older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

let s:cpo_save = &cpo
set cpo-=C

CompilerSet errorformat=
      \%f:%l:\ (%tEBUG/0)\ %m,
      \%f:%l:\ (%tNFO/1)\ %m,
      \%f:%l:\ (%tARNING/2)\ %m,
      \%f:%l:\ (%tRROR/3)\ %m,
      \%f:%l:\ (%tEVERE/3)\ %m,
      \%D%*\\a[%*\\d]:\ Entering\ directory\ `%f',
      \%X%*\\a[%*\\d]:\ Leaving\ directory\ `%f',
      \%DMaking\ %*\\a\ in\ %f

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: set sts=2 sw=2:
