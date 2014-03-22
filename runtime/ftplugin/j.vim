" Vim filetype plugin
" Language:	J
" Maintainer:	David Bürgin <676c7473@gmail.com>
" URL:		https://github.com/glts/vim-j
" Last Change:	2014-03-17

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

setlocal iskeyword=48-57,65-90,_,97-122
setlocal comments=:NB.
setlocal commentstring=NB.\ %s
setlocal formatoptions-=t
setlocal shiftwidth=2 softtabstop=2 expandtab
setlocal matchpairs=(:)

let b:undo_ftplugin = 'setlocal matchpairs< expandtab< softtabstop< shiftwidth< formatoptions< commentstring< comments< iskeyword<'
