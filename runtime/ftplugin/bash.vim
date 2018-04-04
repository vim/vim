" Vim filetype plugin file
" Language: bash
" Maintainer: Mahmoud Al-Qudsi <mqudsi@neosmart.net>
" Last Changed: 3 April 2018

if exists("b:did_ftplugin_bash") | finish | endif

let b:is_bash=1
runtime! ftplugin/sh.vim
runtime! ftplugin/sh_*.vim ftplugin/sh/*.vim
noautocmd setlocal syntax=bash

let b:did_ft_plugin_bash=1
