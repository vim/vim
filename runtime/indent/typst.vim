" Vim indent file
" Language: Typst
" Maintainer: Kaj Munhoz Arfvidsson <kajarfvidsson@gmail.com>
" Upstream: https://github.com/kaarmu/typst.vim

if exists('b:did_indent')
  finish
endif

let b:did_indent = 1

let s:cpo_save = &cpoptions
set cpoptions&vim

setlocal autoindent
setlocal indentexpr=TypstIndent(v:lnum)
" setlocal indentkeys=... " We use the default

function! TypstIndent(lnum) abort " {{{1
    let s:sw = shiftwidth()
    
    let l:plnum = prevnonblank(a:lnum - 1)
    if l:plnum == 0
	    return 0
    endif

    let l:line = getline(a:lnum)
    let l:pline = getline(l:plnum)

    if l:pline =~ '\v[\{\[\(]\s*$'
      return indent(l:plnum) + s:sw
    endif

    if l:line =~ '\v[\}\]\)]$'
      return indent(a:lnum) - s:sw
    endif

    return indent(l:plnum)
endfunction
" }}}1

let &cpoptions = s:cpo_save
unlet s:cpo_save

" vim: et sts=2 sw=2 ft=vim
