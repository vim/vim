" Vim syntax file
" Language:	R (GNU S)
" Maintainer:	Vaidotas Zemlys <zemlys@gmail.com>
" Last Change:  2006 Apr 30
" Filenames:	*.R *.Rout *.r *.Rhistory *.Rt *.Rout.save *.Rout.fail
" URL:		http://uosis.mif.vu.lt/~zemlys/vim-syntax/r.vim

" First maintainer Tom Payne <tom@tompayne.org>
" Modified to make syntax less colourful and added the highlighting of
" R assignment arrow

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if version >= 600
  setlocal iskeyword=@,48-57,_,.
else
  set iskeyword=@,48-57,_,.
endif

syn case match

" Comment
syn match rComment /\#.*/

" Constant
" string enclosed in double quotes
syn region rString start=/"/ skip=/\\\\\|\\"/ end=/"/
" string enclosed in single quotes
syn region rString start=/'/ skip=/\\\\\|\\'/ end=/'/
" number with no fractional part or exponent
syn match rNumber /\d\+/
" floating point number with integer and fractional parts and optional exponent
syn match rFloat /\d\+\.\d*\([Ee][-+]\=\d\+\)\=/
" floating point number with no integer part and optional exponent
syn match rFloat /\.\d\+\([Ee][-+]\=\d\+\)\=/
" floating point number with no fractional part and optional exponent
syn match rFloat /\d\+[Ee][-+]\=\d\+/

" Identifier
" identifier with leading letter and optional following keyword characters
syn match rIdentifier /\a\k*/
" identifier with leading period, one or more digits, and at least one non-digit keyword character
syn match rIdentifier /\.\d*\K\k*/

" Statement
syn keyword rStatement   break next return
syn keyword rConditional if else
syn keyword rRepeat      for in repeat while

" Constant
syn keyword rConstant LETTERS letters month.ab month.name pi
syn keyword rConstant NULL
syn keyword rBoolean  FALSE TRUE
syn keyword rNumber   NA
syn match rArrow /<\{1,2}-/

" Type
syn keyword rType array category character complex double function integer list logical matrix numeric vector data.frame 

" Special
syn match rDelimiter /[,;:]/

" Error
syn region rRegion matchgroup=Delimiter start=/(/ matchgroup=Delimiter end=/)/ transparent contains=ALLBUT,rError,rBraceError,rCurlyError
syn region rRegion matchgroup=Delimiter start=/{/ matchgroup=Delimiter end=/}/ transparent contains=ALLBUT,rError,rBraceError,rParenError
syn region rRegion matchgroup=Delimiter start=/\[/ matchgroup=Delimiter end=/]/ transparent contains=ALLBUT,rError,rCurlyError,rParenError
syn match rError      /[)\]}]/
syn match rBraceError /[)}]/ contained
syn match rCurlyError /[)\]]/ contained
syn match rParenError /[\]}]/ contained

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_r_syn_inits")
  if version < 508
    let did_r_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink rComment     Comment
  HiLink rConstant    Constant
  HiLink rString      String
  HiLink rNumber      Number
  HiLink rBoolean     Boolean
  HiLink rFloat       Float
  HiLink rStatement   Statement
  HiLink rConditional Conditional
  HiLink rRepeat      Repeat
  HiLink rIdentifier  Normal
  HiLink rArrow	      Statement	
  HiLink rType        Type
  HiLink rDelimiter   Delimiter
  HiLink rError       Error
  HiLink rBraceError  Error
  HiLink rCurlyError  Error
  HiLink rParenError  Error
  delcommand HiLink
endif

let b:current_syntax="r"

" vim: ts=8 sw=2

