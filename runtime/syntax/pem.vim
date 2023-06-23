
" Vim syntax file
" Language:	PEM
" Maintainer:	ObserverOfTime <chronobserver@disroot.org>
" Filenames:	*.pem,*.cer,*.crt,*.csr
" Last Change:	2023 Jun 24

if exists('b:current_syntax')
    finish
endif

let s:cpo_save = &cpoptions
set cpoptions&vim

syn region pemHeader matchgroup=pemDashes
            \ start=/^-----\zeBEGIN/ end=/-----$/ contains=pemBegin
syn region pemFooter matchgroup=pemDashes
            \ start=/^-----\zeEND/ end=/-----$/ contains=pemEnd

syn keyword pemBegin contained BEGIN nextgroup=pemLabel skipwhite
syn keyword pemEnd contained END nextgroup=pemLabel skipwhite

syn match pemLabel contained /[^-]\+/

hi def link pemBegin Keyword
hi def link pemEnd Keyword
hi def link pemLabel Label
hi def link pemDashes Delimiter

let b:current_syntax = 'pem'

let &cpoptions = s:cpo_save
unlet s:cpo_save
