" vimball : construct a file containing both paths and files
" Author: Charles E. Campbell, Jr.
" Date:   May 01, 2006
" Version: 13
" GetLatestVimScripts: 1502 1 :AutoInstall: vimball.vim
" Copyright: (c) 2004-2006 by Charles E. Campbell, Jr.
"            The VIM LICENSE applies to Vimball.vim, and Vimball.txt
"            (see |copyright|) except use "Vimball" instead of "Vim".
"            No warranty, express or implied.
"  *** ***   Use At-Your-Own-Risk!   *** ***

" ---------------------------------------------------------------------
"  Load Once: {{{1
if &cp || exists("g:loaded_vimball")
 finish
endif
let s:keepcpo        = &cpo
let g:loaded_vimball = "v13"
set cpo&vim

" =====================================================================
"  Functions: {{{1

" ---------------------------------------------------------------------
" MkVimball: creates a vimball given a list of paths to files {{{2
" Vimball Format:
"     path
"     filesize
"     [file]
"     path
"     filesize
"     [file]
fun! vimball#MkVimball(line1,line2,writelevel,vimballname) range
"  call Dfunc("MkVimball(line1=".a:line1." line2=".a:line2." writelevel=".a:writelevel." vimballname<".a:vimballname.">")
  let vbname= substitute(a:vimballname,'\.[^.]*$','','e').'.vba'
  if !a:writelevel && filereadable(vbname)
   echohl Error | echoerr "(MkVimball) file<".vbname."> exists; use ! to insist" | echohl None
"   call Dret("MkVimball : file<".vbname."> already exists; use ! to insist")
   return
  endif

  " user option bypass
  call s:SaveSettings()

  " go to vim plugin home
  for home in split(&rtp,',') + ['']
   if isdirectory(home) | break | endif
  endfor
  if home == ""
   let home= substitute(&rtp,',.*$','','')
  endif
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   let home= substitute(home,'/','\\','ge')
  endif
"  call Decho("home<".home.">")

  " save current directory
  let curdir = getcwd()
  call s:ChgDir(home)

  " record current tab, initialize while loop index
  let curtabnr = tabpagenr()
  let linenr   = a:line1
"  call Decho("curtabnr=".curtabnr)

  while linenr <= a:line2
   let svfile  = getline(linenr)
"   call Decho("svfile<".svfile.">")
 
   if !filereadable(svfile)
    echohl Error | echo "unable to read file<".svfile.">" | echohl None
	call s:ChgDir(curdir)
	call s:RestoreSettings()
"    call Dret("MkVimball")
    return
   endif
 
   " create/switch to mkvimball tab
   if !exists("vbtabnr")
    tabnew
    silent! file Vimball
    let vbtabnr= tabpagenr()
   else
    exe "tabn ".vbtabnr
   endif
 
   let lastline= line("$") + 1
   if lastline == 2 && getline("$") == ""
	call setline(1,'" Vimball Archiver by Charles E. Campbell, Jr., Ph.D.')
	call setline(2,'UseVimball')
	call setline(3,'finish')
	let lastline= 4
   endif
   call setline(lastline  ,svfile)
   call setline(lastline+1,0)

   " write the file from the tab
   let svfilepath= s:Path(svfile,'')
"   call Decho("exe $r ".svfilepath)
   exe "$r ".svfilepath

   call setline(lastline+1,line("$") - lastline - 1)
"   call Decho("lastline=".lastline." line$=".line("$"))

  " restore to normal tab
   exe "tabn ".curtabnr
   let linenr= linenr + 1
  endwhile

  " write the vimball
  exe "tabn ".vbtabnr
  call s:ChgDir(curdir)
  if a:writelevel
   let vbnamepath= s:Path(vbname,'')
"   call Decho("exe w! ".vbnamepath)
   exe "w! ".vbnamepath
  else
   let vbnamepath= s:Path(vbname,'')
"   call Decho("exe w ".vbnamepath)
   exe "w ".vbnamepath
  endif
"  call Decho("Vimball<".vbname."> created")
  echo "Vimball<".vbname."> created"

  " remove the evidence
  setlocal nomod bh=wipe
  exe "tabn ".curtabnr
  exe "tabc ".vbtabnr

  " restore options
  call s:RestoreSettings()

"  call Dret("MkVimball")
endfun

" ---------------------------------------------------------------------
" Vimball: {{{2
fun! vimball#Vimball(really)
"  call Dfunc("Vimball(really=".a:really.")")

  if getline(1) !~ '^" Vimball Archiver by Charles E. Campbell, Jr., Ph.D.$'
   echoerr "(Vimball) The current file does not appear to be a Vimball!"
"   call Dret("Vimball")
   return
  endif

  " set up standard settings
  call s:SaveSettings()
  let curtabnr = tabpagenr()

  " set up vimball tab
  tabnew
  silent! file Vimball
  let vbtabnr= tabpagenr()
  let didhelp= ""

  " go to vim plugin home
  for home in split(&rtp,',') + ['']
   if isdirectory(home) | break | endif
  endfor
  if home == ""
   let home= substitute(&rtp,',.*$','','')
  endif
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   let home= substitute(home,'/','\\','ge')
  endif
"  call Decho("home<".home.">")

  " save current directory
  let curdir = getcwd()
  call s:ChgDir(home)

  let linenr  = 4
  let filecnt = 0

  " give title to listing of (extracted) files from Vimball Archive
  if a:really
   echohl Title | echomsg "Vimball Archive" | echohl None
  else
   echohl Title | echomsg "Vimball Archive Listing" | echohl None
   echohl Statement | echomsg "files would be placed under: ".home | echohl None
  endif

  " apportion vimball contents to various files
"  call Decho("exe tabn ".curtabnr)
  exe "tabn ".curtabnr
"  call Decho("linenr=".linenr." line$=".line("$"))
  while 1 < linenr && linenr < line("$")
   let fname   = getline(linenr)
   let fsize   = getline(linenr+1)
   let filecnt = filecnt + 1
   if a:really
    echomsg "extracted <".fname.">: ".fsize." lines"
   else
    echomsg "would extract <".fname.">: ".fsize." lines"
   endif
"   call Decho("using L#".linenr.": will extract file<".fname.">")
"   call Decho("using L#".(linenr+1).": fsize=".fsize)

   " make directories if they don't exist yet
"   call Decho("making directories if they don't exist yet")
   if a:really
    let fnamebuf= fname
    while fnamebuf =~ '/'
    	let dirname  = home."/".substitute(fnamebuf,'/.*$','','e')
    	let fnamebuf = substitute(fnamebuf,'^.\{-}/\(.*\)$','\1','e')
     if !isdirectory(dirname)
"     call Decho("making <".dirname.">")
      call mkdir(dirname)
     endif
    endwhile
   endif
   call s:ChgDir(home)

   " grab specified qty of lines and place into "a" buffer
   " (skip over path/filename and qty-lines)
   let linenr   = linenr + 2
   let lastline = linenr + fsize - 1
"   call Decho("exe ".linenr.",".lastline."yank a")
   exe "silent ".linenr.",".lastline."yank a"

   " copy "a" buffer into tab
"   call Decho('copy "a buffer into tab#'.vbtabnr)
   exe "tabn ".vbtabnr
   silent! %d
   silent put a
   1
   silent d

   " write tab to file
   if a:really
    let fnamepath= s:Path(home."/".fname,'')
"    call Decho("exe w! ".fnamepath)
    exe "silent w! ".fnamepath
    echo "wrote ".fnamepath
   endif

   " return to tab with vimball
"   call Decho("exe tabn ".curtabnr)
   exe "tabn ".curtabnr

   " set up help if its a doc/*.txt file
"   call Decho("didhelp<".didhelp."> fname<".fname.">")
   if a:really && didhelp == "" && fname =~ 'doc/[^/]\+\.txt$'
   	let didhelp= substitute(fname,'^\(.*\<doc\)[/\\][^.]*\.txt$','\1','e')
"	call Decho("didhelp<".didhelp.">")
   endif

   " update for next file
"   let oldlinenr = linenr " Decho
   let linenr    = linenr + fsize
"   call Decho("update linenr= [linenr=".oldlinenr."] + [fsize=".fsize."] = ".linenr)
  endwhile

  " set up help
"  call Decho("about to set up help: didhelp<".didhelp.">")
  if didhelp != ""
   let htpath= escape(substitute(s:Path(home."/".didhelp,'"'),'"','','ge'),' ')
"   call Decho("exe helptags ".htpath)
   exe "helptags ".htpath
   echo "did helptags"
  endif

  " make sure a "Press ENTER..." prompt appears to keep the messages showing!
  while filecnt <= &ch
   echomsg " "
   let filecnt= filecnt + 1
  endwhile

  " restore events, delete tab and buffer
  exe "tabn ".vbtabnr
  setlocal nomod bh=wipe
  exe "tabn ".curtabnr
  exe "tabc ".vbtabnr
  call s:RestoreSettings()
  call s:ChgDir(curdir)

"  call Dret("Vimball")
endfun

" ---------------------------------------------------------------------
" vimball#Decompress: attempts to automatically decompress vimballs {{{2
fun! vimball#Decompress(fname)
"  call Dfunc("Decompress(fname<".a:fname.">)")

  " decompression:
  if     expand("%") =~ '.*\.gz'  && executable("gunzip")
   exe "!gunzip ".a:fname
   let fname= substitute(a:fname,'\.gz$','','')
   exe "e ".escape(fname,' \')
   call vimball#ShowMesg("Source this file to extract it! (:so %)")
  elseif expand("%") =~ '.*\.bz2' && executable("bunzip2")
   exe "!bunzip2 ".a:fname
   let fname= substitute(a:fname,'\.bz2$','','')
   exe "e ".escape(fname,' \')
   call vimball#ShowMesg("Source this file to extract it! (:so %)")
  elseif expand("%") =~ '.*\.zip' && executable("unzip")
   exe "!unzip ".a:fname
   let fname= substitute(a:fname,'\.zip$','','')
   exe "e ".escape(fname,' \')
   call vimball#ShowMesg("Source this file to extract it! (:so %)")
  endif

"  call Dret("Decompress")
endfun

" ---------------------------------------------------------------------
" ChgDir: change directory (in spite of Windoze) {{{2
fun! s:ChgDir(newdir)
"  call Dfunc("ChgDir(newdir<".a:newdir.">)")
  if (has("win32") || has("win95") || has("win64") || has("win16"))
    exe 'silent cd '.escape(substitute(a:newdir,'/','\\','g'),' ')
  else
   exe 'silent cd '.escape(a:newdir,' ')
  endif
"  call Dret("ChgDir")
endfun

" ---------------------------------------------------------------------
" Path: {{{2
fun! s:Path(cmd,quote)
"  call Dfunc("Path(cmd<".a:cmd."> quote<".a:quote.">)")
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   let cmdpath= a:quote.substitute(a:cmd,'/','\\','ge').a:quote
  else
   let cmdpath= a:quote.a:cmd.a:quote
  endif
  if a:quote == ""
   let cmdpath= escape(cmdpath,' ')
  endif
"  call Dret("Path <".cmdpath.">")
  return cmdpath
endfun

" ---------------------------------------------------------------------
" vimball#ShowMesg: {{{2
fun! vimball#ShowMesg(msg)
"  call Dfunc("vimball#ShowMesg(msg<".a:msg.">)")
  let ich= 1
  echohl WarningMsg | echo a:msg | echohl None
  while ich < &ch
   echo " "
   let ich= ich + 1
  endwhile
"  call Dret("vimball#ShowMesg")
endfun

" ---------------------------------------------------------------------
" s:SaveSettings: {{{2
fun! s:SaveSettings()
"  call Dfunc("SaveSettings()")
  let s:makeep  = getpos("'a")
  let s:regakeep= @a
  if exists("&acd")
   let s:acdkeep = &acd
  endif
  let s:eikeep  = &ei
  let s:fenkeep = &fen
  let s:hidkeep = &hidden
  let s:ickeep  = &ic
  let s:repkeep = &report
  let s:vekeep  = &ve
  if exists("&acd")
   set ei=all ve=all noacd nofen noic report=999 nohid
  else
   set ei=all ve=all nofen noic report=999 nohid
  endif
"  call Dret("SaveSettings")
endfun

" ---------------------------------------------------------------------
" s:RestoreSettings: {{{2
fun! s:RestoreSettings()
"  call Dfunc("RestoreSettings()")
  let @a      = s:regakeep
  if exists("&acd")
   let &acd   = s:acdkeep
  endif
  let &ei     = s:eikeep
  let &fen    = s:fenkeep
  let &hidden = s:hidkeep
  let &ic     = s:ickeep
  let &report = s:repkeep
  let &ve     = s:vekeep
  if s:makeep[0] != 0
   " restore mark a
"   call Decho("restore mark-a: makeep=".string(makeep))
   call setpos("'a",s:makeep)
  endif
  if exists("&acd")
   unlet s:regakeep s:acdkeep s:eikeep s:fenkeep s:hidkeep s:ickeep s:repkeep s:vekeep s:makeep
  else
   unlet s:regakeep s:eikeep s:fenkeep s:hidkeep s:ickeep s:repkeep s:vekeep s:makeep
  endif
"  call Dret("RestoreSettings")
endfun

" ---------------------------------------------------------------------
let &cpo= s:keepcpo
unlet s:keepcpo
" =====================================================================
" Modelines: {{{1
" vim: fdm=marker
