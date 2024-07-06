" Vim indent file
" Language:	Mojo
" Maintainer:	Riley Bruins <ribru17@gmail.com>
" Last Change:	2024 Jul 07

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

setlocal cinkeys-=0#
setlocal indentkeys-=0#

let b:undo_indent = 'setl cinkeys< indentkeys<'
