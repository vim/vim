" Vim compiler file
" Language:		Test::Unit - Ruby Unit Testing Framework
" Maintainer:		Doug Kearns <dougkearns@gmail.com>
" URL:			https://github.com/vim-ruby/vim-ruby
" Release Coordinator:	Doug Kearns <dougkearns@gmail.com>
" Last Change:		2014 Mar 23
"			2024 Apr 03 by Vim Project (removed :CompilerSet definition)
"			2025 Sep 26 by Vim project: remove nowrap modeline (#18399)

if exists("current_compiler")
  finish
endif
let current_compiler = "rubyunit"

let s:cpo_save = &cpo
set cpo-=C

CompilerSet makeprg=testrb
" CompilerSet makeprg=ruby\ -Itest
" CompilerSet makeprg=m

CompilerSet errorformat=\%W\ %\\+%\\d%\\+)\ Failure:,
			\%C%m\ [%f:%l]:,
			\%E\ %\\+%\\d%\\+)\ Error:,
			\%C%m:,
			\%C\ \ \ \ %f:%l:%.%#,
			\%C%m,
			\%Z\ %#,
			\%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: sw=2 sts=2 ts=8:
