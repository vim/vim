" Vim filetype plugin file
" Language:	Javascript
" Maintainer:	Bram Moolenaar (for now)
" Last Change:  2006 Jan 30

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

if exists('&ofu')
  setlocal ofu=javascriptcomplete#CompleteJS
endif
