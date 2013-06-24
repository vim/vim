" Vim syntax file
" Language:             Innovation Data Processing upstream.log file
" Maintainer:           Rob Owens <rowens@fdrinnovation.com>
" Latest Revision:      2013-06-17

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Date:
syn match upstreamlog_Date /\u\l\l \u\l\l\s\{1,2}\d\{1,2} \d\d:\d\d:\d\d \d\d\d\d/
" Msg Types:
syn match upstreamlog_MsgD /Msg #\(Agt\|PC\|Srv\)\d\{4,5}D/ nextgroup=upstreamlog_Process skipwhite
syn match upstreamlog_MsgE /Msg #\(Agt\|PC\|Srv\)\d\{4,5}E/ nextgroup=upstreamlog_Process skipwhite
syn match upstreamlog_MsgI /Msg #\(Agt\|PC\|Srv\)\d\{4,5}I/ nextgroup=upstreamlog_Process skipwhite
syn match upstreamlog_MsgW /Msg #\(Agt\|PC\|Srv\)\d\{4,5}W/ nextgroup=upstreamlog_Process skipwhite
" Processes:
syn region upstreamlog_Process start="(" end=")" contained
" IP Address:
syn match upstreamlog_IPaddr / \d\{1,3}\.\d\{1,3}\.\d\{1,3}\.\d\{1,3}/
" Profile:
syn region upstreamlog_Profile start="Profile name \zs" end="\"\S\{1,8}\""
syn region upstreamlog_Profile start=" Profile: \zs" end="\S\{1,8}"
syn region upstreamlog_Profile start="  Profile: \zs" end="\ze, "
syn region upstreamlog_Profile start="Backup Profile: \zs" end="\ze Version date"
syn region upstreamlog_Profile start="Full of \zs" end="\ze$"
syn region upstreamlog_Profile start="Incr. of \zs" end="\ze$"
" Target:
syn region upstreamlog_Target start="Computer: \zs" end="\ze[\]\)]" 
syn region upstreamlog_Target start="Computer name \zs" end="\ze," 

hi def link upstreamlog_Date	Underlined
hi def link upstreamlog_MsgD	Type
hi def link upstreamlog_MsgE	Error
hi def link upstreamlog_MsgW	Constant
hi def link upstreamlog_Process	Statement
hi def link upstreamlog_IPaddr	Identifier
hi def link upstreamlog_Profile	Identifier
hi def link upstreamlog_Target	Identifier

let b:current_syntax = "upstreamlog"
