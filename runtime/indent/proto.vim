" Vim indent file
" Language:	Protobuf
" Maintainer:	Johannes Zellner <johannes@zellner.org>
" Last Change:	2002 Mar 15

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

" Protobuf is like indenting C
setlocal cindent
setlocal expandtab
setlocal shiftwidth=2

let b:undo_indent = "setl cin<"

" vim: sw=2 sts=2 et
