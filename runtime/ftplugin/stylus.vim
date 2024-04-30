" Vim filetype plugin
" Language: Stylus
" Maintainer: Marc Harter
" Credits: Tim Pope

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

let s:save_cpo = &cpo
set cpo-=C

" Define some defaults in case the included ftplugins don't set them.
let s:undo_ftplugin = ""
let s:browsefilter = "All Files (*.*)\t*.*\n"

runtime! ftplugin/html.vim ftplugin/html_*.vim ftplugin/html/*.vim
unlet! b:did_ftplugin

" Override our defaults if these were set by an included ftplugin.
if exists("b:undo_ftplugin")
  let s:undo_ftplugin = b:undo_ftplugin
  unlet b:undo_ftplugin
endif
if exists("b:browsefilter")
  let s:browsefilter = b:browsefilter
  unlet b:browsefilter
endif
  let s:match_words = b:match_words

" Change the browse dialog on Win32 to show mainly Styl-related files
if has("gui_win32")
  let b:browsefilter="Stylus Files (*.styl)\t*.styl\n" . s:browsefilter
endif

setlocal comments= commentstring=//\ %s

setlocal suffixesadd=.styl

" Add '-' and '#' to the what makes up a keyword.
" This means that 'e' and 'w' work properly now, for properties
" and valid variable names.
setl iskeyword+=#,-

" Add a Stylus command (to see if it's valid)
:au! Stylus !clear; cat % | stylus

setlocal formatoptions+=r

let b:undo_ftplugin = "setl suf< isk< for< cms< com< "
      \ " | unlet! b:browsefilter b:match_words | " . s:undo_ftplugin

let &cpo = s:save_cpo

" vim:set sw=2:
