" Vim syntax file
" Language:    R Help File
" Maintainer:  Johannes Ranke <jranke@uni-bremen.de>
" Last Change: 2009 Mai 12
" Version:     0.7.2
" SVN:		   $Id: rhelp.vim 86 2009-05-12 19:23:47Z ranke $
" Remarks:     - Now includes R syntax highlighting in the appropriate
"                sections if an r.vim file is in the same directory or in the
"                default debian location.
"              - There is no Latex markup in equations
"              - Thanks to Will Gray for finding and fixing a bug

" Version Clears: {{{1
" For version 5.x: Clear all syntax items
" For version 6.x and 7.x: Quit when a syntax file was already loaded
if version < 600 
  syntax clear
elseif exists("b:current_syntax")
  finish
endif 

syn case match

" R help identifiers {{{
syn region rhelpIdentifier matchgroup=rhelpSection	start="\\name{" end="}" 
syn region rhelpIdentifier matchgroup=rhelpSection	start="\\alias{" end="}" 
syn region rhelpIdentifier matchgroup=rhelpSection	start="\\pkg{" end="}" 
syn region rhelpIdentifier matchgroup=rhelpSection	start="\\item{" end="}" contained contains=rhelpDots
syn region rhelpIdentifier matchgroup=rhelpSection start="\\method{" end=/}/ contained

" Highlighting of R code using an existing r.vim syntax file if available {{{1
syn include @R syntax/r.vim
syn match rhelpDots		"\\dots" containedin=@R
syn region rhelpRcode matchgroup=Delimiter start="\\examples{" matchgroup=Delimiter transparent end=/}/ contains=@R,rhelpSection
syn region rhelpRcode matchgroup=Delimiter start="\\usage{" matchgroup=Delimiter transparent end=/}/ contains=@R,rhelpIdentifier,rhelpS4method
syn region rhelpRcode matchgroup=Delimiter start="\\synopsis{" matchgroup=Delimiter transparent end=/}/ contains=@R
syn region rhelpRcode matchgroup=Delimiter start="\\special{" matchgroup=Delimiter transparent end=/}/ contains=@R contained
syn region rhelpRcode matchgroup=Delimiter start="\\code{" matchgroup=Delimiter transparent end=/}/ contains=@R,rhelpLink contained
syn region rhelpS4method matchgroup=Delimiter start="\\S4method{.*}(" matchgroup=Delimiter transparent end=/)/ contains=@R,rhelpDots contained

" Strings {{{1
syn region rhelpString start=/"/ end=/"/ 

" Special characters  ( \$ \& \% \# \{ \} \_) {{{1
syn match rhelpSpecialChar        "\\[$&%#{}_]"

" Special Delimiters {{{1
syn match rhelpDelimiter		"\\cr"
syn match rhelpDelimiter		"\\tab "

" Keywords {{{1
syn match rhelpKeyword	"\\R"
syn match rhelpKeyword	"\\ldots"
syn match rhelpKeyword  "--"
syn match rhelpKeyword  "---"
syn match rhelpKeyword  "<"
syn match rhelpKeyword  ">"

" Links {{{1
syn region rhelpLink matchgroup=rhelpSection start="\\link{" end="}" contained keepend
syn region rhelpLink matchgroup=rhelpSection start="\\link\[.\{-}\]{" end="}" contained keepend
syn region rhelpLink matchgroup=rhelpSection start="\\linkS4class{" end="}" contained keepend

" Type Styles {{{1
syn match rhelpType		"\\emph\>"
syn match rhelpType		"\\strong\>"
syn match rhelpType		"\\bold\>"
syn match rhelpType		"\\sQuote\>"
syn match rhelpType		"\\dQuote\>"
syn match rhelpType		"\\preformatted\>"
syn match rhelpType		"\\kbd\>"
syn match rhelpType		"\\samp\>"
syn match rhelpType		"\\eqn\>"
syn match rhelpType		"\\deqn\>"
syn match rhelpType		"\\file\>"
syn match rhelpType		"\\email\>"
syn match rhelpType		"\\url\>"
syn match rhelpType		"\\var\>"
syn match rhelpType		"\\env\>"
syn match rhelpType		"\\option\>"
syn match rhelpType		"\\command\>"
syn match rhelpType		"\\dfn\>"
syn match rhelpType		"\\cite\>"
syn match rhelpType		"\\acronym\>"

" rhelp sections {{{1
syn match rhelpSection		"\\encoding\>"
syn match rhelpSection		"\\title\>"
syn match rhelpSection		"\\description\>"
syn match rhelpSection		"\\concept\>"
syn match rhelpSection		"\\arguments\>"
syn match rhelpSection		"\\details\>"
syn match rhelpSection		"\\value\>"
syn match rhelpSection		"\\references\>"
syn match rhelpSection		"\\note\>"
syn match rhelpSection		"\\author\>"
syn match rhelpSection		"\\seealso\>"
syn match rhelpSection		"\\keyword\>"
syn match rhelpSection		"\\docType\>"
syn match rhelpSection		"\\format\>"
syn match rhelpSection		"\\source\>"
syn match rhelpSection     "\\itemize\>"
syn match rhelpSection     "\\describe\>"
syn match rhelpSection     "\\enumerate\>"
syn match rhelpSection     "\\item "
syn match rhelpSection     "\\item$"
syn match rhelpSection		"\\tabular{[lcr]*}"
syn match rhelpSection		"\\dontrun\>"
syn match rhelpSection		"\\dontshow\>"
syn match rhelpSection		"\\testonly\>"
syn match rhelpSection		"\\donttest\>"

" Freely named Sections {{{1
syn region rhelpFreesec matchgroup=Delimiter start="\\section{" matchgroup=Delimiter transparent end=/}/ 

" R help file comments {{{1
syn match rhelpComment /%.*$/ contained 

" Error {{{1
syn region rhelpRegion matchgroup=Delimiter start=/(/ matchgroup=Delimiter end=/)/ transparent contains=ALLBUT,rhelpError,rhelpBraceError,rhelpCurlyError
syn region rhelpRegion matchgroup=Delimiter start=/{/ matchgroup=Delimiter end=/}/ transparent contains=ALLBUT,rhelpError,rhelpBraceError,rhelpParenError
syn region rhelpRegion matchgroup=Delimiter start=/\[/ matchgroup=Delimiter end=/]/ transparent contains=ALLBUT,rhelpError,rhelpCurlyError,rhelpParenError
syn match rhelpError      /[)\]}]/
syn match rhelpBraceError /[)}]/ contained
syn match rhelpCurlyError /[)\]]/ contained
syn match rhelpParenError /[\]}]/ contained

" Define the default highlighting {{{1
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_rhelp_syntax_inits")
  if version < 508
    let did_rhelp_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink rhelpIdentifier  Identifier
  HiLink rhelpString      String
  HiLink rhelpKeyword     Keyword
  HiLink rhelpDots        Keyword
  HiLink rhelpLink        Underlined
  HiLink rhelpType	      Type
  HiLink rhelpSection     PreCondit
  HiLink rhelpError       Error
  HiLink rhelpBraceError  Error
  HiLink rhelpCurlyError  Error
  HiLink rhelpParenError  Error
  HiLink rhelpDelimiter   Delimiter
  HiLink rhelpComment     Comment
  HiLink rhelpRComment    Comment
  HiLink rhelpSpecialChar SpecialChar
  delcommand HiLink
endif 

let   b:current_syntax = "rhelp"
" vim: foldmethod=marker:
