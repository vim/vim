" Vim syntax file
" Language:     SNOBOL4
" Maintainer:   Rafal Sulejman <rms@poczta.onet.pl>
" Last change:  2004 May 16

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syntax case ignore
" Vanilla Snobol4 keywords
syn keyword	snobol4Keywoard   any apply arb arbno arg array
syn keyword	snobol4Keywoard   break
syn keyword	snobol4Keywoard   char clear code collect convert copy
syn keyword	snobol4Keywoard   data datatype date define detach differ dump dupl
syn keyword	snobol4Keywoard   endfile eq eval
syn keyword	snobol4Keywoard   field
syn keyword	snobol4Keywoard   ge gt ident
syn keyword	snobol4Keywoard   input integer item
syn keyword	snobol4Keywoard   le len lgt local lpad lt
syn keyword	snobol4Keywoard   ne notany
syn keyword	snobol4Keywoard   opsyn output
syn keyword	snobol4Keywoard   pos prototype
syn keyword	snobol4Keywoard   remdr replace rpad rpos rtab
syn keyword	snobol4Keywoard   size span stoptr
syn keyword	snobol4Keywoard   tab table time trace trim
syn keyword	snobol4Keywoard   unload
syn keyword	snobol4Keywoard   value
" Spitbol keywords
" CSNOBOL keywords
syn keyword	snobol4Keywoard   sset

syn region      snobol4String       matchgroup=Quote start=+"+ skip=+\\"+ end=+"+
syn region      snobol4String       matchgroup=Quote start=+'+ skip=+\\'+ end=+'+
syn match       snobol4Label        "^[^- \t][^ \t]*"
syn match       snobol4Statement    "^-[^ ][^ ]*"
syn match       snobol4Comment      "^*.*$"
syn match       Constant            "\.[a-z][a-z0-9\-]*"
"syn match       snobol4Label        ":\([sf]*([^)]*)\)*" contains=ALLBUT,snobol4ParenError
syn region       snobol4Label        start=":(" end=")" contains=ALLBUT,snobol4ParenError
syn region       snobol4Label        start=":f(" end=")" contains=ALLBUT,snobol4ParenError
syn region       snobol4Label        start=":s(" end=")" contains=ALLBUT,snobol4ParenError
syn match       snobol4Number       "\<\d*\(\.\d\d*\)*\>"
" Parens matching
syn cluster     snobol4ParenGroup   contains=snobol4ParenError
syn region      snobol4Paren        transparent start='(' end=')' contains=ALLBUT,@snobol4ParenGroup,snobol4ErrInBracket
syn match       snobol4ParenError   display "[\])]"
syn match       snobol4ErrInParen   display contained "[\]{}]\|<%\|%>"
syn region      snobol4Bracket      transparent start='\[\|<:' end=']\|:>' contains=ALLBUT,@snobol4ParenGroup,snobol4ErrInParen
syn match       snobol4ErrInBracket display contained "[);{}]\|<%\|%>"

" optional shell shebang line
syn match	snobol4Comment    "^\#\!.*$"


" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_snobol4_syntax_inits")
  if version < 508
    let did_snobol4_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink snobol4Label		Label
  HiLink snobol4Conditional	Conditional
  HiLink snobol4Repeat		Repeat
  HiLink snobol4Number		Number
  HiLink snobol4Error		Error
  HiLink snobol4Statement	PreProc
  HiLink snobol4String		String
  HiLink snobol4Comment		Comment
  HiLink snobol4Special		Special
  HiLink snobol4Todo		Todo
  HiLink snobol4Keyword		Statement
  HiLink snobol4Function	Statement
  HiLink snobol4Keyword		Keyword
  HiLink snobol4MathsOperator	Operator
  HiLink snobol4ParenError      snobol4Error
  HiLink snobol4ErrInParen      snobol4Error
  HiLink snobol4ErrInBracket    snobol4Error

  delcommand HiLink
endif

let b:current_syntax = "snobol4"
" vim: ts=8
