" Vim compiler file
" Compiler:	XO
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "xo"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=xo
" CompilerSet makeprg=npx\ xo
exe 'CompilerSet makeprg=' .. escape(
			\ get(b:, 'node_makeprg', get(g:, 'node_makeprg', 'npx')) .. ' xo' ..
			\ ' --reporter compact ' ..
                        \ get(b:, 'xo_makeprg_params', get(g:, 'xo_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%f:\ line\ %l\\,\ col\ %c\\,\ %trror\ %m,
		       \%f:\ line\ %l\\,\ col\ %c\\,\ %tarning\ %m,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
