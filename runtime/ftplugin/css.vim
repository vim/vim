" Vim filetype plugin file
" Language:         CSS
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms< inc<"

setlocal comments=s1:/*,mb:*,ex:*/ commentstring&

let &l:include = '^\s*@import\s\+\%(url(\)\='
