" Vim indent file
" Language:	Lisp
" Maintainer:	noone
" Last Change:	2005 Mar 28

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

" Autoindent is the best we can do.
setlocal ai

let b:undo_indent = "setl ai<"
