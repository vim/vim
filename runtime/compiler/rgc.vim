" Vim compiler file
" Language: RGC
" License: Apache-2.0

if exists("current_compiler")
  finish
endif
let current_compiler = "rgc"

let s:cpo_save = &cpo
set cpo&vim

CompilerSet makeprg=rgc\ \"%\"\ -o\ \"%\<\"
CompilerSet errorformat=%f:%l:%c:\ %trror:\ %m,%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save