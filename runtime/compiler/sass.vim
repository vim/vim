" Vim compiler file
" Compiler:	Sass
" Maintainer:	Tim Pope <vimNOSPAM@tpope.org>
" Last Change:	2016 Aug 29
"		2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "sass"

let s:cpo_save = &cpo
set cpo-=C

" CompilerSet makeprg=sass
" CompilerSet makeprg=npx\ sass
exe 'CompilerSet makeprg=' .. escape(
			\ (!empty(get(b:, 'sass_makeprg', get(g:, 'sass_makeprg', ''))) ?
			\   get(b:, 'sass_makeprg', get(g:, 'sass_makeprg', '')) :
			\   (get(b:, 'node_makeprg', get(g:, 'node_makeprg', 'npx')) .. ' sass'))
			\ .. ' ' ..
                        \ get(b:, 'sass_makeprg_params', get(g:, 'sass_makeprg_params', '')), ' \|"')

CompilerSet errorformat=
      \%f:%l:%m\ (Sass::Syntax%trror),
      \%ESyntax\ %trror:%m,
      \%C%\\s%\\+on\ line\ %l\ of\ %f,
      \%Z%.%#,
      \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save

" vim:set sw=2 sts=2:
