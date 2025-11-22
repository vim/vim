" Vim compiler file
" Compiler:	Jest
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
" Last Change:  2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "jest"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ --no-install\ jest\ --no-colors

" CompilerSet makeprg=jest
" CompilerSet makeprg=npx\ jest
exe 'CompilerSet makeprg=' .. escape(
			\ (!empty(get(b:, 'jest_makeprg', get(g:, 'jest_makeprg', ''))) ?
			\   get(b:, 'jest_makeprg', get(g:, 'jest_makeprg', '')) :
			\   (get(b:, 'node_makeprg', get(g:, 'node_makeprg', 'npx')) .. ' jest'))
			\ .. ' --no-colors ' ..
                        \ get(b:, 'jest_makeprg_params', get(g:, 'jest_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%-A\ \ ●\ Console,
		       \%E\ \ ●\ %m,
		       \%Z\ %\\{4}%.%#Error:\ %f:\ %m\ (%l:%c):%\\=,
		       \%Z\ %\\{6}at\ %\\S%#\ (%f:%l:%c),
		       \%Z\ %\\{6}at\ %\\S%#\ %f:%l:%c,
		       \%+C\ %\\{4}%\\w%.%#,
		       \%+C\ %\\{4}%[-+]%.%#,
		       \%-C%.%#,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
