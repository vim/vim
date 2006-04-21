" Vim indent file
" Language:         Makefile
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetMakeIndent()
setlocal indentkeys=!^F,o,O

if exists("*GetMakeIndent")
  finish
endif

let s:rule_rx = '^[^ \t#:][^#:]*:\{1,2}\%([^=:]\|$\)'
let s:continuation_rx = '\\$'
let s:assignment_rx = '^\s*\h\w*\s*+\==\s*\zs.*\\$'

function GetMakeIndent()
  let lnum = v:lnum - 1
  if lnum == 0
    return 0
  endif

  let line = getline(lnum)
  let ind = indent(lnum)

  if line =~ s:rule_rx
    return ind + &ts
  elseif line =~ s:continuation_rx
    while lnum > 0 && line =~ s:continuation_rx && line !~ s:assignment_rx
      let lnum -= 1
      let line = getline(lnum)
    endwhile
    if line =~ s:assignment_rx
      call cursor(lnum, 1)
      return search(s:assignment_rx, 'W') != 0 ? virtcol('.') - 1 : 0
    else
      return 0
    endif
  else
    let pnum = lnum - 1
    if pnum == 0
      return ind
    endif

    return getline(pnum) =~ s:continuation_rx ? 0 : ind
  endif
endfunction
