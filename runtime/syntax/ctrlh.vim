" Vim syntax file
" Language:	CTRL-H (e.g., ASCII manpages)
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2001 Apr 25

" Existing syntax is kept, this file can be used as an addition

" Recognize underlined text: _^Hx
syntax match CtrlHUnderline /_\b./  contains=CtrlHHide

" Recognize bold text: x^Hx
syntax match CtrlHBold /\(.\)\b\1/  contains=CtrlHHide

" Hide the CTRL-H (backspace)
syntax match CtrlHHide /.\b/  contained

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_ctrlh_syntax_inits")
  if version < 508
    let did_ctrlh_syntax_inits = 1
    hi link CtrlHHide Ignore
    hi CtrlHUnderline term=underline cterm=underline gui=underline
    hi CtrlHBold term=bold cterm=bold gui=bold
  else
    hi def link CtrlHHide Ignore
    hi def CtrlHUnderline term=underline cterm=underline gui=underline
    hi def CtrlHBold term=bold cterm=bold gui=bold
  endif
endif

" vim: ts=8
