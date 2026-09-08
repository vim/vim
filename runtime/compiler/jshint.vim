" Vim compiler file
" Compiler:     JSHint
" Maintainer:   Doug Kearns <dougkearns@gmail.com>
" Last Change:  2024 Apr 03
" Last Change:  2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "jshint"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ jshint\ --verbose

" CompilerSet makeprg=jshint
" CompilerSet makeprg=npx\ jshint
exe 'CompilerSet makeprg=' .. escape(
                        \ (!empty(get(b:, 'jshint_makeprg', get(g:, 'jshint_makeprg', ''))) ?
                        \   get(b:, 'jshint_makeprg', get(g:, 'jshint_makeprg', '')) :
                        \   (get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' jshint'))
                        \ .. ' --verbose ' ..
                        \ get(b:, 'jshint_makeprg_params', get(g:, 'jshint_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%f:\ line\ %l\\,\ col\ %c\\,\ %m\ (%t%n),
                       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
