" Vim syntax file
" Language:	Vim spell file
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2005 Mar 22

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn match vimspellError		".*"
syn match vimspellRegion	"^---$"
syn match vimspellRegion	"^\(-\l\l\)\+$"
syn match vimspellOK		"^!\=[>+]\=[[:alpha:]]\S*"
syn match vimspellOK		"^!\=+\S*"
syn match vimspellError		"\s\+$"
syn match vimspellOK		"^$"
syn match vimspellComment	"^#.*"

" Define the default highlighting.
" Only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_diff_syntax_inits")
  command -nargs=+ HiLink hi def link <args>

  HiLink vimspellComment	Comment
  HiLink vimspellRegion		DiffAdd
  HiLink vimspellError		Error

  delcommand HiLink
endif

let b:current_syntax = "vimspell"

" vim: ts=8 sw=2
