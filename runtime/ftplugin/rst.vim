" Vim filetype plugin file
" Language:	    reStructuredText Documentation Format
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/ftplugin/pcp/rst/
" Latest Revision:  2004-04-25
" arch-tag:	    618bf504-81ba-4518-bad2-43ba2b844a26

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms<"

setlocal comments=fb:..
setlocal commentstring=..\ %s
setlocal expandtab
setlocal sts=2 sw=2

" vim: set sts=2 sw=2:
