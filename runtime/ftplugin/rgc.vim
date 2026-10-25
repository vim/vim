" Vim ftplugin file
" Language: RGC
" License: Apache-2.0

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal commentstring=#\ %s
setlocal shiftwidth=4
setlocal softtabstop=4
setlocal tabstop=4
setlocal expandtab

let b:undo_ftplugin = "setlocal commentstring< shiftwidth< softtabstop< tabstop< expandtab<"
