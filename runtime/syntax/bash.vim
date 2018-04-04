" Vim syntax plugin file
" Language: bash
" Maintainer: Mahmoud Al-Qudsi <mqudsi@neosmart.net>
" Last Changed: 3 April 2018

if &compatible || v:version < 603
	finish
endif

if exists("b:current_syntax") | finish | endif

let b:is_bash = 1
runtime! syntax/sh.vim
runtime! syntax/sh_*.vim syntax/sh/*.vim
noautocmd setlocal syntax=bash
