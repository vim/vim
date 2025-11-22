" Vim compiler file
" Compiler:	TypeScript Runner
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "node"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=ts-node
" CompilerSet makeprg=npx\ ts-node
exe 'CompilerSet makeprg=' .. escape(
			\ (!empty(get(b:, 'ts_node_makeprg', get(g:, 'ts_node_makeprg', ''))) ?
			\   get(b:, 'ts_node_makeprg', get(g:, 'ts_node_makeprg', '')) :
			\   (get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' ts-node'))
			\ .. ' ' ..
                        \ get(b:, 'ts_node_makeprg_params', get(g:, 'ts_node_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%f\ %#(%l\\,%c):\ %trror\ TS%n:\ %m,
		       \%E%f:%l,
		       \%+Z%\\w%\\+Error:\ %.%#,
		       \%C%p^%\\+,
		       \%C%.%#,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
