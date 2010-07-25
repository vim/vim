" Vim filetype plugin file
" Language:     Vim's quickfix window
" Maintainer:   Lech Lorens <Lech.Lorens@gmail.com>
" Last Changed: 22 Jul 2010

if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl stl<"

" Display the command that produced the list in the quickfix window:
setlocal stl=%q%{exists('w:quickfix_title')?\ '\ '.w:quickfix_title\ :\ ''}
