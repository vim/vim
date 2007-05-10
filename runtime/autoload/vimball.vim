" vimball.vim : construct a file containing both paths and files
" Author:	Charles E. Campbell, Jr.
" Date:		May 07, 2007
" Version:	22
" GetLatestVimScripts: 1502 1 :AutoInstall: vimball.vim
" Copyright: (c) 2004-2006 by Charles E. Campbell, Jr.
"            The VIM LICENSE applies to Vimball.vim, and Vimball.txt
"            (see |copyright|) except use "Vimball" instead of "Vim".
"            No warranty, express or implied.
"  *** ***   Use At-Your-Own-Risk!   *** ***

" ---------------------------------------------------------------------
"  Load Once: {{{1
if &cp || exists("g:loaded_vimball") || v:version < 700
 finish
endif
let s:keepcpo        = &cpo
let g:loaded_vimball = "v22"
set cpo&vim

" =====================================================================
" Constants: {{{1
if !exists("s:USAGE")
 let s:USAGE   = 0
 let s:WARNING = 1
 let s:ERROR   = 2
endif

" =====================================================================
"  Functions: {{{1

" ---------------------------------------------------------------------
" vimball#MkVimball: creates a vimball given a list of paths to files {{{2
" Vimball Format:
"     path
"     filesize
"     [file]
"     path
"     filesize
"     [file]
fun! vimball#MkVimball(line1,line2,writelevel,...) range
"  call Dfunc("MkVimball(line1=".a:line1." line2=".a:line2." writelevel=".a:writelevel." vimballname<".a:1.">) a:0=".a:0)
  if a:1 =~ '.vim' || a:1 =~ '.txt'
   let vbname= substitute(a:1,'\.\a\{3}$','.vba','')
  else
   let vbname= a:1
  endif
  if vbname !~ '\.vba$'
   let vbname= vbname.'.vba'
  endif
"  call Decho("vbname<".vbname.">")
  if a:1 =~ '[\/]'
   call vimball#ShowMesg(s:ERROR,"(MkVimball) vimball name<".a:1."> should not include slashes")
"   call Dret("MkVimball : vimball name<".a:1."> should not include slashes")
   return
  endif
  if !a:writelevel && filereadable(vbname)
   call vimball#ShowMesg(s:ERROR,"(MkVimball) file<".vbname."> exists; use ! to insist")
"   call Dret("MkVimball : file<".vbname."> already exists; use ! to insist")
   return
  endif

  " user option bypass
  call s:SaveSettings()

  if a:0 >= 2
   " allow user to specify where to get the files
   let home= expand(a:2)
  else
   " use first existing directory from rtp
   let home= s:VimballHome()
  endif

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
    call vimball#ShowMesg(s:ERROR,"unable to read file<".svfile.">")
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
	let lastline= line("$") + 1
   endif
   call setline(lastline  ,substitute(svfile,'$','	[[[1',''))
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
" vimball#Vimball: extract and distribute contents from a vimball {{{2
fun! vimball#Vimball(really,...)
"  call Dfunc("vimball#Vimball(really=".a:really.") a:0=".a:0)

  if getline(1) !~ '^" Vimball Archiver by Charles E. Campbell, Jr., Ph.D.$'
   echoerr "(Vimball) The current file does not appear to be a Vimball!"
"   call Dret("vimball#Vimball")
   return
  endif

  " set up standard settings
  call s:SaveSettings()
  let curtabnr = tabpagenr()

  " set up vimball tab
"  call Decho("setting up vimball tab")
  tabnew
  silent! file Vimball
  let vbtabnr= tabpagenr()
  let didhelp= ""

  " go to vim plugin home
  if a:0 > 0
   let home= expand(a:1)
  else
   let home= s:VimballHome()
  endif
"  call Decho("home<".home.">")

  " save current directory and remove older same-named vimball, if any
  let curdir = getcwd()
"  call Decho("home<".home.">")
"  call Decho("curdir<".curdir.">")

  call s:ChgDir(home)
  call vimball#RmVimball()

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
   let fname   = substitute(getline(linenr),'\t\[\[\[1$','','')
   let fname   = substitute(fname,'\\','/','g')
   let fsize   = getline(linenr+1)
   let filecnt = filecnt + 1
"   call Decho("fname<".fname."> fsize=".fsize." filecnt=".filecnt)

   if a:really
    echomsg "extracted <".fname.">: ".fsize." lines"
   else
    echomsg "would extract <".fname.">: ".fsize." lines"
   endif
"   call Decho("using L#".linenr.": will extract file<".fname.">")
"   call Decho("using L#".(linenr+1).": fsize=".fsize)

   " Allow AsNeeded/ directory to take place of plugin/ directory
   " when AsNeeded/filename is filereadable
   if fname =~ '\<plugin/'
   	let anfname= substitute(fname,'\<plugin/','AsNeeded/','')
	if filereadable(anfname)
"	 call Decho("using anfname<".anfname."> instead of <".fname.">")
	 let fname= anfname
	endif
   endif

   " make directories if they don't exist yet
   if a:really
"    call Decho("making directories if they don't exist yet (fname<".fname.">)")
    let fnamebuf= substitute(fname,'\\','/','g')
	let dirpath = substitute(home,'\\','/','g')
    while fnamebuf =~ '/'
     let dirname  = dirpath."/".substitute(fnamebuf,'/.*$','','')
	 let dirpath  = dirname
     let fnamebuf = substitute(fnamebuf,'^.\{-}/\(.*\)$','\1','')
"	 call Decho("dirname<".dirname.">")
     if !isdirectory(dirname)
"      call Decho("making <".dirname.">")
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
	call s:RecordInVar(home,"call delete('".fnamepath."')")
   endif

   " return to tab with vimball
"   call Decho("exe tabn ".curtabnr)
   exe "tabn ".curtabnr

   " set up help if its a doc/*.txt file
"   call Decho("didhelp<".didhelp."> fname<".fname.">")
   if a:really && didhelp == "" && fname =~ 'doc/[^/]\+\.txt$'
   	let didhelp= substitute(fname,'^\(.*\<doc\)[/\\][^.]*\.txt$','\1','')
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
   let htpath= escape(substitute(s:Path(home."/".didhelp,'"'),'"','','g'),' ')
"   call Decho("exe helptags ".htpath)
   exe "helptags ".htpath
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
  exe "tabn ".vbtabnr
  setlocal nomod bh=wipe
  exe "tabn ".curtabnr
  exe "tabc ".vbtabnr
  call s:RestoreSettings()
  call s:ChgDir(curdir)

"  call Dret("vimball#Vimball")
endfun

" ---------------------------------------------------------------------
" vimball#RmVimball: remove any files, remove any directories made by any {{{2
"               previous vimball extraction based on a file of the current
"               name.
"  Usage:  RmVimball  (assume current file is a vimball; remove)
"          RmVimball vimballname
fun! vimball#RmVimball(...)
"  call Dfunc("vimball#RmVimball() a:0=".a:0)
  if exists("g:vimball_norecord")
"   call Dret("vimball#RmVimball : (g:vimball_norecord)")
   return
  endif
  let eikeep= &ei
  set ei=all
"  call Decho("turned off all events")

  if a:0 == 0
   let curfile= '^'.expand("%:tr")
  else
   if a:1 =~ '[\/]'
    call vimball#ShowMesg(s:USAGE,"RmVimball vimballname [path]")
"    call Dret("vimball#RmVimball : suspect a:1<".a:1.">")
    return
   endif
   let curfile= a:1
  endif
  if curfile !~ '.vba$'
   let curfile= curfile.".vba: "
  else
   let curfile= curfile.": "
  endif
  if a:0 >= 2
   let home= expand(a:2)
  else
   let home= s:VimballHome()
  endif
  let curdir = getcwd()
