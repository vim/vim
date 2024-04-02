" Vim indent file
" Language:            Octave
" Maintainer:          Nguyễn Gia Phong <vn.mcsinyx@gmail.com>
" Original Maintainer: Marcus Aurelius Farias <marcus.cf@bol.com.br>
" First Author:        Max Ischenko <mfi@ukr.net>
" Last Change:         2019-10-16

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

let s:beginBlock = ['for', 'parfor', 'function', 'if', 'switch',
                   \'try', 'unwind_protect', 'while', 'do', 'classdef',
                   \'enumeration', 'events', 'methods', 'properties']
let s:midBlock = ['case', 'catch', 'else', 'elseif', 'otherwise',
                 \'unwind_protect_cleanup']
let s:endBlock = ['end', 'endfor', 'endparfor', 'endfunction', 'endif',
                 \'end_try_catch', 'end_unwind_protect', 'endwhile',
                 \'endclassdef', 'endenumeration', 'endevents',
                 \'endproperties', 'endswitch', 'until', 'endmethods']
let s:openBlock = s:beginBlock + s:midBlock
let s:closeBlock = s:midBlock + s:endBlock

" To make Vim call GetOctaveIndent() when it finds a block closer
" on the current line ('else' is default and includes 'elseif').
setlocal indentkeys+=0=end,0=until,0=case,0=catch,0=otherwise
setlocal indentkeys+=0=unwind_protect_cleanup

" Only define the function once.
if exists("*GetOctaveIndent")
  finish
endif

function! GetOctaveIndent()
  " Find a non-blank line above the current line.
  let prevlnum = prevnonblank(v:lnum - 1)

  " Hit the start of the file, use zero indent.
  if prevlnum == 0
    return 0
  endif

  let ind = indent(prevlnum)
  let prevl = getline(prevlnum)
  let l = getline(v:lnum)

  " Add a 'shiftwidth' after lines starting a block:
  let openCol = match(prevl, '^\s*\%(' . join(s:openBlock, '\>\|') . '\>\)') + 1
  let hasNoEnd = prevl !~ ('\<' . join(s:endBlock, '\>\|\<') . '\>')
  if openCol && hasNoEnd
    let openSynID = synID(prevlnum, openCol, 1)
    if synIDattr(openSynID, "name") != "octaveComment"
      let ind = ind + shiftwidth()
    endif
  endif

  " Subtract a 'shiftwidth' on closure of blocks,
  " i.e. the part that required 'indentkeys'.
  let closeCol = match(l, '^\s*\%(' . join(s:closeBlock, '\>\|') . '\>\)') + 1
  if closeCol
    let closeSynID = synID(v:lnum, closeCol, 1)
    if synIDattr(closeSynID, "name") != "octaveComment"
      let ind = ind - shiftwidth()
    endif
  endif

  return ind
endfunction

setlocal indentexpr=GetOctaveIndent()
setlocal autoindent
