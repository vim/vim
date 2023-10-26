" Vim filetype plugin file
" Language:     Objdump
" Maintainer:   Colin Kennedy <colinvfx@gmail.com>
" Last Change:  2023 October 25

if exists("b:did_ftplugin")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

let b:did_ftplugin = 1

setlocal commentstring=#\ %s

let &cpo = s:cpo_save
unlet s:cpo_save
