" ---------------------------------------------------------------------
" GetLatestVimScripts.vim
"  Author:		Charles E. Campbell, Jr.
"  Date:		Feb 15, 2006
"  Version:		20
"  Installing:	:help glvs-install
"  Usage:		:help glvs
"
" GetLatestVimScripts: 642 1 :AutoInstall: GetLatestVimScripts.vim
" ---------------------------------------------------------------------
" Initialization:	{{{1
" if you're sourcing this file, surely you can't be
" expecting vim to be in its vi-compatible mode
if &cp
  if &verbose
	echo "GetLatestVimScripts is not vi-compatible; not loaded (you need to set nocp)"
  endif
 finish
endif
let s:keepfo  = &fo
let s:keepcpo = &cpo
set cpo&vim

if exists("loaded_GetLatestVimScripts")
 finish
endif
let g:loaded_GetLatestVimScripts= "v20"

" ---------------------------------------------------------------------
"  Global Variables: {{{1
" allow user to change the command for obtaining scripts (does fetch work?)
if !exists("g:GetLatestVimScripts_wget")
 let g:GetLatestVimScripts_wget= "wget"
endif
if !exists("g:GetLatestVimScripts_options")
 let g:GetLatestVimScripts_options= "-q -O"
endif
if !exists("g:GetLatestVimScripts_allowautoinstall")
 let g:GetLatestVimScripts_allowautoinstall= 1
endif

"" For debugging:
"let g:GetLatestVimScripts_wget    = "echo"
"let g:GetLatestVimScripts_options = "options"

" check if s:autoinstall is possible
let s:autoinstall= ""
if g:GetLatestVimScripts_allowautoinstall

 if (has("win32") || has("gui_win32") || has("gui_win32s") || has("win16") || has("win64") || has("win32unix") || has("win95")) && &shell != "bash"
  " windows (but not cygwin/bash)
  let s:dotvim= "vimfiles"
  if !exists("g:GetLatestVimScripts_mv")
   let g:GetLatestVimScripts_mv= "ren"
  endif

 else
  " unix
  let s:dotvim= ".vim"
  if !exists("g:GetLatestVimScripts_mv")
   let g:GetLatestVimScripts_mv= "mv"
  endif
 endif

 if exists('$HOME') && isdirectory(expand("$HOME")."/".s:dotvim)
  let s:autoinstall= $HOME."/".s:dotvim
 endif
" call Decho("s:autoinstall<".s:autoinstall.">")
else
" call Decho("g:GetLatestVimScripts_allowautoinstall=".g:GetLatestVimScripts_allowautoinstall.": :AutoInstall: disabled")
endif

" ---------------------------------------------------------------------
"  Public Interface: {{{1
com!        -nargs=0 GetLatestVimScripts call <SID>GetLatestVimScripts()
silent! com -nargs=0 GLVS                call <SID>GetLatestVimScripts()

" ---------------------------------------------------------------------
"  GetOneScript: (Get Latest Vim Script) this function operates {{{1
"    on the current line, interpreting two numbers and text as
"    ScriptID, SourceID, and Filename.
"    It downloads any scripts that have newer versions from vim.sf.net.
fun! <SID>GetOneScript(...)
"   call Dfunc("GetOneScript()")

 " set options to allow progress to be shown on screen
  let t_ti= &t_ti
  let t_te= &t_te
  let rs  = &rs
  set t_ti= t_te= nors

 " put current line on top-of-screen and interpret it into
 " a      script identifer  : used to obtain webpage
 "        source identifier : used to identify current version
 " and an associated comment: used to report on what's being considered
  if a:0 >= 3
   let scriptid = a:1
   let srcid    = a:2
   let cmmnt    = a:3
"   call Decho("scriptid<".scriptid.">")
"   call Decho("srcid   <".srcid.">")
"   call Decho("cmmnt   <".cmmnt.">")
  else
   let curline  = getline(".")
   let parsepat = '^\s*\(\d\+\)\s\+\(\d\+\)\s\+\(.\{-}\)$'
   try
    let scriptid = substitute(curline,parsepat,'\1','e')
   catch /^Vim\%((\a\+)\)\=:E486/
   	let scriptid= 0
   endtry
   try
    let srcid    = substitute(curline,parsepat,'\2','e')
   catch /^Vim\%((\a\+)\)\=:E486/
   	let srcid= 0
   endtry
   try
    let cmmnt    = substitute(curline,parsepat,'\3','e')
   catch /^Vim\%((\a\+)\)\=:E486/
   	let cmmnt= ""
   endtry
