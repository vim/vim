" ---------------------------------------------------------------------
" getscript.vim
"  Author:	Charles E. Campbell, Jr.
"  Date:	Jul 10, 2008
"  Version:	31
"  Installing:	:help glvs-install
"  Usage:	:help glvs
"
" GetLatestVimScripts: 642 1 :AutoInstall: getscript.vim
"redraw!|call inputsave()|call input("Press <cr> to continue")|call inputrestore()
" ---------------------------------------------------------------------
" Initialization:	{{{1
" if you're sourcing this file, surely you can't be
" expecting vim to be in its vi-compatible mode!
if &cp
 echoerr "GetLatestVimScripts is not vi-compatible; not loaded (you need to set nocp)"
 finish
endif
let s:keepcpo = &cpo
set cpo&vim
"DechoTabOn

if exists("g:loaded_getscript")
 finish
endif
let g:loaded_getscript= "v31"

" ---------------------------
" Global Variables: {{{1
" ---------------------------
" Cygwin Detection ------- {{{2
if !exists("g:getscript_cygwin")
 if has("win32") || has("win95") || has("win64") || has("win16")
  if &shell =~ '\%(\<bash\>\|\<zsh\>\)\%(\.exe\)\=$'
   let g:getscript_cygwin= 1
  else
   let g:getscript_cygwin= 0
  endif
 else
  let g:getscript_cygwin= 0
 endif
endif
" shell quoting character {{{2
if exists("g:netrw_shq") && !exists("g:getscript_shq")
 let g:getscript_shq= g:netrw_shq
elseif !exists("g:getscript_shq")
 if exists("&shq") && &shq != ""
  let g:getscript_shq= &shq
 elseif exists("&sxq") && &sxq != ""
  let g:getscript_shq= &sxq
 elseif has("win32") || has("win95") || has("win64") || has("win16")
  if g:getscript_cygwin
   let g:getscript_shq= "'"
  else
   let g:getscript_shq= '"'
  endif
 else
  let g:getscript_shq= "'"
 endif
" call Decho("g:getscript_shq<".g:getscript_shq.">")
endif

" wget vs curl {{{2
if !exists("g:GetLatestVimScripts_wget")
 if executable("wget")
  let g:GetLatestVimScripts_wget= "wget"
 elseif executable("curl")
  let g:GetLatestVimScripts_wget= "curl"
 else
  let g:GetLatestVimScripts_wget    = 'echo "GetLatestVimScripts needs wget or curl"'
  let g:GetLatestVimScripts_options = ""
 endif
endif

" options that wget and curl require:
if !exists("g:GetLatestVimScripts_options")
 if g:GetLatestVimScripts_wget == "wget"
  let g:GetLatestVimScripts_options= "-q -O"
 elseif g:GetLatestVimScripts_wget == "curl"
  let g:GetLatestVimScripts_options= "-s -O"
 else
  let g:GetLatestVimScripts_options= ""
 endif
endif

" by default, allow autoinstall lines to work
if !exists("g:GetLatestVimScripts_allowautoinstall")
 let g:GetLatestVimScripts_allowautoinstall= 1
endif

"" For debugging:
"let g:GetLatestVimScripts_wget    = "echo"
"let g:GetLatestVimScripts_options = "options"

" ---------------------------------------------------------------------
" Check If AutoInstall Capable: {{{1
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
"else "Decho
" call Decho("g:GetLatestVimScripts_allowautoinstall=".g:GetLatestVimScripts_allowautoinstall.": :AutoInstall: disabled")
endif

" ---------------------------------------------------------------------
"  Public Interface: {{{1
com!        -nargs=0 GetLatestVimScripts call getscript#GetLatestVimScripts()
com!        -nargs=0 GetScript           call getscript#GetLatestVimScripts()
silent! com -nargs=0 GLVS                call getscript#GetLatestVimScripts()

" ---------------------------------------------------------------------
" GetLatestVimScripts: this function gets the latest versions of {{{1
"                      scripts based on the list in
"   (first dir in runtimepath)/GetLatest/GetLatestVimScripts.dat
fun! getscript#GetLatestVimScripts()
"  call Dfunc("GetLatestVimScripts() autoinstall<".s:autoinstall.">")

" insure that wget is executable
  if executable(g:GetLatestVimScripts_wget) != 1
   echoerr "GetLatestVimScripts needs ".g:GetLatestVimScripts_wget." which apparently is not available on your system"
