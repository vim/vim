" Vim filetype plugin file
" Language:	gdb
" Maintainer:	Michaël Peeters <NOSPAMm.vim@noekeon.org>
" Last Changed: 21 Oct 2017

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Make sure the continuation lines below do not cause problems in
" compatibility mode.
" let s:save_cpo = &cpo
" set cpo-=C

setlocal commentstring=#%s

" Undo the stuff we changed.
let b:undo_ftplugin = "setlocal cms<"

" Restore the saved compatibility options.
" let &cpo = s:save_cpo
" unlet s:save_cpo
