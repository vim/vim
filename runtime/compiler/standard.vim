" Vim compiler file
" Compiler:    Standard for JavaScript
" Maintainer:  Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change: 2020 August 20
"		2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "standard"

" CompilerSet makeprg=standard
" CompilerSet makeprg=npx\ standard
exe 'CompilerSet makeprg=' .. escape(
			\ get(b:, 'node_makeprg', get(g:, 'node_makeprg', 'npx')) .. ' standard' ..
			\ ' ' ..
                        \ get(b:, 'standard_makeprg_params', get(g:, 'standard_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%f:%l:%c:\ %m,%-G%.%#
