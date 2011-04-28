" Vim syntax file
" Language:         directory pager
" Maintainer:       Thilo Six <T.Six@gmx.de>
" Derived From:	    Nikolai Weibull's dircolors.vim
" Latest Revision:  2011-04-09
"
" usage: $ ls -la | view -c "set ft=dirpager" -
"

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim
setlocal nowrap

syn keyword  DirPagerTodo	contained FIXME TODO XXX NOTE

syn region   DirPagerExe	start='^...x\|^......x\|^.........x' end='$'	contains=DirPagerTodo,@Spell
syn region   DirPagerDir	start='^d' end='$'	contains=DirPagerTodo,@Spell
syn region   DirPagerLink	start='^l' end='$'	contains=DirPagerTodo,@Spell

hi def link  DirPagerTodo	Todo
hi def	     DirPagerExe	ctermfg=Green	    guifg=Green
hi def	     DirPagerDir	ctermfg=Blue	    guifg=Blue
hi def	     DirPagerLink	ctermfg=Cyan	    guifg=Cyan

let b:current_syntax = "dirpager"

let &cpo = s:cpo_save
unlet s:cpo_save

