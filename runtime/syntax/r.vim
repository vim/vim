" Vim syntax file
" Language:	      R (GNU S)
" Maintainer:	      Jakson Aquino <jalvesaq@gmail.com>
" Former Maintainers: Vaidotas Zemlys <zemlys@gmail.com>
" 		      Tom Payne <tom@tompayne.org>
" Last Change:	      Wed Sep 29, 2010  09:31AM
" Filenames:	      *.R *.r *.Rhistory *.Rt
" 
" NOTE: The highlighting of R functions is defined in the
" r-plugin/functions.vim, which is part of vim-r-plugin2:
" http://www.vim.org/scripts/script.php?script_id=2628
"
" Some lines of code were borrowed from Zhuojun Chen.

if exists("b:current_syntax")
  finish
endif

setlocal iskeyword=@,48-57,_,.

syn case match

" Comment
syn match rComment contains=@Spell "\#.*"

" string enclosed in double quotes
syn region rString contains=rSpecial,rStrError,@Spell start=/"/ skip=/\\\\\|\\"/ end=/"/
" string enclosed in single quotes
syn region rString contains=rSpecial,rStrError,@Spell start=/'/ skip=/\\\\\|\\'/ end=/'/

syn match rStrError display contained "\\."

" New line, carriage return, tab, backspace, bell, feed, vertical tab, backslash
syn match rSpecial display contained "\\\(n\|r\|t\|b\|a\|f\|v\|'\|\"\)\|\\\\"

" Hexadecimal and Octal digits
syn match rSpecial display contained "\\\(x\x\{1,2}\|[0-8]\{1,3}\)"

" Unicode characters
syn match rSpecial display contained "\\u\x\{1,4}"
syn match rSpecial display contained "\\U\x\{1,8}"
syn match rSpecial display contained "\\u{\x\{1,4}}"
syn match rSpecial display contained "\\U{\x\{1,8}}"


syn match rDollar "\$"

" Statement
syn keyword rStatement   break next return
syn keyword rConditional if else
syn keyword rRepeat      for in repeat while

" Constant (not really)
syn keyword rConstant T F LETTERS letters month.ab month.name pi
syn keyword rConstant R.version.string

" Constant
syn keyword rConstant NULL
syn keyword rBoolean  FALSE TRUE
syn keyword rNumber   NA NA_integer_ NA_real_ NA_complex_ NA_character_ 
syn keyword rNumber   Inf NaN 

" integer
syn match rInteger "\<\d\+L"
syn match rInteger "\<0x\([0-9]\|[a-f]\|[A-F]\)\+L"
syn match rInteger "\<\d\+[Ee]+\=\d\+L"

syn match rOperator    "[\*\!\&\+\-\<\>\=\^\|\~\`/:@]"
syn match rOperator    "%\{2}\|%\*%\|%\/%\|%in%\|%o%\|%x%"

syn match rComplex "\<\d\+i"
syn match rComplex "\<0x\([0-9]\|[a-f]\|[A-F]\)\+i"
syn match rComplex "\<\d\+\.\d*\([Ee][-+]\=\d\+\)\=i"
syn match rComplex "\<\.\d\+\([Ee][-+]\=\d\+\)\=i"
syn match rComplex "\<\d\+[Ee][-+]\=\d\+i"

" number with no fractional part or exponent
syn match rNumber "\<\d\+\>"
" hexadecimal number 
syn match rNumber "\<0x\([0-9]\|[a-f]\|[A-F]\)\+"

" floating point number with integer and fractional parts and optional exponent
syn match rFloat "\<\d\+\.\d*\([Ee][-+]\=\d\+\)\="
" floating point number with no integer part and optional exponent
syn match rFloat "\<\.\d\+\([Ee][-+]\=\d\+\)\="
" floating point number with no fractional part and optional exponent
syn match rFloat "\<\d\+[Ee][-+]\=\d\+"

syn match rArrow "<\{1,2}-"
syn match rArrow "->\{1,2}"

" Special
syn match rDelimiter "[,;:]"

" Error
syn region rRegion matchgroup=Delimiter start=/(/ matchgroup=Delimiter end=/)/ transparent contains=ALLBUT,rError,rBraceError,rCurlyError
syn region rRegion matchgroup=Delimiter start=/{/ matchgroup=Delimiter end=/}/ transparent contains=ALLBUT,rError,rBraceError,rParenError
syn region rRegion matchgroup=Delimiter start=/\[/ matchgroup=Delimiter end=/]/ transparent contains=ALLBUT,rError,rCurlyError,rParenError
syn match rError      "[)\]}]"
syn match rBraceError "[)}]" contained
syn match rCurlyError "[)\]]" contained
syn match rParenError "[\]}]" contained

" Functions that may add new objects
syn keyword rPreProc     library require attach detach source

" Type
syn keyword rType array category character complex double function integer list logical matrix numeric vector data.frame 

" Define the default highlighting.
hi def link rArrow       Statement	
hi def link rBoolean     Boolean
hi def link rBraceError  Error
hi def link rComment     Comment
hi def link rComplex     Number
hi def link rConditional Conditional
hi def link rConstant    Constant
hi def link rCurlyError  Error
hi def link rDelimiter   Delimiter
hi def link rDollar      SpecialChar
hi def link rError       Error
hi def link rFloat       Float
hi def link rInteger     Number
hi def link rNumber      Number
hi def link rOperator    Operator
hi def link rParenError  Error
hi def link rPreProc     PreProc
hi def link rRepeat      Repeat
hi def link rSpecial     SpecialChar
hi def link rStatement   Statement
hi def link rString      String
hi def link rStrError    Error
hi def link rType        Type

let b:current_syntax="r"

" vim: ts=8 sw=2
