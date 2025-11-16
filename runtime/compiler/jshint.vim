" Vim compiler file
" Compiler:	JSHint
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
" Last Change:  2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "jshint"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ jshint\ --verbose

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=jshint\ --verbose
else
  CompilerSet makeprg=npx\ jshint\ --verbose
endif
CompilerSet errorformat=%f:\ line\ %l\\,\ col\ %c\\,\ %m\ (%t%n),
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
