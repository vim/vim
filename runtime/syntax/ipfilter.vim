" ipfilter syntax file
" Language: ipfilter configuration file
" Maintainer: Hendrik Scholz <hendrik@scholz.net>
" Last Change: 2003 May 11
"
" http://raisdorf.net/files/misc/ipfilter.vim
"
" This will also work for OpenBSD pf but there might be some tags that are
" not correctly identified.
" Please send comments to hendrik@scholz.net

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" comments
syn match ipfComment /#/
"syn match ipfComment /#.*/

syn keyword ipfQuick quick log dup-to
syn keyword ipfAny all any
" rule Action type
syn region ipfActionBlock start=/^block/ end=/$/ contains=ipfQuick,ipfAny
syn region ipfActionPass  start=/^pass/ end=/$/ contains=ipfQuick,ipfAny
syn region ipfActionMisc  start=/^log/ end=/$/ contains=ipfQuick,ipfAny
syn region ipfActionMisc  start=/^count/ end=/$/ contains=ipfQuick,ipfAny
syn region ipfActionMisc  start=/^skip/ end=/$/ contains=ipfQuick,ipfAny
syn region ipfActionMisc  start=/^auth/ end=/$/ contains=ipfQuick,ipfAny
syn region ipfActionMisc  start=/^call/ end=/$/ contains=ipfQuick,ipfAny

hi def link ipfComment		Comment
hi def link ipfActionBlock	String
hi def link ipfActionPass	Type
hi def link ipfActionMisc	Label
"hi def link ipfQuick		Error
hi def link ipfQuick		Special
hi def link ipfAny		Todo


