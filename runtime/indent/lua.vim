" Vim indent file
" Language:	Lua script
" Maintainer:	Marcus Aurelius Farias <marcuscf@vant.com.br>
" First Author:	Max Ischenko <mfi@ukr.net>
" Last Change:	2003 Jan 20

" Only define the function once.
if exists("*GetLuaIndent") | finish | endif

setlocal indentexpr=GetLuaIndent()

" To make Vim call GetLuaIndent() when it finds '\s*end' or '\s*until'
" on the current line (else is default).
setlocal indentkeys+=0=end,0=until

setlocal autoindent

function! GetLuaIndent()
  " Find a non-blank line above the current line.
  let lnum = prevnonblank(v:lnum - 1)

  " Hit the start of the file, use zero indent.
  if lnum == 0
    return 0
  endif

  " Add a 'shiftwidth' after lines beginning with:
  " function, if, for, while, repeat, else, elseif, '{'
  let ind = indent(lnum)
  let flag = 0
  if getline(lnum) =~ '^\s*\(function\>\|if\>\|for\>\|while\>\|repeat\>\|else\>\|elseif\>\|do\>\)' || getline(lnum) =~ '{\s*$' || getline(lnum) =~ '\s*=\s*function'
    let ind = ind + &sw
    let flag = 1
  endif

  " Subtract a 'shiftwidth' after lines ending with
  " 'end' when they begin with while, if, for, etc.
  if flag == 1 && getline(lnum) =~ '\<end\>\|\<until\>'
    let ind = ind - &sw
  endif

  " Subtract a 'shiftwidth' on end, else (and elseif), until and '}'
  " This is the part that requires 'indentkeys'.
  if getline(v:lnum) =~ '^\s*\(end\|else\|until\|}\)'
    let ind = ind - &sw
  endif

  return ind
endfunction
