" Vim compiler file
" Compiler:	TypeScript Runner
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
"               2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "node"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ ts-node

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=ts-node
else
  CompilerSet makeprg=npx\ ts-node
endif
CompilerSet errorformat=%f\ %#(%l\\,%c):\ %trror\ TS%n:\ %m,
		       \%E%f:%l,
		       \%+Z%\\w%\\+Error:\ %.%#,
		       \%C%p^%\\+,
		       \%C%.%#,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
