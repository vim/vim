" Language   : Netrw Remote-Directory Listing Syntax
" Maintainer : Charles E. Campbell, Jr.
" Last change: Jun 25, 2004
" Version    : 2

" Syntax Clearing: {{{1
if version < 600
 syntax clear
elseif exists("b:current_syntax")
 finish
endif

" Directory List Syntax Highlighting: {{{1
syn match netrwDir				"^.*/$"			contains=netrwClassify
syn match netrwClassify 			"[*=|@/]$"
syn match netrwSlash contained			"/"
syn match netrwSymLink				"^.*@$"			contains=netrwClassify
syn match netrwComment				'".*$'			contains=netrwHide,netrwSortBy,netrwSortSeq
syn match netrwHide				'^"\s*Hiding:'		skipwhite nextgroup=netrwHidePat
syn match netrwHidePat contained		"[^,]\+"		skipwhite nextgroup=netrwHideSep
syn match netrwHideSep contained transparent	","			skipwhite nextgroup=netrwHidePat
syn match netrwSortBy  contained transparent	"Sorted by"		skipwhite nextgroup=netrwList
syn match netrwSortSeq contained transparent	"Sort sequence:"	skipwhite nextgroup=netrwList
syn match netrwList    contained		".*$"	contains=netrwComma
syn match netrwComma	contained	","

" Highlighting Links: {{{1
if !exists("did_drchip_dbg_syntax")
 let did_drchip_netrwlist_syntax= 1
 hi link netrwClassify	Function
 hi link netrwComment	Comment
 hi link netrwDir	Directory
 hi link netrwHidePat	String
 hi link netrwList	String
 hi link netrwSymLink	Special

 hi link netrwComma	netrwComment
 hi link netrwHide	netrwComment
endif

" Current Syntax: {{{1
let   b:current_syntax = "netrwlist"
" vim: ts=8 fdm=marker
