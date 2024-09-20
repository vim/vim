" Vim syntax file
" Language:	Miranda
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Sep 20

syn iskeyword a-z,A-Z,48-57,_,'

" Operators {{{1
" list
syn match   mirandaOperator /\%([:#!]\|++\|--\)/
" relational
syn match   mirandaOperator _\%(\\/\|&\|\~\)\|\%(>\|>=\|=\|\~=\|<=\|<\)_
" arithmetic
syn match   mirandaOperator _[+\-*/^]_
syn keyword mirandaOperator div mod
" function
syn match   mirandaOperator /\.\|$\a\@=/

" Reserved {{{1
syn keyword mirandaReserved
      \ abstype if otherwise readvals show type where with
     "\ div mod

" Functions {{{1
syn keyword mirandaFunction
      \ abs and arctan cjustify code concat const converse cos decode digit
      \ drop dropwhile e entier error exp filemode filter foldl foldl1 foldr
      \ foldr1 force fst getenv hd hugenum id index init integer iterate last
      \ lay layn letter limit lines ljustify log log10 map map2 max max2
      \ member merge min min2 mkset neg numval or pi postfix product read
      \ readb rep repeat reverse rjustify scan seq showfloat showhex shownum
      \ showoct showscaled sin snd sort spaces sqrt subtract sum system take
      \ takewhile tinynum tl transpose undef until zip2 zip3 zip4 zip5 zip6
      \ zip

" Constants {{{1
syn keyword mirandaConstant e pi tinynum
syn keyword mirandaUndef    undef

" Constructors {{{1
syn keyword mirandaFunction
      \ Appendfile Appendfileb Closefile Exit Stderr Stdout Stdoutb System
      \ Tofile Tofileb

syn keyword mirandaType	   bool char num sys_message
" syn match   mirandaTypeVar /\*\+/

" Literals {{{1
syn keyword mirandaBoolean True False

" Numbers {{{2
syn match   mirandaNatural /\<\d\+\>/			   display
syn match   mirandaFloat   /\<\d\+e[+-]\=\d\+\>/	   display
syn match   mirandaFloat   /\<\d*\.\d\+\%(e[+-]\=\d\+\)\=/ display

" Strings {{{2
" syn match mirandaEscape /\\[ntfrb\\'"]/ contained
syn match   mirandaEscape /\\./        contained
syn match   mirandaEscape /\\\d\{1,3}/ contained
syn match   mirandaEscape /\\$/        contained

syn match   mirandaCharacter /\<'\%(\\.\|\%(\\$\n\)\=[[:print:]]\)'\>/	     contains=mirandaEscape
syn region  mirandaString    start=/"/ skip=/\\\\\|\\"\|\\$/ end=/"/ end=/$/ contains=mirandaEscape,mirandaEscapeError

" Comments {{{1
syn keyword mirandaTodo contained TODO FIXME XXX NOTE
syn match   mirandaComment /||.*/ contains=mirandaTodo,@Spell

" Compiler directives {{{1
syn match   mirandaCompilerDirective /%\%(list\|nolist\)\>/
syn match   mirandaCompilerDirective /%\%(include\|insert\)\>/ nextgroup=mirandaFileId skipwhite
syn region  mirandaFileId start=/</ end=/>/ contained oneline
syn region  mirandaFileId start=/"/ end=/"/ contained oneline
syn match   mirandaCompilerDirective /%\%(export\|free\)\>/

" Highlighting {{{1
hi def link mirandaBoolean Boolean
hi def link mirandaCharacter Character
hi def link mirandaComment Comment
hi def link mirandaCompilerDirective PreProc
hi def link mirandaConstant Constant
hi def link mirandaEscape Special
hi def link mirandaFileId mirandaString
hi def link mirandaFloat Float
hi def link mirandaFunction Function
hi def link mirandaNatural Number
hi def link mirandaOperator Operator
hi def link mirandaReserved Keyword
hi def link mirandaString String
hi def link mirandaType Type

" vim: nowrap sw=2 sts=2 ts=8 noet fdm=marker:
