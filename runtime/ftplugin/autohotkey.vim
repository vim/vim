" Vim filetype plugin file
" Language:     AutoHotkey
" Maintainer:   Peter Aronoff <peteraronoff@fastmail.com>
" Last Changed: 2024 Jul 25
" 2025 Sep 26 by Vim Project: remove nowrap modeline (#18399)

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal comments=:;
setlocal commentstring=;\ %s

let b:undo_ftplugin = "setlocal comments< commentstring<"

" vim: sw=2 sts=2 ts=8 noet:
