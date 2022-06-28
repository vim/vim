" Vim filetype plugin file
" Language:	vue

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Copied from ftplugin/html.vim
" Original thanks to Johannes Zellner and Benji Fisher.
if exists("loaded_matchit")
    let b:match_ignorecase = 1
    let b:match_words = '<:>,' .
    \ '<\@<=[ou]l\>[^>]*\%(>\|$\):<\@<=li\>:<\@<=/[ou]l>,' .
    \ '<\@<=dl\>[^>]*\%(>\|$\):<\@<=d[td]\>:<\@<=/dl>,' .
    \ '<\@<=\([^/][^ \t>]*\)[^>]*\%(>\|$\):<\@<=/\1>'
endif
