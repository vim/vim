" Vim indent file
" Language:	    XFree86 Configuration File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/xf86conf/
" Latest Revision:  2004-04-25
" arch-tag:	    8a42f7b6-5088-49cf-b15b-07696a91c015

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetXF86ConfIndent()
setlocal indentkeys=!^F,o,O,=End

" Only define the function once.
if exists("*GetXF86ConfIndent")
  finish
endif

function GetXF86ConfIndent()
  let lnum = prevnonblank(v:lnum - 1)

  if lnum == 0
    return 0
  endif

  let ind = indent(lnum)
  let line = getline(lnum)

  if line =~? '^\s*\(Sub\)\=Section'
    let ind = ind + &sw
  elseif getline(v:lnum) =~? '^\s*End'
    let ind = ind - &sw
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
