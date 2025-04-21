" Vim sytax file
" Author: koe <ukoe@protonmail.com>
" Maintainer: FabricSoul <fabric.soul7@gmail.com>
" Language:	cobweb
" Last Change:	2025 Apr 20
" For bugs, patches and license go to https://github.com/UkoeHB/vim-cob/tree/main

if exists("b:current_syntax")
  finish
endif

syn keyword basicLanguageKeywords auto none nan inf

syn region cobStruct start="(" end=")" transparent fold
syn region cobSeq start="\[" end="\]" transparent fold
syn region cobMap start="{" end="}" transparent fold

syn region cobString oneline start=/"/ skip=/\\\\\|\\"/ end=/"/

syn match cobSection /\<#\w\+\ze:/ display

syn match cobIdentifier /\<[A-Z]\w*\s*\ze(/ display

syn match cobKey /\<\w\+\ze:/ display

syn match cobInteger /\<[+-]\=[0-9]\(_\=\d\)*\>/ display

syn match cobFloat /\<[+-]\=[0-9]\(_\=\d\)*\.\d\+\>/ display
syn match cobFloat /\<[+-]\=[0-9]\(_\=\d\)*\(\.[0-9]\(_\=\d\)*\)\=[eE][+-]\=[0-9]\(_\=\d\)*\>/ display

syn match cobBoolean /\<\%(true\|false\)\>/ display

syn keyword cobTodo TODO FIXME XXX BUG contained

syn match cobComment /\/\/.*/ contains=cobTodo
syn region cobComment start="/\*" end="\*/" fold extend contains=cobTodo,cobCommentB

hi def link cobString String
hi def link cobInteger Number
hi def link cobFloat Float
hi def link cobBoolean Boolean
hi def link cobTodo Todo
hi def link cobComment Comment
hi def link cobSection PreProc
hi def link cobIdentifier Identifier
hi def link cobKey Keyword

let b:current_syntax = "cob"
