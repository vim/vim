" Vim syntax file
" Language:	Vivado mss file
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2024 Oct 22
if exists("b:current_syntax")
  finish
endif

syn keyword	mssKeyword	BEGIN END PARAMETER
hi def link mssKeyword		Keyword

let b:current_syntax = "mss"
