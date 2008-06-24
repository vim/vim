" Vim syntax file
" Language:		ProMeLa
" Maintainer:		Maurizio Tranchero <maurizio.tranchero@polito.it> - <maurizio.tranchero@gmail.com>
" First Release:	Mon Oct 16 08:49:46 CEST 2006
" Last Change:		Sat May 16 12:20:43 CEST 2007
" Version:		0.2

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" case is significant
" syn case ignore
" ProMeLa Keywords
syn keyword promelaStatement	proctype if else while chan do od fi break goto unless
syn keyword promelaStatement	active assert label atomic
syn keyword promelaFunctions	skip timeout run
" check what it is the following
" ProMeLa Types
syn keyword promelaType			bit bool byte short int
" ProMeLa Regions
syn region promelaComment		start="\/\/" end="$" keepend
syn region promelaString		start="\"" end="\""
" syn region promelaComment		start="//" end="$" contains=ALL
" syn region promelaComment		start="/\*" end="\*/" contains=ALL
" ProMeLa Comment
syn match promelaComment	"\/.*$"
syn match promelaComment	"/\*.*\*/"
" Operators and special characters
syn match promelaOperator	"!"
syn match promelaOperator	"?"
syn match promelaOperator	"->"
syn match promelaOperator	"="
syn match promelaOperator	"+"
syn match promelaOperator	"*"
syn match promelaOperator	"/"
syn match promelaOperator	"-"
syn match promelaOperator	"<"
syn match promelaOperator	">"
syn match promelaOperator	"<="
syn match promelaOperator	">="
syn match promelaSpecial	"\["
syn match promelaSpecial	"\]"
syn match promelaSpecial	";"
syn match promelaSpecial	"::"

" Class Linking
hi def link promelaStatement		Statement
hi def link promelaType			Type
hi def link promelaComment		Comment
hi def link promelaOperator		Type
hi def link promelaSpecial		Special
hi def link promelaFunctions		Special
hi def link promelaString		String

let b:current_syntax = "promela"
