" Vim filetype plugin
" Language:     Asymptote
" Maintainer:	AvidSeeker <avidseeker7@protonmail.com>
" Last Change:  2024 Jul 13
"

if exists("b:did_ftplugin")
  finish
endif
let g:did_ftplugin = 1

setlocal commentstring=/*\ %s\ */
setlocal makeprg=asy\ %
setlocal errorformat=%f:\ %l.%c:\ %m

let b:undo_ftplugin = "setl commentstring< makeprg< errorformat<"
