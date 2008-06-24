" Language   : Netrw Remote-Directory Listing Syntax
" Maintainer : Charles E. Campbell, Jr.
" Last change: Feb 06, 2008
" Version    : 12
" ---------------------------------------------------------------------

" Syntax Clearing: {{{1
if version < 600
 syntax clear
elseif exists("b:current_syntax")
 finish
endif

" ---------------------------------------------------------------------
" Directory List Syntax Highlighting: {{{1
syn cluster NetrwGroup		contains=netrwHide,netrwSortBy,netrwSortSeq,netrwQuickHelp,netrwVersion,netrwCopyTgt
syn cluster NetrwTreeGroup	contains=netrwDir,netrwSymLink,netrwExe

syn match  netrwSpecial		"\%(\S\+ \)*\S\+[*|=]\ze\%(\s\{2,}\|$\)"		contains=netrwClassify
syn match  netrwDir		"\.\{1,2}/"						contains=netrwClassify
syn match  netrwDir		"\%(\S\+ \)*\S\+/"					contains=netrwClassify
syn match  netrwSizeDate	"\<\d\+\s\d\{1,2}/\d\{1,2}/\d\{4}\s"			contains=netrwDateSep skipwhite nextgroup=netrwTime
syn match  netrwSymLink		"\%(\S\+ \)*\S\+@\ze\%(\s\{2,}\|$\)"  			contains=netrwClassify
syn match  netrwExe		"\%(\S\+ \)*\S\+\*\ze\%(\s\{2,}\|$\)" 			contains=netrwClassify
syn match  netrwTreeBar		"^\%(| \)*"						contains=netrwTreeBarSpace	nextgroup=@netrwTreeGroup
syn match  netrwTreeBarSpace	" "				contained

syn match  netrwClassify	"[*=|@/]\ze\%(\s\{2,}\|$\)"	contained
syn match  netrwDateSep		"/"				contained
syn match  netrwTime		"\d\{1,2}:\d\{2}:\d\{2}"	contained		contains=netrwTimeSep
syn match  netrwTimeSep		":"

syn match  netrwComment		'".*\%(\t\|$\)'						contains=@NetrwGroup
syn match  netrwHide		'^"\s*\(Hid\|Show\)ing:'	skipwhite nextgroup=netrwHidePat
syn match  netrwSlash		"/"				contained
syn match  netrwHidePat		"[^,]\+"			contained skipwhite nextgroup=netrwHideSep
syn match  netrwHideSep		","				contained transparent skipwhite nextgroup=netrwHidePat
syn match  netrwSortBy		"Sorted by"			contained transparent skipwhite nextgroup=netrwList
syn match  netrwSortSeq		"Sort sequence:"		contained transparent skipwhite nextgroup=netrwList
syn match  netrwCopyTgt		"Copy/Move Tgt:"		contained transparent skipwhite nextgroup=netrwList
syn match  netrwList		".*$"				contained		contains=netrwComma
syn match  netrwComma		","				contained
syn region netrwQuickHelp	matchgroup=Comment start="Quick Help:\s\+" end="$"	contains=netrwHelpCmd keepend contained
syn match  netrwHelpCmd		"\S\ze:"			contained skipwhite nextgroup=netrwCmdSep
syn match  netrwCmdSep		":"				contained nextgroup=netrwCmdNote
syn match  netrwCmdNote		".\{-}\ze  "			contained
syn match  netrwVersion		"(netrw.*)"			contained

" -----------------------------
" Special filetype highlighting {{{1
" -----------------------------
if exists("g:netrw_special_syntax") && netrw_special_syntax
 syn match netrwBak		"\(\S\+ \)*\S\+\.bak\>"				contains=netrwTreeBar
 syn match netrwCompress	"\(\S\+ \)*\S\+\.\%(gz\|bz2\|Z\|zip\)\>"	contains=netrwTreeBar
 syn match netrwData		"\(\S\+ \)*\S\+\.dat\>"				contains=netrwTreeBar
 syn match netrwHdr		"\(\S\+ \)*\S\+\.h\>"				contains=netrwTreeBar
 syn match netrwLib		"\(\S\+ \)*\S*\.\%(a\|so\|lib\|dll\)\>"		contains=netrwTreeBar
 syn match netrwMakeFile	"\<[mM]akefile\>\|\(\S\+ \)*\S\+\.mak\>"	contains=netrwTreeBar
 syn match netrwObj		"\(\S\+ \)*\S*\.\%(o\|obj\)\>"			contains=netrwTreeBar
 syn match netrwTags    	"\<tags\>"					contains=netrwTreeBar
 syn match netrwTags		"\<\(ANmenu\|ANtags\)\>"			contains=netrwTreeBar
 syn match netrwTilde		"\(\S\+ \)*\S\+\~\>"				contains=netrwTreeBar
 syn match netrwTmp		"\<tmp\(\S\+ \)*\S\+\>\|\(\S\+ \)*\S*tmp\>"	contains=netrwTreeBar
endif

" ---------------------------------------------------------------------
" Highlighting Links: {{{1
if !exists("did_drchip_netrwlist_syntax")
 let did_drchip_netrwlist_syntax= 1
 hi link netrwClassify	Function
 hi link netrwCmdSep	Delimiter
 hi link netrwComment	Comment
 hi link netrwDir	Directory
 hi link netrwHelpCmd	Function
 hi link netrwHidePat	Statement
 hi link netrwList	Statement
 hi link netrwVersion	Identifier
 hi link netrwSymLink	Question
 hi link netrwExe	PreProc
 hi link netrwDateSep	Delimiter

 hi link netrwTreeBar	Special
 hi link netrwTimeSep	netrwDateSep
 hi link netrwComma	netrwComment
 hi link netrwHide	netrwComment
 hi link netrwMarkFile	Identifier

 " special syntax highlighting (see :he g:netrw_special_syntax)
 hi link netrwBak	NonText
 hi link netrwCompress	Folded
 hi link netrwData	DiffChange
 hi link netrwLib	DiffChange
 hi link netrwMakefile	DiffChange
 hi link netrwObj	Folded
 hi link netrwTilde	Folded
 hi link netrwTmp	Folded
 hi link netrwTags	Folded
endif

" Current Syntax: {{{1
let   b:current_syntax = "netrwlist"
" ---------------------------------------------------------------------
" vim: ts=8 fdm=marker
