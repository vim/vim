" Vim indent file
" Language:    Typst
" Previous Maintainer:  Gregory Anders
"                       Luca Saccarola <github.e41mv@aleeas.com>
" Maintainer:  This runtime file is looking for a new maintainer.
" Last Change: 2026 Apr 26
" Based on:    https://github.com/kaarmu/typst.vim

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal autoindent
setlocal indentexpr=typst#indentexpr()

let b:undo_indent = "setl ai< inde<"
