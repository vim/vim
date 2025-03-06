" Vim filetype plugin file
" Language:             tera
" Previous Maintainer:  Muntasir Mahmud <muntasir.joypurhat@gmail.com>
" Latest Revision:      2025-03-06

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal commentstring={#\ %s\ #}

let b:undo_ftplugin = "setlocal commentstring<"
