" Vim syntax file
" Language:     Mathematica
" Maintainer:   Wolfgang Waltenberger <wwalten@ben.tuwien.ac.at>
" Last Change:  Thu 26 Apr 2001 13:20:03 CEST

if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

syn match mmaError "\*)"
syn match mmaFixme "FIXME"
syn region mmaComment start=+(\*+ end=+\*)+ skipempty contains=mmaFixme
syn match mmaMessage "\a*::\a*"
syn region mmaString start=+'+    end=+'+
syn region mmaString start=+"+    end=+"+
syn region mmaString start=+\\\"+ end=+\"+
syn region mmaString start=+\"+   end=+\"+

syn match mmaVariable "$\a*"

syn match mmaPattern "[A-Za-z01-9`]*_\{1,3}"
syn match mmaPattern "[A-Za-z01-9`]*_\{1,3}\(Integer\|Real\|Pattern\|Symbol\)"
syn match mmaPattern "[A-Za-z01-9`]*_\{1,3}\(Rational\|Complex\|Head\)"
syn match mmaPattern "[A-Za-z01-9`]*_\{1,3}?[A-Za-z01-9`]*"

" prefix/infix/postfix notations
syn match mmaGenericFunction "[A-Za-z01-9`]*\s*\(\[\|@\)"he=e-1
syn match mmaGenericFunction "[A-Za-z01-9`]*\s*\(/@\|@@\)"he=e-2
syn match mmaGenericFunction "\~\s*[A-Za-z01-9`]*\s*\~"hs=s+1,he=e-1
syn match mmaGenericFunction "//\s*[A-Za-z01-9`]*"hs=s+2
syn match mmaOperator "/;"

syn match mmaPureFunction "#\d*"
syn match mmaPureFunction "&"

syn match mmaUnicode "\\\[[a-zA-Z01-9]*\]"

if version >= 508 || !exists("did_mma_syn_inits")
	if version < 508
		let did_mma_syn_inits = 1
		command -nargs=+ HiLink hi link <args>
	else
		command -nargs=+ HiLink hi def link <args>
	endif

	HiLink mmaOperator	   Operator
	HiLink mmaVariable	   Identifier
	HiLink mmaString	   String
	HiLink mmaUnicode	   String
	HiLink mmaMessage	   Identifier
	HiLink mmaPattern	   Identifier
	HiLink mmaGenericFunction  Function
	HiLink mmaError		   Error
	HiLink mmaFixme		   Error
	HiLink mmaComment	   Comment
	HiLink mmaPureFunction	   Operator

	delcommand HiLink
endif

let b:current_syntax = "mma"
