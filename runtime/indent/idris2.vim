" Vim indent file
" Language:            Idris2
" Maintainer:          Serhii Khoma <srghma@gmail.com>
" Author:              raichoo <raichoo@googlemail.com>
" Last Change:         2024 Nov 05
" License:             Vim (see :h license)
" Repository:          https://github.com/ShinKage/idris2-nvim
"
" indentation for idris (idris-lang.org)
"
" Based on haskell indentation by motemen <motemen@gmail.com>
"
"
"
" Modify g:idris_indent_if and g:idris_indent_case to
" change indentation for `if'(default 3) and `case'(default 5).
" Example (in .vimrc):
" > let g:idris_indent_if = 2

if exists('b:did_indent')
  finish
endif

let b:did_indent = 1

if !exists('g:idris_indent_if')
  " if bool
  " >>>then ...
  " >>>else ...
  let g:idris_indent_if = 3
endif

if !exists('g:idris_indent_case')
  " case xs of
  " >>>>>[]      => ...
  " >>>>>(y::ys) => ...
  let g:idris_indent_case = 5
endif

if !exists('g:idris_indent_let')
  " let x : Nat = O in
  " >>>>x
  let g:idris_indent_let = 4
endif

if !exists('g:idris_indent_rewrite')
  " rewrite prf in expr
  " >>>>>>>>x
  let g:idris_indent_rewrite = 8
endif

if !exists('g:idris_indent_where')
  " where f : Nat -> Nat
  " >>>>>>f x = x
  let g:idris_indent_where = 6
endif

if !exists('g:idris_indent_do')
  " do x <- a
  " >>>y <- b
  let g:idris_indent_do = 3
endif

setlocal indentexpr=GetIdrisIndent()
setlocal indentkeys=!^F,o,O,}

function! GetIdrisIndent()
  let prevline = getline(v:lnum - 1)

  if prevline =~ '\s\+(\s*.\+\s\+:\s\+.\+\s*)\s\+->\s*$'
    return match(prevline, '(')
  elseif prevline =~ '\s\+{\s*.\+\s\+:\s\+.\+\s*}\s\+->\s*$'
    return match(prevline, '{')
  endif

  if prevline =~ '[!#$%&*+./<>?@\\^|~-]\s*$'
    let s = match(prevline, '[:=]')
    if s > 0
      return s + 2
    else
      return match(prevline, '\S')
    endif
  endif

  if prevline =~ '[{([][^})\]]\+$'
    return match(prevline, '[{([]')
  endif

  if prevline =~ '\<let\>\s\+.\+\<in\>\s*$'
    return match(prevline, '\<let\>') + g:idris_indent_let
  endif

  if prevline =~ '\<rewrite\>\s\+.\+\<in\>\s*$'
    return match(prevline, '\<rewrite\>') + g:idris_indent_rewrite
  endif

  if prevline !~ '\<else\>'
    let s = match(prevline, '\<if\>.*\&.*\zs\<then\>')
    if s > 0
      return s
    endif

    let s = match(prevline, '\<if\>')
    if s > 0
      return s + g:idris_indent_if
    endif
  endif

  if prevline =~ '\(\<where\>\|\<do\>\|=\|[{([]\)\s*$'
    return match(prevline, '\S') + &shiftwidth
  endif

  if prevline =~ '\<where\>\s\+\S\+.*$'
    return match(prevline, '\<where\>') + g:idris_indent_where
  endif

  if prevline =~ '\<do\>\s\+\S\+.*$'
    return match(prevline, '\<do\>') + g:idris_indent_do
  endif

  if prevline =~ '^\s*\<\(co\)\?data\>\s\+[^=]\+\s\+=\s\+\S\+.*$'
    return match(prevline, '=')
  endif

  if prevline =~ '\<with\>\s\+([^)]*)\s*$'
    return match(prevline, '\S') + &shiftwidth
  endif

  if prevline =~ '\<case\>\s\+.\+\<of\>\s*$'
    return match(prevline, '\<case\>') + g:idris_indent_case
  endif

  if prevline =~ '^\s*\(\<namespace\>\|\<\(co\)\?data\>\)\s\+\S\+\s*$'
    return match(prevline, '\(\<namespace\>\|\<\(co\)\?data\>\)') + &shiftwidth
  endif

  if prevline =~ '^\s*\(\<using\>\|\<parameters\>\)\s*([^(]*)\s*$'
    return match(prevline, '\(\<using\>\|\<parameters\>\)') + &shiftwidth
  endif

  if prevline =~ '^\s*\<mutual\>\s*$'
    return match(prevline, '\<mutual\>') + &shiftwidth
  endif

  let line = getline(v:lnum)

  if (line =~ '^\s*}\s*' && prevline !~ '^\s*;')
    return match(prevline, '\S') - &shiftwidth
  endif

  return match(prevline, '\S')
endfunction

" vim:et:sw=2:sts=2
