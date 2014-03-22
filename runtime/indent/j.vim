" Vim indent file
" Language:	J
" Maintainer:	David Bürgin <676c7473@gmail.com>
" URL:		https://github.com/glts/vim-j
" Last Change:	2014-03-17

if exists('b:did_indent')
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetJIndent()
setlocal indentkeys-=0{,0},\:,0#
setlocal indentkeys+=0),=case.,=catch.,=catchd.,=catcht.,=do.,=else.,=elseif.,=end.,=fcase.

let b:undo_indent = 'setlocal indentkeys< indentexpr<'

if exists('*GetJIndent')
  finish
endif

function GetJIndent()
  let prevlnum = prevnonblank(v:lnum-1)
  if prevlnum == 0
    return 0
  endif

  let indent = indent(prevlnum)
  if getline(prevlnum) =~# '^\s*\%(case\|catch[dt]\=\|do\|else\%(if\)\=\|fcase\|for\%(_\a\k*\)\=\|if\|select\|try\|whil\%(e\|st\)\)\.'
    if getline(prevlnum) !~# '\<end\.'
      let indent += shiftwidth()
    endif
  endif
  if getline(v:lnum) =~# '^\s*\%(\%(case\|catch[dt]\=\|do\|else\%(if\)\=\|end\|fcase\)\.\)\|)'
    let indent -= shiftwidth()
  endif
  return indent
endfunction
