" Vim syntax file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/
" Latest Revision:  2004-11-11
" arch-tag:	    356fad6d-ff6b-453c-bd25-7fc63c4758bc

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Todo
syn keyword sieveTodo	  contained TODO FIXME XXX NOTE

" Comments
syn region  sieveComment  matchgroup=sieveComment start='/\*' end='\*/' contains=sieveTodo
syn region  sieveComment  matchgroup=sieveComment start='#' end='$' contains=sieveTodo

syn case ignore

" Tags
syn match   sieveTag	  ':\h\w*'

" Numbers
syn match   sieveNumber	  '\d\+[KMG]\='

" Specials
syn match   sieveSpecial  '\\["\\]'

" Strings
syn region  sieveString	  matchgroup=sieveString start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=sieveSpecial
syn region  sieveString	  matchgroup=sieveString start='text:' end='\n.\n'

" Keywords
syn keyword sieveConditional  if elsif else
syn keyword sieveTest	      address allof anyof envelope exists false header not size true
syn keyword sievePreProc      require stop
syn keyword sieveAction	      reject fileinto redirect keep discard
syn match   sieveKeyword      '\<\h\w*\>'

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_sieve_syn_inits")
  if version < 508
    let did_sieve_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink sieveTodo	  Todo
  HiLink sieveComment	  Comment
  HiLink sieveTag	  Type
  HiLink sieveNumber	  Number
  HiLink sieveSpecial	  Special
  HiLink sieveString	  String
  HiLink sieveConditional Conditional
  HiLink sieveTest	  Keyword
  HiLink sievePreProc	  PreProc
  HiLink sieveAction	  Keyword
  HiLink sieveKeyword	  Keyword

  delcommand HiLink
endif

let b:current_syntax = "sieve"

" vim: set sts=2 sw=2:
