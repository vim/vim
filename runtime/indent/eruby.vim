" Vim indent file
" Language:		eRuby
" Maintainer:		Tim Pope <vimNOSPAM@tpope.info>
" Info:			$Id$
" URL:			http://vim-ruby.rubyforge.org
" Anon CVS:		See above site
" Release Coordinator:	Doug Kearns <dougkearns@gmail.com>

if exists("b:did_indent")
  finish
endif

runtime! indent/ruby.vim
unlet! b:did_indent

runtime! indent/html.vim
unlet! b:did_indent

let b:did_indent = 1

setlocal indentexpr=GetErubyIndent(v:lnum)
setlocal indentkeys=o,O,*<Return>,<>>,{,},0),0],o,O,!^F,=end,=else,=elsif,=rescue,=ensure,=when

" Only define the function once.
if exists("*GetErubyIndent")
  finish
endif

function! GetErubyIndent(lnum)
  let vcol = col('.')
  call cursor(a:lnum,1)
  let inruby = searchpair('<%','','%>')
  call cursor(a:lnum,vcol)
  if inruby && getline(a:lnum) !~ '^<%'
    let ind = GetRubyIndent()
  else
    let ind = HtmlIndentGet(a:lnum)
  endif
  let lnum = prevnonblank(a:lnum-1)
  let line = getline(lnum)
  let cline = getline(a:lnum)
  if cline =~# '<%\s*\%(end\|else\|\%(ensure\|rescue\|elsif\|when\).\{-\}\)\s*\%(-\=%>\|$\)'
    let ind = ind - &sw
  endif
  if line =~# '\<do\%(\s*|[^|]*|\)\=\s*-\=%>'
    let ind = ind + &sw
  elseif line =~# '<%\s*\%(module\|class\|def\|if\|for\|while\|until\|else\|elsif\|case\|when\|unless\|begin\|ensure\|rescue\)\>.*%>'
    let ind = ind + &sw
  endif
  if line =~# '^\s*<%[=#]\=\s*$' && cline !~# '^\s*end\>'
    let ind = ind + &sw
  endif
  if cline =~# '^\s*-\=%>\s*$'
    let ind = ind - &sw
  endif
  return ind
endfunction

" vim:set sw=2 sts=2 ts=8 noet ff=unix:
