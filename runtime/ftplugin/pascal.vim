" Vim filetype plugin file
" Language:	pascal
" Maintainer:	Dan Sharp <dwsharp at hotmail dot com>
" Last Changed: 2005 Sep 05
" URL:		http://mywebpage.netscape.com/sharppeople/vim/ftplugin

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

if exists("loaded_matchit")
    let b:match_words='\<\%(begin\|case\|try\)\>:\<end\>'
endif

" Undo the stuff we changed.
let b:undo_ftplugin = "unlet! b:match_words"
