" Vim syntax file
" Language:             Innovation Data Processing USW2KAgt.log file
" Maintainer:           Rob Owens <rowens@fdrinnovation.com>
" Latest Revision:      2013-06-17

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Date:
syn match usw2kagentlog_Date /\u\l\l \u\l\l\s\{1,2}\d\{1,2} \d\d:\d\d:\d\d \d\d\d\d/
" Msg Types:
syn match usw2kagentlog_MsgD /Msg #\(Agt\|PC\|Srv\)\d\{4,5}D/ nextgroup=usw2kagentlog_Process skipwhite
syn match usw2kagentlog_MsgE /Msg #\(Agt\|PC\|Srv\)\d\{4,5}E/ nextgroup=usw2kagentlog_Process skipwhite
syn match usw2kagentlog_MsgI /Msg #\(Agt\|PC\|Srv\)\d\{4,5}I/ nextgroup=usw2kagentlog_Process skipwhite
syn match usw2kagentlog_MsgW /Msg #\(Agt\|PC\|Srv\)\d\{4,5}W/ nextgroup=usw2kagentlog_Process skipwhite
" Processes:
syn region usw2kagentlog_Process start="(" end=")" contained
syn region usw2kagentlog_Process start="Starting the processing for a \zs\"" end="\ze client request"
syn region usw2kagentlog_Process start="Ending the processing for a \zs\"" end="\ze client request"
" IP Address:
syn match usw2kagentlog_IPaddr / \d\{1,3}\.\d\{1,3}\.\d\{1,3}\.\d\{1,3}/
" Profile:
syn region usw2kagentlog_Profile start="Profile name \zs" end="\"\S\{1,8}\""
syn region usw2kagentlog_Profile start=" Profile: \zs" end="\S\{1,8}"
syn region usw2kagentlog_Profile start="  Profile: \zs" end="\ze, "
syn region usw2kagentlog_Profile start="Backup Profile: \zs" end="\ze Version date"
syn region usw2kagentlog_Profile start="Full of \zs" end="\ze$"
syn region usw2kagentlog_Profile start="Incr. of \zs" end="\ze$"
syn region usw2kagentlog_Profile start="profile name \zs\"" end="\""
" Target:
syn region usw2kagentlog_Target start="Computer: \zs" end="\ze[\]\)]" 
syn region usw2kagentlog_Target start="Computer name \zs" end="\ze," 
" Agent Keywords:
syn keyword usw2kagentlog_Agentword opened closed

hi def link usw2kagentlog_Date		Underlined
hi def link usw2kagentlog_MsgD		Type
hi def link usw2kagentlog_MsgE		Error
hi def link usw2kagentlog_MsgW		Constant
hi def link usw2kagentlog_Process	Statement
hi def link usw2kagentlog_IPaddr	Identifier
hi def link usw2kagentlog_Profile	Identifier
hi def link usw2kagentlog_Target	Identifier
hi def link usw2kagentlog_Agentword	Special

let b:current_syntax = "usw2kagentlog"
