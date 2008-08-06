" Vim filetype plugin file
" Language:         Vim help file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2008-07-09

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl fo< tw<"

setlocal formatoptions+=tcroql textwidth=78

let &cpo = s:cpo_save
unlet s:cpo_save
