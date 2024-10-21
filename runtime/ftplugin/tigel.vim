" Vim filetype plugin file
" Language:	TI DSP gel
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2024 Oct 22

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,:///,://
setlocal commentstring=/*\ %s\ */

let b:undo_ftplugin = "setl commentstring< comments<"
