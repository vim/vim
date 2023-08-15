" Vim filetype plugin file
" Language: qml
" Last change: 2014 Feb 8

if exists( 'b:did_ftplugin' )
   finish
endif
let b:did_ftplugin = 1

let s:cpoptions_save = &cpoptions
set cpoptions&vim

" command for undo
let b:undo_ftplugin =
   \ 'let b:browsefilter = "" | ' .
   \ 'setlocal ' .
   \    'comments< '.
   \    'commentstring< ' .
   \    'formatoptions< ' .
   \    'indentexpr<'

if has( 'gui_win32' )
\ && !exists( 'b:browsefilter' )
   let b:browsefilter =
      \ 'qml files (*.qml)\t*.qml\n' .
      \ 'All files (*.*)\t*.*\n'
endif

" Set 'comments' to format dashed lists in comments.
setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,://
setlocal commentstring=//%s

setlocal formatoptions-=t
setlocal formatoptions+=croql

let &cpoptions = s:cpoptions_save
unlet s:cpoptions_save