"   call Decho("curline <".curline.">")
"   call Decho("parsepat<".parsepat.">")
"   call Decho("scriptid<".scriptid.">")
"   call Decho("srcid   <".srcid.">")
"   call Decho("cmmnt   <".cmmnt.">")
  endif

  if scriptid == 0 || srcid == 0
   " When looking for :AutoInstall: lines, skip scripts that
   " have  0 0 scriptname
"   call Dret("GetOneScript : skipping a scriptid==srcid==0 line")
   return
  endif

  let doautoinstall= 0
  if cmmnt =~ ":AutoInstall:"
"   call Decho("cmmnt<".cmmnt."> has :AutoInstall:...")
   let aicmmnt= substitute(cmmnt,'\s\+:AutoInstall:\s\+',' ','')
"   call Decho("aicmmnt<".aicmmnt."> s:autoinstall=".s:autoinstall)
   if s:autoinstall != ""
    let doautoinstall = g:GetLatestVimScripts_allowautoinstall
   endif
  else
   let aicmmnt= cmmnt
  endif
"  call Decho("aicmmnt<".aicmmnt.">: doautoinstall=".doautoinstall)

  exe "norm z\<CR>"
  redraw!
"  call Decho('considering <'.aicmmnt.'> scriptid='.scriptid.' srcid='.srcid)
  echomsg 'considering <'.aicmmnt.'> scriptid='.scriptid.' srcid='.srcid

  " grab a copy of the plugin's vim.sf.net webpage
  let scriptaddr = 'http://vim.sf.net/script.php?script_id='.scriptid
  let tmpfile    = tempname()
  let v:errmsg   = ""

  " make three tries at downloading the description
  let itry= 1
  while itry <= 3
"   	call Decho("try#".itry." to download description of <".aicmmnt."> with addr=".scriptaddr)
  	if has("win32") || has("win16") || has("win95")
"     call Decho("silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".tmpfile.' "'.scriptaddr.'"')
     exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".tmpfile.' "'.scriptaddr.'"'
	else
"     call Decho("silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".tmpfile." '".scriptaddr."'")
     exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".tmpfile." '".scriptaddr."'"
	endif
	if itry == 1
    exe "silent vsplit ".tmpfile
	else
	 silent! e %
	endif
  
   " find the latest source-id in the plugin's webpage
   silent! 1
   let findpkg= search('Click on the package to download','W')
   if findpkg > 0
    break
   endif
   let itry= itry + 1
  endwhile
"  call Decho(" --- end downloading tries while loop --- itry=".itry)

  " testing: did finding /Click on the package.../ fail?
  if findpkg == 0 || itry >= 4
    silent q!
    call delete(tmpfile)
   " restore options
	let &t_ti        = t_ti
	let &t_te        = t_te
	let &rs          = rs
	let s:downerrors = s:downerrors + 1
"  	call Decho("***warning*** couldn'".'t find "Click on the package..." in description page for <'.aicmmnt.">")
  	echomsg "***warning*** couldn'".'t find "Click on the package..." in description page for <'.aicmmnt.">"
"	call Dret("GetOneScript : srch for /Click on the package/ failed")
  	return
  endif
"  call Decho('found "Click on the package to download"')

  let findsrcid= search('src_id=','W')
  if findsrcid == 0
    silent q!
    call delete(tmpfile)
   " restore options
	let &t_ti        = t_ti
	let &t_te        = t_te
	let &rs          = rs
	let s:downerrors = s:downerrors + 1
"  	call Decho("***warning*** couldn'".'t find "src_id=" in description page for <'.aicmmnt.">")
  	echomsg "***warning*** couldn'".'t find "src_id=" in description page for <'.aicmmnt.">"
"	call Dret("GetOneScript : srch for /src_id/ failed")
  	return
  endif
"  call Decho('found "src_id=" in description page')

  let srcidpat   = '^\s*<td class.*src_id=\(\d\+\)">\([^<]\+\)<.*$'
  let latestsrcid= substitute(getline("."),srcidpat,'\1','')
  let fname      = substitute(getline("."),srcidpat,'\2','')
"  call Decho("srcidpat<".srcidpat."> latestsrcid<".latestsrcid."> fname<".fname.">")
  silent q!
  call delete(tmpfile)

  " convert the strings-of-numbers into numbers
  let srcid       = srcid       + 0
  let latestsrcid = latestsrcid + 0
"   call Decho("srcid=".srcid." latestsrcid=".latestsrcid." fname<".fname.">")

  " has the plugin's most-recent srcid increased, which indicates
  " that it has been updated
  if latestsrcid > srcid
   let s:downloads= s:downloads + 1
   if fname == bufname("%")
    " GetLatestVimScript has to be careful about downloading itself
    let fname= "NEW_".fname
   endif

   " the plugin has been updated since we last obtained it, so download a new copy
