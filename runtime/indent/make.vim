" Vim indent file
" Language:	    Makefile
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/make/
" Latest Revision:  2004-04-25
" arch-tag:	    b539e147-a05c-4860-98af-1d2436db2f4b

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetMakeIndent()
setlocal indentkeys=!^F,o,O

" Only define the function once.
if exists("*GetMakeIndent")
  finish
endif

function s:GetStringWidth(line, str)
  let end = matchend(a:line, a:str)
  let width = 0
  let i = 0
  while i < end
    if a:line[i] != "\t"
      let width = width + 1
    else
      let width = width + &ts - (width % &ts)
    endif
    let i = i + 1
  endwhile
  return width
endfunction

function GetMakeIndent()
  if v:lnum == 1
    return 0
  endif

  let ind = indent(v:lnum - 1)
  let line = getline(v:lnum - 1)

  if line == ''
    let ind = 0
  elseif line =~ '^[^ \t#:][^#:]*:\{1,2}\([^=:]\|$\)'
    let ind = ind + &ts
  elseif line =~ '^\s*\h\w*\s*=\s*.\+\\$'
    let ind = s:GetStringWidth(line, '=\s*')
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
