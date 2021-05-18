" Vim indent file
" Language:	generic Changelog file
" Maintainer:	no one
" Last Change:	2021 May 17 

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

setlocal ai

let b:undo_indent = "setl ai<"
