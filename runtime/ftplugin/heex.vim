" Elixir filetype plugin
" Language: HEEx
" Maintainer:	Mitchell Hanberg <vimNOSPAM@mitchellhanberg.com>
" Last Change: 2022 September 19

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:save_cpo = &cpo
set cpo&vim

setlocal shiftwidth=2 softtabstop=2 expandtab

setlocal comments=:<%#
setlocal commentstring=<%#\ %s\ %>

let &cpo = s:save_cpo
unlet s:save_cpo
