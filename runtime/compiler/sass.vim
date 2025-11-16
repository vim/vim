" Vim compiler file
" Compiler:	Sass
" Maintainer:	Tim Pope <vimNOSPAM@tpope.org>
" Last Change:	2016 Aug 29
"		2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
"               2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "sass"

let s:cpo_save = &cpo
set cpo-=C

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=sass
else
  CompilerSet makeprg=npx\ sass
endif

CompilerSet errorformat=
      \%f:%l:%m\ (Sass::Syntax%trror),
      \%ESyntax\ %trror:%m,
      \%C%\\s%\\+on\ line\ %l\ of\ %f,
      \%Z%.%#,
      \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save

" vim:set sw=2 sts=2:
