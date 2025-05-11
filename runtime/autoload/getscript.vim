" ---------------------------------------------------------------------
" getscript.vim
"  Maintainer: This runtime file is looking for a new maintainer.
"  Original Author: Charles E. Campbell
"  Date:	Jan 21, 2014
"  Version:	37
"  Installing:	:help glvs-install
"  Usage:	:help glvs
"  Last Change:	{{{1
"   2024 Sep 08 by Vim Project: several small fixes (#15640)
"   2024 Sep 23 by Vim Project: runtime dir selection fix (#15722)
"                               autoloading search path fix
"                               substitution of hardcoded commands with global variables
"   2024 Nov 12 by Vim Project: fix problems on Windows (#16036)
"   2025 Feb 28 by Vim Project: add support for bzip3 (#16755)
"   2025 May 11 by Vim Project: check network connectivity (#17249)
"  }}}
"
" GetLatestVimScripts: 642 1 :AutoInstall: getscript.vim
"redraw!|call inputsave()|call input("Press <cr> to continue")|call inputrestore()
" ---------------------------------------------------------------------
" Initialization:	{{{1
" if you're sourcing this file, surely you can't be
" expecting vim to be in its vi-compatible mode!
if exists("g:loaded_getscript")
 finish
endif
let g:loaded_getscript= "v37"
if &cp
 echoerr "GetLatestVimScripts is not vi-compatible; not loaded (you need to set nocp)"
 finish
endif
if v:version < 901
 echohl WarningMsg
 echo "***warning*** this version of GetLatestVimScripts needs vim 9.1"
 echohl Normal
 finish
endif
let s:keepcpo = &cpo
set cpo&vim
"DechoTabOn

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

" wget vs curl {{{2
if !exists("g:GetLatestVimScripts_wget")
 if executable("wget")
  let g:GetLatestVimScripts_wget= "wget"
 elseif executable("curl.exe")
  " enforce extension: windows powershell desktop version has a curl alias that hides curl.exe
  let g:GetLatestVimScripts_wget= "curl.exe"
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
 elseif g:GetLatestVimScripts_wget =~ "curl"
  let g:GetLatestVimScripts_options= "-s -o"
 else
  let g:GetLatestVimScripts_options= ""
 endif
endif

" by default, allow autoinstall lines to work
if !exists("g:GetLatestVimScripts_allowautoinstall")
 let g:GetLatestVimScripts_allowautoinstall= 1
endif

" set up default scriptaddr address
if !exists("g:GetLatestVimScripts_scriptaddr")
 let g:GetLatestVimScripts_scriptaddr = 'https://www.vim.org/scripts/script.php?script_id='
endif

if !exists("g:GetLatestVimScripts_downloadaddr")
  let g:GetLatestVimScripts_downloadaddr = 'https://www.vim.org/scripts/download_script.php?src_id='
endif

" define decompression tools (on windows this allows redirection to wsl or git tools).
" Note tar is available as builtin since Windows 11.
if !exists("g:GetLatestVimScripts_bunzip2")
 let g:GetLatestVimScripts_bunzip2= "bunzip2"
endif

if !exists("g:GetLatestVimScripts_bunzip3")
 let g:GetLatestVimScripts_bunzip3= "bunzip3"
endif

if !exists("g:GetLatestVimScripts_gunzip")
 let g:GetLatestVimScripts_gunzip= "gunzip"
endif

if !exists("g:GetLatestVimScripts_unxz")
 let g:GetLatestVimScripts_unxz= "unxz"
endif

if !exists("g:GetLatestVimScripts_unzip")
 let g:GetLatestVimScripts_unzip= "unzip"
endif

"" For debugging:
"let g:GetLatestVimScripts_wget    = "echo"
"let g:GetLatestVimScripts_options = "options"

