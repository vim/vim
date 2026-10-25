" Vim syntax file
" Language: RGC
" Maintainer: rgc-lang
" License: Apache-2.0

if exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword rgcKeyword if elif else for in while break continue pass return match case from public
syn keyword rgcDecl def class enum interface import
syn keyword rgcOperatorKeyword and or not
syn keyword rgcSelf self

" Types / constants / builtins
syn keyword rgcType int float bool str
syn keyword rgcConstant True False None
syn keyword rgcBuiltin print range to_string args read_file write_file system len

" Borrow annotations
syn match rgcBorrow "@var\b"
syn match rgcBorrow "@[A-Za-z_][A-Za-z0-9_]*"

" Numbers
syn match rgcNumber "\<0[xX][0-9a-fA-F_]\+\>"
syn match rgcNumber "\<[0-9][0-9_]*\(\.[0-9][0-9_]*\)\?\([eE][+-]\?[0-9]\+\)\?\>"

" Strings
syn region rgcString start=/\v"/ skip=/\v\\./ end=/\v"/

" Comments
syn match rgcComment "\v#.*$" contains=rgcTodo
syn region rgcBlockComment start=/\v\/\*/ end=/\v\*\// contains=rgcTodo

syn keyword rgcTodo TODO FIXME XXX HACK NOTE contained

hi def link rgcKeyword Keyword
hi def link rgcDecl Statement
hi def link rgcOperatorKeyword Operator
hi def link rgcSelf Identifier
hi def link rgcType Type
hi def link rgcConstant Constant
hi def link rgcBuiltin Function
hi def link rgcBorrow PreProc
hi def link rgcNumber Number
hi def link rgcString String
hi def link rgcComment Comment
hi def link rgcBlockComment Comment
hi def link rgcTodo Todo

let b:current_syntax = "rgc"