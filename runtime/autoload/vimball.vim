" vimball.vim : construct a file containing both paths and files
" Maintainer: This runtime file is looking for a new maintainer.
" Original Author:	Charles E. Campbell
" Date:			Apr 11, 2016
" Version:	37 (with modifications from the Vim Project)
" GetLatestVimScripts: 1502 1 :AutoInstall: vimball.vim
"  Last Change:
"   2025 Feb 28 by Vim Project: add support for bzip3 (#16755)
"   2026 Apr 05 by Vim Project: Detect path traversal attacks
"   2026 Apr 09 by Vim Project: Detect more path traversal attacks
" Copyright: (c) 2004-2011 by Charles E. Campbell
"            The VIM LICENSE applies to Vimball.vim, and Vimball.txt
"            (see |copyright|) except use "Vimball" instead of "Vim".
"            No warranty, express or implied.
"  *** ***   Use At-Your-Own-Risk!   *** ***

" ---------------------------------------------------------------------
"  Load Once: {{{1
if &cp || exists("g:loaded_vimball")
 finish
endif
let g:loaded_vimball = "v37"
if v:version < 704
 echohl WarningMsg
 echo "***warning*** this version of vimball needs vim 7.4"
 echohl Normal
 finish
endif
let s:keepcpo= &cpo
set cpo&vim

" =====================================================================
" Constants: {{{1
if !exists("s:USAGE")
 let s:USAGE   = 0
 let s:WARNING = 1
 let s:ERROR   = 2

 " determine if cygwin is in use or not
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
endif

" =====================================================================
"  Functions: {{{1

" ---------------------------------------------------------------------
" vimball#MkVimball: creates a vimball given a list of paths to files {{{2
" Input:
"     line1,line2: a range of lines containing paths to files to be included in the vimball
"     writelevel : if true, force a write to filename.vmb, even if it exists
"                  (usually accomplished with :MkVimball! ...
"     filename   : base name of file to be created (ie. filename.vmb)
" Output: a filename.vmb using vimball format:
"     path
"     filesize
"     [file]
"     path
"     filesize
"     [file]
fun! vimball#MkVimball(line1,line2,writelevel,...) range
  if a:1 =~ '\.vim$' || a:1 =~ '\.txt$'
   let vbname= substitute(a:1,'\.\a\{3}$','.vmb','')
  else
   let vbname= a:1
  endif
  if vbname !~ '\.vmb$'
   let vbname= vbname.'.vmb'
  endif
  if !a:writelevel && a:1 =~ '[\/]'
   call vimball#ShowMesg(s:ERROR,"(MkVimball) vimball name<".a:1."> should not include slashes; use ! to insist")
   return
  endif
  if !a:writelevel && filereadable(vbname)
   call vimball#ShowMesg(s:ERROR,"(MkVimball) file<".vbname."> exists; use ! to insist")
   return
  endif

  " user option bypass
  call vimball#SaveSettings()

  if a:0 >= 2
   " allow user to specify where to get the files
   let home= expand(a:2)
  else
   " use first existing directory from rtp
   let home= vimball#VimballHome()
  endif

  " save current directory
  let curdir = getcwd()
  call s:ChgDir(home)

  " record current tab, initialize while loop index
  let curtabnr = tabpagenr()
  let linenr   = a:line1

  while linenr <= a:line2
   let svfile  = getline(linenr)
 
   if !filereadable(svfile)
    call vimball#ShowMesg(s:ERROR,"unable to read file<".svfile.">")
    call s:ChgDir(curdir)
    call vimball#RestoreSettings()
    return
   endif
 
   " create/switch to mkvimball tab
   if !exists("vbtabnr")
    tabnew
    sil! file Vimball
    let vbtabnr= tabpagenr()
   else
    exe "tabn ".vbtabnr
   endif
 
   let lastline= line("$") + 1
   if lastline == 2 && getline("$") == ""
    call setline(1,'" Vimball Archiver by Charles E. Campbell')
    call setline(2,'UseVimball')
    call setline(3,'finish')
    let lastline= line("$") + 1
   endif
   call setline(lastline  ,substitute(svfile,'$','	[[[1',''))
   call setline(lastline+1,0)

   " write the file from the tab
   exe "$r ".fnameescape(svfile)

   call setline(lastline+1,line("$") - lastline - 1)

  " restore to normal tab
   exe "tabn ".curtabnr
   let linenr= linenr + 1
  endwhile

  " write the vimball
  exe "tabn ".vbtabnr
  call s:ChgDir(curdir)
  setlocal ff=unix
  if a:writelevel
   exe "w! ".fnameescape(vbname)
  else
   exe "w ".fnameescape(vbname)
  endif
  echo "Vimball<".vbname."> created"

  " remove the evidence
  setlocal nomod bh=wipe
  exe "tabn ".curtabnr
  exe "tabc! ".vbtabnr

  " restore options
  call vimball#RestoreSettings()

