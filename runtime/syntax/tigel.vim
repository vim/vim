" Vim syntax file
" Language:	TI DSP gel
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2024 Oct 22

if exists("b:current_syntax")
  finish
endif

" TODO: support gel specific keywords
runtime! syntax/c.vim

let b:current_syntax = "tigel"
