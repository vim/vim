" Vim compiler file
" Compiler:	csslint for CSS
" Maintainer:	Daniel Moch <daniel@danielmoch.com>
" Last Change:	2016 May 21
"		2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
"               2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "csslint"

" CompilerSet makeprg=csslint
" CompilerSet makeprg=npx\ csslint
exe 'CompilerSet makeprg=' .. escape(
                      \ (!empty(get(b:, 'csslint_makeprg', get(g:, 'csslint_makeprg', ''))) ?
                      \   get(b:, 'csslint_makeprg', get(g:, 'csslint_makeprg', '')) :
		      \   (get(b:, 'javascript_node_makeprg', get(g:, 'javascript_node_makeprg', 'npx')) .. ' csslint'))
		      \ .. ' --format=compact ' ..
                      \ get(b:, 'csslint_makeprg_params', get(g:, 'csslint_makeprg_params', '')), ' \|"')

CompilerSet errorformat=%-G,%-G%f:\ lint\ free!,%f:\ line\ %l\\,\ col\ %c\\,\ %trror\ -\ %m,%f:\ line\ %l\\,\ col\ %c\\,\ %tarning\ -\ %m,%f:\ line\ %l\\,\ col\ %c\\,\ %m
