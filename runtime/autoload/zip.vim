" zip.vim: Handles browsing zipfiles
"            AUTOLOAD PORTION
" Date:			May 01, 2006
" Version:		9
" Maintainer:	Charles E Campbell, Jr <drchipNOSPAM at campbellfamily dot biz>
" License:		Vim License  (see vim's :help license)
" Copyright:    Copyright (C) 2005 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               zipPlugin.vim is provided *as is* and comes with no warranty
"               of any kind, either expressed or implied. By using this
"               plugin, you agree that in no event will the copyright
"               holder be liable for any damages resulting from the use
"               of this software.

" ---------------------------------------------------------------------
" Initialization: {{{1
let s:keepcpo= &cpo
set cpo&vim
if exists("g:loaded_zip")
 finish
endif

let g:loaded_zip     = "v9"
let s:zipfile_escape = ' ?&;\'

" ----------------
"  Functions: {{{1
" ----------------

" ---------------------------------------------------------------------
" zip#Browse: {{{2
fun! zip#Browse(zipfile)
"  call Dfunc("zip#Browse(zipfile<".a:zipfile.">)")
  let repkeep= &report
  set report=10

  " sanity checks
  if !executable("unzip")
   echohl Error | echo "***error*** (zip#Browse) unzip not available on your system"
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("zip#Browse")
   return
  endif
  if !filereadable(a:zipfile)
   if a:zipfile !~# '^\a\+://'
    " if its an url, don't complain, let url-handlers such as vim do its thing
    echohl Error | echo "***error*** (zip#Browse) File not readable<".a:zipfile.">" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
   let &report= repkeep
"   call Dret("zip#Browse : file<".a:zipfile."> not readable")
   return
  endif
"  call Decho("passed sanity checks")
  if &ma != 1
   set ma
  endif
  let w:zipfile= a:zipfile

  setlocal noswapfile
  setlocal buftype=nofile
  setlocal bufhidden=hide
  setlocal nobuflisted
  setlocal nowrap
  set ft=tar

  " give header
  exe "$put ='".'\"'." zip.vim version ".g:loaded_zip."'"
  exe "$put ='".'\"'." Browsing zipfile ".a:zipfile."'"
  exe "$put ='".'\"'." Select a file with cursor and press ENTER"."'"
  $put =''
  0d
  $

"  call Decho("exe silent r! unzip -l '".a:zipfile."'")
  exe "silent r! unzip -l '".a:zipfile."'"
  if v:shell_error != 0
   echohl WarningMsg | echo "***warning*** (zip#Browse) ".a:zipfile." is not a zip file" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   silent %d
   let eikeep= &ei
   set ei=BufReadCmd,FileReadCmd
   exe "r ".a:zipfile
   let &ei= eikeep
   1d
"   call Dret("zip#Browse")
   return
  endif
"  call Decho("line 6: ".getline(6))
  let namecol= stridx(getline(6),'Name') + 1
"  call Decho("namecol=".namecol)
  4,$g/^\s*----/d
  4,$g/^\s*\a/d
  $d
  if namecol > 0
   exe 'silent 4,$s/^.*\%'.namecol.'c//'
  endif

  setlocal noma nomod ro
  noremap <silent> <buffer> <cr> :call <SID>ZipBrowseSelect()<cr>

  let &report= repkeep
"  call Dret("zip#Browse")
endfun

" ---------------------------------------------------------------------
" ZipBrowseSelect: {{{2
fun! s:ZipBrowseSelect()
"  call Dfunc("ZipBrowseSelect() zipfile<".w:zipfile."> curfile<".expand("%").">")
  let repkeep= &report
  set report=10
  let fname= getline(".")

  " sanity check
  if fname =~ '^"'
   let &report= repkeep
"   call Dret("ZipBrowseSelect")
   return
  endif
  if fname =~ '/$'
   echohl Error | echo "***error*** (zip#Browse) Please specify a file, not a directory" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("ZipBrowseSelect")
   return
  endif

"  call Decho("fname<".fname.">")

  " get zipfile to the new-window
  let zipfile= substitute(w:zipfile,'.zip$','','e')
  let curfile= expand("%")
"  call Decho("zipfile<".zipfile.">")
"  call Decho("curfile<".curfile.">")

  new
  wincmd _
  let s:zipfile_{winnr()}= curfile
"  call Decho("exe e zipfile:".escape(zipfile,s:zipfile_escape).'::'.escape(fname,s:zipfile_escape))
  exe "e zipfile:".escape(zipfile,s:zipfile_escape).'::'.escape(fname,s:zipfile_escape)
  filetype detect

  let &report= repkeep
"  call Dret("ZipBrowseSelect : s:zipfile_".winnr()."<".s:zipfile_{winnr()}.">")
endfun

" ---------------------------------------------------------------------
" zip#Read: {{{2
fun! zip#Read(fname,mode)
"  call Dfunc("zip#Read(fname<".a:fname.">,mode=".a:mode.")")
  let repkeep= &report
  set report=10

  if has("unix")
   let zipfile = substitute(a:fname,'zipfile:\(.\{-}\)::[^\\].*$','\1','')
   let fname   = substitute(a:fname,'zipfile:.\{-}::\([^\\].*\)$','\1','')
  else
   let zipfile = substitute(a:fname,'^.\{-}zipfile:\(.\{-}\)::[^\\].*$','\1','')
   let fname   = substitute(a:fname,'^.\{-}zipfile:.\{-}::\([^\\].*\)$','\1','')
  endif
