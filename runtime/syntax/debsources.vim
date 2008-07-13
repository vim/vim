" Vim syntax file
" Language:     Debian sources.list
" Maintainer:   Debian Vim Maintainers <pkg-vim-maintainers@lists.alioth.debian.org>
" Former Maintainer: Matthijs Mohlmann <matthijs@cacholong.nl>
" Last Change: 2008-04-25
" URL: http://git.debian.org/?p=pkg-vim/vim.git;a=blob_plain;f=runtime/syntax/debsources.vim;hb=debian

" Standard syntax initialization
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" case sensitive
syn case match

" A bunch of useful keywords
syn match debsourcesKeyword        /\(deb-src\|deb\|main\|contrib\|non-free\|restricted\|universe\|multiverse\)/

" Match comments
syn match debsourcesComment        /#.*/

" Match uri's
syn match debsourcesUri            +\(http://\|ftp://\|[rs]sh://\|debtorrent://\|\(cdrom\|copy\|file\):\)[^' 	<>"]\++
syn match debsourcesDistrKeyword   +\([[:alnum:]_./]*\)\(sarge\|etch\|lenny\|\(old\)\=stable\|testing\|unstable\|sid\|experimental\|dapper\|feisty\|gutsy\|hardy\|intrepid\)\([-[:alnum:]_./]*\)+

" Associate our matches and regions with pretty colours
hi def link debsourcesLine            Error
hi def link debsourcesKeyword         Statement
hi def link debsourcesDistrKeyword    Type
hi def link debsourcesComment         Comment
hi def link debsourcesUri             Constant

let b:current_syntax = "debsources"

" vim: ts=8
