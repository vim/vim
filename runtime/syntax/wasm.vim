" Vim syntax file
" Author: @rhysd (www.github.com/rhysd)
" Language: WebAssembly

if exists("b:current_syntax")
  finish
endif

syn cluster wastCluster       contains=wastModuleKeyword,wastInst,wastString,wastNamedVar,wastUnnamedVar,wastFloat,wastNumber,wastComment,wastList
syn keyword wastModuleKeyword module export func contained
syn match   wastInst          "\%((\s*\)\@<=\<[[:alnum:]_.]\+\>" contained display
syn match   wastNamedVar      "$\@<!$[^$][^[:space:])]*" contained display
syn match   wastUnnamedVar    "$$\d\+" contained display
syn region  wastString        start=+"+ skip=+\\\\\|\\"+ end=+"+
syn match   wastFloat         "\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\=" display contained
syn match   wastFloat         "\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>" display contained
syn match   wastFloat         "\d\+e[-+]\=\d\+[fl]\=\>" display contained
syn match   wastNumber        "\<\d\+\>" display contained
syn match   wastNumber        "\<0x\x\+\>" display contained
syn match   wastNumber        "\<0o\o\+\>" display contained
syn region  wastComment       start=";" end="$" display
syn region  wastList          matchgroup=wastListDelimiter start="(" matchgroup=wastListDelimiter end=")" contains=@wastCluster

syn sync lines=100

hi def link wastModuleKeyword PreProc
hi def link wastListDelimiter Delimiter
hi def link wastInst          Statement
hi def link wastString        String
hi def link wastNamedVar      Identifier
hi def link wastUnnamedVar    PreProc
hi def link wastFloat         Float
hi def link wastNumber        Number
hi def link wastComment       Comment

let b:current_syntax = "wast"
