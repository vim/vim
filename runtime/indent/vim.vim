" Vim indent file
" Language:	Vim script
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2003 May 25

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetVimIndent()
setlocal indentkeys+==end,=else,=cat,=fina,=END,0\\

" Only define the function once.
if exists("*GetVimIndent")
  finish
endif

function GetVimIndent()
  " Find a non-blank line above the current line.
  let lnum = prevnonblank(v:lnum - 1)

  " If the current line doesn't start with '\' and below a line that starts
  " with '\', use the indent of the line above it.
  if getline(v:lnum) !~ '^\s*\\'
    while lnum > 0 && getline(lnum) =~ '^\s*\\'
      let lnum = lnum - 1
    endwhile
  endif

  " At the start of the file use zero indent.
  if lnum == 0
    return 0
  endif

  " Add a 'shiftwidth' after :if, :while, :try, :catch, :finally, :function
  " and :else.  Add it three times for a line that starts with '\' after
  " a line that doesn't.
  let ind = indent(lnum)
  if getline(v:lnum) =~ '^\s*\\' && v:lnum > 1 && getline(lnum) !~ '^\s*\\'
    let ind = ind + &sw * 3
  elseif getline(lnum) =~ '\(^\||\)\s*\(if\|wh\%[ile]\|try\|cat\%[ch]\|fina\%[lly]\|fu\%[nction]\|el\%[seif]\)\>'
    let ind = ind + &sw
  elseif getline(lnum) =~ '^\s*aug\%[roup]' && getline(lnum) !~ '^\s*aug\%[roup]\s*!\=\s\+END'
    let ind = ind + &sw
  endif

  " If the previous line contains an "end" after a pipe, but not in an ":au"
  " command.
  if getline(lnum) =~ '|\s*\(ene\@!\)' && getline(lnum) !~ '^\s*au\%[tocmd]'
    let ind = ind - &sw
  endif


  " Subtract a 'shiftwidth' on a :endif, :endwhile, :catch, :finally, :endtry,
  " :endfun, :else and :augroup END.
  if getline(v:lnum) =~ '^\s*\(ene\@!\|cat\|fina\|el\|aug\%[roup]\s*!\=\s\+END\)'
    let ind = ind - &sw
  endif

  return ind
endfunction

" vim:sw=2
