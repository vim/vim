" tar.vim: Handles browsing tarfiles
"            AUTOLOAD PORTION
" Date:			Aug 08, 2008
" Version:		23 + modifications by Bram
" Maintainer:	Charles E Campbell, Jr <NdrOchip@ScampbellPfamily.AbizM-NOSPAM>
" License:		Vim License  (see vim's :help license)
"
"	Contains many ideas from Michael Toren's <tar.vim>
"
" Copyright:    Copyright (C) 2005-2008 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               tar.vim and tarPlugin.vim are provided *as is* and comes
"               with no warranty of any kind, either expressed or implied.
"               By using this plugin, you agree that in no event will the
"               copyright holder be liable for any damages resulting from
"               the use of this software.

" ---------------------------------------------------------------------
" Load Once: {{{1
let s:keepcpo= &cpo
set cpo&vim
if &cp || exists("g:loaded_tar") || v:version < 700
 finish
endif
let g:loaded_tar= "v23b"
"call Decho("loading autoload/tar.vim")
if v:version < 701 || (v:version == 701 && !has("patch299"))
 echoerr "(autoload/tar.vim) need vim v7.1 with patchlevel 299"
endif

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

if !exists("g:netrw_cygwin")
 if has("win32") || has("win95") || has("win64") || has("win16")
  if &shell =~ '\%(\<bash\>\|\<zsh\>\)\%(\.exe\)\=$'
   let g:netrw_cygwin= 1
  else
   let g:netrw_cygwin= 0
  endif
 else
  let g:netrw_cygwin= 0
 endif
endif

" set up shell quoting character
if !exists("g:tar_shq")
 if exists("&shq") && &shq != ""
  let g:tar_shq= &shq
 elseif has("win32") || has("win95") || has("win64") || has("win16")
  if exists("g:netrw_cygwin") && g:netrw_cygwin
   let g:tar_shq= "'"
  else
   let g:tar_shq= '"'
  endif
 else
  let g:tar_shq= "'"
 endif
" call Decho("g:tar_shq<".g:tar_shq.">")
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
   redraw!
   echohl Error | echo '***error*** (tar#Browse) "'.g:tar_cmd.'" not available on your system'
"   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("tar#Browse")
   return
  endif
  if !filereadable(a:tarfile)
"   call Decho('a:tarfile<'.a:tarfile.'> not filereadable')
   if a:tarfile !~# '^\a\+://'
    " if its an url, don't complain, let url-handlers such as vim do its thing
    redraw!
    echohl Error | echo "***error*** (tar#Browse) File not readable<".a:tarfile.">" | echohl None
"    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
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
  let lastline= line("$")
  call setline(lastline+1,'" tar.vim version '.g:loaded_tar)
  call setline(lastline+2,'" Browsing tarfile '.a:tarfile)
  call setline(lastline+3,'" Select a file with cursor and press ENTER')
  $put =''
  0d
  $

  let tarfile= a:tarfile
  if has("win32") && executable("cygpath")
   " assuming cygwin
   let tarfile=substitute(system("cygpath -u ".s:Escape(tarfile,0)),'\n$','','e')
  endif
  let curlast= line("$")
  if tarfile =~# '\.\(gz\|tgz\)$'
"   call Decho("1: exe silent r! gzip -d -c -- ".s:Escape(tarfile,1)." | ".g:tar_cmd." -".g:tar_browseoptions." - ")
   exe "silent r! gzip -d -c -- ".s:Escape(tarfile,1)." | ".g:tar_cmd." -".g:tar_browseoptions." - "
  elseif tarfile =~# '\.lrp'
"   call Decho("2: exe silent r! cat -- ".s:Escape(tarfile,1)."|gzip -d -c -|".g:tar_cmd." -".g:tar_browseoptions." - ")
   exe "silent r! cat -- ".s:Escape(tarfile,1)."|gzip -d -c -|".g:tar_cmd." -".g:tar_browseoptions." - "
  elseif tarfile =~# '\.bz2$'
"   call Decho("3: exe silent r! bzip2 -d -c -- ".s:Escape(tarfile,1)." | ".g:tar_cmd." -".g:tar_browseoptions." - ")
   exe "silent r! bzip2 -d -c -- ".s:Escape(tarfile,1)." | ".g:tar_cmd." -".g:tar_browseoptions." - "
  else
   if tarfile =~ '^\s*-'
    " A file name starting with a dash may be taken as an option.  Prepend ./ to avoid that.
    let tarfile = substitute(tarfile, '-', './-', '')
   endif
"   call Decho("4: exe silent r! ".g:tar_cmd." -".g:tar_browseoptions." ".s:Escape(tarfile,1))
   exe "silent r! ".g:tar_cmd." -".g:tar_browseoptions." ".s:Escape(tarfile,1)
  endif
  if v:shell_error != 0
   redraw!
   echohl WarningMsg | echo "***warning*** (tar#Browse) please check your g:tar_browseoptions<".g:tar_browseoptions.">"
"   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"   call Dret("tar#Browse : a:tarfile<".a:tarfile.">")
   return
  endif
  if line("$") == curlast || ( line("$") == (curlast + 1) && getline("$") =~ '\c\%(warning\|error\|inappropriate\|unrecognized\)')
   redraw!
   echohl WarningMsg | echo "***warning*** (tar#Browse) ".a:tarfile." doesn't appear to be a tar file" | echohl None
"   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   silent %d
   let eikeep= &ei
   set ei=BufReadCmd,FileReadCmd
   exe "r ".fnameescape(a:tarfile)
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

  if !exists("g:tar_secure") && fname =~ '^\s*-\|\s\+-'
   redraw!
   echohl WarningMsg | echo '***error*** (tar#BrowseSelect) rejecting tarfile member<'.fname.'> because of embedded "-"; See :help tar-options'
"   call Dret('tar#BrowseSelect : rejecting tarfile member<'.fname.'> because of embedded "-"')
   return
  endif

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
   let tarfile=substitute(system("cygpath -u ".s:Escape(tarfile,0)),'\n$','','e')
  endif

  new
  if !exists("g:tar_nomax") || g:tar_nomax == 0
   wincmd _
  endif
  let s:tblfile_{winnr()}= curfile
  call tar#Read("tarfile:".tarfile.'::'.fname,1)
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
  let tarfile = substitute(a:fname,'tarfile:\(.\{-}\)::.*$','\1','')
  let fname   = substitute(a:fname,'tarfile:.\{-}::\(.*\)$','\1','')
  if has("win32") && executable("cygpath")
   " assuming cygwin
   let tarfile=substitute(system("cygpath -u ".s:Escape(tarfile,0)),'\n$','','e')
  endif
"  call Decho("tarfile<".tarfile.">")
"  call Decho("fname<".fname.">")

  if      fname =~ '\.gz$'  && executable("zcat")
   let decmp= "|zcat"
   let doro = 1
  elseif  fname =~ '\.bz2$' && executable("bzcat")
   let decmp= "|bzcat"
   let doro = 1
  else
   let decmp=""
   let doro = 0
   if fname =~ '\.gz$\|\.bz2$\|\.Z$\|\.zip$'
    setlocal bin
   endif
  endif

  if exists("g:tar_secure")
   let tar_secure= " -- "
  else
   let tar_secure= " "
  endif
  if tarfile =~# '\.\(gz\|tgz\)$'
"   call Decho("5: exe silent r! gzip -d -c -- ".s:Escape(tarfile,1)."| ".g:tar_cmd.' -'.g:tar_readoptions.' - '.tar_secure.s:Escape(fname,1))
   exe "silent r! gzip -d -c -- ".s:Escape(tarfile,1)."| ".g:tar_cmd." -".g:tar_readoptions." - ".tar_secure.s:Escape(fname,1).decmp
  elseif tarfile =~# '\.lrp$'
"   call Decho("6: exe silent r! cat ".s:Escape(tarfile,1)." | gzip -d -c - | ".g:tar_cmd." -".g:tar_readoptions." - ".tar_secure.s:Escape(fname,1).decmp)
   exe "silent r! cat -- ".s:Escape(tarfile,1)." | gzip -d -c - | ".g:tar_cmd." -".g:tar_readoptions." - ".tar_secure.s:Escape(fname,1).decmp
  elseif tarfile =~# '\.bz2$'
"   call Decho("7: exe silent r! bzip2 -d -c ".s:Escape(tarfile,1)."| ".g:tar_cmd." -".g:tar_readoptions." - ".tar_secure.s:Escape(fname,1).decmp)
   exe "silent r! bzip2 -d -c -- ".s:Escape(tarfile,1)."| ".g:tar_cmd." -".g:tar_readoptions." - ".tar_secure.s:Escape(fname,1).decmp
  else
   if tarfile =~ '^\s*-'
    " A file name starting with a dash may be taken as an option.  Prepend ./ to avoid that.
    let tarfile = substitute(tarfile, '-', './-', '')
   endif
"   call Decho("8: exe silent r! ".g:tar_cmd." -".g:tar_readoptions." "s:Escape(tarfile,1).tar_secure..s:Escape(fname,1).decmp)
   exe "silent r! ".g:tar_cmd." -".g:tar_readoptions." ".s:Escape(tarfile,1).tar_secure.s:Escape(fname,1).decmp
  endif

  if doro
   " because the reverse process of compressing changed files back into the tarball is not currently supported
   setlocal ro
  endif

  let w:tarfile= a:fname
  exe "file tarfile::".fnameescape(fname)

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

  if !exists("g:tar_secure") && a:fname =~ '^\s*-\|\s\+-'
   redraw!
   echohl WarningMsg | echo '***error*** (tar#Write) rejecting tarfile member<'.a:fname.'> because of embedded "-"; See :help tar-options'
"   call Dret('tar#Write : rejecting tarfile member<'.fname.'> because of embedded "-"')
   return
  endif

  " sanity checks
  if !executable(g:tar_cmd)
   redraw!
   echohl Error | echo '***error*** (tar#Browse) "'.g:tar_cmd.'" not available on your system'
"   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let &report= repkeep
"   call Dret("tar#Write")
   return
  endif
  if !exists("*mkdir")
   redraw!
   echohl Error | echo "***error*** (tar#Write) sorry, mkdir() doesn't work on your system" | echohl None
"   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
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
   exe "cd ".fnameescape(tmpdir)
  catch /^Vim\%((\a\+)\)\=:E344/
   redraw!
   echohl Error | echo "***error*** (tar#Write) cannot cd to temporary directory" | Echohl None
"   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
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

  let tarfile = substitute(w:tarfile,'tarfile:\(.\{-}\)::.*$','\1','')
  let fname   = substitute(w:tarfile,'tarfile:.\{-}::\(.*\)$','\1','')

  " handle compressed archives
  if tarfile =~# '\.gz'
   call system("gzip -d -- ".s:Escape(tarfile,0))
   let tarfile = substitute(tarfile,'\.gz','','e')
   let compress= "gzip -- ".s:Escape(tarfile,0)
"   call Decho("compress<".compress.">")
  elseif tarfile =~# '\.tgz'
   call system("gzip -d -- ".s:Escape(tarfile,0))
   let tarfile = substitute(tarfile,'\.tgz','.tar','e')
   let compress= "gzip -- ".s:Escape(tarfile,0)
   let tgz     = 1
"   call Decho("compress<".compress.">")
  elseif tarfile =~# '\.bz2'
   call system("bzip2 -d -- ".s:Escape(tarfile,0))
   let tarfile = substitute(tarfile,'\.bz2','','e')
   let compress= "bzip2 -- ".s:Escape(tarfile,0)
"   call Decho("compress<".compress.">")
  endif
"  call Decho("tarfile<".tarfile.">")

  if v:shell_error != 0
   redraw!
   echohl Error | echo "***error*** (tar#Write) sorry, unable to update ".tarfile." with ".fname | echohl None
"   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
  else

"   call Decho("tarfile<".tarfile."> fname<".fname.">")
 
   if fname =~ '/'
    let dirpath = substitute(fname,'/[^/]\+$','','e')
    if executable("cygpath")
     let dirpath = substitute(system("cygpath ".s:Escape(dirpath, 0)),'\n','','e')
    endif
    call mkdir(dirpath,"p")
   endif
   if tarfile !~ '/'
    let tarfile= curdir.'/'.tarfile
   endif
   if tarfile =~ '^\s*-'
    " A file name starting with a dash may be taken as an option.  Prepend ./ to avoid that.
    let tarfile = substitute(tarfile, '-', './-', '')
   endif
"   call Decho("tarfile<".tarfile."> fname<".fname.">")
 
   if exists("g:tar_secure")
    let tar_secure= " -- "
   else
    let tar_secure= " "
   endif
   exe "w! ".fnameescape(fname)
   if executable("cygpath")
    let tarfile = substitute(system("cygpath ".s:Escape(tarfile,0)),'\n','','e')
   endif
 
   " delete old file from tarfile
"   call Decho("system(".g:tar_cmd." --delete -f ".s:Escape(tarfile,0)." -- ".s:Escape(fname,0).")")
   call system(g:tar_cmd." --delete -f ".s:Escape(tarfile,0).tar_secure.s:Escape(fname,0))
   if v:shell_error != 0
    redraw!
    echohl Error | echo "***error*** (tar#Write) sorry, unable to update ".fnameescape(tarfile)." with ".fnameescape(fname) | echohl None
"    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   else
 
    " update tarfile with new file 
"    call Decho(g:tar_cmd." -".g:tar_writeoptions." ".s:Escape(tarfile,0).tar_secure.s:Escape(fname,0))
    call system(g:tar_cmd." -".g:tar_writeoptions." ".s:Escape(tarfile,0).tar_secure.s:Escape(fname,0))
    if v:shell_error != 0
     redraw!
     echohl Error | echo "***error*** (tar#Write) sorry, unable to update ".fnameescape(tarfile)." with ".fnameescape(fname) | echohl None
"     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
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
    exe "e! ".fnameescape(tarfile)
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
  exe "cd ".fnameescape(curdir)
  setlocal nomod

  let &report= repkeep
"  call Dret("tar#Write")
endfun

" ---------------------------------------------------------------------
" Rmdir: {{{2
fun! s:Rmdir(fname)
"  call Dfunc("Rmdir(fname<".a:fname.">)")
  if has("unix")
   call system("/bin/rm -rf -- ".s:Escape(a:fname,0))
  elseif has("win32") || has("win95") || has("win64") || has("win16")
   if &shell =~? "sh$"
    call system("/bin/rm -rf -- ".s:Escape(a:fname,0))
   else
    call system("del /S ".s:Escape(a:fname,0))
   endif
  endif
"  call Dret("Rmdir")
endfun

" ---------------------------------------------------------------------
" s:Escape: {{{2
fun s:Escape(name,isfilt)
  " shellescape() was added by patch 7.0.111
  if exists("*shellescape")
   if a:isfilt
    let qnameq= shellescape(a:name,1)
   else
    let qnameq= shellescape(a:name)
   endif
  else
   let qnameq= g:tar_shq . a:name . g:tar_shq
  endif
  return qnameq
endfun

" ---------------------------------------------------------------------
" Modelines And Restoration: {{{1
let &cpo= s:keepcpo
unlet s:keepcpo
" vim:ts=8 fdm=marker