"  call Decho("zipfile<".zipfile.">")
"  call Decho("fname  <".fname.">")

"  call Decho("exe r! unzip -p '".zipfile."' '".fname."'")
  exe "silent r! unzip -p '".zipfile."' '".fname."'"

  " cleanup
  0d
  set nomod

  let &report= repkeep
"  call Dret("zip#Read")
endfun

" ---------------------------------------------------------------------
" zip#Write: {{{2
fun! zip#Write(fname)
"  call Dfunc("zip#Write(fname<".a:fname.">) zipfile_".winnr()."<".s:zipfile_{winnr()}.">")
  let repkeep= &report
  set report=10

  " sanity checks
  if !executable("zip")
   echohl Error | echo "***error*** (zip#Write) sorry, your system doesn't appear to have the zip pgm" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("zip#Write")
   return
  endif
  if !exists("*mkdir")
   echohl Error | echo "***error*** (zip#Write) sorry, mkdir() doesn't work on your system" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("zip#Write")
   return
  endif

  let curdir= getcwd()
  let tmpdir= tempname()
"  call Decho("orig tempname<".tmpdir.">")
  if tmpdir =~ '\.'
   let tmpdir= substitute(tmpdir,'\.[^.]*$','','e')
  endif
"  call Decho("tmpdir<".tmpdir.">")
  call mkdir(tmpdir,"p")

  " attempt to change to the indicated directory
  try
   exe "cd ".escape(tmpdir,' \')
  catch /^Vim\%((\a\+)\)\=:E344/
   echohl Error | echo "***error*** (zip#Write) cannot cd to temporary directory" | Echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("zip#Write")
   return
  endtry
"  call Decho("current directory now: ".getcwd())

  " place temporary files under .../_ZIPVIM_/
  if isdirectory("_ZIPVIM_")
   call s:Rmdir("_ZIPVIM_")
  endif
  call mkdir("_ZIPVIM_")
  cd _ZIPVIM_
"  call Decho("current directory now: ".getcwd())

  if has("unix")
   let zipfile = substitute(a:fname,'zipfile:\(.\{-}\)::[^\\].*$','\1','')
   let fname   = substitute(a:fname,'zipfile:.\{-}::\([^\\].*\)$','\1','')
  else
   let zipfile = substitute(a:fname,'^.\{-}zipfile:\(.\{-}\)::[^\\].*$','\1','')
   let fname   = substitute(a:fname,'^.\{-}zipfile:.\{-}::\([^\\].*\)$','\1','')
  endif
"  call Decho("zipfile<".zipfile.">")
"  call Decho("fname  <".fname.">")

  if fname =~ '/'
   let dirpath = substitute(fname,'/[^/]\+$','','e')
   if executable("cygpath")
    let dirpath = substitute(system("cygpath ".dirpath),'\n','','e')
   endif
"   call Decho("mkdir(dirpath<".dirpath.">,p)")
   call mkdir(dirpath,"p")
  endif
  if zipfile !~ '/'
   let zipfile= curdir.'/'.zipfile
  endif
"  call Decho("zipfile<".zipfile."> fname<".fname.">")

  exe "w! ".escape(fname,s:zipfile_escape)
  if executable("cygpath")
   let zipfile = substitute(system("cygpath ".zipfile),'\n','','e')
  endif

"  call Decho("zip -u '".zipfile.".zip' '".fname."'")
  call system("zip -u '".zipfile.".zip' '".fname."'")
  if v:shell_error != 0
   echohl Error | echo "***error*** (zip#Write) sorry, unable to update ".zipfile." with ".fname | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()

  elseif s:zipfile_{winnr()} =~ '^\a\+://'
   " support writing zipfiles across a network
   let netzipfile= s:zipfile_{winnr()}
"   call Decho("handle writing <".zipfile.".zip> across network as <".netzipfile.">")
   1split|enew
   let binkeep= &binary
   let eikeep = &ei
   set binary ei=all
   exe "e! ".zipfile.".zip"
   call netrw#NetWrite(netzipfile)
   let &ei     = eikeep
   let &binary = binkeep
   q!
   unlet s:zipfile_{winnr()}
  endif
  
  " cleanup and restore current directory
  cd ..
  call s:Rmdir("_ZIPVIM_")
  exe "cd ".escape(curdir,' \')
  setlocal nomod

  let &report= repkeep
"  call Dret("zip#Write")
endfun

" ---------------------------------------------------------------------
" Rmdir: {{{2
fun! s:Rmdir(fname)
"  call Dfunc("Rmdir(fname<".a:fname.">)")
  if has("unix")
   call system("/bin/rm -rf ".a:fname)
  elseif has("win32") || has("win95") || has("win64") || has("win16")
   if &shell =~? "sh$"
    call system("/bin/rm -rf ".a:fname)
   else
    call system("del /S ".a:fname)
   endif
  endif
"  call Dret("Rmdir")
endfun

" ------------------------------------------------------------------------
" Modelines And Restoration: {{{1
let &cpo= s:keepcpo
unlet s:keepcpo
" vim:ts=8 fdm=marker
