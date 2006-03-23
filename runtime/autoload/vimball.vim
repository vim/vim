" vimball : construct a file containing both paths and files
" Author: Charles E. Campbell, Jr.
" Date:   Mar 22, 2006
" Version: 5
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
let g:loaded_vimball = "v5"
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
  let eikeep= &ei
  set ei=all

  let home   = substitute(&rtp,',.*$','','')
  let curdir = getcwd()
  exe "cd ".home

  " record current tab, initialize while loop index
  let curtabnr = tabpagenr()
  let linenr   = a:line1
"  call Decho("curtabnr=".curtabnr)

  while linenr <= a:line2
   let svfile  = getline(linenr)
"   call Decho("svfile<".svfile.">")
 
   if !filereadable(svfile)
    echohl Error | echo "unable to read file<".svfile.">" | echohl None
    let &ei= eikeep
    exe "cd ".curdir
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
   exe "$r ".svfile
   call setline(lastline+1,line("$") - lastline - 1)
"   call Decho("lastline=".lastline." line$=".line("$"))

  " restore to normal tab
   exe "tabn ".curtabnr
   let linenr= linenr + 1
  endwhile

  " write the vimball
  exe "tabn ".vbtabnr
  exe "cd ".curdir
  if a:writelevel
   exe "w! ".vbname
  else
   exe "w ".vbname
  endif
"  call Decho("Vimball<".vbname."> created")
  echo "Vimball<".vbname."> created"

  " remove the evidence
  setlocal nomod bh=wipe
  exe "tabn ".curtabnr
  exe "tabc ".vbtabnr

  " restore options
  let &ei= eikeep

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

  " initialize
  let regakeep = @a
  let eikeep   = &ei
  let vekeep   = &ve
  let makeep   = getpos("'a")
  let curtabnr = tabpagenr()
  set ei=all ve=all

  " set up vimball tab
  tabnew
  silent! file Vimball
  let vbtabnr= tabpagenr()
  let didhelp= ""

  " go to vim plugin home
  let home   = substitute(&rtp,',.*$','','')
  let curdir = getcwd()
"  call Decho("exe cd ".home)
  exe "cd ".home
  let linenr  = 4
  let filecnt = 0

  " give title to listing of (extracted) files from Vimball Archive
  if a:really
   echohl Title | echomsg "Vimball Archive" | echohl None
  else
   echohl Title | echomsg "Vimball Archive Listing" | echohl None
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
"   call Decho(linenr.": will extract file<".fname.">")
"   call Decho((linenr+1).": fsize=".fsize)

   " make directories if they don't exist yet
   let fnamebuf= fname
   while fnamebuf =~ '/'
   	let dirname  = substitute(fnamebuf,'/.*$','','e')
   	let fnamebuf = substitute(fnamebuf,'^.\{-}/\(.*\)$','\1','e')
	if !isdirectory(dirname)
"	 call Decho("making <".dirname.">")
	 call mkdir(dirname)
	endif
	exe "cd ".dirname
   endwhile
   exe "cd ".home

   " grab specified qty of lines and place into "a" buffer
   exe linenr
   norm! jjma
   exe (linenr + fsize + 1)
   silent norm! "ay'a
"   call Decho("yanked ".fsize." lines into register-a")

"   call Decho("didhelp<".didhelp."> fname<".fname.">")
   if didhelp == "" && fname =~ 'doc/[^/]\+\.txt$'
   	let didhelp= substitute(fname,'^\(.*\<doc\)[/\\][^.]*\.txt$','\1','e')
"	call Decho("didhelp<".didhelp.">")
   endif

   " copy "a" buffer into tab
"   call Decho('copy "a buffer into tab#'.vbtabnr)
   exe "tabn ".vbtabnr
   silent! %d
   silent norm! "aPGdd1G
"   call Decho("rega<".@a.">")

   " write tab to file
   if a:really
"    call Decho("exe w! ".fname)
    exe "silent w! ".fname
   endif

"   call Decho("exe tabn ".curtabnr)
   exe "tabn ".curtabnr
"   let oldlinenr = linenr " Decho
   let linenr    = linenr + fsize + 2
"   call Decho("update linenr= [linenr=".oldlinenr."] + [fsize=".fsize."] + 2 = ".linenr)
  endwhile

  " set up help
"  call Decho("about to set up help: didhelp<".didhelp.">")
  if didhelp != ""
"   call Decho("exe helptags ".home."/".didhelp)
   exe "helptags ".home."/".didhelp
   echomsg "did helptags"
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
  let &ei= eikeep
  let @a = regakeep
  if makeep[0] != 0
   " restore mark a
"   call Decho("restore mark-a: makeep=".string(makeep))
   call setpos("'a",makeep)
   ka
  endif
  exe "cd ".curdir

"  call Dret("Vimball")
endfun

let &cpo= s:keepcpo
unlet s:keepcpo
" =====================================================================
" Modelines: {{{1
" vim: fdm=marker
