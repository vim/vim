" Vim syntax file
" Language:	OpenCL
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2024 Oct 22
" Maintainer:	Wu, Zhenyu <wuzhenyu@ustc.edu>

if exists("b:current_syntax")
  finish
endif

" TODO: support openCL specific keywords
runtime! syntax/c.vim

let current_syntax = "opencl"
