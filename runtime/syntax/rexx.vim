" Vim syntax file
" Language:	Rexx
" Maintainer:	Thomas Geulig <geulig@nentec.de>
" Last Change:	2001 May 2
" URL:		http://mywebpage.netscape.com/sharpPeople/vim/syntax/rexx.vim
"
" Special Thanks to Dan Sharp <dwsharp@hotmail.com> for comments and additions
" (and providing the webspace)

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case ignore

" A Keyword is the first symbol in a clause.  A clause begins at the start
" of a line or after a semicolon.  THEN, ELSE, OTHERWISE, and colons are always
" followed by an implied semicolon.
syn match rexxClause "\(^\|;\|:\|then \|else \|otherwise \)\s*\w\+" contains=ALL

" Considered keywords when used together in a phrase and begin a clause
syn match rexxKeyword contained "\<signal\( on \(error\|failure\|halt\|notready\|novalue\|syntax\|lostdigits\)\(\s\+name\)\=\)\=\>"
syn match rexxKeyword contained "\<signal off \(error\|failure\|halt\|notready\|novalue\|syntax\|lostdigits\)\>"
syn match rexxKeyword contained "\<call off \(error\|failure\|halt\|notready\)\>"
syn match rexxKeyword contained "\<parse \(upper \)\=\(arg\|linein\|pull\|source\|var\|value\|version\)\>"
syn match rexxKeyword contained "\<numeric \(digits\|form \(scientific\|engineering\|value\)\|fuzz\)\>"
syn match rexxKeyword contained "\<\(address\|trace\)\( value\)\=\>"
syn match rexxKeyword contained "\<procedure\( expose\)\=\>"
syn match rexxKeyword contained "\<do\( forever\)\=\>"

" Another keyword phrase, separated to aid highlighting in rexxFunction
syn match rexxKeyword2 contained "\<call\( on \(error\|failure\|halt\|notready\)\(\s\+name\)\=\)\=\>"

" Considered keywords when they begin a clause
syn match rexxKeyword contained "\<\(arg\|drop\|end\|exit\|if\|interpret\|iterate\|leave\|nop\)\>"
syn match rexxKeyword contained "\<\(options\|pull\|push\|queue\|return\|say\|select\|trace\)\>"

" Conditional phrases
syn match rexxConditional "\(^\s*\| \)\(to\|by\|for\|until\|while\|then\|when\|otherwise\|else\)\( \|\s*$\)" contains=ALLBUT,rexxConditional
syn match rexxConditional contained "\<\(to\|by\|for\|until\|while\|then\|when\|else\|otherwise\)\>"

" Assignments -- a keyword followed by an equal sign becomes a variable
syn match rexxAssign "\<\w\+\s*=\s*" contains=rexxSpecialVariable

" Functions/Procedures
syn match rexxFunction	"\<\h\w*\(/\*\s*\*/\)*("me=e-1 contains=rexxComment,rexxConditional,rexxKeyword
syn match rexxFunction	"\<\(arg\|trace\)\(/\*\s*\*/\)*("me=e-1
syn match rexxFunction	"\<call\( on \(error\|failure\|halt\|notready\)\(\s\+name\)\=\)\=\>\s\+\w\+\>" contains=rexxKeyword2

" String constants
syn region rexxString	  start=+"+ skip=+\\\\\|\\'+ end=+"+
syn region rexxString	  start=+'+ skip=+\\\\\|\\"+ end=+'+
syn match  rexxCharacter  +"'[^\\]'"+

" Catch errors caused by wrong parenthesis
syn region rexxParen transparent start='(' end=')' contains=ALLBUT,rexxParenError,rexxTodo,rexxUserLabel,rexxKeyword
syn match rexxParenError	 ")"
syn match rexxInParen		"[\\[\\]{}]"

" Comments
syn region rexxComment		start="/\*" end="\*/" contains=rexxTodo,rexxComment
syn match  rexxCommentError	"\*/"

syn keyword rexxTodo contained	TODO FIXME XXX

" Highlight User Labels
syn match rexxUserLabel		 "\<\I\i*\s*:"me=e-1

" Special Variables
syn keyword rexxSpecialVariable  sigl rc result
syn match   rexxCompoundVariable "\<\w\+\.\w*\>"

if !exists("rexx_minlines")
  let rexx_minlines = 10
endif
exec "syn sync ccomment rexxComment minlines=" . rexx_minlines

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_rexx_syn_inits")
  if version < 508
    let did_rexx_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink rexxUserLabel		Function
  HiLink rexxCharacter		Character
  HiLink rexxParenError		rexxError
  HiLink rexxInParen		rexxError
  HiLink rexxCommentError	rexxError
  HiLink rexxError		Error
  HiLink rexxKeyword		Statement
  HiLink rexxKeyword2		rexxKeyword
  HiLink rexxFunction		Function
  HiLink rexxString		String
  HiLink rexxComment		Comment
  HiLink rexxTodo		Todo
  HiLink rexxSpecialVariable	Special
  HiLink rexxConditional	rexxKeyword

  delcommand HiLink
endif

let b:current_syntax = "rexx"

"vim: ts=8
