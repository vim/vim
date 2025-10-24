" Vim compiler file
" Language:    TOML
" Maintainer:  Konfekt
" Last Change: 2025 Oct 17

if exists("current_compiler") | finish | endif
let current_compiler = "tombi"

let s:cpo_save = &cpo
set cpo&vim

" NOTE: requires sed to strip ANSI color codes!
CompilerSet makeprg=tombi\ lint\ $*\ \|\ sed\ -r\ \"s/\\x1B(\\[[0-9;]*[JKmsu]\|\\(B)//g\"
CompilerSet errorformat=%E%*\\sError:\ %m,%Z%*\\sat\ %f:%l:%c
CompilerSet errorformat+=%W%*\\sWarning:\ %m,%Z%*\\sat\ %f:%l:%c
CompilerSet errorformat+=%-G1\ file\ failed\ to\ be\ linted

let &cpo = s:cpo_save
unlet s:cpo_save
