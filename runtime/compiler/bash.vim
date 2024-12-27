" Vim compiler file
" Compiler:     Bash Syntax Checker
" Maintainer:   @konfekt
" Last Change:  2024 Dec 27

if exists("current_compiler") | finish | endif
let current_compiler = "bash"

let s:cpo_save = &cpo
set cpo&vim

CompilerSet makeprg=bash\ -n
CompilerSet errorformat=%f:\ line\ %l:\ %m

let &cpo = s:cpo_save
unlet s:cpo_save
