" Vim indent file
" Language:	    Eterm configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/eterm/
" Latest Revision:  2004-04-25
" arch-tag:	    a22a92b1-c59f-4f47-8207-b21db6549b21

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetEtermIndent()
setlocal indentkeys=!^F,o,O,=end

" Only define the function once.
if exists("*GetEtermIndent")
  finish
endif

function GetEtermIndent()
  " Find a non-blank line above the current line.
  let lnum = prevnonblank(v:lnum - 1)

  " Hit the start of the file, use zero indent.
  if lnum == 0
    return 0
  endif

  let line = getline(lnum)
  let ind = indent(lnum)

  if line =~ '^\s*begin\>'
    let ind = ind + &sw
  endif

  let line = getline(v:lnum)

  " Check for closing brace on current line
  if line =~ '^\s*end\>'
    let ind = ind - &sw
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
