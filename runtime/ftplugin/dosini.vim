" Vim filetype plugin file
" Language:         Configuration File (ini file) for MSDOS/MS Windows
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2007-08-23

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms< fo<"

setlocal comments=:; commentstring=;\ %s formatoptions-=t formatoptions+=croql
