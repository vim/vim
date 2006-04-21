" Vim syntax file
" Language:         screen(1) configuration file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn match   screenEscape    '\\.'

syn keyword screenTodo      contained TODO FIXME XXX NOTE

syn region  screenComment   display oneline start='#' end='$'
                            \ contains=screenTodo,@Spell

syn region  screenString    display oneline start=+"+ skip=+\\"+ end=+"+
                            \ contains=screenVariable,screenSpecial

syn region  screenLiteral   display oneline start=+'+ skip=+\\'+ end=+'+

syn match   screenVariable  contained display '$\(\h\w*\|{\h\w*}\)'

syn keyword screenBoolean   on off

syn match   screenNumbers   display '\<\d\+\>'

syn match   screenSpecials  contained
                            \ '%\([%aAdDhlmMstuwWyY?:{]\|[0-9]*n\|0?cC\)'

syn keyword screenCommands  acladd aclchg acldel aclgrp aclumask activity
                            \ addacl allpartial at attrcolor autodetach
                            \ bell_msg bind bindkey bufferfile caption chacl
                            \ chdir clear colon command compacthist console
                            \ copy copy_regcrlf debug detach digraph dinfo
                            \ crlf displays dumptermcap echo exec fit focus
                            \ height help history info kill lastmsg license
                            \ lockscreen markkeys meta msgminwait msgwait
                            \ multiuser nethack next nonblock number only
                            \ other partial_state password paste pastefont
                            \ pow_break pow_detach_msg prev printcmd process
                            \ quit readbuf readreg redisplay register
                            \ remove removebuf reset resize screen select
                            \ sessionname setenv shelltitle silencewait
                            \ verbose sleep sorendition split startup_message
                            \ stuff su suspend time title umask version wall
                            \ width writebuf xoff xon defmode hardstatus
                            \ altscreen break breaktype copy_reg defbreaktype
                            \ defencoding deflog encoding eval ignorecase
                            \ ins_reg maxwin partial pow_detach setsid source
                            \ unsetenv windowlist windows defautonuke autonuke
                            \ defbce bce defc1 c1 defcharset charset defescape
                            \ escape defflow flow defkanji kanji deflogin
                            \ login defmonitor monitor defhstatus hstatus
                            \ defobuflimit obuflimit defscrollback scrollback
                            \ defshell shell defsilence silence defslowpaste
                            \ slowpaste defutf8 utf8 defwrap wrap defwritelock
                            \ writelock defzombie zombie defgr gr hardcopy
                            \ hardcopy_append hardcopydir hardstatus log
                            \ logfile login logtstamp mapdefault mapnotnext
                            \ maptimeout term termcap terminfo termcapinfo
                            \ vbell vbell_msg vbellwait

hi def link screenEscape    Special
hi def link screenComment   Comment
hi def link screenTodo      Todo
hi def link screenString    String
hi def link screenLiteral   String
hi def link screenVariable  Identifier
hi def link screenBoolean   Boolean
hi def link screenNumbers   Number
hi def link screenSpecials  Special
hi def link screenCommands  Keyword

let b:current_syntax = "screen"

let &cpo = s:cpo_save
unlet s:cpo_save
