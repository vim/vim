" Vim syntax file
" Language:	    Relax NG compact syntax
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/rnc/
" Latest Revision:  2004-05-22
" arch-tag:	    061ee0a2-9efa-4e2a-b1a9-14cf5172d645

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk @,48-57,_,-,.
delcommand SetIsk

" Todo
syn keyword rncTodo	    contained TODO FIXME XXX NOTE

" Comments
syn region  rncComment	    matchgroup=rncComment start='^\s*#' end='$' contains=rncTodo

" Operators
syn match   rncOperator	    '[-|,&+?*~]'
syn match   rncOperator	    '\%(|&\)\=='
syn match   rncOperator	    '>>'

" Namespaces
syn match   rncNamespace    '\<\k\+:'

" Quoted Identifier
syn match   rncQuoted	    '\\\k\+\>'

" Special Characters
syn match   rncSpecial	    '\\x{\x\+}'

" Annotations
syn region Annotation	    transparent start='\[' end='\]' contains=ALLBUT,rncComment,rncTodo

" Literals
syn region  rncLiteral	    matchgroup=rncLiteral oneline start=+"+ end=+"+ contains=rncSpecial
syn region  rncLiteral	    matchgroup=rncLiteral oneline start=+'+ end=+'+
syn region  rncLiteral	    matchgroup=rncLiteral start=+"""+ end=+"""+ contains=rncSpecial
syn region  rncLiteral	    matchgroup=rncLiteral start=+'''+ end=+'''+

" Delimiters
syn match   rncDelimiter    '[{},()]'

" Keywords
syn keyword rncKeyword	    datatypes default div empty external grammar
syn keyword rncKeyword	    include inherit list mixed name namespace
syn keyword rncKeyword	    notAllowed parent start string text token

" Identifiers
syn match   rncIdentifier   '\k\+\_s*\%(=\|&=\||=\)\@=' nextgroup=rncOperator
syn keyword rncKeyword	    nextgroup=rncIdName skipwhite skipempty element attribute
syn match   rncIdentifier   contained '\k\+'

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_rnc_syn_inits")
  if version < 508
    let did_rnc_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink rncTodo	Todo
  HiLink rncComment	Comment
  HiLink rncOperator	Operator
  HiLink rncNamespace	Identifier
  HiLink rncQuoted	Special
  HiLink rncSpecial	SpecialChar
  HiLink rncLiteral	String
  HiLink rncDelimiter	Delimiter
  HiLink rncKeyword	Keyword
  HiLink rncIdentifier	Identifier

  delcommand HiLink
endif

let b:current_syntax = "rnc"

" vim: set sts=2 sw=2:
