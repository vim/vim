" Vim indent file
" Language:	    Tcl
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/tcl/
" Latest Revision:  2004-05-21
" arch-tag:	    64fab1fa-d670-40ab-a191-55678f20ceb0

" only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetTclIndent()
setlocal indentkeys-=:,0#
setlocal indentkeys+=0]

" only define the function once.
if exists("*GetTclIndent")
  finish
endif

function s:PrevNonBlankNonComment(lnum)
  let lnum = prevnonblank(a:lnum)
  while lnum > 0
    let line = getline(lnum)
    if line !~ '^\s*\(#\|$\)'
      break
    endif
    let lnum = prevnonblank(lnum - 1)
  endwhile
  return lnum
endfunction

function! GetTclIndent()
  let lnum = s:PrevNonBlankNonComment(v:lnum - 1)

  if lnum == 0
    return 0
  endif

  let line = getline(lnum)
  let ind = indent(lnum)

  " TODO: Possible improvement, check that 'begin' and 'end' aren't inside a
  " comment or string.  This will mess it up.  As I am pressed for time and
  " stuff like this is unlikely to happen I won't deal with it in this
  " version.
  let open = 0
  let begin = match(line, '{', 0)
  while begin > -1
    let end = match(line, '}', begin + 1)
    if end < 0
      let open = open + 1
    else
      let tmp = match(line, '{', begin + 1)
      if tmp != -1 && tmp < end
	let open = open + 1
      endif
    endif
    let begin = match(line, '{', begin + 1)
  endwhile

  let begin = match(line, '[', 0)
  while begin > -1
    let end = match(line, ']', begin + 1)
    if end < 0
      let open = open + 1
    else
      let tmp = match(line, '{', begin + 1)
      if tmp != -1 && tmp < end
	let open = open + 1
      endif
    endif
    let begin = match(line, '{', begin + 1)
  endwhile

  let close = 0
  let prev = 0
  let end = matchend(line, '^\s*}.*}', prev)
  while end > -1
    let begin = match(line, '{', prev + 1)
    if begin < 0 || begin > prev
      let close = close + 1
    endif
    let prev = end
    let end = match(line, '}', prev + 1)
  endwhile

  let prev = 0
  let end = match(line, ']', prev)
  while end > -1
    let begin = match(line, '[', prev + 1)
    if begin < 0 || begin > prev
      let close = close + 1
    endif
    let prev = end
    let end = match(line, ']', prev + 1)
  endwhile

  let ind = ind + (open - close) * &sw

  let line = getline(v:lnum)

  let close = 0
  let prev = 0
  let end = match(line, '}', prev)
  while end > -1
    let begin = match(line, '{', prev + 1)
    if begin < 0 || begin > prev
      let close = close + 1
    endif
    let prev = end
    let end = match(line, '}', prev + 1)
  endwhile

  let ind = ind - close * &sw

  return ind >= 0 ? ind : 0
endfunction

" vim: set sts=2 sw=2:
