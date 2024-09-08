if exists("current_compiler") | finish | endif
if exists("b:current_compiler") | unlet b:current_compiler | endif

let s:cpo_save = &cpo
set cpo&vim

CompilerSet makeprg&
CompilerSet errorformat&

let &cpo = s:cpo_save
unlet s:cpo_save