"   call Decho("...downloading new <".fname.">")
   echomsg "...downloading new <".fname.">"
   if has("win32") || has("gui_win32") || has("gui_win32s") || has("win16") || has("win64") || has("win32unix") || has("win95")
"    call Decho("windows: silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".fname.' "'.'http://vim.sf.net/scripts/download_script.php?src_id='.latestsrcid.'"')
    exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".fname.' "'.'http://vim.sf.net/scripts/download_script.php?src_id='.latestsrcid.'"'
   else
"    call Decho("unix: silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".fname." '".'http://vim.sf.net/scripts/download_script.php?src_id='.latestsrcid."'")
    exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".fname." '".'http://vim.sf.net/scripts/download_script.php?src_id='.latestsrcid."'"
   endif

   " AutoInstall: only if doautoinstall is so indicating
   if doautoinstall
"   	call Decho("attempting to do autoinstall: getcwd<".getcwd()."> filereadable(".fname.")=".filereadable(fname))
	if filereadable(fname)
"	 call Decho("move <".fname."> to ".s:autoinstall)
"	 call Decho("DISABLED for testing")
   	 exe "silent !"g:GetLatestVimScripts_mv." ".fname." ".s:autoinstall
	 let curdir= escape(substitute(getcwd(),'\','/','ge'),"|[]*'\" #")
	 exe "cd ".s:autoinstall
	 if fname =~ '\.bz2$'
"	  call Decho("attempt to bunzip2 ".fname)
	  exe "silent !bunzip2 ".fname
	  let fname= substitute(fname,'\.bz2$','','')
	 elseif fname =~ '\.gz$'
"	  call Decho("attempt to gunzip ".fname)
	  exe "silent !gunzip ".fname
	  let fname= substitute(fname,'\.gz$','','')
	 endif
	 if fname =~ '\.zip$'
"	  call Decho("attempt to unzip ".fname)
	  exe "silent !unzip -o".fname
	 elseif fname =~ '\.tar$'
"	  call Decho("attempt to untar ".fname)
	  exe "silent !tar -oxvf ".fname
	 endif
	 if fname =~ '.vim$'
"	  call Decho("attempt to simply move ".fname." to plugin")
	  exe "silent !".g:GetLatestVimScripts_mv." ".fname." plugin"
	 endif
	 exe "helptags ../".s:dotvim."/doc"
	 exe "cd ".curdir
	endif
   endif

   " update the data in the <GetLatestVimScripts.dat> file
   let modline=scriptid." ".latestsrcid." ".cmmnt
   call setline(line("."),modline)
"   call Decho("modline<".modline."> (updated GetLatestVimScripts.dat file)")
  endif

 " restore options
  let &t_ti= t_ti
  let &t_te= t_te
  let &rs  = rs

"  call Dret("GetOneScript")
endfun

" ---------------------------------------------------------------------
" GetLatestVimScripts: this function gets the latest versions of {{{1
" scripts based on the list in
"
"   (first dir in runtimepath)/GetLatest/GetLatestVimScripts.dat
fun! <SID>GetLatestVimScripts()
"  call Dfunc("GetLatestVimScripts() autoinstall<".s:autoinstall.">")

" insure that wget is executable
  if executable(g:GetLatestVimScripts_wget) != 1
   echoerr "GetLatestVimScripts needs ".g:GetLatestVimScripts_wget." which apparently is not available on your system"
"   call Dret("GetLatestVimScripts : wget not executable/availble")
   return
  endif

  " Find the .../GetLatest sudirectory under the runtimepath
  let rtplist= &rtp
  while rtplist != ""
   let datadir= substitute(rtplist,',.*$','','e')."/GetLatest"
   if isdirectory(datadir)
"   	call Decho("found directory<".datadir.">")
    break
   endif
   unlet datadir
   if rtplist =~ ','
    let rtplist= substitute(rtplist,'^.\{-},','','e')
   else
   	let rtplist= ""
   endif
  endwhile

  " Sanity checks: readability and writability
  if !exists("datadir")
   echoerr "Unable to find a GetLatest subdirectory on your runtimepath"
"   call Dret("GetLatestVimScripts : unable to find a GetLatest subdirectory")
   return
  endif
  if filewritable(datadir) != 2
   echoerr "Your ".datadir." isn't writable"
"   call Dret("GetLatestVimScripts : non-writable directory<".datadir.">")
   return
  endif
  let datafile= datadir."/GetLatestVimScripts.dat"
  if !filereadable(datafile)
   echoerr "Your data file<".datafile."> isn't readable"
"   call Dret("GetLatestVimScripts : non-readable datafile<".datafile.">")
   return
  endif
  if !filewritable(datafile)
   echoerr "Your data file<".datafile."> isn't writable"
"   call Dret("GetLatestVimScripts : non-writable datafile<".datafile.">")
   return
  endif
"  call Decho("datadir  <".datadir.">")
"  call Decho("datafile <".datafile.">")

  " don't let any events interfere (like winmanager's, taglist's, etc)
  let eikeep= &ei
  set ei=all

  " record current directory, change to datadir, open split window with
  " datafile
  let origdir= getcwd()
  exe "cd ".escape(substitute(datadir,'\','/','ge'),"|[]*'\" #")
  split
  exe "e ".escape(substitute(datafile,'\','/','ge'),"|[]*'\" #")
  res 1000
  let s:downloads = 0
  let s:downerrors= 0

  " Check on dependencies mentioned in plugins
"  call Decho(" ")
"  call Decho("searching plugins for GetLatestVimScripts dependencies")
  let lastline    = line("$")
  let plugins     = globpath(&rtp,"plugin/*.vim")
  let foundscript = 0

"  call Decho("plugins<".plugins."> lastline#".lastline)
  while plugins != ""
   let plugin = substitute(plugins,'\n.*$','','e')
   let plugins= (plugins =~ '\n')? substitute(plugins,'^.\{-}\n\(.*\)$','\1','e') : ""
   $
"   call Decho(".dependency checking<".plugin."> line$=".line("$"))
   exe "silent r ".plugin
   while search('^"\s\+GetLatestVimScripts:\s\+\d\+\s\+\d\+','W') != 0
    let newscript= substitute(getline("."),'^"\s\+GetLatestVimScripts:\s\+\d\+\s\+\d\+\s\+\(.*\)$','\1','e')
    let llp1     = lastline+1

	if newscript !~ '^"'
	 " found a "GetLatestVimScripts: # #" line in the script; check if its already in the datafile
	 let curline     = line(".")
	 let noai_script = substitute(newscript,'\s*:AutoInstall:\s*','','e')
	 exe llp1
	 let srchline    = search('\<'.noai_script.'\>','bW')
"	 call Decho("..newscript<".newscript."> noai_script<".noai_script."> srch=".srchline." lastline=".lastline)

	 if srchline == 0
	  " found a new script to permanently include in the datafile
	  let keep_rega   = @a
	  let @a          = substitute(getline(curline),'^"\s\+GetLatestVimScripts:\s\+','','')
	  exe lastline."put a"
	  echomsg "Appending <".@a."> to ".datafile." for ".newscript
"	  call Decho("..APPEND (".noai_script.")<".@a."> to GetLatestVimScripts.dat")
	  let @a          = keep_rega
	  let lastline    = llp1
	  let curline     = curline     + 1
	  let foundscript = foundscript + 1
"	 else	" Decho
"	  call Decho("..found <".noai_script."> (already in datafile at line#".srchline.")")
	 endif

	 let curline = curline + 1
	 exe curline
	endif

   endwhile
   let llp1= lastline + 1
"   call Decho(".deleting lines: ".llp1.",$d")
   exe "silent! ".llp1.",$d"
  endwhile

  if foundscript == 0
   set nomod
  endif

  " Check on out-of-date scripts using GetLatest/GetLatestVimScripts.dat
  set lz
"  call Decho(" --- end of dependency checking loop --- ")
"  call Decho("call GetOneScript on lines at end of datafile<".datafile.">")
  1
  /^-----/,$g/^\s*\d/call <SID>GetOneScript()

  " Final report (an echomsg)
  try
   silent! ?^-------?
  catch /^Vim\%((\a\+)\)\=:E114/
"   call Dret("GetLatestVimScripts : nothing done!")
   return
  endtry
  exe "norm! kz\<CR>"
  let s:msg = ""
  if s:downloads == 1
  let s:msg = "Downloaded one updated script to <".datadir.">"
  elseif s:downloads == 2
   let s:msg= "Downloaded two updated scripts to <".datadir.">"
  elseif s:downloads > 1
   let s:msg= "Downloaded ".s:downloads." updated scripts to <".datadir.">"
  else
   let s:msg= "Everything was already current"
  endif
  if s:downerrors > 0
   let s:msg= s:msg." (".s:downerrors." downloading errors)"
  endif
  echomsg s:msg
  " save the file
  if &mod
   wq
  else
   q
  endif

  " restore events and current directory
  exe "cd ".escape(substitute(origdir,'\','/','ge'),"|[]*'\" #")
  let &ei= eikeep
  set nolz
"  call Dret("GetLatestVimScripts : did ".s:downloads." downloads")
endfun
" ---------------------------------------------------------------------

" Restore Options: {{{1
let &fo = s:keepfo
let &cpo= s:keepcpo

" vim: ts=8 sts=2 fdm=marker nowrap
