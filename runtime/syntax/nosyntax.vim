" Vim syntax support file
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2000 Jul 15

" This file is used for ":syntax off".
" It removes the autocommands and stops highlighting for all buffers.

if !has("syntax")
  finish
endif

" remove all syntax autocommands and remove the syntax for each buffer
augroup syntaxset
  au!
  au BufEnter * syn clear
  au BufEnter * if exists("b:current_syntax") | unlet b:current_syntax | endif
  doautoall syntaxset BufEnter *
  au!
augroup END

" Just in case: remove all autocommands for the Syntax event
au! Syntax

if exists("syntax_on")
  unlet syntax_on
endif
if exists("syntax_manual")
  unlet syntax_manual
endif
