" Vim indent file
" Language:	    CSS
" Maintainer:	    Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-12-20

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetCSSIndent()
setlocal indentkeys=0{,0},!^F,o,O
setlocal nosmartindent

if exists("*GetCSSIndent")
  finish
endif

function s:prevnonblanknoncomment(lnum)
  let lnum = a:lnum
  while lnum > 1
    let lnum = prevnonblank(lnum)
    let line = getline(lnum)
    if line =~ '\*/'
      while lnum > 1 && line !~ '/\*'
        let lnum -= 1
      endwhile
      if line =~ '^\s*/\*'
        let lnum -= 1
      else
        break
      endif
    else
      break
    endif
  endwhile
  return lnum
endfunction

function s:count_braces(lnum, count_open)
  let n_open = 0
  let n_close = 0
  let line = getline(a:lnum)
  let pattern = '[{}]'
  let i = match(line, pattern)
  while i != -1
    if synIDattr(synID(a:lnum, i + 1, 0), 'name') !~ 'css\%(Comment\|StringQ\{1,2}\)'
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

function GetCSSIndent()
  let line = getline(v:lnum)
  if line =~ '^\s*\*'
    return cindent(v:lnum)
  elseif line =~ '^\s*}'
    return indent(v:lnum) - &sw
  endif

  let pnum = s:prevnonblanknoncomment(v:lnum - 1)
  if pnum == 0
    return 0
  endif

  let ind = indent(pnum) + s:count_braces(pnum, 1) * &sw

  let pline = getline(pnum)
  if pline =~ '}\s*$'
    let ind -= (s:count_braces(pnum, 0) - (pline =~ '^\s*}' ? 1 : 0)) * &sw
  endif

  return ind
endfunction
