" Vim indent file
" Language:	    readline configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/readline/
" Latest Revision:  2004-04-25
" arch-tag:	    ee681235-3abf-4a42-8587-edabd409a980

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetReadlineIndent()
setlocal indentkeys=!^F,o,O,=$else,=$endif

" Only define the function once.
if exists("*GetReadlineIndent")
  finish
endif

function GetReadlineIndent()
  let lnum = prevnonblank(v:lnum - 1)

  if lnum == 0
    return 0
  endif

  let line = getline(lnum)
  let ind = indent(lnum)

  " increase indent if previous line started with $if or $else
  if  line =~ '^\s*$\(if\|else\)\>'
    let ind = ind + &sw
  endif

  let line = getline(v:lnum)

  " decrease indent if this line starts with $else or $endif
  if line =~ '^\s*$\(else\|endif\)\>'
    let ind = ind - &sw
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
