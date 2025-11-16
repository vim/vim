" Vim compiler file
" Compiler:	JSON Lint
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
" Last Change:  2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "jsonlint"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ jsonlint\ --compact\ --quiet

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=jsonlint\ --compact\ --quiet
else
  CompilerSet makeprg=npx\ jsonlint\ --compact\ --quiet
endif
CompilerSet errorformat=%f:\ line\ %l\\,\ col\ %c\\,\ found:\ %m,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