endfun

" ---------------------------------------------------------------------
" vimball#Vimball: extract and distribute contents from a vimball {{{2
"                  (invoked the the UseVimball command embedded in 
"                  vimballs' prologue)
fun! vimball#Vimball(really,...)

  if getline(1) !~ '^" Vimball Archiver'
   echoerr "(Vimball) The current file does not appear to be a Vimball!"
   return
  endif

  " set up standard settings
  call vimball#SaveSettings()
  let curtabnr    = tabpagenr()
  let vimballfile = expand("%:tr")

  " set up vimball tab
  tabnew
  sil! file Vimball
  let vbtabnr= tabpagenr()
  let didhelp= ""

  " go to vim plugin home
  if a:0 > 0
   " let user specify the directory where the vimball is to be unpacked.
   " If, however, the user did not specify a full path, set the home to be below the current directory
   let home= expand(a:1)
   if has("win32") || has("win95") || has("win64") || has("win16")
    if home !~ '^\a:[/\\]'
      let home= getcwd().'/'.a:1
    endif
   elseif home !~ '^/'
    let home= getcwd().'/'.a:1
   endif
  else
   let home= vimball#VimballHome()
  endif

  " save current directory and remove older same-named vimball, if any
  let curdir = getcwd()

  call s:ChgDir(home)
  let s:ok_unablefind= 1
  call vimball#RmVimball(vimballfile)
  unlet s:ok_unablefind

  let linenr  = 4
  let filecnt = 0

  " give title to listing of (extracted) files from Vimball Archive
  if a:really
   echohl Title     | echomsg "Vimball Archive"         | echohl None
  else             
   echohl Title     | echomsg "Vimball Archive Listing" | echohl None
   echohl Statement | echomsg "files would be placed under: ".home | echohl None
  endif

  " apportion vimball contents to various files
  exe "tabn ".curtabnr
  while 1 < linenr && linenr < line("$")
   let fname   = substitute(getline(linenr),'\t\[\[\[1$','','')
   let fname   = substitute(fname,'\\','/','g')
   let fname   = resolve(simplify(fname))
   let fsize   = substitute(getline(linenr+1),'^\(\d\+\).\{-}$','\1','')+0
   let fenc    = substitute(getline(linenr+1),'^\d\+\s*\(\S\{-}\)$','\1','')
   let filecnt = filecnt + 1
   " Do not allow a leading / or .. anywhere in the file name
   if fname =~ '\.\.' || fname =~ '^/'
     echomsg "(Vimball) Path Traversal Attack detected, aborting..."
     exe "tabn ".curtabnr
     bw! Vimball
     call s:ChgDir(curdir)
     return
   endif

   if a:really
    echomsg "extracted <".fname.">: ".fsize." lines"
   else
    echomsg "would extract <".fname.">: ".fsize." lines"
   endif

   " Allow AsNeeded/ directory to take place of plugin/ directory
   " when AsNeeded/filename is filereadable or was present in VimballRecord
   if fname =~ '\<plugin/'
     let anfname= substitute(fname,'\<plugin/','AsNeeded/','')
     if filereadable(anfname) || (exists("s:VBRstring") && s:VBRstring =~# anfname)
       let fname= anfname
     endif
   endif

   " make directories if they don't exist yet
   if a:really
    let fnamebuf= substitute(fname,'\\','/','g')
    let dirpath = substitute(home,'\\','/','g')
    while fnamebuf =~ '/'
     let dirname  = dirpath."/".substitute(fnamebuf,'/.*$','','')
     let dirpath  = dirname
     let fnamebuf = substitute(fnamebuf,'^.\{-}/\(.*\)$','\1','')
     if !isdirectory(dirname)
      call mkdir(dirname)
      call s:RecordInVar(home,"rmdir('".dirname."')")
     endif
    endwhile
   endif
   call s:ChgDir(home)

   " grab specified qty of lines and place into "a" buffer
   " (skip over path/filename and qty-lines)
   let linenr   = linenr + 2
   let lastline = linenr + fsize - 1
   " no point in handling a zero-length file
   if lastline >= linenr
    exe "silent ".linenr.",".lastline."yank a"

    " copy "a" buffer into tab
    exe "tabn ".vbtabnr
    setlocal ma
    sil! %d
    silent put a
    1
    sil! d

    " write tab to file
    if a:really
     let fnamepath= home."/".fname
    if fenc != ""
      exe "silent w! ++enc=".fnameescape(fenc)." ".fnameescape(fnamepath)
    else
      exe "silent w! ".fnameescape(fnamepath)
    endif
    echo "wrote ".fnameescape(fnamepath)
    call s:RecordInVar(home,"call delete('".fnamepath."')")
    endif

    " return to tab with vimball
    exe "tabn ".curtabnr

    " set up help if it's a doc/*.txt file
    if a:really && didhelp == "" && fname =~ 'doc/[^/]\+\.\(txt\|..x\)$'
      let didhelp= substitute(fname,'^\(.*\<doc\)[/\\][^.]*\.\(txt\|..x\)$','\1','')
    endif
   endif

   " update for next file
   let linenr= linenr + fsize
  endwhile

  " set up help
  if didhelp != ""
   let htpath= home."/".didhelp
   exe "helptags ".fnameescape(htpath)
   echo "did helptags"
  endif

  " make sure a "Press ENTER..." prompt appears to keep the messages showing!
  while filecnt <= &ch
   echomsg " "
   let filecnt= filecnt + 1
  endwhile

  " record actions in <.VimballRecord>
  call s:RecordInFile(home)

  " restore events, delete tab and buffer
  exe "sil! tabn ".vbtabnr
  setlocal nomod bh=wipe
  exe "sil! tabn ".curtabnr
  exe "sil! tabc! ".vbtabnr
  call vimball#RestoreSettings()
  call s:ChgDir(curdir)
endfun

" ---------------------------------------------------------------------
" vimball#RmVimball: remove any files, remove any directories made by any {{{2
"               previous vimball extraction based on a file of the current
"               name.
"  Usage:  RmVimball  (assume current file is a vimball; remove)
"          RmVimball vimballname
fun! vimball#RmVimball(...)
  if exists("g:vimball_norecord")
   return
  endif

  if a:0 == 0
   let curfile= expand("%:tr")
  else
   if a:1 =~ '[\/]'
    call vimball#ShowMesg(s:USAGE,"RmVimball vimballname [path]")
    return
   endif
   let curfile= a:1
  endif
  if curfile =~ '\.vmb$'
   let curfile= substitute(curfile,'\.vmb','','')
  elseif curfile =~ '\.vba$'
   let curfile= substitute(curfile,'\.vba','','')
  endif
  if a:0 >= 2
   let home= expand(a:2)
  else
   let home= vimball#VimballHome()
  endif
  let curdir = getcwd()

  call s:ChgDir(home)
  if filereadable(".VimballRecord")
   keepalt keepjumps 1split 
   sil! keepalt keepjumps e .VimballRecord
   let keepsrch= @/
   if search('^\M'.curfile."\m: ".'cw')
    let foundit= 1
   elseif search('^\M'.curfile.".\mvmb: ",'cw')
    let foundit= 2
   elseif search('^\M'.curfile.'\m[-0-9.]*\.vmb: ','cw')
    let foundit= 2
   elseif search('^\M'.curfile.".\mvba: ",'cw')
    let foundit= 1
   elseif search('^\M'.curfile.'\m[-0-9.]*\.vba: ','cw')
    let foundit= 1
   else
    let foundit = 0
   endif
   if foundit
    if foundit == 1
     let exestring  = substitute(getline("."),'^\M'.curfile.'\m\S\{-}\.vba: ','','')
    else
     let exestring  = substitute(getline("."),'^\M'.curfile.'\m\S\{-}\.vmb: ','','')
    endif
    let s:VBRstring= substitute(exestring,'call delete(','','g')
    let s:VBRstring= substitute(s:VBRstring,"[')]",'','g')
    sil! keepalt keepjumps exe exestring
    sil! keepalt keepjumps d
    let exestring= strlen(substitute(exestring,'call delete(.\{-})|\=',"D","g"))
    echomsg "removed ".exestring." files"
   else
    let s:VBRstring= ''
    let curfile    = substitute(curfile,'\.vmb','','')
    if !exists("s:ok_unablefind")
     call vimball#ShowMesg(s:WARNING,"(RmVimball) unable to find <".curfile."> in .VimballRecord")
    endif
   endif
   sil! keepalt keepjumps g/^\s*$/d
   sil! keepalt keepjumps wq!
   let @/= keepsrch
  endif
  call s:ChgDir(curdir)
endfun

" ---------------------------------------------------------------------
" vimball#Decompress: attempts to automatically decompress vimballs {{{2
fun! vimball#Decompress(fname,...)
  " decompression:
  if     expand("%") =~ '.*\.gz'  && executable("gunzip")
   " handle *.gz with gunzip
   silent exe "!gunzip ".shellescape(a:fname)
   if v:shell_error != 0
    call vimball#ShowMesg(s:WARNING,"(vimball#Decompress) gunzip may have failed with <".a:fname.">")
   endif
   let fname= substitute(a:fname,'\.gz$','','')
   exe "e ".escape(fname,' \')
   if a:0 == 0| call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)") | endif

  elseif expand("%") =~ '.*\.gz' && executable("gzip")
   " handle *.gz with gzip -d
   silent exe "!gzip -d ".shellescape(a:fname)
   if v:shell_error != 0
    call vimball#ShowMesg(s:WARNING,'(vimball#Decompress) "gzip -d" may have failed with <'.a:fname.">")
   endif
   let fname= substitute(a:fname,'\.gz$','','')
   exe "e ".escape(fname,' \')
   if a:0 == 0| call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)") | endif

  elseif expand("%") =~ '.*\.bz2' && executable("bunzip2")
   " handle *.bz2 with bunzip2
   silent exe "!bunzip2 ".shellescape(a:fname)
   if v:shell_error != 0
    call vimball#ShowMesg(s:WARNING,"(vimball#Decompress) bunzip2 may have failed with <".a:fname.">")
   endif
   let fname= substitute(a:fname,'\.bz2$','','')
   exe "e ".escape(fname,' \')
   if a:0 == 0| call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)") | endif

  elseif expand("%") =~ '.*\.bz2' && executable("bzip2")
   " handle *.bz2 with bzip2 -d
   silent exe "!bzip2 -d ".shellescape(a:fname)
   if v:shell_error != 0
    call vimball#ShowMesg(s:WARNING,'(vimball#Decompress) "bzip2 -d" may have failed with <'.a:fname.">")
   endif
   let fname= substitute(a:fname,'\.bz2$','','')
   exe "e ".escape(fname,' \')
   if a:0 == 0| call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)") | endif

  elseif expand("%") =~ '.*\.bz3' && executable("bunzip3")
   " handle *.bz3 with bunzip3
   silent exe "!bunzip3 ".shellescape(a:fname)
   if v:shell_error != 0
    call vimball#ShowMesg(s:WARNING,"(vimball#Decompress) bunzip3 may have failed with <".a:fname.">")
   endif
   let fname= substitute(a:fname,'\.bz3$','','')
   exe "e ".escape(fname,' \')
   if a:0 == 0| call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)") | endif

  elseif expand("%") =~ '.*\.bz3' && executable("bzip3")
   " handle *.bz3 with bzip3 -d
   silent exe "!bzip3 -d ".shellescape(a:fname)
   if v:shell_error != 0
    call vimball#ShowMesg(s:WARNING,'(vimball#Decompress) "bzip3 -d" may have failed with <'.a:fname.">")
   endif
   let fname= substitute(a:fname,'\.bz3$','','')
   exe "e ".escape(fname,' \')
   if a:0 == 0| call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)") | endif

  elseif expand("%") =~ '.*\.zip' && executable("unzip")
   " handle *.zip with unzip
   silent exe "!unzip ".shellescape(a:fname)
   if v:shell_error != 0
    call vimball#ShowMesg(s:WARNING,"(vimball#Decompress) unzip may have failed with <".a:fname.">")
   endif
   let fname= substitute(a:fname,'\.zip$','','')
   exe "e ".escape(fname,' \')
   if a:0 == 0| call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)") | endif
  endif

  if a:0 == 0| setlocal noma bt=nofile fmr=[[[,]]] fdm=marker | endif
endfun

" ---------------------------------------------------------------------
" vimball#ShowMesg: {{{2
fun! vimball#ShowMesg(level,msg)

  let rulerkeep   = &ruler
  let showcmdkeep = &showcmd
  set noruler noshowcmd
  redraw!

  if &fo =~# '[ta]'
   echomsg "***vimball*** ".a:msg
  else
   if a:level == s:WARNING || a:level == s:USAGE
    echohl WarningMsg
   elseif a:level == s:ERROR
    echohl Error
   endif
   echomsg "***vimball*** ".a:msg
   echohl None
  endif

  if a:level != s:USAGE
   call inputsave()|let ok= input("Press <cr> to continue")|call inputrestore()
  endif

  let &ruler   = rulerkeep
  let &showcmd = showcmdkeep
endfun
" =====================================================================
" s:ChgDir: change directory (in spite of Windoze) {{{2
fun! s:ChgDir(newdir)
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   try
    exe 'silent cd '.fnameescape(substitute(a:newdir,'/','\\','g'))
   catch  /^Vim\%((\a\+)\)\=:E/
    call mkdir(fnameescape(substitute(a:newdir,'/','\\','g')))
    exe 'silent cd '.fnameescape(substitute(a:newdir,'/','\\','g'))
   endtry
  else
   try
    exe 'silent cd '.fnameescape(a:newdir)
   catch  /^Vim\%((\a\+)\)\=:E/
    call mkdir(fnameescape(a:newdir))
    exe 'silent cd '.fnameescape(a:newdir)
   endtry
  endif
endfun

" ---------------------------------------------------------------------
" s:RecordInVar: record a un-vimball command in the .VimballRecord file {{{2
fun! s:RecordInVar(home,cmd)
  if !exists("s:recordfile")
   let s:recordfile= a:cmd
  else
   let s:recordfile= s:recordfile."|".a:cmd
  endif
endfun

" ---------------------------------------------------------------------
" s:RecordInFile: {{{2
fun! s:RecordInFile(home)
  if exists("g:vimball_norecord")
   return
  endif

  if exists("s:recordfile") || exists("s:recorddir")
   let curdir= getcwd()
   call s:ChgDir(a:home)
   keepalt keepjumps 1split 

   let cmd= expand("%:tr").": "

   sil! keepalt keepjumps e .VimballRecord
   setlocal ma
   $
   if exists("s:recordfile") && exists("s:recorddir")
    let cmd= cmd.s:recordfile."|".s:recorddir
   elseif exists("s:recorddir")
    let cmd= cmd.s:recorddir
   elseif exists("s:recordfile")
    let cmd= cmd.s:recordfile
   else
    return
   endif

   " put command into buffer, write .VimballRecord `file
   keepalt keepjumps put=cmd
   sil! keepalt keepjumps g/^\s*$/d
   sil! keepalt keepjumps wq!
   call s:ChgDir(curdir)

   if exists("s:recorddir")
    unlet s:recorddir
   endif
   if exists("s:recordfile")
    unlet s:recordfile
   endif
  endif
endfun

" ---------------------------------------------------------------------
" vimball#VimballHome: determine/get home directory path (usually from rtp) {{{2
fun! vimball#VimballHome()
  if exists("g:vimball_home")
   let home= g:vimball_home
  else
   " go to vim plugin home
   for home in split(&rtp,',') + ['']
    if isdirectory(home) && filewritable(home) | break | endif
    let basehome= substitute(home,'[/\\]\.vim$','','')
    if isdirectory(basehome) && filewritable(basehome)
     let home= basehome."/.vim"
     break
    endif
   endfor
   if home == ""
    " just pick the first directory
    let home= substitute(&rtp,',.*$','','')
   endif
   if (has("win32") || has("win95") || has("win64") || has("win16"))
    let home= substitute(home,'/','\\','g')
   endif
  endif
  " insure that the home directory exists
  if !isdirectory(home)
   call mkdir(home)
  endif
  return home
endfun

" ---------------------------------------------------------------------
" vimball#SaveSettings: {{{2
fun! vimball#SaveSettings()
"  call Dfunc("SaveSettings()")
  let s:makeep  = getpos("'a")
  let s:regakeep= @a
  if exists("+acd")
   let s:acdkeep = &acd
  endif
  let s:eikeep  = &ei
  let s:fenkeep = &l:fen
  let s:hidkeep = &hidden
  let s:ickeep  = &ic
  let s:lzkeep  = &lz
  let s:pmkeep  = &pm
  let s:repkeep = &report
  let s:vekeep  = &ve
  let s:ffkeep  = &l:ff
  let s:swfkeep = &l:swf
  if exists("+acd")
   setlocal ei=all ve=all noacd nofen noic report=999 nohid bt= ma lz pm= ff=unix noswf
  else
   setlocal ei=all ve=all       nofen noic report=999 nohid bt= ma lz pm= ff=unix noswf
  endif
  " vimballs should be in unix format
  setlocal ff=unix
endfun

" ---------------------------------------------------------------------
" vimball#RestoreSettings: {{{2
fun! vimball#RestoreSettings()
  let @a      = s:regakeep
  if exists("+acd")
   let &acd   = s:acdkeep
  endif
  let &l:fen  = s:fenkeep
  let &hidden = s:hidkeep
  let &ic     = s:ickeep
  let &lz     = s:lzkeep
  let &pm     = s:pmkeep
  let &report = s:repkeep
  let &ve     = s:vekeep
  let &ei     = s:eikeep
  let &l:ff   = s:ffkeep
  if s:makeep[0] != 0
   " restore mark a
   call setpos("'a",s:makeep)
  endif
  if exists("+acd")
   unlet s:acdkeep
  endif
  unlet s:regakeep s:eikeep s:fenkeep s:hidkeep s:ickeep s:repkeep s:vekeep s:makeep s:lzkeep s:pmkeep s:ffkeep
endfun

let &cpo = s:keepcpo
unlet s:keepcpo

" ---------------------------------------------------------------------
" Modelines: {{{1
" vim: fdm=marker et
