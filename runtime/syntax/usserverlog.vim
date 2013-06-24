" Vim syntax file
" Language:             Innovation Data Processing usserver.log file
" Maintainer:           Rob Owens <rowens@fdrinnovation.com>
" Latest Revision:      2013-06-17

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Date:
syn match usserver_Date /\u\l\l \u\l\l\s\{1,2}\d\{1,2} \d\d:\d\d:\d\d \d\d\d\d/
" Msg Types:
syn match usserver_MsgD /Msg #\(Agt\|PC\|Srv\)\d\{4,5}D/ nextgroup=usserver_Process skipwhite
syn match usserver_MsgE /Msg #\(Agt\|PC\|Srv\)\d\{4,5}E/ nextgroup=usserver_Process skipwhite
syn match usserver_MsgI /Msg #\(Agt\|PC\|Srv\)\d\{4,5}I/ nextgroup=usserver_Process skipwhite
syn match usserver_MsgW /Msg #\(Agt\|PC\|Srv\)\d\{4,5}W/ nextgroup=usserver_Process skipwhite
" Processes:
syn region usserver_Process start="(" end=")" contained
" IP Address:
syn match usserver_IPaddr /\( \|(\)\zs\d\{1,3}\.\d\{1,3}\.\d\{1,3}\.\d\{1,3}/
" Profile:
syn region usserver_Profile start="Profile name \zs" end="\"\S\{1,8}\""
syn region usserver_Profile start=" Profile: \zs" end="\S\{1,8}"
syn region usserver_Profile start=", profile: \zs" end="\S\{1,8}\ze,"
syn region usserver_Profile start=" profile \zs" end="\S\{1,8}"
syn region usserver_Profile start="  Profile: \zs" end="\ze, "
syn region usserver_Profile start="Backup Profile: \zs" end="\ze Version date"
syn region usserver_Profile start="Full of \zs" end="\ze$"
syn region usserver_Profile start="Incr. of \zs" end="\ze$"
syn region usserver_Profile start="Profile=\zs" end="\S\{1,8}\ze,"
" Target:
syn region usserver_Target start="Computer: \zs" end="\ze[\]\)]" 
syn region usserver_Target start="Computer name \zs" end="\ze," 
syn region usserver_Target start="Registration add request successful \zs" end="$"
syn region usserver_Target start="request to registered name \zs" end=" "
syn region usserver_Target start=", sending to \zs" end="$"

hi def link usserver_Date	Underlined
hi def link usserver_MsgD	Type
hi def link usserver_MsgE	Error
hi def link usserver_MsgW	Constant
hi def link usserver_Process	Statement
hi def link usserver_IPaddr	Identifier
hi def link usserver_Profile	Identifier
hi def link usserver_Target	Identifier

let b:current_syntax = "usserver"
