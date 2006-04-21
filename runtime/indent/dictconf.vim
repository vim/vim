" Vim indent file
" Language:         dict(1) configuration file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentkeys=0{,0},!^F,o,O cinwords= autoindent smartindent
inoremap <buffer> # X#
