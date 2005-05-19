" Vim indent file
" Language:	Lisp
" Maintainer:    Sergey Khorev <sergey.khorev@gmail.com>
" URL:		 http://iamphet.nm.ru/vim
" Last Change:	2005 May 19

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

setlocal ai nosi

let b:undo_indent = "setl ai< si<"
