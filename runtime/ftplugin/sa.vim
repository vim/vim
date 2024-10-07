" Vim filetype plugin file
" Language:	TI linear assembly language


if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

setlocal comments=:;
setlocal commentstring=;\ %s

let b:undo_ftplugin = "setl commentstring< comments<"
