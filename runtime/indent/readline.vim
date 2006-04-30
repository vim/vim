" Vim indent file
" Language:	    readline configuration file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetReadlineIndent()
setlocal indentkeys=!^F,o,O,=$else,=$endif

if exists("*GetReadlineIndent")
  finish
endif

function GetReadlineIndent()
  let lnum = prevnonblank(v:lnum - 1)
  if lnum == 0
    return 0
  endif

  let ind = indent(lnum)

  if getline(lnum) =~ '^\s*$\(if\|else\)\>'
    let ind = ind + &sw
  endif

  if getline(v:lnum) =~ '^\s*$\(else\|endif\)\>'
    let ind = ind - &sw
  endif

  return ind
endfunction
