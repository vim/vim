vim9script

# Vim syntax file
# Language: Odin
# Maintainer: Maxim Kim <habamax@gmail.com>
# Website: https://github.com/habamax/vim-odin
# Last Change: 2026-06-01

if exists("b:current_syntax")
  finish
endif

syntax keyword odinKeyword using transmute cast auto_cast distinct where dynamic
syntax keyword odinKeyword struct enum union matrix bit_field bit_set
syntax keyword odinKeyword package proc map import foreign typeid
syntax keyword odinKeyword return defer
syntax keyword odinKeyword asm context

syntax keyword odinConditional if when else do for switch case fallthrough
syntax keyword odinConditional continue break
syntax keyword odinConditional or_continue or_break or_return or_else

syntax keyword odinType bool b8 b16 b32 b64
syntax keyword odinType int i8 i16 i32 i64 i128
syntax keyword odinType uint u8 u16 u32 u64 u128 uintptr
syntax keyword odinType i16le i32le i64le i128le u16le u32le u64le u128le
syntax keyword odinType i16be i32be i64be i128be u16be u32be u64be u128be
syntax keyword odinType f16 f32 f64
syntax keyword odinType f16le f32le f64le
syntax keyword odinType f16be f32be f64be
syntax keyword odinType complex32 complex64 complex128
syntax keyword odinType quaternion64 quaternion128 quaternion256
syntax keyword odinType rune
syntax keyword odinType string cstring
syntax keyword odinType rawptr
syntax keyword odinType any

syntax keyword odinBool true false
syntax keyword odinNull nil
syntax match odinUninitialized '\s\+---\(\s\|$\)'

syntax keyword odinOperator in not_in
syntax match odinOperator "?" display
syntax match odinOperator "->" display

syntax match odinTodo "TODO" contained
syntax match odinTodo "XXX" contained
syntax match odinTodo "FIXME" contained
syntax match odinTodo "HACK" contained

syntax region odinRawString start=+`+ end=+`+
syntax region odinChar start=+'+ skip=+\\\\\|\\'+ end=+'+
syntax region odinString start=+"+ skip=+\\\\\|\\'+ end=+"+ contains=odinEscape
syntax match odinEscape display contained /\\\([abefnrtv\\'"]\|\o\{3}\|x\x\{2}\|u\x\{4}\|U\x\{8}\)/

syntax match odinProcedure "\v<\w*>(\s*::\s*proc)@="

syntax match odinAttribute "@\ze\<\w\+\>" display
syntax region odinAttribute
      \ matchgroup=odinAttribute
      \ start="@\ze(" end="\ze)"
      \ transparent oneline

syntax match odinInteger "\v-?<[0-9]+%(_[0-9]+)*>" display
syntax match odinFloat "\v-?<[0-9]+%(_[0-9]+)*%(\.[0-9]+%(_[0-9]+)*)%([eE][+-]=[0-9]+%(_[0-9]+)*)=" display
syntax match odinHex "\v<0[xX][0-9A-Fa-f]+%(_[0-9A-Fa-f]+)*>" display
syntax match odinDoz "\v<0[zZ][0-9A-Ba-b]+%(_[0-9A-Ba-b]+)*>" display
syntax match odinOct "\v<0[oO][0-7]+%(_[0-7]+)*>" display
syntax match odinBin "\v<0[bB][01]+%(_[01]+)*>" display

syntax match odinAddressOf "&" display
syntax match odinDeref "\^" display

syntax match odinMacro "#\<\w\+\>" display
syntax region odinFeature matchgroup=odinMacro start="#+\<\w\+\>" end="$" oneline display

syntax match odinTemplate "$\<\w\+\>"

syntax region odinLineComment start=/\/\// end=/$/  contains=@Spell,odinTodo
syntax region odinBlockComment start=/\/\*/ end=/\*\// contains=@Spell,odinTodo,odinBlockComment
syn sync ccomment odinBlockComment

highlight def link odinKeyword Statement
highlight def link odinConditional Conditional
highlight def link odinOperator Operator

highlight def link odinString String
highlight def link odinRawString String
highlight def link odinChar Character
highlight def link odinEscape Special

highlight def link odinProcedure Function

highlight def link odinMacro PreProc

highlight def link odinLineComment Comment
highlight def link odinBlockComment Comment

highlight def link odinTodo Todo

highlight def link odinAttribute Statement
highlight def link odinType Type
highlight def link odinBool Boolean
highlight def link odinNull Constant
highlight def link odinUninitialized Constant
highlight def link odinInteger Number
highlight def link odinFloat Float
highlight def link odinHex Number
highlight def link odinOct Number
highlight def link odinBin Number
highlight def link odinDoz Number

b:current_syntax = "odin"
