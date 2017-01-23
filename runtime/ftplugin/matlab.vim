" Vim filetype plugin file
" Language:	matlab
" Maintainer:	Jake Wasserman <jwasserman at gmail dot com>
" Last Changed: 2017 Jan 23

" Contributors:
" Charles Campbell
" Alex Burka

if exists("b:did_ftplugin")
	finish
endif
let b:did_ftplugin = 1

let s:save_cpo = &cpo
set cpo-=C

if exists("loaded_matchit")
 let s:conditionalEnd = '\%(([^()]*\)\@!\<end\>\%([^()]*)\)\@!'
 let b:match_words=
   \ '\<\%(if\|switch\|for\|while\)\>:\<\%(elseif\|case\|break\|continue\|else\|otherwise\)\>:'.s:conditionalEnd.','.
   \ '\<function\>:\<return\>:\<endfunction\>'
 unlet s:conditionalEnd
endif

setlocal suffixesadd=.m
setlocal suffixes+=.asv

" redefine section movement commands for cell mode
function! s:NextSection(type, backwards, visual)
	if a:visual
		normal! gv
	endif
	if a:backwards
		let dir = '?'
	else
		let dir = '/'
	endif
	execute 'silent normal! ' . dir . '^%%' . "\r"
endfunction
noremap <script> <buffer> <silent> ]] :call <SID>NextSection(1, 0, 0)<cr>
noremap <script> <buffer> <silent> [[ :call <SID>NextSection(1, 1, 0)<cr>
noremap <script> <buffer> <silent> ][ :call <SID>NextSection(2, 0, 0)<cr>
noremap <script> <buffer> <silent> [] :call <SID>NextSection(2, 1, 0)<cr>
vnoremap <script> <buffer> <silent> ]] :<c-u>call <SID>NextSection(1, 0, 1)<cr>
vnoremap <script> <buffer> <silent> [[ :<c-u>call <SID>NextSection(1, 1, 1)<cr>
vnoremap <script> <buffer> <silent> ][ :<c-u>call <SID>NextSection(2, 0, 1)<cr>
vnoremap <script> <buffer> <silent> [] :<c-u>call <SID>NextSection(2, 1, 1)<cr>

let b:undo_ftplugin = "setlocal suffixesadd< suffixes< "
	\ . "| unlet! b:match_words"
	\ . "| nunmap ]] | nunmap [[ | nunmap ][ | nunmap []"
	\ . "| vunmap ]] | vunmap [[ | vunmap ][ | vunmap []"

let &cpo = s:save_cpo
unlet s:save_cpo
