" Vim syntax file
" Language:	Luau
" Maintainer:	None yet
" Last Change:	2023 Apr 30
" 2025 Sep 26 by Vim Project: remove nowrap modeline (#18399)

if exists("b:current_syntax")
  finish
endif

" Luau is a superset of lua
runtime! syntax/lua.vim

let b:current_syntax = "luau"

" vim: sw=2 sts=2 ts=8 noet:
