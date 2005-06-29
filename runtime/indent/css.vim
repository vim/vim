" Vim indent file
" Language:         CSS
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetCSSIndent()
setlocal indentkeys=0{,0},!^F,o,O

if exists("*GetCSSIndent")
  finish
endif

function s:LookupLine(lnum)
  let lnum = prevnonblank(a:lnum - 1)
  while lnum > 0
    let line = getline(lnum)

    if line =~ '\*/'
      while lnum > 0 && line !~ '/\*'
        let lnum -= 1
        let line = getline(lnum)
      endwhile
    endif

    if line !~ '^\s*/\*'
      return lnum
    end
  endwhile
  return lnum
endfunction

function GetCSSIndent()
  let lnum = prevnonblank(v:lnum - 1)
  if lnum == 0
    return 0
  endif

  let ind = indent(lnum)

  if substitute(getline(lnum), '/\*.*', '', 'e') =~ '{\(.*}\)\@!'
    let ind = ind + &sw
  endif

  if getline(v:lnum) =~ '^\s*}'
    let ind = ind - &sw
  endif

  return ind
endfunction
