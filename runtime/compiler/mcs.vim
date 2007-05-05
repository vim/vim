" Vim compiler file
" Compiler:    Mono C# Compiler
" Maintainer:  Jarek Sobiecki <harijari@go2.pl>
" Latest Revision: 2006-06-18

if exists("current_compiler")
  finish
endif
let current_compiler = "mcs"

let s:cpo_save = &cpo
set cpo-=C

setlocal errorformat=
         \%E%f(%l\\,%c):\ error\ CS%n:%m,
         \%W%f(%l\\,%c):\ warning\ CS%n:%m,
         \%E%>syntax\ error\\,%m,%Z%f(%l\\,%c):\ error\ CS%n:%m,
         \%D%*\\a[%*\\d]:\ Entering\ directory\ `%f',
         \%X%*\\a[%*\\d]:\ Leaving\ directory\ `%f',
         \%DMaking\ %*\\a\ in\ %f,
         \%G-%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
