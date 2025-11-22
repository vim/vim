" Vim compiler file
" Compiler:	TypeDoc
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "typedoc"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ typedoc

" CompilerSet makeprg=typedoc
" CompilerSet makeprg=npx\ typedoc
exe 'CompilerSet makeprg=' .. escape(
			\ (!empty(get(b:, 'typedoc_makeprg', get(g:, 'typedoc_makeprg', ''))) ?
			\   get(b:, 'typedoc_makeprg', get(g:, 'typedoc_makeprg', '')) :
			\   (get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' typedoc'))
			\ .. ' ' ..
                        \ get(b:, 'typedoc_makeprg_params', get(g:, 'typedoc_makeprg_params', '')), ' \|"')
CompilerSet errorformat=%EError:\ %f(%l),
		       \%WWarning:\ %f(%l),
		       \%+IDocumentation\ generated\ at\ %f,
		       \%Z\ %m,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
