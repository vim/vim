" Vim syntax file
" Language:         YAML (YAML Ain't Markup Language)
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2007-06-27

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword yamlTodo
      \ contained
      \ TODO
      \ FIXME
      \ XXX
      \ NOTE

syn region  yamlComment
      \ display
      \ oneline
      \ start='%(^|s)#'
      \ end='$'
      \ contains=yamlTodo,@Spell

syn match   yamlNodeProperty
      \ '!\%(![^\\^% \t]\+\|[^!][^:/ \t]*\)'

syn match   yamlAnchor
      \ '&.+'

syn match   yamlAlias
      \ '*.+'

syn match   yamlDelimiter
      \ '[-,:]'
syn match   yamlBlock
      \ '[[]{}>|]'
syn match   yamlOperator
      \ '[?+-]'
syn match   yamlKey
      \ 'w+(s+w+)*zes*:'

syn region  yamlString
      \ start=+"+
      \ skip=+\"+
      \ end=+"+
      \ contains=yamlEscape
syn region  yamlString
      \ start=+'+
      \ skip=+''+
      \ end=+'+
      \ contains=yamlSingleEscape
syn match   yamlEscape
      \ contained
      \ display
      \ +\[\"abefnrtv^0_ NLP]+
syn match   yamlEscape
      \ contained
      \ display
      \ '\xx{2}'
syn match   yamlEscape
      \ contained
      \ display
      \ '\ux{4}'
syn match   yamlEscape
      \ contained
      \ display
      \ '\Ux{8}'

" TODO: how do we get 0x85, 0x2028, and 0x2029 into this?
" XXX: Em, what is going on here?  This can't be right.  Leave out until we
" figure out what this is meant to do.
"syn match   yamlEscape
"      \ contained
"      \ display
"      \ '\%(rn|[rn])'
syn match   yamlSingleEscape
      \ contained
      \ display
      \ +''+

" TODO: sexagecimal and fixed (20:30.15 and 1,230.15)
syn match   yamlNumber
      \ display
      \ '\<[+-]\=\d\+\%(\.\d\+\%([eE][+-]\=\d\+\)\=\)\='
syn match   yamlNumber
      \ display
      \ '0o+'
syn match   yamlNumber
      \ display
      \ '0xx+'
syn match   yamlNumber
      \ display
      \ '([+-]=[iI]nf)'
syn match   yamlNumber
      \ display
      \ '(NaN)'

syn match   yamlConstant
      \ '<[~yn]>'
syn keyword yamlConstant
      \ true
      \ True
      \ TRUE
      \ false
      \ False
      \ FALSE
syn keyword yamlConstant
      \ yes
      \ Yes
      \ on
      \ ON
      \ no
      \ No
      \ off
      \ OFF
syn keyword yamlConstant
      \ null
      \ Null
      \ NULL
      \ nil
      \ Nil
      \ NIL

syn match   yamlTimestamp
      \ '\d\d\d\d-\%(1[0-2]\|\d\)-\%(3[0-2]\|2\d\|1\d\|\d\)\%( \%([01]\d\|2[0-3]\):[0-5]\d:[0-5]\d.\d\d [+-]\%([01]\d\|2[0-3]\):[0-5]\d\|t\%([01]\d\|2[0-3]\):[0-5]\d:[0-5]\d.\d\d[+-]\%([01]\d\|2[0-3]\):[0-5]\d\|T\%([01]\d\|2[0-3]\):[0-5]\d:[0-5]\d.\dZ\)\='

syn region  yamlDocumentHeader
      \ start='---'
      \ end='$'
      \ contains=yamlDirective
syn match   yamlDocumentEnd
      \ '\.\.\.'

syn match   yamlDirective
      \ contained
      \ '%[^:]+:.+'

syn match   yamlIndentation
      \ '^s*'
      \ contains=yamlIndentationError

syn match   yamlIndentationError
      \ 't'

hi def link yamlTodo            Todo
hi def link yamlComment         Comment
hi def link yamlDocumentHeader  PreProc
hi def link yamlDocumentEnd     PreProc
hi def link yamlDirective       Keyword
hi def link yamlNodeProperty    Type
hi def link yamlAnchor          Type
hi def link yamlAlias           Type
hi def link yamlDelimiter       Delimiter
hi def link yamlBlock           Operator
hi def link yamlOperator        Operator
hi def link yamlKey             Identifier
hi def link yamlString          String
hi def link yamlEscape          SpecialChar
hi def link yamlSingleEscape    SpecialChar
hi def link yamlNumber          Number
hi def link yamlConstant        Constant
hi def link yamlTimestamp       Number
hi def link yamlIndentationError  Error

let b:current_syntax = "yaml"

let &cpo = s:cpo_save
unlet s:cpo_save
