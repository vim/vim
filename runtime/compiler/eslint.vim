" Vim compiler file
" Compiler:    ESLint for JavaScript
" Maintainer:  Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change: 2024 Nov 30
"              2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "eslint"

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=eslint\ --format\ stylish
else
  CompilerSet makeprg=npx\ eslint\ --format\ stylish
endif
CompilerSet errorformat=%-P%f,\%\\s%#%l:%c\ %#\ %trror\ \ %m,\%\\s%#%l:%c\ %#\ %tarning\ \ %m,\%-Q,\%-G%.%#,
