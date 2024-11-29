" Vim compiler file
" Compiler:    ESLint for JavaScript
" Maintainer:  Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change: 2020 August 20
"	       2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
"	       2024 Nov 29 by romainl (changed errorformat and makeprg)

if exists("current_compiler")
  finish
endif
let current_compiler = "eslint"

CompilerSet makeprg=npx\ eslint\ --format\ stylish
CompilerSet errorformat=%-P%f,\%\\s%#%l:%c\ %#\ %trror\ \ %m,\%\\s%#%l:%c\ %#\ %tarning\ \ %m,\%-Q,\%-G%.%#,
