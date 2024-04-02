" Vim filetype plugin
" Language:	jq
" Maintainer:	Vito C <vito.blog@gmail.com>
" Upstream:	https://github.com/vito-c/jq.vim
" Last Change:	2015 Nov 28

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

let s:save_cpoptions = &cpoptions
set cpoptions&vim

let b:undo_ftplugin = 'setl commentstring<'

setlocal commentstring=#%s
compiler jq

let &cpoptions = s:save_cpoptions
unlet s:save_cpoptions
