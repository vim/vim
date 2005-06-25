" Vim indent file
" Language:	Lua script
" Maintainer:	Marcus Aurelius Farias <marcus.cf 'at' bol.com.br>
" First Author:	Max Ischenko <mfi 'at' ukr.net>
" Last Change:	2005 Jun 23

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetLuaIndent()

" To make Vim call GetLuaIndent() when it finds '\s*end' or '\s*until'
" on the current line ('else' is default and includes 'elseif').
setlocal indentkeys+=0=end,0=until

setlocal autoindent

" Only define the function once.
if exists("*GetLuaIndent")
  finish
endif

function! GetLuaIndent()
  " Find a non-blank line above the current line.
  let lnum = prevnonblank(v:lnum - 1)

  " Hit the start of the file, use zero indent.
  if lnum == 0
    return 0
  endif

  " Add a 'shiftwidth' after lines that start a block:
  " 'function', 'if', 'for', 'while', 'repeat', 'else', 'elseif', '{'
  let ind = indent(lnum)
  let flag = 0
  let prevline = getline(lnum)
  if prevline =~ '^\s*\%(if\>\|for\>\|while\>\|repeat\>\|else\>\|elseif\>\|do\>\|then\>\)'
        \ || prevline =~ '{\s*$' || prevline =~ '\<function\>\s*\%(\k\|[.:]\)\{-}\s*('
    let ind = ind + &shiftwidth
    let flag = 1
  endif

  " Subtract a 'shiftwidth' after lines ending with
  " 'end' when they begin with 'while', 'if', 'for', etc. too.
  if flag == 1 && prevline =~ '\<end\>\|\<until\>'
    let ind = ind - &shiftwidth
  endif

  " Subtract a 'shiftwidth' on end, else (and elseif), until and '}'
  " This is the part that requires 'indentkeys'.
  if getline(v:lnum) =~ '^\s*\%(end\|else\|until\|}\)'
    let ind = ind - &shiftwidth
  endif

  return ind
endfunction
