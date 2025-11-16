" Vim compiler file
" Compiler:	TypeDoc
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Apr 03
"               2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "typedoc"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=npx\ typedoc

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=typedoc
else
  CompilerSet makeprg=npx\ typedoc
endif
CompilerSet errorformat=%EError:\ %f(%l),
		       \%WWarning:\ %f(%l),
		       \%+IDocumentation\ generated\ at\ %f,
		       \%Z\ %m,
		       \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
