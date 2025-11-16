" Vim compiler file
" Compiler:	csslint for CSS
" Maintainer:	Daniel Moch <daniel@danielmoch.com>
" Last Change:	2016 May 21
"		2024 Apr 03 by The Vim Project (removed :CompilerSet definition)
"               2025 Nov 16 by The Vim Project (check if in webdev repo)

if exists("current_compiler")
  finish
endif
let current_compiler = "csslint"

if empty(findfile('package.json', '.;'))
  CompilerSet makeprg=csslint\ --format=compact
else
  CompilerSet makeprg=npx\ csslint\ --format=compact
endif
CompilerSet errorformat=%-G,%-G%f:\ lint\ free!,%f:\ line\ %l\\,\ col\ %c\\,\ %trror\ -\ %m,%f:\ line\ %l\\,\ col\ %c\\,\ %tarning\ -\ %m,%f:\ line\ %l\\,\ col\ %c\\,\ %m
