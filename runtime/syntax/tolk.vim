" Vim syntax file
" Language:     Tolk
" Maintainer:   redavy <hello.redavy@proton.me>
" Upstream:     https://github.com/redavy/vim-tolk
" Last Update:  24 May 2026

if exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword tolkKeyword  do if as fun asm get try var val lazy
syn keyword tolkKeyword  else enum true tolk const false throw
syn keyword tolkKeyword  redef while catch return assert import
syn keyword tolkKeyword  global repeat mutate contract

" Operators
syn match tolkOperator  "="
syn match tolkOperator  "+="
syn match tolkOperator  "-="
syn match tolkOperator  "*="
syn match tolkOperator  "/="
syn match tolkOperator  "%="
syn match tolkOperator  "<<="
syn match tolkOperator  ">>="
syn match tolkOperator  "&="
syn match tolkOperator  "|="
syn match tolkOperator  "^="

syn match tolkOperator  "=="
syn match tolkOperator  "<"
syn match tolkOperator  ">"
syn match tolkOperator  "<="
syn match tolkOperator  ">="
syn match tolkOperator  "!="
syn match tolkOperator  "<=>"
syn match tolkOperator  "<<"
syn match tolkOperator  ">>"
syn match tolkOperator  "\~>>"
syn match tolkOperator  "^>>"
syn match tolkOperator  "-"
syn match tolkOperator  "+"
syn match tolkOperator  "|"
syn match tolkOperator  "^"
syn match tolkOperator  "*"
syn match tolkOperator  "/"
syn match tolkOperator  "%"
syn match tolkOperator  "\~/"
syn match tolkOperator  "^/"
syn match tolkOperator  "&"
syn match tolkOperator  "\~"
syn match tolkOperator  "."
syn match tolkOperator  "!"
syn match tolkOperator  "&&"
syn match tolkOperator  "||"

syn match tolkOperator  "->"

" Strings
syn region tolkString  start=+"+ end=+"+
syn region tolkString  start=+'+ end=+'+

" Numbers
syn match tolkNumber  "\<[0-9]\+\>"
syn match tolkNumber  "\<0[xX][0-9a-fA-F]\+\>"
syn match tolkNumber  "\<[0-9]\+\.[0-9]\+\>"

" Comments
syn match  tolkComment  "//.*$"
syn region tolkComment  start="/\*" end="\*/"

" Types
syn match tolkType  "\<[A-Z][a-zA-Z0-9_]*\>"

" Functions
syn match tolkFunction  "\<[a-z_][a-zA-Z0-9_]*\s*("me=e-1

" Attributes/Annotations
syn match tolkAttribute  "@[a-zA-Z_][a-zA-Z0-9_]*"

" Highlights
highlight link tolkKeyword    Keyword
highlight link tolkOperator   Operator
highlight link tolkString     String
highlight link tolkNumber     Number
highlight link tolkComment    Comment
highlight link tolkType       Type
highlight link tolkFunction   Function
highlight link tolkAttribute  PreProc

let b:current_syntax = "tolk"
