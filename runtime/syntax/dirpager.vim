" Vim syntax file
" Language:	  directory pager
" Maintainer:	  Thilo Six <T.Six@gmx.de>
" Derived From:	  Nikolai Weibulls dircolors.vim
" Last Change:	  2011 Dec 11
" Modeline:	  vim: ts=8:sw=2:sts=2:
"
" usage: $ ls -la | view -c "set ft=dirpager" -
"
"
",----[ ls(1posix) ]--------------------------------------------------
"
" The  <entry type> character shall describe the type of file, as
"       follows:
"
"       d	Directory.
"       b	Block special file.
"       c	Character special file.
"       l (ell)	Symbolic link.
"       p	FIFO.
"       -	Regular file.
"`--------------------------------------------------------------------
"

if exists("b:current_syntax") || &compatible
  finish
endif

setlocal nowrap

syn keyword  DirPagerTodo	contained FIXME TODO XXX NOTE

syn region   DirPagerExe	start='^...x\|^......x\|^.........x' end='$'	contains=DirPagerTodo,@Spell
syn region   DirPagerDir	start='^d' end='$'	contains=DirPagerTodo,@Spell
syn region   DirPagerLink	start='^l' end='$'	contains=DirPagerTodo,@Spell
syn region   DirPagerSpecial	start='^b' end='$'	contains=DirPagerTodo,@Spell
syn region   DirPagerSpecial	start='^c' end='$'	contains=DirPagerTodo,@Spell
syn region   DirPagerFifo	start='^p' end='$'	contains=DirPagerTodo,@Spell

hi def link  DirPagerTodo	Todo
hi def	     DirPagerExe	ctermfg=Green	    guifg=Green
hi def	     DirPagerDir	ctermfg=Blue	    guifg=Blue
hi def	     DirPagerLink	ctermfg=Cyan	    guifg=Cyan
hi def	     DirPagerSpecial	ctermfg=Yellow	    guifg=Yellow
hi def	     DirPagerFifo	ctermfg=Brown	    guifg=Brown

let b:current_syntax = "dirpager"

