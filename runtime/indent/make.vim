" Vim indent file
" Language:         Makefile
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetMakeIndent()
setlocal indentkeys=!^F,o,O

if exists("*GetMakeIndent")
  finish
endif

function s:GetStringWidth(line, str)
  let end = matchend(a:line, a:str)
  let width = 0
  for c in a:line
    if c == "\t"
      let width += &ts - (width % &ts)
    else
      let width += 1
    endif
  endfor
  return width
endfunction

function GetMakeIndent()
  let lnum = v:lnum - 1
  if lnum == 0
    return 0
  endif

  let line = getline(lnum)
  if line == ''
    return 0
  elseif line =~ '^[^ \t#:][^#:]*:\{1,2}\%([^=:]\|$\)'
    return indent(lnum) + &ts
  elseif line =~ '^\s*\h\w*\s*+\==\s*.\+\\$'
    return s:GetStringWidth(line, '+\==\s*')
  endif
endfunction