"  call Decho("home   <".home.">")
"  call Decho("curfile<".curfile.">")
"  call Decho("curdir <".curdir.">")

  call s:ChgDir(home)
  if filereadable(".VimballRecord")
"   call Decho(".VimballRecord is readable")
"   call Decho("curfile<".curfile.">")
   keepalt keepjumps 1split 
   silent! keepalt keepjumps e .VimballRecord
   let keepsrch= @/
   if search(curfile,'cw')
   	let exestring= substitute(getline("."),curfile,'','')
"	call Decho("exe ".exestring)
	silent! keepalt keepjumps exe exestring
	silent! keepalt keepjumps d
   else
"   	call Decho("unable to find <".curfile."> in .VimballRecord")
   endif
   silent! keepalt keepjumps g/^\s*$/d
   silent! keepalt keepjumps wq!
   let @/= keepsrch
  endif
  call s:ChgDir(curdir)

  " restoring events
"  call Decho("restoring events")
  let &ei= eikeep

"  call Dret("vimball#RmVimball")
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
   call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)")
  elseif expand("%") =~ '.*\.bz2' && executable("bunzip2")
   exe "!bunzip2 ".a:fname
   let fname= substitute(a:fname,'\.bz2$','','')
   exe "e ".escape(fname,' \')
   call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)")
  elseif expand("%") =~ '.*\.zip' && executable("unzip")
   exe "!unzip ".a:fname
   let fname= substitute(a:fname,'\.zip$','','')
   exe "e ".escape(fname,' \')
   call vimball#ShowMesg(s:USAGE,"Source this file to extract it! (:so %)")
  endif
  set noma bt=nofile fmr=[[[,]]] fdm=marker

"  call Dret("Decompress")
endfun

" ---------------------------------------------------------------------
" vimball#ShowMesg: {{{2
fun! vimball#ShowMesg(level,msg)
"  call Dfunc("vimball#ShowMesg(level=".a:level." msg<".a:msg.">)")
  let rulerkeep   = &ruler
  let showcmdkeep = &showcmd
  set noruler noshowcmd
  redraw!

  if &fo =~ '[ta]'
   echomsg "***vimball*** " a:msg
  else
   if a:level == s:WARNING || a:level == s:USAGE
    echohl WarningMsg
   elseif a:level == s:ERROR
    echohl Error
   endif
   echomsg "***vimball*** " a:msg
   echohl None
  endif

  if a:level != s:USAGE
   call inputsave()|let ok= input("Press <cr> to continue")|call inputrestore()
  endif

  let &ruler   = rulerkeep
  let &showcmd = showcmdkeep

"  call Dret("vimball#ShowMesg")
endfun

" ---------------------------------------------------------------------
let &cpo= s:keepcpo
unlet s:keepcpo
" =====================================================================
" s:ChgDir: change directory (in spite of Windoze) {{{2
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
" s:Path: prepend and append quotes, do escaping, as necessary {{{2
fun! s:Path(cmd,quote)
"  call Dfunc("Path(cmd<".a:cmd."> quote<".a:quote.">)")
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   let cmdpath= a:quote.substitute(a:cmd,'/','\\','g').a:quote
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
" s:RecordInVar: record a un-vimball command in the .VimballRecord file {{{2
fun! s:RecordInVar(home,cmd)
"  call Dfunc("RecordInVar(home<".a:home."> cmd<".a:cmd.">)")
  if a:cmd =~ '^rmdir'
"   if !exists("s:recorddir")
"    let s:recorddir= substitute(a:cmd,'^rmdir',"call s:Rmdir",'')
"   else
"    let s:recorddir= s:recorddir."|".substitute(a:cmd,'^rmdir',"call s:Rmdir",'')
"   endif
"   call Decho("recorddir=".s:recorddir)
  elseif !exists("s:recordfile")
   let s:recordfile= a:cmd
"   call Decho("recordfile=".s:recordfile)
  else
   let s:recordfile= s:recordfile."|".a:cmd
"   call Decho("recordfile=".s:recordfile)
  endif
"  call Dret("RecordInVar")
endfun

" ---------------------------------------------------------------------
" s:RecordInFile: {{{2
fun! s:RecordInFile(home)
"  call Dfunc("RecordInFile()")
  if exists("g:vimball_norecord")
"   call Dret("RecordInFile : (g:vimball_norecord)")
   return
  endif

  if exists("s:recordfile") || exists("s:recorddir")
   let curdir= getcwd()
   call s:ChgDir(a:home)
   keepalt keepjumps 1split 
   let cmd= expand("%:tr").": "
   silent! keepalt keepjumps e .VimballRecord
   $
   if exists("s:recordfile") && exists("s:recorddir")
   	let cmd= cmd.s:recordfile."|".s:recorddir
   elseif exists("s:recorddir")
   	let cmd= cmd.s:recorddir
   elseif exists("s:recordfile")
   	let cmd= cmd.s:recordfile
   else
"    call Dret("RecordInFile")
	return
   endif
   keepalt keepjumps put=cmd
   silent! keepalt keepjumps g/^\s*$/d
   silent! keepalt keepjumps wq!
   call s:ChgDir(curdir)
   if exists("s:recorddir") |unlet s:recorddir |endif
   if exists("s:recordfile")|unlet s:recordfile|endif
  else
"   call Decho("s:record[file|dir] doesn't exist")
  endif

"  call Dret("RecordInFile")
endfun

" ---------------------------------------------------------------------
" s:Rmdir: {{{2
"fun! s:Rmdir(dirname)
""  call Dfunc("s:Rmdir(dirname<".a:dirname.">)")
"  if (has("win32") || has("win95") || has("win64") || has("win16")) && &shell !~? 'sh$'
"    call system("del ".a:dirname)
"  else
"   call system("rmdir ".a:dirname)
"  endif
""  call Dret("s:Rmdir")
"endfun

" ---------------------------------------------------------------------
" s:VimballHome: determine/get home directory path (usually from rtp) {{{2
fun! s:VimballHome()
"  call Dfunc("VimballHome()")
  if exists("g:vimball_home")
   let home= g:vimball_home
  else
   " go to vim plugin home
   for home in split(&rtp,',') + ['']
    if isdirectory(home) && filewritable(home) | break | endif
   endfor
   if home == ""
    " just pick the first directory
    let home= substitute(&rtp,',.*$','','')
   endif
   if (has("win32") || has("win95") || has("win64") || has("win16"))
    let home= substitute(home,'/','\\','g')
   endif
  endif
"  call Dret("VimballHome <".home.">")
  return home
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
  let s:lzkeep  = &lz
  let s:pmkeep  = &pm
  let s:repkeep = &report
  let s:vekeep  = &ve
  if exists("&acd")
   set ei=all ve=all noacd nofen noic report=999 nohid bt= ma lz pm=
  else
   set ei=all ve=all nofen noic report=999 nohid bt= ma lz pm=
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
  let &fen    = s:fenkeep
  let &hidden = s:hidkeep
  let &ic     = s:ickeep
  let &lz     = s:lzkeep
  let &pm     = s:pmkeep
  let &report = s:repkeep
  let &ve     = s:vekeep
  let &ei     = s:eikeep
  if s:makeep[0] != 0
   " restore mark a
"   call Decho("restore mark-a: makeep=".string(makeep))
   call setpos("'a",s:makeep)
  endif
  if exists("&acd")
   unlet s:regakeep s:acdkeep s:eikeep s:fenkeep s:hidkeep s:ickeep s:repkeep s:vekeep s:makeep s:lzkeep s:pmkeep
  else
   unlet s:regakeep s:eikeep s:fenkeep s:hidkeep s:ickeep s:repkeep s:vekeep s:makeep s:lzkeep s:pmkeep
  endif
  set bt=nofile noma
"  call Dret("RestoreSettings")
endfun

" ---------------------------------------------------------------------
" Modelines: {{{1
" vim: fdm=marker
