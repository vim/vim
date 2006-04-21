" Vim filetype plugin file
" Language:         BDF font definition
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms< fo<"

setlocal comments=b:COMMENT commentstring=COMMENT\ %s
setlocal formatoptions-=t formatoptions+=croql
