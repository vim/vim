" Vim syntax file
" Language:	sgml catalog file
" Maintainer:	Johannes Zellner <johannes@zellner.org>
" Last Change:	Tue, 27 Apr 2004 14:54:59 CEST
" Filenames:	/etc/sgml.catalog
" $Id$

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn case ignore

" strings
syn region  catalogString start=+"+ skip=+\\\\\|\\"+ end=+"+ keepend
syn region  catalogString start=+'+ skip=+\\\\\|\\'+ end=+'+ keepend

syn region  catalogComment      start=+--+   end=+--+ contains=catalogTodo
syn keyword catalogTodo		TODO FIXME XXX contained display
syn keyword catalogKeyword	DOCTYPE OVERRIDE PUBLIC DTDDECL ENTITY display


" The default highlighting.
hi def link catalogString		     String
hi def link catalogComment		     Comment
hi def link catalogTodo			     Todo
hi def link catalogKeyword		     Statement

let b:current_syntax = "catalog"
