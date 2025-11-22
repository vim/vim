" Vim compiler file
" Compiler:	JSON Lint
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
" Last Change:  2025 Nov 22 by The Vim Project (default to npx)

if exists("current_compiler")
  finish
endif
let current_compiler = "jsonlint"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ jsonlint\ --compact\ --quiet

" CompilerSet makeprg=jsonlint
" CompilerSet makeprg=npx\ jsonlint
if !empty(escape(get(b:, 'jsonlint_makeprg', get(g:, 'jsonlint_makeprg', ''))))
  execute $'CompilerSet makeprg={escape(get(b:, 'jsonlint_makeprg', get(g:, 'jsonlint_makeprg', 'jsonlint')), ' \|"')}'
else
  exe 'CompilerSet makeprg=' .. escape(
			\ get(b:, 'node_makeprg', get(g:, 'node_makeprg', 'npx')) .. ' jsonlint' ..
			\ ' --compact --quiet ' ..
                        \ get(b:, 'jsonlint_makeprg_params', get(g:, 'jsonlint_makeprg_params', '')), ' \|"')
endif
CompilerSet errorformat=%f:\ line\ %l\\,\ col\ %c\\,\ found:\ %m,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
