" Vim filetype plugin file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2007-09-18

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl com< cms< fo<"

setlocal comments=:# commentstring=#\ %s formatoptions-=t formatoptions+=croql

let s:cpo_save = &cpo
set cpo&vim
