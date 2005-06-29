" Vim syntax file
" Language:         libao.conf(5) configuration file
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword libaoTodo     contained TODO FIXME XXX NOTE

syn region  libaoComment  matchgroup=libaoComment start='^\s*#' end='$'
                          \ contains=libaoTodo,@Spell

syn keyword libaoKeyword  default_driver

hi def link libaoTodo     Todo
hi def link libaoComment  Comment
hi def link libaoKeyword  Keyword

let b:current_syntax = "libao"

let &cpo = s:cpo_save
unlet s:cpo_save
