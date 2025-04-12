" Vim filetype plugin file
" Language:            Gleam
" Maintainer:          Kirill Morozov <kirill@robotix.pro>
" Previous Maintainer: Trilowy (https://github.com/trilowy)
" Last Change:         2025-04-12

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

setlocal comments=://,:///,:////
setlocal commentstring=//\ %s
setlocal formatprg=gleam\ format\ --stdin

let b:undo_ftplugin = "setlocal comments< commentstring< formatprg<"

" vim: sw=2 sts=2 et
