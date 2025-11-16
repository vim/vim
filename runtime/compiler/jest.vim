" Vim compiler file
" Compiler:	Jest
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
" Last Change:  2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "jest"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ --no-install\ jest\ --no-colors

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=jest\ --no-colors
else
  CompilerSet makeprg=npx\ jest\ --no-colors
endif
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
