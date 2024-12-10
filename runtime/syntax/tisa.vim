" Vim syntax file
" Language:	TI linear assembly language
" Maintainer:	Wu, Zhenyu <wuzhenyu@ustc.edu>
" Last Change:	2024 Oct 22

if exists("b:current_syntax")
  finish
endif

" TODO: not same asm, just better than none
runtime! syntax/asm.vim

let b:current_syntax = "tisa"
