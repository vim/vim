" Vim compiler file
" Compiler:         reStructuredText Documentation Format
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("current_compiler")
  finish
endif
let current_compiler = "rst"

if exists(":CompilerSet") != 2
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
