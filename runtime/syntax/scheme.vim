" Vim syntax file
" Language:	Scheme (R5RS)
" Maintainer:	Dirk van Deun <dirk@igwe.vub.ac.be>
" Last Change:	April 30, 1998

" This script incorrectly recognizes some junk input as numerals:
" parsing the complete system of Scheme numerals using the pattern
" language is practically impossible: I did a lax approximation.

" Suggestions and bug reports are solicited by the author.

" Initializing:

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case ignore

" Fascist highlighting: everything that doesn't fit the rules is an error...

syn match	schemeError	oneline    ![^ \t()";]*!
syn match	schemeError	oneline    ")"

" Quoted and backquoted stuff

syn region schemeQuoted matchgroup=Delimiter start="['`]" end=![ \t()";]!me=e-1 contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc

syn region schemeQuoted matchgroup=Delimiter start="['`](" matchgroup=Delimiter end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc
syn region schemeQuoted matchgroup=Delimiter start="['`]#(" matchgroup=Delimiter end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc

syn region schemeStrucRestricted matchgroup=Delimiter start="(" matchgroup=Delimiter end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc
syn region schemeStrucRestricted matchgroup=Delimiter start="#(" matchgroup=Delimiter end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc

syn region schemeUnquote matchgroup=Delimiter start="," end=![ \t()";]!me=e-1 contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc
syn region schemeUnquote matchgroup=Delimiter start=",@" end=![ \t()";]!me=e-1 contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc

syn region schemeUnquote matchgroup=Delimiter start=",(" end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc
syn region schemeUnquote matchgroup=Delimiter start=",@(" end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc

syn region schemeUnquote matchgroup=Delimiter start=",#(" end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc
syn region schemeUnquote matchgroup=Delimiter start=",@#(" end=")" contains=ALLBUT,schemeStruc,schemeSyntax,schemeFunc

" R5RS Scheme Functions and Syntax:

if version < 600
  set iskeyword=33,35-39,42-58,60-90,94,95,97-122,126,_
else
  setlocal iskeyword=33,35-39,42-58,60-90,94,95,97-122,126,_
endif

syn keyword schemeSyntax lambda and or if cond case define let let* letrec
syn keyword schemeSyntax begin do delay set! else =>
syn keyword schemeSyntax quote quasiquote unquote unquote-splicing
syn keyword schemeSyntax define-syntax let-syntax letrec-syntax syntax-rules

syn keyword schemeFunc not boolean? eq? eqv? equal? pair? cons car cdr set-car!
syn keyword schemeFunc set-cdr! caar cadr cdar cddr caaar caadr cadar caddr
syn keyword schemeFunc cdaar cdadr cddar cdddr caaaar caaadr caadar caaddr
syn keyword schemeFunc cadaar cadadr caddar cadddr cdaaar cdaadr cdadar cdaddr
syn keyword schemeFunc cddaar cddadr cdddar cddddr null? list? list length
syn keyword schemeFunc append reverse list-ref memq memv member assq assv assoc
syn keyword schemeFunc symbol? symbol->string string->symbol number? complex?
syn keyword schemeFunc real? rational? integer? exact? inexact? = < > <= >=
syn keyword schemeFunc zero? positive? negative? odd? even? max min + * - / abs
syn keyword schemeFunc quotient remainder modulo gcd lcm numerator denominator
syn keyword schemeFunc floor ceiling truncate round rationalize exp log sin cos
syn keyword schemeFunc tan asin acos atan sqrt expt make-rectangular make-polar
syn keyword schemeFunc real-part imag-part magnitude angle exact->inexact
syn keyword schemeFunc inexact->exact number->string string->number char=?
syn keyword schemeFunc char-ci=? char<? char-ci<? char>? char-ci>? char<=?
syn keyword schemeFunc char-ci<=? char>=? char-ci>=? char-alphabetic? char?
syn keyword schemeFunc char-numeric? char-whitespace? char-upper-case?
syn keyword schemeFunc char-lower-case?
syn keyword schemeFunc char->integer integer->char char-upcase char-downcase
syn keyword schemeFunc string? make-string string string-length string-ref
syn keyword schemeFunc string-set! string=? string-ci=? string<? string-ci<?
syn keyword schemeFunc string>? string-ci>? string<=? string-ci<=? string>=?
syn keyword schemeFunc string-ci>=? substring string-append vector? make-vector
syn keyword schemeFunc vector vector-length vector-ref vector-set! procedure?
syn keyword schemeFunc apply map for-each call-with-current-continuation
syn keyword schemeFunc call-with-input-file call-with-output-file input-port?
syn keyword schemeFunc output-port? current-input-port current-output-port
syn keyword schemeFunc open-input-file open-output-file close-input-port
syn keyword schemeFunc close-output-port eof-object? read read-char peek-char
syn keyword schemeFunc write display newline write-char call/cc
syn keyword schemeFunc list-tail string->list list->string string-copy
syn keyword schemeFunc string-fill! vector->list list->vector vector-fill!
syn keyword schemeFunc force with-input-from-file with-output-to-file
syn keyword schemeFunc char-ready? load transcript-on transcript-off eval
syn keyword schemeFunc dynamic-wind port? values call-with-values
syn keyword schemeFunc scheme-report-environment null-environment
syn keyword schemeFunc interaction-environment

" Writing out the complete description of Scheme numerals without
" using variables is a day's work for a trained secretary...
" This is a useful lax approximation:

syn match	schemeNumber	oneline    "[-#+0-9.][-#+/0-9a-f@i.boxesfdl]*"
syn match	schemeError	oneline    ![-#+0-9.][-#+/0-9a-f@i.boxesfdl]*[^-#+/0-9a-f@i.boxesfdl \t()";][^ \t()";]*!

syn match	schemeOther	oneline    ![+-][ \t()";]!me=e-1
syn match	schemeOther	oneline    ![+-]$!
" ... so that a single + or -, inside a quoted context, would not be
" interpreted as a number (outside such contexts, it's a schemeFunc)

syn match	schemeDelimiter	oneline    !\.[ \t()";]!me=e-1
syn match	schemeDelimiter	oneline    !\.$!
" ... and a single dot is not a number but a delimiter

" Simple literals:

syn match	schemeBoolean	oneline    "#[tf]"
syn match	schemeError	oneline    !#[tf][^ \t()";]\+!

syn match	schemeChar	oneline    "#\\"
syn match	schemeChar	oneline    "#\\."
syn match       schemeError	oneline    !#\\.[^ \t()";]\+!
syn match	schemeChar	oneline    "#\\space"
syn match	schemeError	oneline    !#\\space[^ \t()";]\+!
syn match	schemeChar	oneline    "#\\newline"
syn match	schemeError	oneline    !#\\newline[^ \t()";]\+!

" This keeps all other stuff unhighlighted, except *stuff* and <stuff>:

syn match	schemeOther	oneline    ,[a-z!$%&*/:<=>?^_~][-a-z!$%&*/:<=>?^_~0-9+.@]*,
syn match	schemeError	oneline    ,[a-z!$%&*/:<=>?^_~][-a-z!$%&*/:<=>?^_~0-9+.@]*[^-a-z!$%&*/:<=>?^_~0-9+.@ \t()";]\+[^ \t()";]*,

syn match	schemeOther	oneline    "\.\.\."
syn match	schemeError	oneline    !\.\.\.[^ \t()";]\+!
" ... a special identifier

syn match	schemeConstant	oneline    ,\*[-a-z!$%&*/:<=>?^_~0-9+.@]*\*[ \t()";],me=e-1
syn match	schemeConstant	oneline    ,\*[-a-z!$%&*/:<=>?^_~0-9+.@]*\*$,
syn match	schemeError	oneline    ,\*[-a-z!$%&*/:<=>?^_~0-9+.@]*\*[^-a-z!$%&*/:<=>?^_~0-9+.@ \t()";]\+[^ \t()";]*,

syn match	schemeConstant	oneline    ,<[-a-z!$%&*/:<=>?^_~0-9+.@]*>[ \t()";],me=e-1
syn match	schemeConstant	oneline    ,<[-a-z!$%&*/:<=>?^_~0-9+.@]*>$,
syn match	schemeError	oneline    ,<[-a-z!$%&*/:<=>?^_~0-9+.@]*>[^-a-z!$%&*/:<=>?^_~0-9+.@ \t()";]\+[^ \t()";]*,

" Non-quoted lists, and strings:

syn region schemeStruc matchgroup=Delimiter start="(" matchgroup=Delimiter end=")" contains=ALL
syn region schemeStruc matchgroup=Delimiter start="#(" matchgroup=Delimiter end=")" contains=ALL

syn region	schemeString	start=+"+  skip=+\\[\\"]+ end=+"+

" Comments:

syn match	schemeComment	";.*$"

" Synchronization and the wrapping up...

syn sync match matchPlace grouphere NONE "^[^ \t]"
" ... i.e. synchronize on a line that starts at the left margin

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_scheme_syntax_inits")
  if version < 508
    let did_scheme_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink schemeSyntax		Statement
  HiLink schemeFunc		Function

  HiLink schemeString		String
  HiLink schemeChar		Character
  HiLink schemeNumber		Number
  HiLink schemeBoolean		Boolean

  HiLink schemeDelimiter	Delimiter
  HiLink schemeConstant	Constant

  HiLink schemeComment		Comment
  HiLink schemeError		Error

  delcommand HiLink
endif

let b:current_syntax = "scheme"
