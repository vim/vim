" Vim indent file
" Language:	    reStructuredText Documentation Format
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/rst/
" Latest Revision:  2004-04-25
" arch-tag:	    3fe10f75-24d0-4d94-a924-0ce945958104

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetRSTIndent()
setlocal indentkeys-=:,0# indentkeys-=e

" Only define the function once.
if exists("*GetRSTIndent")
  finish
endif

function GetRSTIndent()
  let lnum = prevnonblank(v:lnum - 1)

  if lnum == 0
    return 0
  endif

  let ind = indent(lnum)
  let line = getline(lnum)

  if line =~ '^\s*[-*+]\s'
    let ind = ind + 2
  elseif line =~ '^\s*\d\+.\s'
    let ind = ind + matchend(substitute(line, '^\s*', '', ''), '\d\+.\s\+')
  endif

  let line = getline(v:lnum - 1)

  if line =~ '^\s*$'
    execute lnum
    call search('^\s*\%([-*+]\s\|\d\+.\s\|\.\.\|$\)', 'bW')
    let line = getline('.')
    if line =~ '^\s*[-*+]'
      let ind = ind - 2
    elseif line =~ '^\s*\d\+\.\s'
      let ind = ind - matchend(substitute(line, '^\s*', '', ''),
	    \ '\d\+\.\s\+')
    elseif line =~ '^\s*\.\.'
      let ind = ind - 3
    else
      let ind = ind
    endif
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
