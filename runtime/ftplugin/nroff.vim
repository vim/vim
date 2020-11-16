" Vim filetype plugin
" Language:	roff(7)
" Maintainer:	Chris Spiegel <cspiegel@gmail.com>
" Last Change:	2020 Oct 15

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal commentstring=.\\\"%s
setlocal comments=:.\\\"
setlocal sections+=Sh

let b:undo_ftplugin = 'setlocal commentstring< comments< sections<'
