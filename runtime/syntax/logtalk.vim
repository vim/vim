" Vim syntax file
"
" Language:	Logtalk
" Maintainer:	Paulo Moura <pmoura@logtalk.org>
" Last Change:	March 12, 2005


" Quit when a syntax file was already loaded:

if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif


" Logtalk is case sensitive:

syn case match


" Logtalk variables

syn match   logtalkVariable		"\<\(\u\|_\)\(\w\)*\>"


" Logtalk clause functor

syn match	logtalkOperator		":-"


" Logtalk quoted atoms and strings

syn region	logtalkString		start=+"+	skip=+\\"+	end=+"+
syn region	logtalkAtom		start=+'+	skip=+\\'+	end=+'+


" Logtalk message sending operators

syn match	logtalkOperator		"::"
syn match	logtalkOperator		"\^\^"


" Logtalk external call

syn region	logtalkExtCall		matchgroup=logtalkExtCallTag		start="{"		matchgroup=logtalkExtCallTag		end="}"		contains=ALL


" Logtalk opening entity directives

syn region	logtalkOpenEntityDir	matchgroup=logtalkOpenEntityDirTag	start=":- object("	matchgroup=logtalkOpenEntityDirTag	end=")\."	contains=ALL
syn region	logtalkOpenEntityDir	matchgroup=logtalkOpenEntityDirTag	start=":- protocol("	matchgroup=logtalkOpenEntityDirTag	end=")\."	contains=ALL
syn region	logtalkOpenEntityDir	matchgroup=logtalkOpenEntityDirTag	start=":- category("	matchgroup=logtalkOpenEntityDirTag	end=")\."	contains=ALL


" Logtalk closing entity directives

syn match	logtalkCloseEntityDir	":- end_\(object\|protocol\|category\)\."


" Logtalk entity relations

syn region	logtalkEntityRel	matchgroup=logtalkEntityRelTag	start="instantiates("	matchgroup=logtalkEntityRelTag	end=")"		contains=logtalkEntity		contained
syn region	logtalkEntityRel	matchgroup=logtalkEntityRelTag	start="specializes("	matchgroup=logtalkEntityRelTag	end=")"		contains=logtalkEntity		contained
syn region	logtalkEntityRel	matchgroup=logtalkEntityRelTag	start="extends("	matchgroup=logtalkEntityRelTag	end=")"		contains=logtalkEntity		contained
syn region	logtalkEntityRel	matchgroup=logtalkEntityRelTag	start="imports("	matchgroup=logtalkEntityRelTag	end=")"		contains=logtalkEntity		contained
syn region	logtalkEntityRel	matchgroup=logtalkEntityRelTag	start="implements("	matchgroup=logtalkEntityRelTag	end=")"		contains=logtalkEntity		contained


" Logtalk directives

syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- alias("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- initialization("	matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- info("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- mode("		matchgroup=logtalkDirTag	end=")\."	contains=logtalkOperator, logtalkAtom
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- dynamic("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn match	logtalkDirTag		":- dynamic\."
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- discontiguous("	matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- multifile("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- public("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- protected("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- private("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- metapredicate("	matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- op("			matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- calls("		matchgroup=logtalkDirTag	end=")\."	contains=ALL
syn region	logtalkDir		matchgroup=logtalkDirTag	start=":- uses("		matchgroup=logtalkDirTag	end=")\."	contains=ALL


" Logtalk built-in predicates

syn match	logtalkBuiltIn		"\<current_\(object\|protocol\|category\)\ze("

syn match	logtalkBuiltIn		"\<create_\(object\|protocol\|category\)\ze("

syn match	logtalkBuiltIn		"\<\(object\|protocol\|category\)_property\ze("

syn match	logtalkBuiltIn		"\<abolish_\(object\|protocol\|category\)\ze("

syn match	logtalkBuiltIn		"\<extends_\(object\|protocol\)\ze("
syn match	logtalkBuiltIn		"\<imp\(orts_category\|lements_protocol\)\ze("
syn match	logtalkBuiltIn		"\<\(instantiates\|specializes\)_class\ze("

syn match	logtalkBuiltIn		"\<\(abolish\|define\)_events\ze("
syn match	logtalkBuiltIn		"\<current_event\ze("

syn match	logtalkBuiltIn		"\<\(current\|set\)_logtalk_flag\ze("

syn match	logtalkBuiltIn		"\<logtalk_\(compile\|l\(ibrary_path\|oad\)\)\ze("

syn match	logtalkBuiltIn		"\<\(for\|retract\)all\ze("


" Logtalk built-in methods

syn match	logtalkBuiltInMethod	"\<parameter\ze("
syn match	logtalkBuiltInMethod	"\<se\(lf\|nder\)\ze("
syn match	logtalkBuiltInMethod	"\<this\ze("

syn match	logtalkBuiltInMethod	"\<current_predicate\ze("
syn match	logtalkBuiltInMethod	"\<predicate_property\ze("

syn match	logtalkBuiltInMethod	"\<abolish\ze("
syn match	logtalkBuiltInMethod	"\<assert\(a\|z\)\ze("
syn match	logtalkBuiltInMethod	"\<clause\ze("
syn match	logtalkBuiltInMethod	"\<retract\ze("
syn match	logtalkBuiltInMethod	"\<retractall\ze("

syn match	logtalkBuiltInMethod	"\<\(bag\|set\)of\ze("
syn match	logtalkBuiltInMethod	"\<f\(ind\|or\)all\ze("

syn match	logtalkBuiltInMethod	"\<before\ze("
syn match	logtalkBuiltInMethod	"\<after\ze("

syn match	logtalkBuiltInMethod	"\<expand_term\ze("
syn match	logtalkBuiltInMethod	"\<phrase\ze("


" Mode operators

syn match	logtalkOperator		"?"
syn match	logtalkOperator		"@"


" Control constructs

syn match	logtalkKeyword		"\<true\>"
syn match	logtalkKeyword		"\<fail\>"
syn match	logtalkKeyword		"\<ca\(ll\|tch\)\ze("
syn match	logtalkOperator		"!"
" syn match	logtalkOperator		","
syn match	logtalkOperator		";"
syn match	logtalkOperator		"-->"
syn match	logtalkOperator		"->"
syn match	logtalkKeyword		"\<throw\ze("


" Term unification

syn match	logtalkOperator		"="
syn match	logtalkKeyword		"\<unify_with_occurs_check\ze("
syn match	logtalkOperator		"\\="


" Term testing

syn match	logtalkKeyword		"\<var\ze("
syn match	logtalkKeyword		"\<atom\ze("
syn match	logtalkKeyword		"\<integer\ze("
syn match	logtalkKeyword		"\<float\ze("
syn match	logtalkKeyword		"\<atomic\ze("
syn match	logtalkKeyword		"\<compound\ze("
syn match	logtalkKeyword		"\<n\(onvar\|umber\)\ze("


" Term comparison

syn match	logtalkOperator		"@=<"
syn match	logtalkOperator		"=="
syn match	logtalkOperator		"\\=="
syn match	logtalkOperator		"@<"
syn match	logtalkOperator		"@>"
syn match	logtalkOperator		"@>="


" Term creation and decomposition

syn match	logtalkKeyword		"\<functor\ze("
syn match	logtalkKeyword		"\<arg\ze("
syn match	logtalkOperator		"=\.\."
syn match	logtalkKeyword		"\<copy_term\ze("


" Arithemtic evaluation

syn match	logtalkOperator		"\<is\>"


" Arithemtic comparison

syn match	logtalkOperator		"=:="
syn match	logtalkOperator		"=\\="
syn match	logtalkOperator		"<"
syn match	logtalkOperator		"=<"
syn match	logtalkOperator		">"
syn match	logtalkOperator		">="


" Stream selection and control

syn match	logtalkKeyword		"\<current_\(input\|output\)\ze("
syn match	logtalkKeyword		"\<set_\(input\|output\)\ze("
syn match	logtalkKeyword		"\<open\ze("
syn match	logtalkKeyword		"\<close\ze("
syn match	logtalkKeyword		"\<flush_output\ze("
syn match	logtalkKeyword		"\<flush_output\>"
syn match	logtalkKeyword		"\<stream_property\ze("
syn match	logtalkKeyword		"\<at_end_of_stream\ze("
syn match	logtalkKeyword		"\<at_end_of_stream\>"
syn match	logtalkKeyword		"\<set_stream_position\ze("


" Character and byte input/output

syn match	logtalkKeyword		"\<\(get\|peek\|put\)_\(char\|code\|byte\)\ze("
syn match	logtalkKeyword		"\<nl\ze("
syn match	logtalkKeyword		"\<nl\>"


" Term input/output

syn match	logtalkKeyword		"\<read_term\ze("
syn match	logtalkKeyword		"\<read\ze("
syn match	logtalkKeyword		"\<write\ze("
syn match	logtalkKeyword		"\<writeq\ze("
syn match	logtalkKeyword		"\<write_\(canonical\|term\)\ze("
syn match	logtalkKeyword		"\<op\ze("
syn match	logtalkKeyword		"\<current_op\ze("
syn match	logtalkKeyword		"\<char_conversion\ze("
syn match	logtalkKeyword		"\<current_char_conversion\ze("


" Logic and control

syn match	logtalkOperator		"\\+"
syn match	logtalkKeyword		"\<once\ze("
syn match	logtalkKeyword		"\<repeat\>"


" Atomic term processing

syn match	logtalkKeyword		"\<atom_\(length\|concat\|chars\|codes\)\ze("
syn match	logtalkKeyword		"\<sub_atom\ze("
syn match	logtalkKeyword		"\<char_code\ze("
syn match	logtalkKeyword		"\<number_\(chars\|codes\)\ze("


" Implementation defined hooks functions

syn match	logtalkKeyword		"\<\(current\|set\)_prolog_flag\ze("
syn match	logtalkKeyword		"\<halt\ze("
syn match	logtalkKeyword		"\<halt\>"


" Evaluable functors

syn match	logtalkOperator		"+"
syn match	logtalkOperator		"-"
syn match	logtalkOperator		"\*"
syn match	logtalkOperator		"//"
syn match	logtalkOperator		"/"
syn match	logtalkKeyword		"\<rem(?=[(])"
syn match	logtalkKeyword		"\<rem\>"
syn match	logtalkKeyword		"\<mod\ze("
syn match	logtalkKeyword		"\<mod\>"
syn match	logtalkKeyword		"\<abs\ze("
syn match	logtalkKeyword		"\<sign\ze("
syn match	logtalkKeyword		"\<float_\(integer\|fractional\)_part\ze("
syn match	logtalkKeyword		"\<float\ze("
syn match	logtalkKeyword		"\<floor\ze("
syn match	logtalkKeyword		"\<truncate\ze("
syn match	logtalkKeyword		"\<round\ze("
syn match	logtalkKeyword		"\<ceiling\ze("


" Other arithemtic functors

syn match	logtalkOperator		"\*\*"
syn match	logtalkKeyword		"\<s\(in\|qrt\)\ze("
syn match	logtalkKeyword		"\<cos\ze("
syn match	logtalkKeyword		"\<atan\ze("
syn match	logtalkKeyword		"\<exp\ze("
syn match	logtalkKeyword		"\<log\ze("


" Bitwise functors

syn match	logtalkOperator		">>"
syn match	logtalkOperator		"<<"
syn match	logtalkOperator		"/\\"
syn match	logtalkOperator		"\\/"
syn match	logtalkOperator		"\\"


" Logtalk end-of-clause

syn match	logtalkOperator		"\."


" Logtalk list operator

syn match	logtalkOperator		"|"


" Logtalk comments

syn region	logtalkBlockComment	start="/\*"	end="\*/"
syn match	logtalkLineComment	"%.*"


" Logtalk numbers

syn match	logtalkNumber		"\<[0-9]\+\>"
syn match	logtalkNumber		"\<[0-9]\+\.[0-9]\+\>"
syn match	logtalkNumber		"\<[0-9]\+\.[0-9]\+[eE][-+][0-9]+\>"
syn match	logtalkNumber		"\<0'[0-9a-zA-Z]\>"
syn match	logtalkNumber		"\<0b[0-1]\+\>"
syn match	logtalkNumber		"\<0o[0-7]\+\>"
syn match	logtalkNumber		"\<0x[0-9a-fA-F]\+\>"


syn sync ccomment logtalkBlockComment maxlines=50


" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet

if version >= 508 || !exists("did_logtalk_syn_inits")
	if version < 508
		let did_logtalk_syn_inits = 1
		command -nargs=+ HiLink hi link <args>
	else
		command -nargs=+ HiLink hi def link <args>
	endif

	HiLink	logtalkBlockComment	Comment
	HiLink	logtalkLineComment	Comment

	HiLink	logtalkOpenEntityDir	Normal
	HiLink	logtalkOpenEntityDirTag	PreProc

	HiLink	logtalkEntity		Normal

	HiLink	logtalkEntityRel	Normal
	HiLink	logtalkEntityRelTag	PreProc

	HiLink	logtalkCloseEntityDir	PreProc

	HiLink	logtalkDir		Normal
	HiLink	logtalkDirTag		PreProc

	HiLink	logtalkAtom		String
	HiLink	logtalkString		String

	HiLink	logtalkNumber		Number

	HiLink	logtalkKeyword		Keyword

	HiLink	logtalkBuiltIn		Keyword
	HiLink	logtalkBuiltInMethod	Keyword

	HiLink	logtalkOperator		Operator

	HiLink	logtalkExtCall		Normal
	HiLink	logtalkExtCallTag	Operator

	HiLink	logtalkVariable		Identifier

	delcommand HiLink

endif


let b:current_syntax = "logtalk"

setlocal ts=4
