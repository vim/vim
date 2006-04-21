" Vim syntax file
" Language:         Sieve filtering language input file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword sieveTodo         contained TODO FIXME XXX NOTE

syn region  sieveComment      start='/\*' end='\*/' contains=sieveTodo,@Spell
syn region  sieveComment      display oneline start='#' end='$'
                              \ contains=sieveTodo,@Spell

syn case ignore

syn match   sieveTag          display ':\h\w*'

syn match   sieveNumber       display '\<\d\+[KMG]\=\>'

syn match   sieveSpecial      display '\\["\\]'

syn region  sieveString       start=+"+ skip=+\\\\\|\\"+ end=+"+
                              \ contains=sieveSpecial
syn region  sieveString       start='text:' end='\n.\n'

syn keyword sieveConditional  if elsif else
syn keyword sieveTest         address allof anyof envelope exists false header
                              \ not size true
syn keyword sievePreProc      require stop
syn keyword sieveAction       reject fileinto redirect keep discard
syn match   sieveKeyword      '\<\h\w*\>'

syn case match

hi def link sieveTodo        Todo
hi def link sieveComment     Comment
hi def link sieveTag         Type
hi def link sieveNumber      Number
hi def link sieveSpecial     Special
hi def link sieveString      String
hi def link sieveConditional Conditional
hi def link sieveTest        Keyword
hi def link sievePreProc     PreProc
hi def link sieveAction      Keyword
hi def link sieveKeyword     Keyword

let b:current_syntax = "sieve"

let &cpo = s:cpo_save
unlet s:cpo_save
