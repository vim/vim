" Vim indent file
" Language:	C
" Maintainer: The Vim Project <https://github.com/vim/vim>
" Former Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2023 Aug 10

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

" C indenting is built-in, thus this is very simple
setlocal cindent

let b:undo_indent = "setl cin<"
