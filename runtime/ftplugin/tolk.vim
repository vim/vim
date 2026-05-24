" Vim filetype plugin file
" Language:     Tolk
" Maintainer:   redavy <hello.redavy@proton.me>
" Upstream:     https://github.com/redavy/vim-tolk
" Last Update:  24 May 2026

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal commentstring=//\ %s

let b:undo_ftplugin = "setlocal commentstring<"
