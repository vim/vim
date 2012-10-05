" Vim syntax file
" Language:	dts/dtsi (device tree files)
" Maintainer:	Daniel Mack <vim@zonque.org>
" Last Change:	2012 Oct 04

if exists("b:current_syntax")
  finish
endif

syntax region dtsComment        start="/\*"  end="\*/"
syntax match  dtsReference      "&[[:alpha:][:digit:]_]\+"
syntax region dtsBinaryProperty start="\[" end="\]" 
syntax match  dtsStringProperty "\".*\""
syntax match  dtsKeyword        "/.*/"
syntax match  dtsLabel          "^[[:space:]]*[[:alpha:][:digit:]_]\+:"
syntax region dtsCellProperty   start="<" end=">" contains=dtsReference,dtsBinaryProperty,dtsStringProperty,dtsComment
syntax region dtsCommentInner   start="/\*"  end="\*/"

hi def link dtsCellProperty     Number
hi def link dtsBinaryProperty   Number
hi def link dtsStringProperty   String
hi def link dtsKeyword          Include
hi def link dtsLabel            Label
hi def link dtsReference        Macro
hi def link dtsComment          Comment
hi def link dtsCommentInner     Comment 
