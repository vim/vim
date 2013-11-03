" Vim syntax file
" Language:	J
" Maintainer:	David Bürgin <676c7473@gmail.com>
" Last Change:	2013-10-06

if exists("b:current_syntax")
  finish
endif

syntax case match
syntax sync minlines=50

syn match jControl /\<\%(assert\|break\|case\|catch[dt]\=\|continue\|do\|else\%(if\)\=\|end\|fcase\|for\|if\|return\|select\|throw\|try\|whil\%(e\|st\)\)\./
syn match jControl /\<\%(for\|goto\|label\)_\a\k*\./

syn region jString oneline start=/'/ skip=/''/ end=/'/

" Patterns for numbers in general, rational numbers, numbers with explicit
" base, infinities, and numbers with extended precision.
"
" Matching J numbers is difficult. The regular expression used for the general
" case roughly embodies this grammar sketch:
"
"         EXP     := /_?\d+(\.\d*)?([eE]_?\d+)?/
"         COMP    := EXP  |  EXP (j|a[dr]) EXP
"         PIEU    := COMP  |  COMP [px] COMP
"
" For the rest, a compromise between correctness and practicality was made.
" See http://www.jsoftware.com/help/dictionary/dcons.htm for reference.
syn match jNumber /\<_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%(\%(j\|a[dr]\)_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\)\=\%([px]_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%(\%(j\|a[dr]\)_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\)\=\)\=/
syn match jNumber /\<_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=r_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=/
syn match jNumber /\<_\=\d\+\%([eE]\d\+\)\=b_\=[0-9a-z]\+/
syn match jNumber /\<__\=\>/
syn match jNumber /\<_\=\d\+x\>/

syn match jComment /NB\..*$/ contains=jTodo,@Spell
syn keyword jTodo TODO FIXME XXX contained

hi def link jControl Statement
hi def link jString String
hi def link jNumber Number
hi def link jComment Comment
hi def link jTodo Todo

let b:current_syntax = "j"
