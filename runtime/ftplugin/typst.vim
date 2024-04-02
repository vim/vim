" Vim filetype plugin file
" Language: Typst
" Maintainer: Kaj Munhoz Arfvidsson <kajarfvidsson@gmail.com>
" Upstream: https://github.com/kaarmu/typst.vim

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

let s:cpo_orig = &cpo
set cpo&vim

call typst#options#init()

compiler typst

" " If you're on typst <v0.8, workaround for https://github.com/typst/typst/issues/1937
" set errorformat^=\/%f:%l:%c:%m

setlocal expandtab
setlocal tabstop=8
setlocal softtabstop=2
setlocal shiftwidth=2

if g:typst_conceal
    setlocal conceallevel=2
endif

setlocal commentstring=//%s
setlocal comments=s1:/*,mb:*,ex:*/,://

setlocal formatoptions+=croq

if has('win32')
    setlocal iskeyword=a-z,A-Z,48-57,_,-,128-167,224-235
else
    setlocal iskeyword=a-z,A-Z,48-57,_,-,192-255
endif

setlocal suffixesadd=.typ

command! -nargs=* -buffer TypstWatch call typst#TypstWatch(<f-args>)

command! -buffer Toc call typst#Toc('vertical')
command! -buffer Toch call typst#Toc('horizontal')
command! -buffer Tocv call typst#Toc('vertical')
command! -buffer Toct call typst#Toc('tab')

let &cpo = s:cpo_orig
unlet s:cpo_orig

" vim: tabstop=8 shiftwidth=4 softtabstop=4 expandtab
