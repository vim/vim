" Vim ftplugin file
" Language:	Justfile
" Maintainer:	Noah Bogart <noah.bogart@hey.com>
" URL:		https://github.com/NoahTheDuke/vim-just.git
" Last Change:	2023 Jul 08

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal iskeyword+=-
setlocal comments=n:#
setlocal commentstring=#\ %s

let b:undo_ftplugin = "setlocal iskeyword< comments< commentstring<"
