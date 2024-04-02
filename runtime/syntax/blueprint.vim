" Vim syntax file
" Language: Android Blueprint Language
" Maintainer: Chris McClellan <chris.mcclellan203@gmail.com>

if exists("b:current_syntax")
  finish
endif

" treat it as js object
runtime! syntax/js.vim

let b:current_syntax = "blueprint"