" ---------------------------------------------------------------------
" Check If AutoInstall Capable: {{{1
let s:autoinstall= ""
if g:GetLatestVimScripts_allowautoinstall

 let s:is_windows = has("win32") || has("gui_win32") || has("gui_win32s") || has("win16") || has("win64") || has("win32unix") || has("win95")
 let s:dotvim= s:is_windows ? "vimfiles" : ".vim"

 if !exists("g:GetLatestVimScripts_mv")
  if &shell =~? '\<pwsh\>\|\<powershell\>'
   let g:GetLatestVimScripts_mv= "move -Force"
  elseif s:is_windows && &shell =~? '\<cmd\>'
   " windows (but not cygwin/bash)
   let g:GetLatestVimScripts_mv= "move /Y"
  else
   " unix or cygwin bash/zsh
   " 'mv' overrides existing files without asking
    let g:GetLatestVimScripts_mv= "mv"
  endif
 endif

 if exists("g:GetLatestVimScripts_autoinstalldir") && isdirectory(g:GetLatestVimScripts_autoinstalldir)
  let s:autoinstall= g:GetLatestVimScripts_autoinstalldir"
 elseif exists('$HOME') && isdirectory(expand("$HOME")."/".s:dotvim)
  let s:autoinstall= $HOME."/".s:dotvim
 endif
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

  if executable(g:GetLatestVimScripts_wget) != 1
   echoerr "GetLatestVimScripts needs ".g:GetLatestVimScripts_wget." which apparently is not available on your system"
   return
  endif

  " Find the .../GetLatest subdirectory under the runtimepath
  for datadir in split(&rtp,',') + ['']
   if isdirectory(datadir."/GetLatest")
    let datadir= datadir . "/GetLatest"
    break
   endif
   if filereadable(datadir."GetLatestVimScripts.dat")
    break
   endif
  endfor

  " Sanity checks: readability and writability
  if datadir == ""
   echoerr 'Missing "GetLatest/" on your runtimepath - see :help glvs-dist-install'
   return
  endif
  if filewritable(datadir) != 2
   echoerr "(getLatestVimScripts) Your ".datadir." isn't writable"
   return
  endif
  let datafile= datadir."/GetLatestVimScripts.dat"
  if !filereadable(datafile)
   echoerr "Your data file<".datafile."> isn't readable"
   return
  endif
  if !filewritable(datafile)
   echoerr "Your data file<".datafile."> isn't writable"
   return
  endif
  " --------------------
  " Passed sanity checks
  " --------------------

  " don't let any event handlers interfere (like winmanager's, taglist's, etc)
  let eikeep  = &ei
  let hlskeep = &hls
  let acdkeep = &acd
  set ei=all hls&vim noacd

  " Edit the datafile (ie. GetLatestVimScripts.dat):
  " 1. record current directory (origdir),
  " 2. change directory to datadir,
  " 3. split window
  " 4. edit datafile
  let origdir= getcwd()
  exe "cd ".fnameescape(substitute(datadir,'\','/','ge'))
  split
  exe "e ".fnameescape(substitute(datafile,'\','/','ge'))
  res 1000
  let s:downloads = 0
  let s:downerrors= 0
  let s:message = []

  " Check on dependencies mentioned in plugins
  let lastline    = line("$")
  let firstdir    = substitute(&rtp,',.*$','','')
  let plugins     = split(globpath(firstdir,"plugin/**/*.vim"),'\n')
  let plugins     += split(globpath(firstdir,"ftplugin/**/*.vim"),'\n')
  let plugins     += split(globpath(firstdir,"AsNeeded/**/*.vim"),'\n')
  let plugins     += split(globpath(firstdir,"pack/*/start/*/plugin/**/*.vim"),'\n')
  let plugins     += split(globpath(firstdir,"pack/*/opt/*/plugin/**/*.vim"),'\n')
  let plugins     += split(globpath(firstdir,"pack/*/start/*/ftplugin/**/*.vim"),'\n')
  let plugins     += split(globpath(firstdir,"pack/*/opt/*/ftplugin/**/*.vim"),'\n')
  let foundscript = 0

  " this loop updates the GetLatestVimScripts.dat file
  " with dependencies explicitly mentioned in the plugins
  " via   GetLatestVimScripts: ... lines
  " It reads the plugin script at the end of the GetLatestVimScripts.dat
  " file, examines it, and then removes it.
  for plugin in plugins

   " read plugin in
   " evidently a :r creates a new buffer (the "#" buffer) that is subsequently unused -- bwiping it
   $
   exe "silent r ".fnameescape(plugin)
   exe "silent bwipe ".bufnr("#")

   while search('^"\s\+GetLatestVimScripts:\s\+\d\+\s\+\d\+','W') != 0
    let depscript   = substitute(getline("."),'^"\s\+GetLatestVimScripts:\s\+\d\+\s\+\d\+\s\+\(.*\)$','\1','e')
    let depscriptid = substitute(getline("."),'^"\s\+GetLatestVimScripts:\s\+\(\d\+\)\s\+.*$','\1','')
    let llp1        = lastline+1

    " found a "GetLatestVimScripts: # #" line in the script;
    " check if it's already in the datafile by searching backwards from llp1,
    " the (prior to reading in the plugin script) last line plus one of the GetLatestVimScripts.dat file,
    " for the script-id with no wrapping allowed.
    let curline     = line(".")
    let noai_script = substitute(depscript,'\s*:AutoInstall:\s*','','e')
    exe llp1
    let srchline    = search('^\s*'.depscriptid.'\s\+\d\+\s\+.*$','bW')
    if srchline == 0
     " this second search is taken when, for example, a   0 0 scriptname  is to be skipped over
     let srchline= search('\<'.noai_script.'\>','bW')
    endif

    if srchline == 0
     " found a new script to permanently include in the datafile
     let keep_rega   = @a
     let @a          = substitute(getline(curline),'^"\s\+GetLatestVimScripts:\s\+','','')
     echomsg "Appending <".@a."> to ".datafile." for ".depscript
     exe lastline."put a"
     let @a          = keep_rega
     let lastline    = llp1
     let curline     = curline     + 1
     let foundscript = foundscript + 1
    endif

    let curline = curline + 1
    exe curline
   endwhile

   " llp1: last line plus one
   let llp1= lastline + 1
   exe "silent! ".llp1.",$d"
  endfor

  if foundscript == 0
   setlocal nomod
  endif

  " --------------------------------------------------------------------
  " Check on out-of-date scripts using GetLatest/GetLatestVimScripts.dat
  " --------------------------------------------------------------------
  setlocal lz
  1
  /^-----/,$g/^\s*\d/call s:GetOneScript()

  " Final report (an echomsg)
  try
   silent! ?^-------?
  catch /^Vim\%((\a\+)\)\=:E114/
   return
  endtry
  exe "norm! kz\<CR>"
  redraw!
  if !empty(s:message)
   echohl WarningMsg
   for mess in s:message
    echom mess
   endfor
   let s:downerrors += len(s:message)
  endif
  let s:msg = ""
  if s:downloads == 1
  let s:msg = "Downloaded one updated script to <".datadir.">"
  elseif s:downloads > 1
   let s:msg= "Downloaded ".s:downloads." updated scripts to <".datadir.">"
  else
   let s:msg= empty(s:message) ? "Everything was already current" : "There were some errors"
  endif
  if s:downerrors > 0
   let s:msg= s:msg." (".s:downerrors." downloading errors)"
  endif
  echomsg s:msg
  " save the file
  if &mod
   silent! w!
  endif
  q!

  " restore events and current directory
  exe "cd ".fnameescape(substitute(origdir,'\','/','ge'))
  let &ei  = eikeep
  let &hls = hlskeep
  let &acd = acdkeep
  setlocal nolz
endfun

" ---------------------------------------------------------------------
"  GetOneScript: (Get Latest Vim script) this function operates {{{1
"    on the current line, interpreting two numbers and text as
"    ScriptID, SourceID, and Filename.
"    It downloads any scripts that have newer versions from vim.sourceforge.net.
fun! s:GetOneScript(...)
 " set options to allow progress to be shown on screen
  let rega= @a
  let t_ti= &t_ti
  let t_te= &t_te
  let rs  = &rs
  let ssl = &ssl

  set t_ti= t_te= nors
  " avoid issues with shellescape() on Windows
  if s:is_windows && &shell =~? '\<cmd\>'
    set noshellslash
  endif

  " restore valures afterwards
  defer execute("let @a = rega | let &t_ti = t_ti | let &t_te = t_te | let &rs = rs | let &ssl = ssl")

 " put current line on top-of-screen and interpret it into
 " a      script identifier  : used to obtain webpage
 "        source identifier : used to identify current version
 " and an associated comment: used to report on what's being considered
  if a:0 >= 3
   let scriptid = a:1
   let srcid    = a:2
   let fname    = a:3
   let cmmnt    = ""
  else
   let curline  = getline(".")
   if curline =~ '^\s*#'
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
  endif

  " plugin author protection from downloading his/her own scripts atop their latest work
  " When looking for :AutoInstall: lines, skip scripts that have   0 0 scriptname
  if scriptid == 0 || srcid == 0
   return
  endif

  let doautoinstall= 0
  if fname =~ ":AutoInstall:"
   let aicmmnt= substitute(fname,'\s\+:AutoInstall:\s\+',' ','')
   if s:autoinstall != ""
    let doautoinstall = g:GetLatestVimScripts_allowautoinstall
   endif
  else
   let aicmmnt= fname
  endif

  exe "norm z\<CR>"
  redraw!
  echo 'considering <'.aicmmnt.'> scriptid='.scriptid.' srcid='.srcid

  " grab a copy of the plugin's vim.sourceforge.net webpage
  let scriptaddr = g:GetLatestVimScripts_scriptaddr.scriptid
  let tmpfile    = tempname()
  let v:errmsg   = ""

  " Check if URLs are reachable
  if !CheckVimScriptURL(scriptid, srcid)
   return
  endif

  " make up to three tries at downloading the description
  let itry= 1
  while itry <= 3
   if has("win32") || has("win16") || has("win95")
    new|exe "silent r!".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".shellescape(tmpfile).' '.shellescape(scriptaddr)|bw!
   else
    exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".shellescape(tmpfile)." ".shellescape(scriptaddr)
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

  " testing: did finding "Click on the package..." fail?
  if findpkg == 0 || itry >= 4
   silent q!
   call delete(tmpfile)
  " restore options
   let &t_ti        = t_ti
   let &t_te        = t_te
   let &rs          = rs
   let s:downerrors = s:downerrors + 1
   echomsg "***warning*** couldn'".'t find "Click on the package..." in description page for <'.aicmmnt.">"
   return
  endif

  let findsrcid= search('src_id=','W')
  if findsrcid == 0
   silent q!
   call delete(tmpfile)
  " restore options
   let &t_ti        = t_ti
   let &t_te        = t_te
   let &rs          = rs
   let s:downerrors = s:downerrors + 1
   echomsg "***warning*** couldn'".'t find "src_id=" in description page for <'.aicmmnt.">"
   return
  endif

  let srcidpat   = '^\s*<td class.*src_id=\(\d\+\)">\([^<]\+\)<.*$'
  let latestsrcid= substitute(getline("."),srcidpat,'\1','')
  let sname      = substitute(getline("."),srcidpat,'\2','') " script name actually downloaded
  silent q!
  call delete(tmpfile)

  " convert the strings-of-numbers into numbers
  let srcid       = srcid       + 0
  let latestsrcid = latestsrcid + 0

  " has the plugin's most-recent srcid increased, which indicates that it has been updated
  if latestsrcid > srcid

   let s:downloads= s:downloads + 1
   if sname == bufname("%")
    " GetLatestVimScript has to be careful about downloading itself
    let sname= "NEW_".sname
   endif

   " -----------------------------------------------------------------------------
   " the plugin has been updated since we last obtained it, so download a new copy
   " -----------------------------------------------------------------------------
   echomsg ".downloading new <".sname.">"
   if has("win32") || has("win16") || has("win95")
    new|exe "silent r!".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".shellescape(sname)." ".shellescape(g:GetLatestVimScripts_downloadaddr.latestsrcid)|bw!
   else
    exe "silent !".g:GetLatestVimScripts_wget." ".g:GetLatestVimScripts_options." ".shellescape(sname)." ".shellescape(g:GetLatestVimScripts_downloadaddr.latestsrcid)
   endif

   " --------------------------------------------------------------------------
   " AutoInstall: only if doautoinstall has been requested by the plugin itself
   " --------------------------------------------------------------------------
   if doautoinstall
    if filereadable(sname)
     exe "silent !".g:GetLatestVimScripts_mv." ".shellescape(sname)." ".shellescape(s:autoinstall)
     let curdir    = fnameescape(substitute(getcwd(),'\','/','ge'))
     let installdir= curdir."/Installed"
     if !isdirectory(installdir)
      call mkdir(installdir)
     endif
     exe "cd ".fnameescape(s:autoinstall)

     " determine target directory for moves
     let firstdir= substitute(&rtp,',.*$','','')
     let pname   = substitute(sname,'\..*','.vim','')
     if filereadable(firstdir.'/AsNeeded/'.pname)
      let tgtdir= "AsNeeded"
     else
      let tgtdir= "plugin"
     endif

     " decompress
     if sname =~ '\.bz2$'
      exe "sil !".g:GetLatestVimScripts_bunzip2." ".shellescape(sname)
      let sname= substitute(sname,'\.bz2$','','')
     elseif sname =~ '\.bz3$'
      exe "sil !".g:GetLatestVimScripts_bunzip3." ".shellescape(sname)
      let sname= substitute(sname,'\.bz3$','','')
     elseif sname =~ '\.gz$'
      exe "sil !".g:GetLatestVimScripts_gunzip." ".shellescape(sname)
      let sname= substitute(sname,'\.gz$','','')
     elseif sname =~ '\.xz$'
      exe "sil !".g:GetLatestVimScripts_unxz." ".shellescape(sname)
      let sname= substitute(sname,'\.xz$','','')
     else
     endif

     " distribute archive(.zip, .tar, .vba, .vmb, ...) contents
     if sname =~ '\.zip$'
      exe "silent !".g:GetLatestVimScripts_unzip." -o ".shellescape(sname)
     elseif sname =~ '\.tar$'
      exe "silent !tar -xvf ".shellescape(sname)
     elseif sname =~ '\.tgz$'
      exe "silent !tar -zxvf ".shellescape(sname)
     elseif sname =~ '\.taz$'
      exe "silent !tar -Zxvf ".shellescape(sname)
     elseif sname =~ '\.tbz$'
      exe "silent !tar -jxvf ".shellescape(sname)
     elseif sname =~ '\.txz$'
      exe "silent !tar -Jxvf ".shellescape(sname)
     elseif sname =~ '\.vba$\|\.vmb$'
      silent 1split
      if exists("g:vimball_home")
       let oldvimballhome= g:vimball_home
      endif
      let g:vimball_home= s:autoinstall
      exe "silent e ".fnameescape(sname)
      silent so %
      silent q
      if exists("oldvimballhome")
       let g:vimball_home= oldvimballhome
      else
       unlet g:vimball_home
      endif
     endif

     " ---------------------------------------------
     " move plugin to plugin/ or AsNeeded/ directory
     " ---------------------------------------------
     if sname =~ '.vim$'
      exe "silent !".g:GetLatestVimScripts_mv." ".shellescape(sname)." ".tgtdir
     else
      exe "silent !".g:GetLatestVimScripts_mv." ".shellescape(sname)." ".installdir
     endif
     if tgtdir != "plugin"
      exe "silent !".g:GetLatestVimScripts_mv." ".shellescape("plugin/".pname)." ".tgtdir
     endif

     " helptags step
     let docdir= substitute(&rtp,',.*','','e')."/doc"
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
  endif
endfun

" CheckVimScriptURL: Check Network Connection {{{1
" Check status code of scriptaddr and downloadaddr
" return v:true if the script is downloadable or v:false in case of errors
fun CheckVimScriptURL(script_id, src_id)
  " doesn't work with powershell
  if !executable('curl') || &shell =~? 'pwsh\|powershell'
    return v:true
  endif
  let output = has("win32") ? ' -o NUL ' : ' -o /dev/null '

  let temp = tempname()
  defer delete(temp)
  let script_url = g:GetLatestVimScripts_scriptaddr . a:script_id
  let download_url = g:GetLatestVimScripts_downloadaddr . a:src_id

  let script_cmd = 'curl -s -I -w "%{http_code}"' . output . shellescape(script_url) . ' >' . shellescape(temp)
  call system(script_cmd)
  let script_status = readfile(temp, 'b')[0]

  let download_cmd = 'curl -s -I -w "%{http_code}"' . output . shellescape(download_url) . ' >' . shellescape(temp)
  call system(download_cmd)
  let download_status = readfile(temp, 'b')[0]

  if script_status !=# '200'
    let s:message += [ printf('Error: Failed to reach script: %s', a:script_id) ]
    return v:false
  endif

  if download_status !=# '200'
    let s:message += [ printf('Error: Failed to download script %s', a:script_id) ]
    return v:false
  endif
  return v:true
endfun

" ---------------------------------------------------------------------
" Restore Options: {{{1
let &cpo= s:keepcpo
unlet s:keepcpo

" ---------------------------------------------------------------------
"  Modelines: {{{1
" vim: ts=8 sts=2 fdm=marker nowrap
