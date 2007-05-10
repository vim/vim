" Vim filetype plugin file
" Language:         CSS
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2007-05-08

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms< inc< fo< ofu<"

setlocal comments=s1:/*,mb:*,ex:*/ commentstring&
setlocal formatoptions-=t formatoptions+=croql
setlocal omnifunc=csscomplete#CompleteCSS

let &l:include = '^\s*@import\s\+\%(url(\)\='
