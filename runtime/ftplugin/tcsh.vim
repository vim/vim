" Vim filetype plugin file
" Language:	tcsh
" Maintainer:	Dan Sharp <dwsharp at hotmail dot com>
" Last Changed: 2003 Sep 29
" URL:		http://mywebpage.netscape.com/sharppeople/vim/ftplugin

if exists("b:did_ftplugin") | finish | endif

" Make sure the continuation lines below do not cause problems in
" compatibility mode.
let s:save_cpo = &cpo
set cpo-=C

" Define some defaults in case the included ftplugins don't set them.
let s:undo_ftplugin = ""
let s:browsefilter = "csh Files (*.csh)\t*.csh\n" .
	    \	     "All Files (*.*)\t*.*\n"

runtime! ftplugin/csh.vim ftplugin/csh_*.vim ftplugin/csh/*.vim
let b:did_ftplugin = 1

" Override our defaults if these were set by an included ftplugin.
if exists("b:undo_ftplugin")
    let s:undo_ftplugin = b:undo_ftplugin
endif
if exists("b:browsefilter")
    let s:browsefilter = b:browsefilter
endif

" Change the :browse e filter to primarily show tcsh-related files.
if has("gui_win32")
    let  b:browsefilter="tcsh Scripts (*.tcsh)\t*.tcsh\n" . s:browsefilter
endif

" Undo the stuff we changed.
let b:undo_ftplugin = "unlet! b:browsefilter | " . s:undo_ftplugin

" Restore the saved compatibility options.
let &cpo = s:save_cpo
