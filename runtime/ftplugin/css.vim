" Vim filetype plugin file
" Language:	    CSS
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/ftplugin/pcp/css/
" Latest Revision:  2004-04-25
" arch-tag:	    5fa7c74f-bf1a-47c4-b06f-6efe8f48db3b

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com<"

setlocal comments=s1:/*,mb:*,ex:*/

" vim: set sts=2 sw=2:
