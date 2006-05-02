" tar.vim: Handles browsing tarfiles
"            AUTOLOAD PORTION
" Date:			May 02, 2006
" Version:		9
" Maintainer:	Charles E Campbell, Jr <drchipNOSPAM at campbellfamily dot biz>
" License:		Vim License  (see vim's :help license)
"
"	Contains many ideas from Michael Toren's <tar.vim>
"
" Copyright:    Copyright (C) 2005 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               tarPlugin.vim is provided *as is* and comes with no warranty
"               of any kind, either expressed or implied. By using this
"               plugin, you agree that in no event will the copyright
"               holder be liable for any damages resulting from the use
"               of this software.

" ---------------------------------------------------------------------
" Initialization: {{{1
let s:keepcpo= &cpo
set cpo&vim
if exists("g:loaded_tar")
 finish
endif
let g:loaded_tar= "v9"
"call Decho("loading autoload/tar.vim")

" ---------------------------------------------------------------------
"  Default Settings: {{{1
if !exists("g:tar_browseoptions")
 let g:tar_browseoptions= "Ptf"
endif
if !exists("g:tar_readoptions")
 let g:tar_readoptions= "OPxf"
endif
if !exists("g:tar_cmd")
 let g:tar_cmd= "tar"
endif
if !exists("g:tar_writeoptions")
 let g:tar_writeoptions= "uf"
endif

" ----------------
"  Functions: {{{1
" ----------------

" ---------------------------------------------------------------------
" tar#Browse: {{{2
fun! tar#Browse(tarfile)
"  call Dfunc("tar#Browse(tarfile<".a:tarfile.">)")
  let repkeep= &report
  set report=10

  " sanity checks
  if !executable(g:tar_cmd)
   echohl Error | echo '***error*** (tar#Browse) "'.g:tar_cmd.'" not available on your system'
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("tar#Browse")
   return
  endif
  if !filereadable(a:tarfile)
"   call Decho('a:tarfile<'.a:tarfile.'> not filereadable')
   if a:tarfile !~# '^\a\+://'
    " if its an url, don't complain, let url-handlers such as vim do its thing
    echohl Error | echo "***error*** (tar#Browse) File not readable<".a:tarfile.">" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
   let &report= repkeep
"   call Dret("tar#Browse : file<".a:tarfile."> not readable")
   return
  endif
  if &ma != 1
   set ma
  endif
  let w:tarfile= a:tarfile

  setlocal noswapfile
  setlocal buftype=nofile
  setlocal bufhidden=hide
  setlocal nobuflisted
  setlocal nowrap
  set ft=tar

  " give header
"  call Decho("printing header")
  exe "$put ='".'\"'." tar.vim version ".g:loaded_tar."'"
  exe "$put ='".'\"'." Browsing tarfile ".a:tarfile."'"
  exe "$put ='".'\"'." Select a file with cursor and press ENTER"."'"
  0d
  $

  let tarfile= a:tarfile
  if has("win32") && executable("cygpath")
   " assuming cygwin
   let tarfile=substitute(system("cygpath -u ".tarfile),'\n$','','e')
  endif
  let curlast= line("$")
  if tarfile =~# '\.\(gz\|tgz\)$'
"   call Decho("exe silent r! gzip -d -c '".tarfile."'| tar -".g:tar_browseoptions." - ")
   exe "silent r! gzip -d -c '".tarfile."'| tar -".g:tar_browseoptions." - "
  elseif tarfile =~# '\.bz2$'
"   call Decho("exe silent r! bzip2 -d -c '".tarfile."'| tar -".g:tar_browseoptions." - ")
   exe "silent r! bzip2 -d -c '".tarfile."'| tar -".g:tar_browseoptions." - "
  else
"   call Decho("exe silent r! ".g:tar_cmd." -".g:tar_browseoptions." '".tarfile."'")
   exe "silent r! ".g:tar_cmd." -".g:tar_browseoptions." '".tarfile."'"
  endif
  if v:shell_error != 0
   echohl WarningMsg | echo "***warning*** (tar#Browse) please check your g:tar_browseoptions<".g:tar_browseoptions.">"
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"   call Dret("tar#Browse : a:tarfile<".a:tarfile.">")
   silent %d
   let eikeep= &ei
   set ei=BufReadCmd,FileReadCmd
   exe "r ".a:tarfile
   let &ei= eikeep
   1d
   return
  endif
  if line("$") == curlast || ( line("$") == (curlast + 1) && getline("$") =~? '\c\%(warning\|error\|inappropriate\|unrecognized\)')
   echohl WarningMsg | echo "***warning*** (tar#Browse) ".a:tarfile." doesn't appear to be a tar file" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   silent %d
   let eikeep= &ei
   set ei=BufReadCmd,FileReadCmd
   exe "r ".a:tarfile
   let &ei= eikeep
   1d
"   call Dret("tar#Browse : a:tarfile<".a:tarfile.">")
   return
  endif

  setlocal noma nomod ro
  noremap <silent> <buffer> <cr> :call <SID>TarBrowseSelect()<cr>

  let &report= repkeep
"  call Dret("tar#Browse : w:tarfile<".w:tarfile.">")
endfun

" ---------------------------------------------------------------------
" TarBrowseSelect: {{{2
fun! s:TarBrowseSelect()
"  call Dfunc("TarBrowseSelect() w:tarfile<".w:tarfile."> curfile<".expand("%").">")
  let repkeep= &report
  set report=10
  let fname= getline(".")
"  call Decho("fname<".fname.">")

  " sanity check
  if fname =~ '^"'
   let &report= repkeep
"   call Dret("TarBrowseSelect")
   return
  endif

  " about to make a new window, need to use w:tarfile
  let tarfile= w:tarfile
  let curfile= expand("%")
  if has("win32") && executable("cygpath")
   " assuming cygwin
   let tarfile=substitute(system("cygpath -u ".tarfile),'\n$','','e')
  endif

  new
  wincmd _
  let s:tblfile_{winnr()}= curfile
  call tar#Read("tarfile:".tarfile.':'.fname,1)
  filetype detect

  let &report= repkeep
"  call Dret("TarBrowseSelect : s:tblfile_".winnr()."<".s:tblfile_{winnr()}.">")
endfun

" ---------------------------------------------------------------------
" tar#Read: {{{2
fun! tar#Read(fname,mode)
"  call Dfunc("tar#Read(fname<".a:fname.">,mode=".a:mode.")")
  let repkeep= &report
  set report=10
  let tarfile = substitute(a:fname,'tarfile:\(.\{-}\):.*$','\1','')
  let fname   = substitute(a:fname,'tarfile:.\{-}:\(.*\)$','\1','')
  if has("win32") && executable("cygpath")
   " assuming cygwin
   let tarfile=substitute(system("cygpath -u ".tarfile),'\n$','','e')
  endif
"  call Decho("tarfile<".tarfile.">")
"  call Decho("fname<".fname.">")

  if tarfile =~# '\.\(gz\|tgz\)$'
"   call Decho("exe silent r! gzip -d -c '".tarfile."'| tar -OPxf - '".fname."'")
   exe "silent r! gzip -d -c '".tarfile."'| tar -".g:tar_readoptions." - '".fname."'"
  elseif tarfile =~# '\.bz2$'
"   call Decho("exe silent r! bzip2 -d -c '".tarfile."'| tar -".g:tar_readoptions." - '".fname."'")
   exe "silent r! bzip2 -d -c '".tarfile."'| tar -".g:tar_readoptions." - '".fname."'"
  else
"   call Decho("exe silent r! tar -".g:tar_readoptions." '".tarfile."' '".fname."'")
   exe "silent r! ".g:tar_cmd." -".g:tar_readoptions." '".tarfile."' '".fname."'"
  endif
  let w:tarfile= a:fname
  exe "file tarfile:".fname

  " cleanup
  0d
  set nomod

  let &report= repkeep
"  call Dret("tar#Read : w:tarfile<".w:tarfile.">")
endfun

" ---------------------------------------------------------------------
" tar#Write: {{{2
fun! tar#Write(fname)
"  call Dfunc("tar#Write(fname<".a:fname.">) w:tarfile<".w:tarfile."> tblfile_".winnr()."<".s:tblfile_{winnr()}.">")
  let repkeep= &report
  set report=10

  " sanity checks
  if !executable(g:tar_cmd)
   echohl Error | echo '***error*** (tar#Browse) "'.g:tar_cmd.'" not available on your system'
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("tar#Write")
   return
  endif
  if !exists("*mkdir")
   echohl Error | echo "***error*** (tar#Write) sorry, mkdir() doesn't work on your system" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("tar#Write")
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
   echohl Error | echo "***error*** (tar#Write) cannot cd to temporary directory" | Echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("tar#Write")
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

  let tarfile = substitute(w:tarfile,'tarfile:\(.\{-}\):.*$','\1','')
  let fname   = substitute(w:tarfile,'tarfile:.\{-}:\(.*\)$','\1','')

  " handle compressed archives
  if tarfile =~# '\.gz'
   call system("gzip -d ".tarfile)
   let tarfile = substitute(tarfile,'\.gz','','e')
   let compress= "gzip '".tarfile."'"
  elseif tarfile =~# '\.tgz'
   call system("gzip -d ".tarfile)
   let tarfile = substitute(tarfile,'\.tgz','.tar','e')
   let compress= "gzip '".tarfile."'"
   let tgz     = 1
  elseif tarfile =~# '\.bz2'
   call system("bzip2 -d ".tarfile)
   let tarfile = substitute(tarfile,'\.bz2','','e')
   let compress= "bzip2 '".tarfile."'"
  endif

  if v:shell_error != 0
   echohl Error | echo "***error*** (tar#Write) sorry, unable to update ".tarfile." with ".fname | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
  else

"   call Decho("tarfile<".tarfile."> fname<".fname.">")
 
   if fname =~ '/'
    let dirpath = substitute(fname,'/[^/]\+$','','e')
    if executable("cygpath")
     let dirpath = substitute(system("cygpath ".dirpath),'\n','','e')
    endif
    call mkdir(dirpath,"p")
   endif
   if tarfile !~ '/'
    let tarfile= curdir.'/'.tarfile
   endif
"   call Decho("tarfile<".tarfile."> fname<".fname.">")
 
   exe "w! ".fname
   if executable("cygpath")
    let tarfile = substitute(system("cygpath ".tarfile),'\n','','e')
   endif
 
   " delete old file from tarfile
"   call Decho("tar --delete -f '".tarfile."' '".fname."'")
   call system("tar --delete -f '".tarfile."' '".fname."'")
   if v:shell_error != 0
    echohl Error | echo "***error*** (tar#Write) sorry, unable to update ".tarfile." with ".fname | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   else
 
    " update tarfile with new file 
"    call Decho("tar -".g:tar_writeoptions." '".tarfile."' '".fname."'")
    call system("tar -".g:tar_writeoptions." '".tarfile."' '".fname."'")
    if v:shell_error != 0
     echohl Error | echo "***error*** (tar#Write) sorry, unable to update ".tarfile." with ".fname | echohl None
     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
    elseif exists("compress")
"     call Decho("call system(".compress.")")
     call system(compress)
     if exists("tgz")
"      call Decho("rename(".tarfile.".gz,".substitute(tarfile,'\.tar$','.tgz','e').")")
      call rename(tarfile.".gz",substitute(tarfile,'\.tar$','.tgz','e'))
     endif
    endif
   endif

   " support writing tarfiles across a network
   if s:tblfile_{winnr()} =~ '^\a\+://'
"    call Decho("handle writing <".tarfile."> across network to <".s:tblfile_{winnr()}.">")
    let tblfile= s:tblfile_{winnr()}
    1split|enew
    let binkeep= &binary
    let eikeep = &ei
    set binary ei=all
    exe "e! ".tarfile
    call netrw#NetWrite(tblfile)
    let &ei     = eikeep
    let &binary = binkeep
    q!
    unlet s:tblfile_{winnr()}
   endif
  endif
  
  " cleanup and restore current directory
  cd ..
  call s:Rmdir("_ZIPVIM_")
  exe "cd ".escape(curdir,' \')
  setlocal nomod

  let &report= repkeep
"  call Dret("tar#Write")
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
