" Vim syntax file
" Language: Android Hardware Interface Description Language (HIDL)
" Maintainer: Chris McClellan <chris.mcclellan203@gmail.com>

if exists("b:current_syntax")
  finish
endif

source $VIMRUNTIME/syntax/c.vim
unlet b:current_syntax

"hidl specific keywords
syn keyword hidlKeywords interface package extends import generates

" Some hidl specific generic types of the form name<T>
syn keyword hidlType vec bitfield fmq_sync fmq_unsync 

hi def link hidlKeywords Keyword
hi def link hidlType Type

let b:current_syntax = "hal"
