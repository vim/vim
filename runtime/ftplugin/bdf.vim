" Vim filetype plugin file
" Language:         BDF font definition
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-22

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms<"

setlocal comments=b:COMMENT commentstring=COMMENT\ %s
