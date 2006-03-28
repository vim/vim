" Vim syntax file
" Language:	Debian sources.list
" Maintainer:	Matthijs Mohlmann <matthijs@cacholong.nl>
" Last Change:	$Date$
" URL: http://www.cacholong.nl/~matthijs/vim/syntax/debsources.vim
" $Revision$

" this is a very simple syntax file - I will be improving it
" add entire DEFINE syntax

" Standard syntax initialization
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" case sensitive
syn case match

" A bunch of useful keywords
syn match debsourcesKeyword        /\(deb-src\|deb\|main\|contrib\|non-free\)/

" Match comments
syn match debsourcesComment        /#.*/

" Match uri's
syn match debsourcesUri            +\(http://\|ftp://\|file:///\)[^' 	<>"]\++
syn match debsourcesDistrKeyword   +\([[:alnum:]_./]*\)\(woody\|sarge\|etch\|old-stable\|stable\|testing\|unstable\|sid\|experimental\|warty\|hoary\|breezy\)\([[:alnum:]_./]*\)+

" Associate our matches and regions with pretty colours
hi def link debsourcesLine            Error
hi def link debsourcesKeyword         Statement
hi def link debsourcesDistrKeyword    Type
hi def link debsourcesComment         Comment
hi def link debsourcesUri             Constant

let b:current_syntax = "debsources"

" vim: ts=8
