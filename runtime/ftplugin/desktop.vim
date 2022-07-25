" Vim filetype plugin file
" Language: XDG desktop entry
" Maintainer: Eisuke Kawashima ( e.kawaschima+vim AT gmail.com )
" Last Change: 2022-07-26

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = v:true
let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = 'setl com< cms<'
setl comments=:#
setl commentstring=#%s

let &cpo = s:cpo_save
unlet s:cpo_save
