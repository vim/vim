" Vim filetype plugin
" Language:	J
" Maintainer:	David Bürgin <676c7473@gmail.com>
" Last Change:	2013-10-06

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal iskeyword=48-57,65-90,_,97-122
setlocal comments=:NB.
setlocal commentstring=NB.\ %s
setlocal formatoptions-=t
setlocal shiftwidth=2 softtabstop=2 expandtab
setlocal matchpairs=(:)

let b:undo_ftplugin = "setl mps< et< sts< sw< fo< cms< com< isk<"