"   call Dret("GetLatestVimScripts : wget not executable/availble")
   return
  endif

  " insure that fnameescape() is available
  if !exists("*fnameescape")
   echoerr "GetLatestVimScripts needs fnameescape() (provided by 7.1.299 or later)"
   return
  endif

  " Find the .../GetLatest subdirectory under the runtimepath
  for datadir in split(&rtp,',') + ['']
   if isdirectory(datadir."/GetLatest")
"    call Decho("found directory<".datadir.">")
    let datadir= datadir . "/GetLatest"
    break
   endif
   if filereadable(datadir."GetLatestVimScripts.dat")
"    call Decho("found ".datadir."/GetLatestVimScripts.dat")
    break
   endif
  endfor

  " Sanity checks: readability and writability
  if datadir == ""
   echoerr 'Missing "GetLatest/" on your runtimepath - see :help glvs-dist-install'
"   call Dret("GetLatestVimScripts : unable to find a GetLatest subdirectory")
   return
  endif

  if filewritable(datadir) != 2
   echoerr "(getLatestVimScripts) Your ".datadir." isn't writable"
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
  let hlskeep= &hls
  set ei=all hls&vim

  " record current directory, change to datadir, open split window with
  " datafile
  let origdir= getcwd()
"  call Decho("exe cd ".fnameescape(substitute(datadir,'\','/','ge')))
  exe "cd ".fnameescape(substitute(datadir,'\','/','ge'))
  split
"  call Decho("exe e ".fnameescape(substitute(datafile,'\','/','ge')))
  exe "e ".fnameescape(substitute(datafile,'\','/','ge'))
  res 1000
  let s:downloads = 0
  let s:downerrors= 0

  " Check on dependencies mentioned in plugins
"  call Decho(" ")
"  call Decho("searching plugins for GetLatestVimScripts dependencies")
  let lastline    = line("$")
"  call Decho("lastline#".lastline)
  let plugins     = split(globpath(&rtp,"plugin/*.vim"),'\n')
  let foundscript = 0
  let firstdir= ""

  for plugin in plugins
"   call Decho("plugin<".plugin.">")

   " don't process plugins in system directories
   if firstdir == ""
    let firstdir= substitute(plugin,'[/\\][^/\\]\+$','','')
"    call Decho("setting firstdir<".firstdir.">")
   else
    let curdir= substitute(plugin,'[/\\][^/\\]\+$','','')
"    call Decho("curdir<".curdir.">")
    if curdir != firstdir
"     call Decho("skipping subsequent plugins: curdir<".curdir."> != firstdir<".firstdir.">")
     break
    endif
   endif

   " read plugin in
   " evidently a :r creates a new buffer (the "#" buffer) that is subsequently unused -- bwiping it
   $
"   call Decho(" ")
"   call Decho(".dependency checking<".plugin."> line$=".line("$"))
"   call Decho("exe silent r ".fnameescape(plugin))
   exe "silent r ".fnameescape(plugin)
   exe "silent bwipe ".bufnr("#")

   while search('^"\s\+GetLatestVimScripts:\s\+\d\+\s\+\d\+','W') != 0
    let newscript= substitute(getline("."),'^"\s\+GetLatestVimScripts:\s\+\d\+\s\+\d\+\s\+\(.*\)$','\1','e')
    let llp1     = lastline+1
"    call Decho("..newscript<".newscript.">")

    " don't process ""GetLatestVimScripts lines -- those that have been doubly-commented out
    if newscript !~ '^"'
     " found a "GetLatestVimScripts: # #" line in the script; check if its already in the datafile
     let curline     = line(".")
     let noai_script = substitute(newscript,'\s*:AutoInstall:\s*','','e')
     exe llp1
     let srchline    = search('\<'.noai_script.'\>','bW')
"     call Decho("..noai_script<".noai_script."> srch=".srchline."curline#".line(".")." lastline#".lastline)

     if srchline == 0
      " found a new script to permanently include in the datafile
      let keep_rega   = @a
      let @a          = substitute(getline(curline),'^"\s\+GetLatestVimScripts:\s\+','','')
      exe lastline."put a"
      echomsg "Appending <".@a."> to ".datafile." for ".newscript
"      call Decho("..APPEND (".noai_script.")<".@a."> to GetLatestVimScripts.dat")
      let @a          = keep_rega
      let lastline    = llp1
      let curline     = curline     + 1
      let foundscript = foundscript + 1
"     else	" Decho
"      call Decho("..found <".noai_script."> (already in datafile at line#".srchline.")")
     endif

     let curline = curline + 1
     exe curline
    endif
   endwhile

   let llp1= lastline + 1
"   call Decho(".deleting lines: ".llp1.",$d")
   exe "silent! ".llp1.",$d"
  endfor
"  call Decho("--- end dependency checking loop ---  foundscript=".foundscript)
"  call Decho(" ")
"  call Dredir("BUFFER TEST (GetLatestVimScripts 1)","ls!")

  if foundscript == 0
   setlocal nomod
  endif

  " Check on out-of-date scripts using GetLatest/GetLatestVimScripts.dat
"  call Decho("begin: checking out-of-date scripts using datafile<".datafile.">")
  setlocal lz
  1
"  /^-----/,$g/^\s*\d/call Decho(getline("."))
  1
  /^-----/,$g/^\s*\d/call s:GetOneScript()
"  call Decho("--- end out-of-date checking --- ")

  " Final report (an echomsg)
  try
   silent! ?^-------?
  catch /^Vim\%((\a\+)\)\=:E114/
"   call Dret("GetLatestVimScripts : nothing done!")
   return
  endtry
  exe "norm! kz\<CR>"
  redraw!
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
   silent! w!
  endif
  q

  " restore events and current directory
  exe "cd ".fnameescape(substitute(origdir,'\','/','ge'))
  let &ei= eikeep
  let &hls= hlskeep
  setlocal nolz
"  call Dredir("BUFFER TEST (GetLatestVimScripts 2)","ls!")
"  call Dret("GetLatestVimScripts : did ".s:downloads." downloads")
endfun

" ---------------------------------------------------------------------
"  GetOneScript: (Get Latest Vim Script) this function operates {{{1
"    on the current line, interpreting two numbers and text as
"    ScriptID, SourceID, and Filename.
"    It downloads any scripts that have newer versions from vim.sf.net.
fun! s:GetOneScript(...)
"   call Dfunc("GetOneScript()")

 " set options to allow progress to be shown on screen
  let rega= @a
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
   let fname    = a:3
   let cmmnt    = ""
"   call Decho("scriptid<".scriptid.">")
"   call Decho("srcid   <".srcid.">")
"   call Decho("fname   <".fname.">")
  else
   let curline  = getline(".")
   if curline =~ '^\s*#'
    let @a= rega
"    call Dret("GetOneScript : skipping a pure comment line")
    return
   endif
   let parsepat = '^\s*\(\d\+\)\s\+\(\d\+\)\s\+\(.\{-}\)\(\s*#.*\)\=$'
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
    let fname= substitute(curline,parsepat,'\3','e')
   catch /^Vim\%((\a\+)\)\=:E486/
    let fname= ""
   endtry
   try
    let cmmnt= substitute(curline,parsepat,'\4','e')
   catch /^Vim\%((\a\+)\)\=:E486/
    let cmmnt= ""
   endtry
"   call Decho("curline <".curline.">")
"   call Decho("parsepat<".parsepat.">")
"   call Decho("scriptid<".scriptid.">")
"   call Decho("srcid   <".srcid.">")
"   call Decho("fname   <".fname.">")
  endif

  if scriptid == 0 || srcid == 0
   " When looking for :AutoInstall: lines, skip scripts that have   0 0 scriptname
   let @a= rega
"   call Dret("GetOneScript : skipping a scriptid==srcid==0 line")
   return
  endif

  let doautoinstall= 0
  if fname =~ ":AutoInstall:"
"   call Decho("case AutoInstall: fname<".fname.">")
   let aicmmnt= substitute(fname,'\s\+:AutoInstall:\s\+',' ','')
"   call Decho("aicmmnt<".aicmmnt."> s:autoinstall=".s:autoinstall)
   if s:autoinstall != ""
    let doautoinstall = g:GetLatestVimScripts_allowautoinstall
   endif
  else
   let aicmmnt= fname
  endif
"  call Decho("aicmmnt<".aicmmnt.">: doautoinstall=".doautoinstall)

  exe "norm z\<CR>"
  redraw!
"  call Decho('considering <'.aicmmnt.'> scriptid='.scriptid.' srcid='.srcid)
  echo 'considering <'.aicmmnt.'> scriptid='.scriptid.' srcid='.srcid

  " grab a copy of the plugin's vim.sf.net webpage
  let scriptaddr = 'http://vim.sf.net/script.php?script_id='.scriptid
  let tmpfile    = tempname()
  let v:errmsg   = ""

  " make up to three tries at downloading the description
  let itry= 1
  while itry <= 3
"   call Decho("try#".itry." to download description of <".aicmmnt."> with addr=".scriptaddr)
   if has("win32") || has("win16") || has("win95")
"    call Decho("new|exe silent r!".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(tmpfile).' '.s:Escape(scriptaddr)."|bw!")
    new|exe "silent r!".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(tmpfile).' '.s:Escape(scriptaddr)|bw!
   else
"    call Decho("exe silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(tmpfile)." ".s:Escape(scriptaddr))
    exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(tmpfile)." ".s:Escape(scriptaddr)
   endif
   if itry == 1
    exe "silent vsplit ".fnameescape(tmpfile)
   else
    silent! e %
   endif
   setlocal bh=wipe
  
   " find the latest source-id in the plugin's webpage
   silent! 1
   let findpkg= search('Click on the package to download','W')
   if findpkg > 0
    break
   endif
   let itry= itry + 1
  endwhile
"  call Decho(" --- end downloading tries while loop --- itry=".itry)

  " testing: did finding "Click on the package..." fail?
  if findpkg == 0 || itry >= 4
   silent q!
   call delete(tmpfile)
  " restore options
   let &t_ti        = t_ti
   let &t_te        = t_te
   let &rs          = rs
   let s:downerrors = s:downerrors + 1
"   call Decho("***warning*** couldn'".'t find "Click on the package..." in description page for <'.aicmmnt.">")
   echomsg "***warning*** couldn'".'t find "Click on the package..." in description page for <'.aicmmnt.">"
"   call Dret("GetOneScript : srch for /Click on the package/ failed")
   let @a= rega
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
"   call Decho("***warning*** couldn'".'t find "src_id=" in description page for <'.aicmmnt.">")
   echomsg "***warning*** couldn'".'t find "src_id=" in description page for <'.aicmmnt.">"
   let @a= rega
"  call Dret("GetOneScript : srch for /src_id/ failed")
   return
  endif
"  call Decho('found "src_id=" in description page')

  let srcidpat   = '^\s*<td class.*src_id=\(\d\+\)">\([^<]\+\)<.*$'
  let latestsrcid= substitute(getline("."),srcidpat,'\1','')
  let sname      = substitute(getline("."),srcidpat,'\2','') " script name actually downloaded
"  call Decho("srcidpat<".srcidpat."> latestsrcid<".latestsrcid."> sname<".sname.">")
  silent q!
  call delete(tmpfile)

  " convert the strings-of-numbers into numbers
  let srcid       = srcid       + 0
  let latestsrcid = latestsrcid + 0
"  call Decho("srcid=".srcid." latestsrcid=".latestsrcid." sname<".sname.">")

  " has the plugin's most-recent srcid increased, which indicates
  " that it has been updated
  if latestsrcid > srcid
"   call Decho("[latestsrcid=".latestsrcid."] <= [srcid=".srcid."]: need to update <".sname.">")

   let s:downloads= s:downloads + 1
   if sname == bufname("%")
    " GetLatestVimScript has to be careful about downloading itself
    let sname= "NEW_".sname
   endif

   " the plugin has been updated since we last obtained it, so download a new copy
"   call Decho("...downloading new <".sname.">")
   echomsg "...downloading new <".sname.">"
   if has("win32") || has("win16") || has("win95")
"    call Decho("new|exe silent r!".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(sname)." ".s:Escape('http://vim.sf.net/scripts/download_script.php?src_id='.latestsrcid)."|q")
    new|exe "silent r!".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(sname)." ".s:Escape('http://vim.sf.net/scripts/download_script.php?src_id='.latestsrcid)|q
   else
"    call Decho("exe silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(sname)." ".s:Escape('http://vim.sf.net/scripts/download_script.php?src_id='))
    exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".s:Escape(sname)." ".s:Escape('http://vim.sf.net/scripts/download_script.php?src_id=').latestsrcid
   endif

   " AutoInstall: only if doautoinstall has been requested by the plugin itself
   if doautoinstall
"    call Decho("attempting to do autoinstall: getcwd<".getcwd()."> filereadable(".sname.")=".filereadable(sname))
    if filereadable(sname)
"     call Decho("exe silent !".g:GetLatestVimScripts_mv." ".s:Escape(sname)." ".s:Escape(s:autoinstall))
     exe "silent !".g:GetLatestVimScripts_mv." ".s:Escape(sname)." ".s:Escape(s:autoinstall)
     let curdir    = escape(substitute(getcwd(),'\','/','ge'),"|[]*'\" #")
     let installdir= curdir."/Installed"
     if !isdirectory(installdir)
      call mkdir(installdir)
     endif
"    call Decho("exe cd ".fnameescape(s:autoinstall))
     exe "cd ".fnameescape(s:autoinstall)
     
     " decompress
     if sname =~ '\.bz2$'
"      call Decho("decompress: attempt to bunzip2 ".sname)
      exe "silent !bunzip2 ".s:Escape(sname)
      let sname= substitute(sname,'\.bz2$','','')
"      call Decho("decompress: new sname<".sname."> after bunzip2")
     elseif sname =~ '\.gz$'
"      call Decho("decompress: attempt to gunzip ".sname)
      exe "silent !gunzip ".s:Escape(sname)
      let sname= substitute(sname,'\.gz$','','')
"      call Decho("decompress: new sname<".sname."> after gunzip")
     endif
     
     " distribute archive(.zip, .tar, .vba) contents
     if sname =~ '\.zip$'
"      call Decho("dearchive: attempt to unzip ".sname)
      exe "silent !unzip -o ".s:Escape(sname)
     elseif sname =~ '\.tar$'
"      call Decho("dearchive: attempt to untar ".sname)
      exe "silent !tar -xvf ".s:Escape(sname)
     elseif sname =~ '\.vba$'
"      call Decho("dearchive: attempt to handle a vimball: ".sname)
      silent 1split
      exe "silent e ".fnameescape(sname)
      silent so %
      silent q
     endif
     
     if sname =~ '.vim$'
"      call Decho("dearchive: attempt to simply move ".sname." to plugin")
      exe "silent !".g:GetLatestVimScripts_mv." ".s:Escape(sname)." plugin"
     else
"      call Decho("dearchive: move <".sname."> to installdir<".installdir.">")
      exe "silent !".g:GetLatestVimScripts_mv." ".s:Escape(sname)." ".installdir
     endif
     
     " helptags step
     let docdir= substitute(&rtp,',.*','','e')."/doc"
"     call Decho("helptags: docdir<".docdir.">")
     exe "helptags ".fnameescape(docdir)
     exe "cd ".fnameescape(curdir)
    endif
    if fname !~ ':AutoInstall:'
     let modline=scriptid." ".latestsrcid." :AutoInstall: ".fname.cmmnt
    else
     let modline=scriptid." ".latestsrcid." ".fname.cmmnt
    endif
   else
    let modline=scriptid." ".latestsrcid." ".fname.cmmnt
   endif

   " update the data in the <GetLatestVimScripts.dat> file
   call setline(line("."),modline)
"   call Decho("update data in ".expand("%")."#".line(".").": modline<".modline.">")
"  else " Decho
"   call Decho("[latestsrcid=".latestsrcid."] <= [srcid=".srcid."], no need to update")
  endif

 " restore options
  let &t_ti = t_ti
  let &t_te = t_te
  let &rs   = rs
  let @a    = rega
"  call Dredir("BUFFER TEST (GetOneScript)","ls!")

"  call Dret("GetOneScript")
endfun

" ---------------------------------------------------------------------
" s:Escape: makes a string safe&suitable for the shell {{{2
fun! s:Escape(name)
"  call Dfunc("s:Escape(name<".a:name.">)")
  if exists("*shellescape")
   " shellescape() was added by patch 7.0.111
   let name= shellescape(a:name)
  else
   let name= g:getscript_shq . a:name . g:getscript_shq
  endif
"  call Dret("s:Escape ".name)
  return name
endfun

" ---------------------------------------------------------------------
" Restore Options: {{{1
let &cpo= s:keepcpo
unlet s:keepcpo

" ---------------------------------------------------------------------
"  Modelines: {{{1
" vim: ts=8 sts=2 fdm=marker nowrap
