" Vim syntax file
" Language:	    Screen Virtual Terminal Emulator/Manager Configuration File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/screen/
" Latest Revision:  2004-05-22
" arch-tag:	    6a97fb8f-fc88-497f-9c55-e946734ba034

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" comments
syn region  screenComment	matchgroup=screenComment start="#" end="$" contains=screenTodo

" todo
syn keyword screenTodo		contained TODO FIXME XXX NOTE

" string (can contain variables)
syn region  screenString	matchgroup=screenString start='"' skip='\\"' end='"\|$' contains=screenVariable,screenSpecial

" literal string
syn region  screenLiteral	matchgroup=screenLiteral start="'" skip="\\'" end="'\|$"

" environment variables
syn match   screenVariable	contained "$\(\h\w*\|{\h\w*}\)"

" booleans
syn keyword screenBoolean	on off

" numbers
syn match   screenNumbers	"\<\d\+\>"

" specials
syn match   screenSpecials	contained "%\([%aAdDhlmMstuwWyY?:{]\|[0-9]*n\|0?cC\)"

" commands
syn keyword screenCommands	acladd aclchg acldel aclgrp aclumask activity addacl allpartial at attrcolor
syn keyword screenCommands	autodetach bell_msg bind bindkey bufferfile caption chacl chdir clear colon
syn keyword screenCommands	command compacthist console copy copy_regcrlf debug detach digraph dinfo crlf
syn keyword screenCommands	displays dumptermcap echo exec fit focus height help history
syn keyword screenCommands	info kill lastmsg license lockscreen markkeys meta msgminwait msgwait
syn keyword screenCommands	multiuser nethack next nonblock number only other partial_state
syn keyword screenCommands	password paste pastefont pow_break pow_detach_msg prev printcmd process
syn keyword screenCommands	quit readbuf readreg redisplay register remove removebuf reset resize screen
syn keyword screenCommands	select sessionname setenv shelltitle silencewait verbose
syn keyword screenCommands	sleep sorendition split startup_message stuff su suspend time
syn keyword screenCommands	title umask version wall width writebuf xoff xon defmode hardstatus
syn keyword screenCommands	altscreen break breaktype copy_reg defbreaktype defencoding deflog encoding
syn keyword screenCommands	eval ignorecase ins_reg maxwin partial pow_detach setsid source unsetenv
syn keyword screenCommands	windowlist windows
syn match   screenCommands	"\<\(def\)\=\(autonuke\|bce\|c1\|charset\|escape\|flow\|kanji\|login\|monitor\|hstatus\|obuflimit\)\>"
syn match   screenCommands	"\<\(def\)\=\(scrollback\|shell\|silence\|slowpaste\|utf8\|wrap\|writelock\|zombie\|gr\)\>"
syn match   screenCommands	"\<hard\(copy\(_append\|dir\)\=\|status\)\>"
syn match   screenCommands	"\<log\(file\|in\|tstamp\)\=\>"
syn match   screenCommands	"\<map\(default\|notnext\|timeout\)\>"
syn match   screenCommands	"\<term\(cap\|info\|capinfo\)\=\>"
syn match   screenCommands	"\<vbell\(_msg\|wait\)\=\>"

if exists("screen_minlines")
    let b:screen_minlines = screen_minlines
else
    let b:screen_minlines = 10
endif
exec "syn sync minlines=" . b:screen_minlines

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_screen_syn_inits")
  if version < 508
    let did_screen_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink screenComment    Comment
  HiLink screenTodo	  Todo
  HiLink screenString	  String
  HiLink screenLiteral    String
  HiLink screenVariable   Identifier
  HiLink screenBoolean    Boolean
  HiLink screenNumbers    Number
  HiLink screenSpecials   Special
  HiLink screenCommands   Keyword
  delcommand HiLink
endif

let b:current_syntax = "screen"

" vim: set sts=2 sw=2:
