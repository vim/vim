" Vim filetype plugin file
" Language:	OpenCL
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2024 Oct 22
" Maintainer:	Wu, Zhenyu <wuzhenyu@ustc.edu>

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,:///,://
setlocal commentstring=/*\ %s\ */ define& include&

let b:undo_ftplugin = "setl commentstring< comments<"
