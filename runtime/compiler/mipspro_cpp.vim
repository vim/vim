" Vim compiler file
" Compiler:	SGI IRIX 6.5 MIPSPro C++ (CC)
" Maintainer:	David Harrison <david_jr@users.sourceforge.net>
" Last Change:	2004 Mar 27

if exists("current_compiler")
  finish
endif
let current_compiler = "mipspro_cpp"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet errorformat=%Ecc\-%n\ %.%#:\ ERROR\ File\ =\ %f\%\\,\ Line\ =\ %l,
		    \%Wcc\-%n\ %.%#:\ WARNING\ File\ =\ %f\%\\,\ Line\ =\ %l,
		    \%Icc\-%n\ %.%#:\ REMARK\ File\ =\ %f\%\\,\ Line\ =\ %l,
		    \%+C\ \ %m.,
		    \%-Z\ \ %p^,
		    \%-G\\s%#,
		    \%-G%.%#
