" Vim syntax file
" Language:	Rexx
" Maintainer:	Thomas Geulig <geulig@nentec.de>
" Last Change:  2005 Dez  9, added some <http://www.ooRexx.org>-coloring,
"                            line comments, do *over*, messages, directives,
"                            highlighting classes, methods, routines and requires
"               Rony G. Flatscher <rony.flatscher@wu-wien.ac.at>
"
" URL:		http://www.geulig.de/vim/rexx.vim
"
" Special Thanks to Dan Sharp <dwsharp@hotmail.com> and Rony G. Flatscher
" <Rony.Flatscher@wu-wien.ac.at> for comments and additions

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case ignore

" add to valid identifier chars
setlocal iskeyword+=.
setlocal iskeyword+=!
setlocal iskeyword+=?

" ---rgf, position important: must be before comments etc. !
syn match rexxOperator "[-=|\/\\\+\*\[\],;<>&\~]"

syn match rexxIdentifier        "\<[a-zA-Z\!\?_]\([a-zA-Z0-9._?!]\)*\>"
syn match rexxEnvironmentSymbol "\<\.\+\([a-zA-Z0-9._?!]\)*\>"


" A Keyword is the first symbol in a clause.  A clause begins at the start
" of a line or after a semicolon.  THEN, ELSE, OTHERWISE, and colons are always
" followed by an implied semicolon.
syn match rexxClause "\(^\|;\|:\|then \|else \|otherwise \)\s*\w\+" contains=ALLBUT,rexxParse2,rexxRaise2


" Considered keywords when used together in a phrase and begin a clause
syn match rexxParse "\<parse\s*\(\(upper\|lower\|caseless\)\s*\)\=\(arg\|linein\|pull\|source\|var\|\<value\>\|version\)\>"
syn match rexxParse2 "\<with\>" contained containedin=rexxParse


syn match rexxKeyword contained "\<numeric \(digits\|form \(scientific\|engineering\|value\)\|fuzz\)\>"
syn match rexxKeyword contained "\<\(address\|trace\)\( value\)\=\>"
syn match rexxKeyword contained "\<procedure\(\s*expose\)\=\>"
syn match rexxKeyword contained "\<do\>\(\s*forever\)\=\>"
syn match rexxKeyword contained "\<use\>\s*\<arg\>"

" Another keyword phrase, separated to aid highlighting in rexxFunction
syn match rexxKeyword contained "\<signal\(\s*\(on\|off\)\s*\(any\|error\|failure\|halt\|lostdigits\|nomethod\|nostring\|notready\|novalue\|syntax\|user\s*\k*\)\(\s\+name\)\=\)\=\>"
syn match rexxKeyword2 contained "\<call\(\s*\(on\|off\)\s*\(any\|error\|failure\|halt\|notready\|user\s*\k*\)\(\s\+name\)\=\)\=\>"


" Considered keywords when they begin a clause
syn match rexxKeyword contained "\<\(arg\|do\|drop\|end\|exit\|expose\|forward\|if\|interpret\|iterate\|leave\|nop\)\>"
syn match rexxKeyword contained "\<\(options\|pull\|push\|queue\|raise\|reply\|return\|say\|select\|trace\)\>"

" Conditional phrases
syn match rexxConditional  "\(^\s*\| \)\(to\|by\|for\|until\|while\|then\|when\|otherwise\|else\|over\)\( \|\s*$\)"
syn match rexxConditional contained "\<\(to\|by\|for\|until\|while\|then\|when\|otherwise\|else\|over\)\>"

" must be after Conditional phrases!
syn match rexxKeyword ".*\<\(then\|else\)\s*\<do\>"

" Raise statement
syn match rexxRaise "\(^\|;\|:\)\s\+\<raise\>\s*\<\(propagate\|error\|failure\|syntax\|user\)\>\="
syn match rexxRaise2 "\<\(additional\|array\|description\|exit\|return\)\>" contained containedin=rexxRaise

" Forward statement keywords
syn match rexxForward  "\(^\|;\|:\)\<forward\>\s*"
syn match rexxForward2 "\<\(arguments\|array\|continue\|message\|class\|to\)\>" contained containedin=rexxForward

" Functions/Procedures
syn match rexxFunction	"\<\w*\(/\*\s*\*/\)*("me=e-1 contains=rexxComment,rexxConditional,rexxKeyword,rexxIdentifier
syn match rexxFunction 	"\<\<[a-zA-Z\!\?_]\([a-zA-Z0-9._?!]\)*\>("me=e-1
syn match rexxFunction	"\<call\s\+\k\+\>"  contains=rexxKeyword2
syn match rexxFunction "[()]"

