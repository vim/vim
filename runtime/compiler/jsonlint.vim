" Vim compiler file
" Compiler:     JSON Lint
" Maintainer:   Doug Kearns <dougkearns@gmail.com>
" Last Change:  2024 Apr 03
" Last Change:  2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "jsonlint"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ jsonlint\ --compact\ --quiet

" CompilerSet makeprg=jsonlint
" CompilerSet makeprg=npx\ jsonlint
exe 'CompilerSet makeprg=' .. escape(
                        \ (!empty(get(b:, 'jsonlint_makeprg', get(g:, 'jsonlint_makeprg', ''))) ?
                        \   get(b:, 'jsonlint_makeprg', get(g:, 'jsonlint_makeprg', '')) :
                        \   (get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' jsonlint'))
                        \ .. ' --compact --quiet ' ..
                        \ get(b:, 'jsonlint_makeprg_params', get(g:, 'jsonlint_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%f:\ line\ %l\\,\ col\ %c\\,\ found:\ %m,
                       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
