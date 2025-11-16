" Vim compiler file
" Compiler:    Standard for JavaScript
" Maintainer:  Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change: 2020 August 20
"		2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
"               2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "standard"

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=standard
else
  CompilerSet makeprg=npx\ standard
endif
CompilerSet errorformat=%f:%l:%c:\ %m,%-G%.%#
