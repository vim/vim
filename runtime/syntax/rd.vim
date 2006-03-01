" Vim syntax file
" Language:    R Help File
" Maintainer:  Johannes Ranke <jranke@uni-bremen.de>
" Last Change: 2006 Mär 01
" Version:     0.5
" Remarks:     - Now includes R syntax highlighting in the appropriate
"                sections if an r.vim file is in the same directory or in the
"                default debian location.
"              - I didn't yet include any special markup for S4 methods.
"              - The two versions of \item{}{} markup are not 
"                distinguished (in the \arguments{} environment, the items to
"                be described are R identifiers, but not in the \describe{}
"                environment).
"              - There is no Latex markup in equations

" Version Clears: {{{1
" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600 
  syntax clear
elseif exists("b:current_syntax")
  finish
endif 

syn case match

" Rd identifiers {{{
syn region rdIdentifier matchgroup=rdSection	start="\\name{" end="}" 
syn region rdIdentifier matchgroup=rdSection	start="\\alias{" end="}" 
syn region rdIdentifier matchgroup=rdSection	start="\\pkg{" end="}" 
syn region rdIdentifier matchgroup=rdSection	start="\\item{" end="}" contained
syn region rdIdentifier matchgroup=rdSection start="\\method{" end=/}/ contained

" Highlighting of R code using an existing r.vim syntax file if available {{{1
let s:syntaxdir = expand("<sfile>:p:h") "look in the directory of this file
let s:rsyntax = s:syntaxdir . "/r.vim"
if filereadable(s:rsyntax)  
  syn include @R <sfile>:p:h/r.vim
elseif filereadable('/usr/share/vim/vim64/syntax/r.vim')  "and debian location
  syn include @R /usr/share/vim/vimcurrent/syntax/r.vim
else 
  syn match rdRComment /\#.*/				"if no r.vim is found, do comments
  syn cluster R contains=rdRComment 
endif
syn region rdRcode matchgroup=Delimiter start="\\examples{" matchgroup=Delimiter transparent end=/}/ contains=@R,rdSection
syn region rdRcode matchgroup=Delimiter start="\\usage{" matchgroup=Delimiter transparent end=/}/ contains=@R,rdIdentifier
syn region rdRcode matchgroup=Delimiter start="\\synopsis{" matchgroup=Delimiter transparent end=/}/ contains=@R
syn region rdRcode matchgroup=Delimiter start="\\special{" matchgroup=Delimiter transparent end=/}/ contains=@R contained
syn region rdRcode matchgroup=Delimiter start="\\code{" matchgroup=Delimiter transparent end=/}/ contains=@R contained

" Strings {{{1
syn region rdString start=/"/ end=/"/ 

" Special TeX characters  ( \$ \& \% \# \{ \} \_) {{{1
syn match rdSpecialChar        "\\[$&%#{}_]"

" Special Delimiters {{{1
syn match rdDelimiter		"\\cr"
syn match rdDelimiter		"\\tab "

" Keywords {{{1
syn match rdKeyword	"\\R"
syn match rdKeyword	"\\dots"
syn match rdKeyword	"\\ldots"

" Links {{{1
syn region rdLink matchgroup=rdSection start="\\link{" end="}" contained keepend
syn region rdLink matchgroup=rdSection start="\\link\[.*\]{" end="}" contained keepend

" Type Styles {{{1
syn match rdType		"\\emph\>"
syn match rdType		"\\strong\>"
syn match rdType		"\\bold\>"
syn match rdType		"\\sQuote\>"
syn match rdType		"\\dQuote\>"
syn match rdType		"\\code\>"
syn match rdType		"\\preformatted\>"
syn match rdType		"\\kbd\>"
syn match rdType		"\\samp\>"
syn match rdType		"\\eqn\>"
syn match rdType		"\\deqn\>"
syn match rdType		"\\file\>"
syn match rdType		"\\email\>"
syn match rdType		"\\url\>"
syn match rdType		"\\var\>"
syn match rdType		"\\env\>"
syn match rdType		"\\option\>"
syn match rdType		"\\command\>"
syn match rdType		"\\dfn\>"
syn match rdType		"\\cite\>"
syn match rdType		"\\acronym\>"

" Rd sections {{{1
syn match rdSection		"\\encoding\>"
syn match rdSection		"\\title\>"
syn match rdSection		"\\description\>"
syn match rdSection		"\\concept\>"
syn match rdSection		"\\arguments\>"
syn match rdSection		"\\details\>"
syn match rdSection		"\\value\>"
syn match rdSection		"\\references\>"
syn match rdSection		"\\note\>"
syn match rdSection		"\\author\>"
syn match rdSection		"\\seealso\>"
syn match rdSection		"\\keyword\>"
syn match rdSection		"\\docType\>"
syn match rdSection		"\\format\>"
syn match rdSection		"\\source\>"
syn match rdSection     "\\itemize\>"
syn match rdSection     "\\describe\>"
syn match rdSection     "\\enumerate\>"
syn match rdSection     "\\item "
syn match rdSection     "\\item$"
syn match rdSection		"\\tabular{[lcr]*}"
syn match rdSection		"\\dontrun\>"
syn match rdSection		"\\dontshow\>"
syn match rdSection		"\\testonly\>"

" Freely named Sections {{{1
syn region rdFreesec matchgroup=Delimiter start="\\section{" matchgroup=Delimiter transparent end=/}/ 

" Rd comments {{{1
syn match rdComment /%.*$/ contained 

" Error {{{1
syn region rdRegion matchgroup=Delimiter start=/(/ matchgroup=Delimiter end=/)/ transparent contains=ALLBUT,rdError,rdBraceError,rdCurlyError
syn region rdRegion matchgroup=Delimiter start=/{/ matchgroup=Delimiter end=/}/ transparent contains=ALLBUT,rdError,rdBraceError,rdParenError
syn region rdRegion matchgroup=Delimiter start=/\[/ matchgroup=Delimiter end=/]/ transparent contains=ALLBUT,rdError,rdCurlyError,rdParenError
syn match rdError      /[)\]}]/
syn match rdBraceError /[)}]/ contained
syn match rdCurlyError /[)\]]/ contained
syn match rdParenError /[\]}]/ contained

" Define the default highlighting {{{1
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_rd_syntax_inits")
  if version < 508
    let did_rd_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink rdIdentifier  Identifier
  HiLink rdString      String
  HiLink rdKeyword     Keyword
  HiLink rdLink        Underlined
  HiLink rdType	       Type
  HiLink rdSection     PreCondit
  HiLink rdError       Error
  HiLink rdBraceError  Error
  HiLink rdCurlyError  Error
  HiLink rdParenError  Error
  HiLink rdDelimiter   Delimiter
  HiLink rdComment     Comment
  HiLink rdRComment    Comment
  HiLink rdSpecialChar SpecialChar
  delcommand HiLink
endif 

let   b:current_syntax = "rd"
" vim: foldmethod=marker:
