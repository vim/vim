" Vim filetype plugin file
" Language: 		SML
" Filenames:		*.sml *.sig
" Maintainer: 		tocariimaa <tocariimaa@firemail.cc>
" Last Change:		2025-10-02

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = 'setl com< cms< fo<'

setlocal formatoptions+=cqort

if has('comments')
  setlocal commentstring=(*\ %s\ *)
  setlocal comments=sr:(*,mb:*,ex:*)
endif

let &cpo = s:cpo_save
unlet s:cpo_save
