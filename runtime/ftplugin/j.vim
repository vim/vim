" Vim filetype plugin
" Language:	J
" Maintainer:	David Bürgin <676c7473@gmail.com>
" Last Change:	2013-09-21

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal iskeyword=48-57,65-90,_,97-122
setlocal comments=:NB.
setlocal commentstring=NB.\ %s
setlocal formatoptions-=t formatoptions+=croql
setlocal shiftwidth=2 softtabstop=2 expandtab

let b:undo_ftplugin = "setl et< sts< sw< fo< cms< com< isk<"
