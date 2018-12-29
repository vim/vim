" Vim filetype plugin file
" Language:             Vim help file
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2018-12-29

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl fo< tw< cole< cocu<"

setlocal formatoptions+=tcroql textwidth=78
if has("conceal")
  setlocal conceallevel=2 'concealcursor'=nc
endif

" Use :help to lookup the keyword under the cursor with K.
setlocal keywordprg=:help

let &cpoptions = s:cpo_save
unlet s:cpo_save
