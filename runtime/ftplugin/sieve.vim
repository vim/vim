" Vim filetype plugin file
" Language:         Sieve filtering language input file
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-07-04

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms< fo<"

setlocal comments=s1:/*,mb:*,ex:*/,:# commentstring=#\ %s
setlocal formatoptions-=t formatoptions+=croql
