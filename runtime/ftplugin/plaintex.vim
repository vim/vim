" plain TeX filetype plugin
" Language:     plain TeX (ft=plaintex)
" Maintainer:   Benji Fisher, Ph.D. <benji@member.AMS.org>
" Version:	1.0
" Last Change:	Wed 22 Mar 2006 09:36:32 AM EST

" Only do this when not done yet for this buffer.
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer.
let b:did_ftplugin = 1

" Avoid problems if running in 'compatible' mode.
let s:save_cpo = &cpo
set cpo&vim

" Set 'comments' to format dashed lists in comments
setlocal com=sO:%\ -,mO:%\ \ ,eO:%%,:%

" Set 'commentstring' to recognize the % comment character:
" (Thanks to Ajit Thakkar.)
setlocal cms=%%s

" Allow "[d" to be used to find a macro definition:
let &l:define='\\\([egx]\|char\|mathchar\|count\|dimen\|muskip\|skip\|toks\)\='
	\ .	'def\|\\font\|\\\(future\)\=let'
	\ . '\|\\new\(count\|dimen\|skip\|muskip\|box\|toks\|read\|write'
	\ .	'\|fam\|insert\)'

" Tell Vim to recognize \input bar :
let &l:include = '\\input'
setlocal suffixesadd=.tex

" The following lines enable the macros/matchit.vim plugin for
" extended matching with the % key.
" There is no default meaning for \(...\) etc., but many users define one.
if exists("loaded_matchit")
  let b:match_ignorecase = 0
    \ | let b:match_skip = 'r:\\\@<!\%(\\\\\)*%'
    \ | let b:match_words = '(:),\[:],{:},\\(:\\),\\\[:\\],' .
    \ '\\begin\s*\({\a\+\*\=}\):\\end\s*\1'
endif " exists("loaded_matchit")

let &cpo = s:save_cpo

" vim:sts=2:sw=2:
