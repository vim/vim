" Vim compiler file
" Compiler:	SGI IRIX 5.3 cc
" Maintainer:	David Harrison <david_jr@users.sourceforge.net>
" Last Change:	2004 Mar 27

if exists("current_compiler")
  finish
endif
let current_compiler = "irix5_c"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet errorformat=\%Ecfe:\ Error:\ %f\\,\ line\ %l:\ %m,
		     \%Wcfe:\ Warning:\ %n:\ %f\\,\ line\ %l:\ %m,
		     \%Wcfe:\ Warning\ %n:\ %f\\,\ line\ %l:\ %m,
		     \%W(%l)\ \ Warning\ %n:\ %m,
		     \%-Z\ %p^,
		     \-G\\s%#,
		     \%-G%.%#
