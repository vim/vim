" Language   : Netrw Remote-Directory Listing Syntax
" Maintainer : Charles E. Campbell, Jr.
" Last change: Jun 10, 2004
" Version    : 1

" Syntax Clearing: {{{1
if version < 600
 syntax clear
elseif exists("b:current_syntax")
 finish
endif

" Directory List Syntax Highlighting: {{{1
syn match netrwDir		"^.*/$" contains=netrwSpecial
syn match netrwSpecial		"[*=@|/]$"
syn match netrwSlash contained	"/"
syn match netrwComment		'".*$'

" Highlighting Links: {{{1
if !exists("did_drchip_dbg_syntax")
 let did_drchip_netrwlist_syntax= 1
 hi link netrwComment	Comment
 hi link netrwDir	Directory
 hi link netrwSpecial	Function
endif

" Current Syntax: {{{1
let   b:current_syntax = "netrwlist"
" vim: ts=8 fdm=marker
