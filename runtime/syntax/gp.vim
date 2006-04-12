" Vim syntax file
" Language:	gp (version 2.2)
" Maintainer:	Karim Belabas <Karim.Belabas@math.u-bordeaux.fr>
" Last change:	2006 Apr 12
" URL:		http://pari.math.u-bordeaux.fr

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" control statements
syntax keyword gpStatement	break return next
syntax keyword gpConditional	if
syntax keyword gpRepeat		until while for fordiv forprime forstep forvec
" storage class
syntax keyword gpScope		local global
" defaults
syntax keyword gpInterfaceKey	colors compatible datadir debug debugfiles
syntax keyword gpInterfaceKey	debugmem echo factor_add_primes format help
syntax keyword gpInterfaceKey	histsize lines log logfile new_galois_format
syntax keyword gpInterfaceKey	output parisize path prettyprinter primelimit
syntax keyword gpInterfaceKey	prompt prompt_cont psfile realprecision secure
syntax keyword gpInterfaceKey	seriesprecision simplify strictmatch TeXstyle timer

syntax match   gpInterface	"^\s*\\[a-z].*"
syntax keyword gpInterface	default
syntax keyword gpInput		read input

" functions
syntax match gpFunRegion "^\s*[a-zA-Z][_a-zA-Z0-9]*(.*)\s*=\s*[^ \t=]"me=e-1 contains=gpFunction,gpArgs
syntax match gpFunRegion "^\s*[a-zA-Z][_a-zA-Z0-9]*(.*)\s*=\s*$" contains=gpFunction,gpArgs
syntax match gpArgs contained "[a-zA-Z][_a-zA-Z0-9]*"
syntax match gpFunction contained "^\s*[a-zA-Z][_a-zA-Z0-9]*("me=e-1

" String and Character constants
" Highlight special (backslash'ed) characters differently
syntax match  gpSpecial contained "\\[ent\\]"
syntax region gpString  start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=gpSpecial

"comments
syntax region gpComment	start="/\*"  end="\*/" contains=gpTodo
syntax match  gpComment "\\\\.*" contains=gpTodo
syntax keyword gpTodo contained	TODO
syntax sync ccomment gpComment minlines=10

"catch errors caused by wrong parenthesis
syntax region gpParen		transparent start='(' end=')' contains=ALLBUT,gpParenError,gpTodo,gpFunction,gpArgs,gpSpecial
syntax match gpParenError	")"
syntax match gpInParen contained "[{}]"

if version >= 508 || !exists("did_gp_syn_inits")
  if version < 508
    let did_gp_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink gpConditional		Conditional
  HiLink gpRepeat		Repeat
  HiLink gpError		Error
  HiLink gpParenError		gpError
  HiLink gpInParen		gpError
  HiLink gpStatement		Statement
  HiLink gpString		String
  HiLink gpComment		Comment
  HiLink gpInterface		Type
  HiLink gpInput		Type
  HiLink gpInterfaceKey		Statement
  HiLink gpFunction		Function
  HiLink gpScope		Type
  " contained ones
  HiLink gpSpecial		Special
  HiLink gpTodo			Todo
  HiLink gpArgs			Type
  delcommand HiLink
endif

let b:current_syntax = "gp"
" vim: ts=8
