" Language   : Netrw Remote-Directory Listing Syntax
" Maintainer : Charles E. Campbell, Jr.
" Last change: Sep 08, 2004
" Version    : 5
" ---------------------------------------------------------------------

" Syntax Clearing: {{{1
if version < 600
 syntax clear
elseif exists("b:current_syntax")
 finish
endif

" ---------------------------------------------------------------------
" Directory List Syntax Highlighting: {{{1
syn cluster NetrwGroup contains=netrwHide,netrwSortBy,netrwSortSeq,netrwQuickHelp,netrwVersion
syn match  netrwDir				"^.*/\%(\t\|$\)"	contains=netrwClassify
syn match  netrwClassify			"[*=|@/]\%(\t\|$\)"
syn match  netrwSymLink				"^.*@\%(\t\|$\)"	contains=netrwClassify
syn match  netrwComment				'".*\%(\t\|$\)'		contains=@NetrwGroup
syn match  netrwHide				'^"\s*\(Hid\|Show\)ing:' skipwhite nextgroup=netrwHidePat
syn match  netrwSlash	contained		"/"
syn match  netrwHidePat	contained		"[^,]\+"		skipwhite nextgroup=netrwHideSep
syn match  netrwHideSep	contained transparent	","			skipwhite nextgroup=netrwHidePat
syn match  netrwSortBy	contained transparent	"Sorted by"		skipwhite nextgroup=netrwList
syn match  netrwSortSeq	contained transparent	"Sort sequence:"	skipwhite nextgroup=netrwList
syn match  netrwList	contained		".*$"			contains=netrwComma
syn match  netrwComma	contained		","
syn region netrwQuickHelp contained matchgroup=Comment start="Quick Help:\s\+" end="$" contains=netrwHelpCmd keepend
syn match  netrwHelpCmd	contained		"\S\ze:"		skipwhite nextgroup=netrwCmdSep
syn match  netrwCmdSep	contained		":"			nextgroup=netrwCmdNote
syn match  netrwCmdNote	contained		".\{-}\ze  "
syn match  netrwVersion contained		"(netrw.*)"

" ---------------------------------------------------------------------
" Highlighting Links: {{{1
if !exists("did_drchip_dbg_syntax")
 let did_drchip_netrwlist_syntax= 1
 hi link netrwClassify	Function
 hi link netrwCmdSep	Delimiter
 hi link netrwComment	Comment
 hi link netrwDir	Directory
 hi link netrwHelpCmd	Function
 hi link netrwHidePat	Statement
 hi link netrwList	Statement
 hi link netrwVersion	Identifier
 hi link netrwSymLink	Special

 hi link netrwComma	netrwComment
 hi link netrwHide	netrwComment
endif

" Current Syntax: {{{1
let   b:current_syntax = "netrwlist"
" ---------------------------------------------------------------------
" vim: ts=8 fdm=marker
