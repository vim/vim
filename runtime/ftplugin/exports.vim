" Vim filetype plugin
" Language:     exports(5) configuration file
" Maintainer:	Matt Perry <matt@mattperry.com>
" Last Change:	2025 Feb 12

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = 'setl com< cms< fo<'

setlocal comments=:# commentstring=#\ %s
setlocal formatoptions-=t formatoptions+=croql

let &cpo = s:cpo_save
unlet s:cpo_save
