" Vim syntax file
" Language:         /var/log/messages file
" Maintainer:       Yakov Lerner <iler.ml@gmail.com>
" Latest Revision:  2006-06-19

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn match   messagesBegin       display '^' nextgroup=messagesDate

syn match   messagesDate        contained display '\a\a\a [ 0-9]\d *'
                                \ nextgroup=messagesHour

syn match   messagesHour        contained display '\d\d:\d\d:\d\d\s*'
                                \ nextgroup=messagesHost

syn match   messagesHost        contained display '\S*\s*'
                                \ nextgroup=messagesLabel

syn match   messagesLabel       contained display '\s*[^:]*:\s*'
                                \ nextgroup=messagesText contains=messagesKernel,messagesPID

syn match   messagesPID         contained display '\[\zs\d\+\ze\]'

syn match   messagesKernel      contained display 'kernel:'


syn match   messagesIP          '\d\+\.\d\+\.\d\+\.\d\+'

syn match   messagesURL         '\w\+://\S\+'

syn match   messagesText        contained display '.*'
                                \ contains=messagesNumber,messagesIP,messagesURL,messagesError

syn match   messagesNumber      contained '0x[0-9a-fA-F]*\|\[<[0-9a-f]\+>\]\|\<\d[0-9a-fA-F]*'

syn match   messagesError       contained '\c.*\<\(FATAL\|ERROR\|ERRORS\|FAILED\|FAILURE\).*'


hi def link messagesDate        Constant
hi def link messagesHour        Type
hi def link messagesHost        Identifier
hi def link messagesLabel       Operator
hi def link messagesPID         Constant
hi def link messagesKernel      Special
hi def link messagesError       ErrorMsg
hi def link messagesIP          Constant
hi def link messagesURL         Underlined
hi def link messagesText        Normal
hi def link messagesNumber      Number

let b:current_syntax = "messages"

let &cpo = s:cpo_save
unlet s:cpo_save
