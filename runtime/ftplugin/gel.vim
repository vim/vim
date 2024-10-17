" Vim filetype plugin file
" Language:	TI DSP gel

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,:///,://
setlocal commentstring=/*\ %s\ */

let b:undo_ftplugin = "setl commentstring< comments<"
