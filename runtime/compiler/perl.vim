" Vim Compiler File
" Compiler:     Perl syntax checks (perl -Wc)
" Maintainer:   Christian J. Robinson <infynity@onewest.net>
" Last Change:  2004 Mar 27

if exists("current_compiler")
  finish
endif
let current_compiler = "perl"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

let s:savecpo = &cpo
set cpo&vim

if getline(1) =~# '-[^ ]*T'
	CompilerSet makeprg=perl\ -WTc\ %
else
	CompilerSet makeprg=perl\ -Wc\ %
endif

CompilerSet errorformat=
	\%-G%.%#had\ compilation\ errors.,
	\%-G%.%#syntax\ OK,
	\%m\ at\ %f\ line\ %l.,
	\%+A%.%#\ at\ %f\ line\ %l\\,%.%#,
	\%+C%.%#

" Explanation:
" %-G%.%#had\ compilation\ errors.,  - Ignore the obvious.
" %-G%.%#syntax\ OK,                 - Don't include the 'a-okay' message.
" %m\ at\ %f\ line\ %l.,             - Most errors...
" %+A%.%#\ at\ %f\ line\ %l\\,%.%#,  - As above, including ', near ...'
" %+C%.%#                            -   ... Which can be multi-line.

let &cpo = s:savecpo
unlet s:savecpo
