" Vim indent file
" Language:         ld(1) script
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetLDIndent()
setlocal indentkeys=0{,0},!^F,o,O

if exists("*GetLDIndent")
  finish
endif

function s:count_braces(lnum, count_open)
  let n_open = 0
  let n_close = 0
  let line = getline(a:lnum)
  let pattern = '[{}]'
  let i = match(line, pattern)
  while i != -1
    if synIDattr(synID(a:lnum, i + 1, 0), 'name') !~ 'ld\%(Comment\|String\)'
      if line[i] == '{'
        let n_open += 1
      elseif line[i] == '}'
        if n_open > 0
          let n_open -= 1
        else
          let n_close += 1
        endif
      endif
    endif
    let i = match(line, pattern, i + 1)
  endwhile
  return a:count_open ? n_open : n_close
endfunction

function GetLDIndent()
  let pnum = prevnonblank(v:lnum - 1)
  if pnum == 0
    return 0
  endif

  return indent(pnum) + s:count_braces(pnum, 1) * &sw
        \ - s:count_braces(v:lnum, 0) * &sw
endfunction
