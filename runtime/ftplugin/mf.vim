" Vim filetype plugin
" Language:	METAFONT
" Maintainer:	Dorai Sitaram <ds26@gte.com>
" URL:		http://www.ccs.neu.edu/~dorai/vimplugins/vimplugins.html
" Last Change:	May 27, 2003

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

setl com=:%
setl fo-=t
