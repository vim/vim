" Vim compiler file
" Compiler:	HP aCC
" Maintainer:	Matthias Ulrich <matthias-ulrich@web.de>
" URL:		http://www.subhome.de/vim/hp_acc.vim
" Last Change:	2004 Mar 27
"
"  aCC --version says: "HP ANSI C++ B3910B A.03.13"
"  This compiler has been tested on:
"       hp-ux 10.20, hp-ux 11.0 and hp-ux 11.11 (64bit)
"
"  Tim Brown's aCC is: "HP ANSI C++ B3910B A.03.33"
"  and it also works fine...

if exists("current_compiler")
  finish
endif
let current_compiler = "hp_acc"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet errorformat=%A%trror\ %n\:\ \"%f\"\\,\ line\ %l\ \#\ %m,
         \%A%tarning\ %n\:\ \"%f\"\\,\ line\ %l\ \#\ %m\ %#,
         \%Z\ \ \ \ %p^%.%#,
         \%-C%.%#

" vim:ts=8:sw=4:cindent
