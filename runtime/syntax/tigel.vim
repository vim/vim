" Vim syntax file
" Language:	TI DSP gel
" Maintainer:	Wu, Zhenyu <wuzhenyu@ustc.edu>
" Last Change:	2024 Oct 22

if exists("b:current_syntax")
  finish
endif

" TODO: support gel specific keywords
runtime! syntax/c.vim

let b:current_syntax = "tigel"
