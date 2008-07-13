" Vim indent file
" Language:	HAML
" Maintainer:	Tim Pope <vimNOSPAM@tpope.info>
" Last Change:	2007 Dec 16

if exists("b:did_indent")
  finish
endif
runtime! indent/ruby.vim
unlet! b:did_indent
let b:did_indent = 1

setlocal autoindent sw=2 et
setlocal indentexpr=GetHamlIndent()
setlocal indentkeys=o,O,*<Return>,},],0),!^F,=end,=else,=elsif,=rescue,=ensure,=when

" Only define the function once.
if exists("*GetHamlIndent")
  finish
endif

let s:attributes = '\%({.\{-\}}\|\[.\{-\}\]\)'
let s:tag = '\%([%.#][[:alnum:]_-]\+\|'.s:attributes.'\)*[<>]*'

if !exists('g:haml_self_closing_tags')
  let g:haml_self_closing_tags = 'meta|link|img|hr|br'
endif

function! GetHamlIndent()
  let lnum = prevnonblank(v:lnum-1)
  if lnum == 0
    return 0
  endif
  let line = substitute(getline(lnum),'\s\+$','','')
  let cline = substitute(substitute(getline(v:lnum),'\s\+$','',''),'^\s\+','','')
  let lastcol = strlen(line)
  let line = substitute(line,'^\s\+','','')
  let indent = indent(lnum)
  let cindent = indent(v:lnum)
  if cline =~# '\v^-\s*%(elsif|else|when)>'
    let indent = cindent < indent ? cindent : indent - &sw
  endif
  let increase = indent + &sw
  if indent == indent(lnum)
    let indent = cindent <= indent ? -1 : increase
  endif
  "let indent = indent == indent(lnum) ? -1 : indent
  "let indent = indent > indent(lnum) + &sw ? indent(lnum) + &sw : indent

  let group = synIDattr(synID(lnum,lastcol,1),'name')

  if line =~ '^!!!'
    return indent
  elseif line =~ '^/\%(\[[^]]*\]\)\=$'
    return increase
  elseif line =~ '^:'
    return increase
  elseif line =~ '^'.s:tag.'[=~-]\s*\%(\%(if\|else\|elsif\|unless\|case\|when\|while\|until\|for\|begin\|module\|class\|def\)\>\%(.*\<end\>\)\@!\|.*do |[^|]*|\s*$\)'
    return increase
  elseif line == '-#'
    return increase
  elseif group =~? '\v^(hamlSelfCloser)$' || line =~? '^%\v%('.g:haml_self_closing_tags.')>'
    return indent
  elseif group =~? '\v^%(hamlTag|hamlAttributesDelimiter|hamlObjectDelimiter|hamlClass|hamlId|htmlTagName|htmlSpecialTagName)$'
    return increase
  elseif synIDattr(synID(v:lnum,1,1),'name') ==? 'hamlRubyFilter'
    return GetRubyIndent()
  else
    return indent
  endif
endfunction

" vim:set sw=2:
