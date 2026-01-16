" Roto filetype plugin file
" Language: Roto
" Maintainer: Bendik Samseth <bendik@samseth.me>
" Latest Revision: 2026-01-16

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal comments=:#
setlocal commentstring=#\ %s

let b:undo_ftplugin = "setl com< cms<"

