" Vim syntax file
" Language:	    fetchmail(1) RC File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/fetchmail/
" Latest Revision:  2004-05-06
" arch-tag:	    59d8adac-6e59-45f6-88cb-f9ba1e009c1f

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" todo
syn keyword fetchmailTodo	contained FIXME TODO XXX NOTE

" comments
syn region  fetchmailComment	start="#" end="$" contains=fetchmailTodo

" numbers
syn match   fetchmailNumber	"\<\d\+\>"

" strings
syn region  fetchmailString	start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=fetchmailStringEsc
syn region  fetchmailString	start=+'+ skip=+\\\\\|\\'+ end=+'+ contains=fetchmailStringEsc

" escape characters in strings
syn match   fetchmailStringEsc	"\\\([ntb]\|0\d*\|x\x\+\)"

" server entries
syn region  fetchmailKeyword	transparent matchgroup=fetchmailKeyword start="\<poll\|skip\|defaults\>" end="\<poll\|skip\|defaults\>" contains=ALLBUT,fetchmailOptions,fetchmailSet

" server options
syn keyword fetchmailServerOpts	contained via proto[col] local[domains] port auth[enticate]
syn keyword fetchmailServerOpts	contained timeout envelope qvirtual aka interface monitor
syn keyword fetchmailServerOpts	contained plugin plugout dns checkalias uidl interval netsec
syn keyword fetchmailServerOpts	contained principal esmtpname esmtppassword
syn match   fetchmailServerOpts	contained "\<no\_s\+\(envelope\|dns\|checkalias\|uidl\)"

" user options
syn keyword fetchmailUserOpts	contained user[name] is to pass[word] ssl sslcert sslkey sslproto folder
syn keyword fetchmailUserOpts	contained smtphost fetchdomains smtpaddress smtpname antispam mda bsmtp
syn keyword fetchmailUserOpts	contained preconnect postconnect keep flush fetchall rewrite stripcr
syn keyword fetchmailUserOpts	contained forcecr pass8bits dropstatus dropdelivered mimedecode idle
syn keyword fetchmailUserOpts	contained limit warnings batchlimit fetchlimit expunge tracepolls properties
syn match   fetchmailUserOpts	contained "\<no\_s\+\(keep\|flush\|fetchall\|rewrite\|stripcr\|forcecr\|pass8bits\|dropstatus\|dropdelivered\|mimedecode\|noidle\)"

syn keyword fetchmailSpecial	contained here there


" noise keywords
syn keyword fetchmailNoise	and with has wants options
syn match   fetchmailNoise	"[:;,]"

" options
syn keyword fetchmailSet	nextgroup=fetchmailOptions skipwhite skipnl set

syn keyword fetchmailOptions	daemon postmaster bouncemail spambounce logfile idfile syslog nosyslog properties
syn match   fetchmailOptions	"\<no\_s\+\(bouncemail\|spambounce\)"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_fetchmail_syn_inits")
  if version < 508
    let did_fetchmail_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink fetchmailComment	Comment
  HiLink fetchmailTodo	Todo
  HiLink fetchmailNumber	Number
  HiLink fetchmailString	String
  HiLink fetchmailStringEsc	SpecialChar
  HiLink fetchmailKeyword	Keyword
  HiLink fetchmailServerOpts	Identifier
  HiLink fetchmailUserOpts	Identifier
  HiLink fetchmailSpecial	Special
  HiLink fetchmailSet		Keyword
  HiLink fetchmailOptions	Identifier
  delcommand HiLink
endif

let b:current_syntax = "fetchmail"

" vim: set sts=2 sw=2:
