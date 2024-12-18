" Vim syntax file
" Language:	TI DSP linker command file
" Maintainer:	Wu, Zhenyu <wuzhenyu@ustc.edu>
" Last Change:	2024 Oct 22

if exists("b:current_syntax")
  finish
endif

" TODO: support C macro defines
runtime! syntax/ld.vim

let b:current_syntax = "tild"
