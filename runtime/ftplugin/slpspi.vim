" Vim filetype plugin file
" Language:         RFC 2614 - An API for Service Location SPI file
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-07-04

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms< fo<"

setlocal comments=:#,:; commentstring=#\ %s
setlocal formatoptions-=t formatoptions+=croql