" String constants
syn region rexxString	  start=+"+ skip=+""+ end=+"\(x\|b\)\=+ oneline
syn region rexxString	  start=+'+ skip=+''+ end=+'\(x\|b\)\=+ oneline

" Catch errors caused by wrong parenthesis
syn region rexxParen transparent start='(' end=')' contains=ALLBUT,rexxParenError,rexxTodo,rexxLabel,rexxKeyword
syn match rexxParenError	 ")"
syn match rexxInParen		"[\\[\\]{}]"

" Comments
syn region rexxComment		start="/\*" end="\*/" contains=rexxTodo,rexxComment
syn match  rexxCommentError	"\*/"
syn match  rexxLineComment       /--.*/

syn keyword rexxTodo contained	TODO FIXME XXX


" ooRexx messages
syn region rexxMessageOperator start="\(\~\|\~\~\)" end="\(\S\|\s\)"me=e-1
syn match rexxMessage "\(\~\|\~\~\)\s*\<\.*[a-zA-Z]\([a-zA-Z0-9._?!]\)*\>" contains=rexxMessageOperator

" Highlight User Labels
syn match rexxLabel		 "^\s*\k*\s*:"me=e-1

syn match rexxLineContinue ",\ze\s*\(--.*\|\/\*.*\)*$"
" the following is necessary, otherwise three consecutive dashes will cause it to highlight the first one
syn match rexxLineContinue "-\ze\(\s+--.*\|\s*\/\*.*\)*$"

" Special Variables
syn keyword rexxSpecialVariable  sigl rc result self super

" Constants
syn keyword rexxConst .true .false .nil

" ooRexx builtin classes, first define dot to be o.k. in keywords
syn keyword rexxBuiltinClass .object .class .method .message
syn keyword rexxBuiltinClass .monitor .alarm
syn keyword rexxBuiltinClass .stem .stream .string
syn keyword rexxBuiltinClass .mutablebuffer
syn keyword rexxBuiltinClass .array .list .queue .directory .table .set
syn keyword rexxBuiltinClass .relation .bag .supplier .regularExpressions

" Windows-only classes
syn keyword rexxBuiltinClass .OLEObject .MenuObject .WindowsClipboard .WindowsEventLog
syn keyword rexxBuiltinClass .WindowsManager .WindowObject .WindowsProgramManager


" ooRexx directives, ---rgf location important, otherwise directives in top of
" file not matched!
syn region rexxClass    start="::\s*class\s*"ms=e+1    end="\ze\(\s\|;\|$\)"
syn region rexxMethod   start="::\s*method\s*"ms=e+1   end="\ze\(\s\|;\|$\)"
syn region rexxRequires start="::\s*requires\s*"ms=e+1 end="\ze\(\s\|;\|$\)"
syn region rexxRoutine  start="::\s*routine\s*"ms=e+1  end="\ze\(\s\|;\|$\)"

syn region rexxDirective start="\(^\|;\)\s*::\s*\w\+"  end="\($\|;\)" contains=rexxString,rexxComment,rexxLineComment,rexxClass,rexxMethod,rexxRoutine,rexxRequires keepend



if !exists("rexx_minlines")
"  let rexx_minlines = 10
  let rexx_minlines = 500
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

  HiLink rexxLabel		Function
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

  HiLink rexxOperator		Operator
  HiLink rexxMessageOperator	rexxOperator
  HiLink rexxLineComment	RexxComment

  HiLink rexxLineContinue	WildMenu

  HiLink rexxDirective		rexxKeyword
  HiLink rexxClass              Type
  HiLink rexxMethod             rexxFunction
  HiLink rexxRequires           Include
  HiLink rexxRoutine            rexxFunction

  HiLink rexxConst		Constant
  HiLink rexxTypeSpecifier	Type
  HiLink rexxBuiltinClass	rexxTypeSpecifier

  HiLink rexxEnvironmentSymbol  rexxConst
  HiLink rexxMessage		rexxFunction

  HiLink rexxParse              rexxKeyword
  HiLink rexxParse2             rexxParse

  HiLink rexxRaise              rexxKeyword
  HiLink rexxRaise2             rexxRaise

  HiLink rexxForward            rexxKeyword
  HiLink rexxForward2           rexxForward

  delcommand HiLink
endif

let b:current_syntax = "rexx"

"vim: ts=8
