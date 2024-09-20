" Vim syntax file
" Language:	Miranda
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Sep 20

if exists("b:current_syntax")
  finish
endif

syn iskeyword a-z,A-Z,48-57,_,'

if miranda#GetFileTypeInfo().literate
  syn include @mirandaTop syntax/shared/miranda.vim
  syn region  mirandaLiterate start="^[^>[:space:]]" end="^\ze\%(\s*\n\)*>\|\%$"
  syn region  mirandaProgram  start="^>" skip="^>" end="^\ze\s*$\|\%$" contains=@mirandaTop

  syn sync linebreaks=2

  hi def link mirandaLiterate Comment
else
  runtime! syntax/shared/miranda.vim
endif

syn match mirandaSharpBang /\%^#!.*/ display

hi def link mirandaSharpBang PreProc

let b:current_syntax = "miranda"

" vim: nowrap sw=2 sts=2 ts=8 noet fdm=marker:
