" Vim syntax file
" Language:	    YAML (YAML Ain't Markup Language)
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/yaml/
" Latest Revision:  2004-05-22
" arch-tag:	    01bf8ef1-335f-4692-a228-4846cb64cd16

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Todo
syn keyword yamlTodo	contained TODO FIXME XXX NOTE

" Comments (4.2.2)
syn region  yamlComment	matchgroup=yamlComment start='\%(^\|\s\)#' end='$' contains=yamlTodo

" Node Properties (4.3.4)
syn match   yamlNodeProperty	'!\%(![^\\^%	 ]\+\|[^!][^:/	 ]*\)'

" Anchors (4.3.6)
syn match   yamlAnchor	'&.\+'

" Aliases (4.3.7)
syn match   yamlAlias	'\*.\+'

" Operators, Blocks, Keys, and Delimiters
syn match   yamlDelimiter   '[-,:]'
syn match   yamlBlock	    '[\[\]{}>|]'
syn match   yamlOperator    '[?+-]'
syn match   yamlKey	    '\w\+\(\s\+\w\+\)*\ze\s*:'

" Strings (4.6.8, 4.6.9)
syn region  yamlString	start=+"+ skip=+\\"+ end=+"+ contains=yamlEscape
syn region  yamlString	start=+'+ skip=+''+ end=+'+ contains=yamlSingleEscape
syn match   yamlEscape	contained +\\[\\"abefnrtv^0_ NLP]+
syn match   yamlEscape	contained '\\x\x\{2}'
syn match   yamlEscape	contained '\\u\x\{4}'
syn match   yamlEscape	contained '\\U\x\{8}'
" TODO: how do we get 0x85, 0x2028, and 0x2029 into this?
syn match   yamlEscape	'\\\%(\r\n\|[\r\n]\)'
syn match   yamlSingleEscape contained +''+

" Numbers
" TODO: sexagecimal and fixed (20:30.15 and 1,230.15)
syn match   yamlNumber	'\<[+-]\=\d\+\%(\.\d\+\%([eE][+-]\=\d\+\)\=\)\='
syn match   yamlNumber	'0\o\+'
syn match   yamlNumber	'0x\x\+'
syn match   yamlNumber	'([+-]\=[iI]nf)'
syn match   yamlNumber	'(NaN)'

" Constants
syn match   yamlConstant    '\<[~yn]\>'
syn keyword yamlConstant    true True TRUE false False FALSE
syn keyword yamlConstant    yes Yes on ON no No off OFF
syn keyword yamlConstant    null Null NULL nil Nil NIL

" Timestamps
syn match   yamlTimestamp   '\d\d\d\d-\%(1[0-2]\|\d\)-\%(3[0-2]\|2\d\|1\d\|\d\)\%( \%([01]\d\|2[0-3]\):[0-5]\d:[0-5]\d.\d\d [+-]\%([01]\d\|2[0-3]\):[0-5]\d\|t\%([01]\d\|2[0-3]\):[0-5]\d:[0-5]\d.\d\d[+-]\%([01]\d\|2[0-3]\):[0-5]\d\|T\%([01]\d\|2[0-3]\):[0-5]\d:[0-5]\d.\dZ\)\='

" Documents (4.3.1)
syn region  yamlDocumentHeader	start='---' end='$' contains=yamlDirective
syn match   yamlDocumentEnd	'\.\.\.'

" Directives (4.3.2)
syn match   yamlDirective   contained '%[^:]\+:.\+'

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_yaml_syn_inits")
  if version < 508
    let did_yaml_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink yamlTodo	    Todo
  HiLink yamlComment	    Comment
  HiLink yamlDocumentHeader PreProc
  HiLink yamlDocumentEnd    PreProc
  HiLink yamlDirective	    Keyword
  HiLink yamlNodeProperty   Type
  HiLink yamlAnchor	    Type
  HiLink yamlAlias	    Type
  HiLink yamlDelimiter	    Delimiter
  HiLink yamlBlock	    Operator
  HiLink yamlOperator	    Operator
  HiLink yamlKey	    Identifier
  HiLink yamlString	    String
  HiLink yamlEscape	    SpecialChar
  HiLink yamlSingleEscape   SpecialChar
  HiLink yamlNumber	    Number
  HiLink yamlConstant	    Constant
  HiLink yamlTimestamp	    Number

  delcommand HiLink
endif

let b:current_syntax = "yaml"

" vim: set sts=2 sw=2:
