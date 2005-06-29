" Vim filetype plugin file
" Language:         login.defs(5) configuration file
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms<"

setlocal comments=:# commentstring=#\ %s
