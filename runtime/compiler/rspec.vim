" Vim compiler file
" Language:		RSpec
" Maintainer:		Tim Pope <vimNOSPAM@tpope.info>
" Info:			$Id: rspec.vim,v 1.2 2008/06/29 04:18:42 tpope Exp $
" URL:			http://vim-ruby.rubyforge.org
" Anon CVS:		See above site
" Release Coordinator:	Doug Kearns <dougkearns@gmail.com>

if exists("current_compiler")
  finish
endif
let current_compiler = "rspec"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

let s:cpo_save = &cpo
set cpo-=C

CompilerSet makeprg=spec

CompilerSet errorformat=
    \%+W'%.%#'\ FAILED,
    \%+I'%.%#'\ FIXED,
    \%-Cexpected:%.%#,
    \%-C\ \ \ \ \ got:%.%#,
    \%E%.%#:in\ `load':\ %f:%l:%m,
    \%C%f:%l:,
    \%W%f:%l:\ warning:\ %m,
    \%E%f:%l:in\ %*[^:]:\ %m,
    \%E%f:%l:\ %m,
    \%-Z%\tfrom\ %f:%l,
    \%-Z%p^%.%#,
    \%-C%.%#,
    \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: nowrap sw=2 sts=2 ts=8:
