" Vim indent file
" Language:	    CSS
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/css/
" Latest Revision:  2004-04-25
" arch-tag:	    ccfd77a0-1c9a-43f7-a407-bbe704541442

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetCSSIndent()
setlocal indentkeys-=:,0# indentkeys-=e

" Only define the function once.
if exists("*GetCSSIndent")
  finish
endif

function! s:LookupLine(lnum)
  " find a non-blank line above the current line
  let lnum = prevnonblank(a:lnum - 1)

  if lnum == 0
    return 0
  endif

  let line = getline(lnum)

  " if the line has an end comment sequence we need to find a line
  " that isn't affected by the comment.
  if line =~ '\*/'
    while line !~ '/\*'
      let lnum = lnum - 1
      let line = getline(lnum)
    endwhile
  endif

  " if the line we found only contained the comment and whitespace
  " we need to find another line to use...
  if line =~ '^\s*/\*'
    return s:LookupLine(lnum)
  else
    return lnum
  endif
endfunction

function GetCSSIndent()
  let lnum = s:LookupLine(v:lnum)

  if lnum == 0
    return 0
  endif

  " remove commented stuff from line
  let line = substitute(getline(lnum), '/\*.\*/', '', 'eg')

  let ind = indent(lnum)

  " check for opening brace on the previous line
  " skip if it also contains a closing brace...
  if line =~ '{\(.*}\)\@!'
    let ind = ind + &sw
  endif

  let line = getline(v:lnum)

  " check for closing brace first on current line
  if line =~ '^\s*}'
    let ind	= ind - &sw
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
