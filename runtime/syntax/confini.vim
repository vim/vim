" Vim syntax file
" Language: confini

" Quit if a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Use the cfg syntax for now, it's similar.
runtime! syntax/cfg.vim
" Only accept '#' as the start of a comment.
syn clear CfgComment
syn match CfgComment "#.*"

let b:current_syntax = 'confini'
