" Vim syntax file
" Language:         GNU Arch inventory file
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

setlocal iskeyword=@,48-57,_,-

syn keyword archTodo    TODO FIXME XXX NOTE

syn region  archComment display matchgroup=archComment
                        \ start='^\%(#\|\s\)' end='$' contains=archTodo,@Spell

syn keyword archKeyword implicit tagline explicit names
syn keyword archKeyword untagged-source
syn keyword archKeyword exclude junk backup precious unrecognized source
                        \ skipwhite nextgroup=archRegex

syn match   archRegex   contained '\s*\zs.*'

hi def link archTodo    Todo
hi def link archComment Comment
hi def link archKeyword Keyword
hi def link archRegex   String

let b:current_syntax = "arch"

let &cpo = s:cpo_save
unlet s:cpo_save
