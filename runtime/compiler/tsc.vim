" Vim compiler file
" Compiler:     TypeScript Compiler
" Maintainer:   Doug Kearns <dougkearns@gmail.com>
" Last Change:  2024 Apr 03
"               2025 Mar 11 by The Vim Project (add comment for Dispatch, add tsc_makeprg variable)
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "tsc"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=tsc
" CompilerSet makeprg=npx\ tsc
exe 'CompilerSet makeprg=' .. escape(
                        \ (!empty(get(b:, 'tsc_makeprg', get(g:, 'tsc_makeprg', ''))) ?
                        \   get(b:, 'tsc_makeprg', get(g:, 'tsc_makeprg', '')) :
                        \   get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' tsc')
                        \ .. ' ' ..
                        \ get(b:, 'tsc_makeprg_params', get(g:, 'tsc_makeprg_params', '')), ' \|"')

CompilerSet errorformat=%f\ %#(%l\\,%c):\ %trror\ TS%n:\ %m,
		       \%trror\ TS%n:\ %m,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
