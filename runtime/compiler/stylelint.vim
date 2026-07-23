" Vim compiler file
" Compiler:     Stylelint
" Maintainer:   Doug Kearns <dougkearns@gmail.com>
" Last Change:  2024 Apr 03
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "stylelint"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ stylelint\ --formatter\ compact

" CompilerSet makeprg=stylelint
" CompilerSet makeprg=npx\ stylelint
exe 'CompilerSet makeprg=' .. escape(
                        \ (!empty(get(b:, 'stylelint_makeprg', get(g:, 'stylelint_makeprg', ''))) ?
                        \   get(b:, 'stylelint_makeprg', get(g:, 'stylelint_makeprg', '')) :
                        \   (get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' stylelint'))
                        \ .. ' --formatter compact ' ..
                        \ get(b:, 'stylelint_makeprg_params', get(g:, 'stylelint_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%f:\ line\ %l\\,\ col\ %c\\,\ %trror\ -\ %m,
                       \%f:\ line\ %l\\,\ col\ %c\\,\ %tarning\ -\ %m,
                       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
