" Vim indent file
" Language:	C++
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2001 Jun 12

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

" C++ indenting is built-in, thus this is very simple
setlocal cindent
