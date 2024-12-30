" VIM syntax file

" Language:	MOM (Macros for GNU Troff)
" Maintainer:	John Marshall <jmarshall@hey.com>
" Previous Maintainer:	Pedro Alejandro López-Valencia <palopezv@gmail.com>
" Previous Maintainer:	Jérôme Plût <Jerome.Plut@ens.fr>

if exists("b:current_syntax")
  finish
endif

syn clear
runtime! syntax/nroff.vim
