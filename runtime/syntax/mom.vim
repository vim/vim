" VIM syntax file
" Language:	nroff
" Maintainer:	John Marshall <jmarshall@hey.com>
" Previous Maintainer:	Pedro Alejandro López-Valencia <palopezv@gmail.com>
" Previous Maintainer:	Jérôme Plût <Jerome.Plut@ens.fr>
" Last Change:	2021 Mar 28

if exists("b:current_syntax")
  finish
endif

syn clear
runtime! syntax/nroff.vim

let s:cpo_save = &cpo
set cpo&vim
