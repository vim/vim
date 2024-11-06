" Vim ftplugin file
" Language:	Ipkg
" Last Change:	2024 Nov 05
" Maintainer:	Serhii Khoma <srghma@gmail.com>
" License:      Vim (see :h license)
" Repository:   https://github.com/ShinKage/idris2-nvim

if exists("b:did_ftplugin")
  finish
endif

setlocal comments=:--
setlocal commentstring=--\ %s
setlocal wildignore+=*.ibc

let b:undo_ftplugin = "
      \ setlocal shiftwidth< tabstop< expandtab<
      \ comments< commentstring< iskeyword< wildignore<
      \"

let b:did_ftplugin = 1
