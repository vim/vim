" Vim filetype plugin file
" Language:             YAML (YAML Ain't Markup Language)
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2008-07-09

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl com< cms< et< fo<"

setlocal comments=:# commentstring=#\ %s expandtab
setlocal formatoptions-=t formatoptions+=croql

if !exists("g:yaml_recommended_style") || g:yaml_recommended_style != 0
  setlocal shiftwidth=2 tabstop=2 softtabstop=2 expandtab
endif

let &cpo = s:cpo_save
unlet s:cpo_save
