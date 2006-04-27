" vimball : construct a file containing both paths and files
" Author: Charles E. Campbell, Jr.
" GetLatestVimScripts: 1502 1 :AutoInstall: vimball.vim
" Copyright: (c) 2004-2006 by Charles E. Campbell, Jr.
"            The VIM LICENSE applies to Vimball.vim, and Vimball.txt
"            (see |copyright|) except use "Vimball" instead of "Vim".
"            No warranty, express or implied.
"  *** ***   Use At-Your-Own-Risk!   *** ***

" ---------------------------------------------------------------------
"  Load Once: {{{1
if &cp || exists("g:loaded_vimball") || exists("g:loaded_vimballplugin")
 finish
endif
let g:loaded_vimballplugin= 1

let s:keepcpo= &cpo
set cpo&vim

" ------------------------------------------------------------------------------
" Public Interface: {{{1
com! -ra -na=+ -bang MkVimball call vimball#MkVimball(<line1>,<line2>,<bang>0,<f-args>)
com! -na=0 UseVimball  call vimball#Vimball(1)
com! -na=0 VimballList call vimball#Vimball(0)
au BufEnter *.vba.gz,*.vba.bz2,*.vba.zip call vimball#Decompress(expand("<amatch>"))
au BufEnter *.vba call vimball#ShowMesg("Source this file to extract it! (:so %)")

let &cpo= s:keepcpo
unlet s:keepcpo
" =====================================================================
" Modelines: {{{1
" vim: fdm=marker
