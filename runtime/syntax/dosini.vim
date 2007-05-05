" Vim syntax file
" Language:	Configuration File (ini file) for MSDOS/MS Windows
" Version Info: @(#)dosini.vim 1.6 97/12/15 08:54:12
" Author:       Sean M. McKee <mckee@misslink.net>
" Maintainer:   Nima Talebi <nima@it.net.au>
" Last Change:	Mon, 26 Jun 2006 22:07:28 +1000


" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" shut case off
syn case ignore

syn match  dosiniLabel		"^.\{-}="
syn region dosiniHeader		start="^\[" end="\]"
syn match  dosiniComment	"^;.*$"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_dosini_syntax_inits")
  if version < 508
    let did_dosini_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

	HiLink dosiniHeader	Special
	HiLink dosiniComment	Comment
	HiLink dosiniLabel	Type

  delcommand HiLink
endif

let b:current_syntax = "dosini"

" vim:ts=8
