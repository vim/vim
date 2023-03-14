" Vim indent file
" Language:             Free-Form ILE RPG
" Maintainer:           Andreas Louv <andreas@louv.dk>
" Last Change:          Mar 14, 2023
" Version:              1

if exists('b:did_indent')
  finish
endif

let b:did_indent = 1

setlocal indentexpr=GetRpgleIndent()
setlocal indentkeys=o,O
setlocal indentkeys+=0*
setlocal indentkeys+=0=~if,0=~elseif,0=~else,0=~endif
setlocal indentkeys+=0=~dou,0=~dow,0=~enddo
setlocal indentkeys+=0=~for,0=~endfor
setlocal indentkeys+=0=~monitor,0=~on-error,0=~endmon
setlocal indentkeys+=0=~select,0=~when,0=~other,0=~endsl
setlocal indentkeys+=0=~dcl-proc,0=~end-proc
setlocal indentkeys+=0=~begsr,0=~endsr
setlocal indentkeys+=0=~dcl-pi,0=~end-pi
setlocal indentkeys+=0=~dcl-pr,0=~end-pr
setlocal indentkeys+=0=~dcl-ds,0=~end-ds
setlocal nosmartindent

let b:undo_indent = 'setlocal indentexpr< indentkeys< smartindent<'

if exists('*GetRpgleIndent')
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

function! GetRpgleIndent()
  let cnum = v:lnum
  let pnum = prevnonblank(cnum - 1)

  " There is no lines to determinate indent, so use what is set in
  " "g:rpgle_indentStart", check if "**FREE" is present or default to "7".
  if pnum == 0
    if exists('g:rpgle_indentStart')
      return g:rpgle_indentStart
    elseif getline(1) =~? '^\*\*FREE$'
      return 0
    else
      return 7
    endif
  endif

  let pind = indent(pnum)

  let pline = getline(pnum)
  let cline = getline(cnum)

  " Continues comments should indent the "*" one space
  if cline =~# '^\s*\*' && pline =~# '^\s*/\*' && pline !~# '\*/'
    return pind + 1
  endif

  " Continues comments should de indent one space when ended
  if pline =~# '^\s*\*.*\*/' || cline =~# '^\s*\\\*.*\*/'
    return pind - 1
  endif

  " A "when" which follows a "select" should be indented:
  " All other "when" should be dedented
  if cline =~? '^\s*\<when\>'
    return pline =~? '^\s*\<select\>;' ?
      \ pind + shiftwidth() : pind - shiftwidth()
  endif

  " "dcl-pi", "dcl-pr", and "dcl-ds" with no parameters should not
  " indent the "end-xx":
  if pline =~? '^\s*\<dcl-pi\>' && cline =~? '^\s*\<end-pi\>'
  \ || pline =~? '^\s*\<dcl-pr\>' && cline =~? '^\s*\<end-pr\>'
  \ || pline =~? '^\s*\<dcl-ds\>' && cline =~? '^\s*\<end-ds\>'
    return pind
  endif

  " "dcl-ds" with "likeds" or "likerec" on the same line doesn't take a
  " definition and should not do any indent:
  if pline =~? '^\s*\<dcl-ds\>' && pline =~? '\<likeds\>\|\<likerec\>'
    return pind
  endif

  " Add indent for opening keywords:
  if pline =~ '^\s*\%(if\|else\|elseif\|dou\|dow\|for\|monitor\|on-error\|' .
      \ 'on-error\|when\|other\|dcl-proc\|begsr\|dcl-pi\|dcl-pr\|dcl-ds\)\>'
    return pind + shiftwidth()
  endif

  " Remove indent for closing keywords:
  if cline =~ '^\s*\<\%(endif\|enddo\|endfor\|endmon\|other\|else\|' .
    \ 'elseif\|on-error\|end-pi\|end-proc\|endsr\|end-pr\|end-ds\)\>'
    return pind - shiftwidth()
  endif

  " "endsl" have to dedent two levels, to handle the extra indent from "when"
  if cline =~? '^\s*\<endsl\>'
    return pind - shiftwidth() * 2
  endif

  " If no match return the same indent as before:
  return pind
endfunction

let &cpo = s:cpo_save
unlet s:cpo_save
