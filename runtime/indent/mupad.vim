" Vim indent file
" Language:    MuPAD source files
" Maintainer:  Dave Silvia <dsilvia@mchsi.com>
" Filenames:   *.mu
" Date:        6/30/2004

if exists("b:did_indent")
	finish
endif

let b:did_indent = 1

runtime indent/GenericIndent.vim

let b:indentStmts=''
let b:dedentStmts=''
let b:allStmts=''
" NOTE:  b:indentStmts, b:dedentStmts, and b:allStmts need to be initialized
"        to '' before callin the functions because 'indent.vim' explicitly
"        'unlet's b:did_indent.  This means that the lists will compound if
"        you change back and forth between buffers.  This is true as of
"        version 6.3, 6/23/2004.
setlocal indentexpr=GenericIndent()
setlocal indentkeys==end_proc,=then,=else,=elif,=end_if,=end_case,=until,=end_repeat,=end_domain,=end_for,=end_while,=end,o,O

call GenericIndentStmts('begin,if,then,else,elif,case,repeat,until,domain,do')
call GenericDedentStmts('end_proc,then,else,elif,end_if,end_case,until,end_repeat,end_domain,end_for,end_while,end')
call GenericAllStmts()


" TODO:  More comprehensive indentstmt, dedentstmt, and indentkeys values.
"
" BUGS:  You tell me!  Probably.  I just haven't found one yet or haven't been
"        told about one.
"
