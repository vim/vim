" Vim indent file
" Language:	    YACC input file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/yacc/
" Latest Revision:  2004-04-25
" arch-tag:	    629aa719-8fe4-4787-adb7-ae94ca801610

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetYaccIndent()
setlocal indentkeys=!^F,o,O

" Only define the function once.
if exists("*GetYaccIndent")
  finish
endif

function GetYaccIndent()
  if v:lnum == 1
    return 0
  endif

  let ind = indent(v:lnum - 1)
  let line = getline(v:lnum - 1)

  if line == ''
    let ind = 0
  elseif line =~ '^\w\+\s*:'
    let ind = ind + matchend(line, '^\w\+\s*')
  elseif line =~ '^\s*;'
    let ind = 0
  else
    let ind = indent(v:lnum)
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
