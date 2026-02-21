" Vim compiler file
" Compiler:    ESLint for JavaScript
" Maintainer:  Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change: 2024 Nov 30
"              2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "eslint"

" CompilerSet makeprg=eslint
" CompilerSet makeprg=npx\ eslint
exe 'CompilerSet makeprg=' .. escape(
                        \ (!empty(get(b:, 'eslint_makeprg', get(g:, 'eslint_makeprg', ''))) ?
                        \   get(b:, 'eslint_makeprg', get(g:, 'eslint_makeprg', '')) :
                        \   (get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' eslint'))
                        \ .. ' --format stylish ' ..
                        \ get(b:, 'eslint_makeprg_params', get(g:, 'eslint_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%-P%f,\%\\s%#%l:%c\ %#\ %trror\ \ %m,\%\\s%#%l:%c\ %#\ %tarning\ \ %m,\%-Q,\%-G%.%#,
