" Vim indent file
" Language:	    Shell Script
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/sh/
" Latest Revision:  2004-04-25
" arch-tag:	    431c7fc1-12a6-4d71-9636-1498ef56b038

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetShIndent()
setlocal indentkeys+==then,=do,=else,=elif,=esac,=fi,=fin,=fil,=done
setlocal indentkeys-=:,0#

" Only define the function once.
if exists("*GetShIndent")
  finish
endif

set cpoptions-=C

function GetShIndent()
  " Find a non-blank line above the current line.
  let lnum = prevnonblank(v:lnum - 1)

  " Hit the start of the file, use zero indent.
  if lnum == 0
    return 0
  endif

  " Add a 'shiftwidth' after if, while, else, case, until, for, function()
  " Skip if the line also contains the closure for the above
  let ind = indent(lnum)
  let line = getline(lnum)
  if line =~ '^\s*\(if\|then\|do\|else\|elif\|case\|while\|until\|for\)\>'
	\ || line =~ '^\s*\<\h\w*\>\s*()\s*{'
	\ || line =~ '^\s*{'
    if line !~ '\(esac\|fi\|done\)\>\s*$' && line !~ '}\s*$'
      let ind = ind + &sw
    endif
  endif

  " Subtract a 'shiftwidth' on a then, do, else, esac, fi, done
  " Retain the indentation level if line matches fin (for find)
  let line = getline(v:lnum)
  if (line =~ '^\s*\(then\|do\|else\|elif\|esac\|fi\|done\)\>' || line =~ '^\s*}')
	\ && line !~ '^\s*fi[ln]\>'
    let ind = ind - &sw
  endif

  return ind
endfunction

" vim: set sts=2 sw=2:
