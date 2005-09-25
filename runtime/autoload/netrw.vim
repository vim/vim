" netrw.vim: Handles file transfer and remote directory listing across a network
"            AUTOLOAD PORTION
" Last Change:	Aug 29, 2005
" Maintainer:	Charles E Campbell, Jr <drchipNOSPAM at campbellfamily dot biz>
" GetLatestVimScripts: 1075 1 :AutoInstall: netrw.vim
" Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               netrw.vim is provided *as is* and comes with no warranty
"               of any kind, either expressed or implied. By using this
"               plugin, you agree that in no event will the copyright
"               holder be liable for any damages resulting from the use
"               of this software.
"
"  But be doers of the Word, and not only hearers, deluding your own selves {{{1
"  (James 1:22 RSV)
" =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
let s:keepcpo= &cpo
set cpo&vim
" call Decho("doing autoload/netrw.vim")
if v:version < 700
 echohl WarningMsg | echo "***netrw*** you need vim version 7.0 or later for version ".g:loaded_netrw." of netrw" | echohl None
 finish
endif

" ---------------------------------------------------------------------
" Default values for global netrw variables {{{1
if !exists("g:netrw_ftpmode")
 let g:netrw_ftpmode= "binary"
endif
if !exists("g:netrw_win95ftp")
 let g:netrw_win95ftp= 1
endif
if !exists("g:netrw_cygwin")
 if has("win32") || has("win95") || has("win64") || has("win16")
  if &shell == "bash"
   let g:netrw_cygwin= 1
  else
   let g:netrw_cygwin= 0
  endif
 else
  let g:netrw_cygwin= 0
 endif
endif
if !exists("g:netrw_list_cmd")
 if executable("ssh")
  " provide a default listing command
  let g:netrw_list_cmd= "ssh HOSTNAME ls -FLa"
 else
"  call Decho("ssh is not executable, can't do remote directory exploring with ssh")
  let g:netrw_list_cmd= ""
 endif
endif
if !exists("g:netrw_ftp_list_cmd")
 if has("unix") || exists("g:netrw_cygwin")
  let g:netrw_ftp_list_cmd= "ls -lF"
 else
  let g:netrw_ftp_list_cmd= "dir"
 endif
endif
if !exists("g:netrw_rm_cmd")
 let g:netrw_rm_cmd    = "ssh HOSTNAME rm"
endif
if !exists("g:netrw_rmf_cmd")
 let g:netrw_rmf_cmd    = "ssh HOSTNAME rm -f"
endif
if !exists("g:netrw_rmdir_cmd")
 let g:netrw_rmdir_cmd = "ssh HOSTNAME rmdir"
endif
if !exists("g:netrw_rename_cmd")
 let g:netrw_rename_cmd= "ssh HOSTNAME mv"
endif
if exists("g:netrw_silent") && g:netrw_silent != 0
 let g:netrw_silentxfer= "silent "
else
 let g:netrw_silentxfer= ""
endif
if !exists("g:netrw_winsize")
 let g:netrw_winsize= ""
endif
if !exists("g:netrw_list_hide")
 let g:netrw_list_hide= ""
endif
if !exists("g:netrw_sort_by")
 " alternatives: date size
 let g:netrw_sort_by= "name"
endif
if !exists("g:netrw_sort_sequence")
 let g:netrw_sort_sequence= '[\/]$,*,\.bak$,\.o$,\.h$,\.info$,\.swp$,\.obj$'
endif
if !exists("g:netrw_sort_direction")
 " alternative: reverse  (z y x ...)
 let g:netrw_sort_direction= "normal"
endif
if !exists("g:netrw_longlist")
 let g:netrw_longlist= 0
endif
if g:netrw_longlist == 0 || g:netrw_longlist == 2
 let g:netrw_list_cmd= "ssh HOSTNAME ls -FLa"
else
 let g:netrw_longlist= 1
 let g:netrw_list_cmd= "ssh HOSTNAME ls -FLa -l"
endif
if !exists("g:netrw_list_cmd")
endif
if !exists("g:netrw_timefmt")
 let g:netrw_timefmt= "%c"
endif
if !exists("g:netrw_local_rmdir")
 let g:netrw_local_rmdir= "rmdir"
endif
if !exists("g:netrw_local_mkdir")
 let g:netrw_local_mkdir= "mkdir"
endif
if !exists("g:netrw_mkdir_cmd")
 let g:netrw_mkdir_cmd= "ssh HOSTNAME mkdir"
endif
if !exists("g:netrw_hide")
 let g:netrw_hide= 1
endif
if !exists("g:netrw_ftp_browse_reject")
 let g:netrw_ftp_browse_reject='^total\s\+\d\+$\|^Trying\s\+\d\+.*$\|^KERBEROS_V\d rejected\|^Security extensions not\|No such file\|: connect to address [0-9a-fA-F:]*: No route to host$'
endif
if !exists("g:netrw_ssh_browse_reject")
  let g:netrw_ssh_browse_reject='^total\s\+\d\+$'
endif
if !exists("g:netrw_keepdir")
 let g:netrw_keepdir= 1
endif
if !exists("s:netrw_cd_escape")
 if has("win32") || has("win95") || has("win64") || has("win16")
  let s:netrw_cd_escape="#% "
 else
  let s:netrw_cd_escape="[]#*$%'\" ?`!&();<>\\"
 endif
endif
if !exists("s:netrw_glob_escape")
 if has("win32") || has("win95") || has("win64") || has("win16")
  let s:netrw_glob_escape= ""
 else
  let s:netrw_glob_escape= '[]*?`{~$'
 endif
endif
if !exists("g:netrw_alto")
 let g:netrw_alto= 0
endif
if !exists("g:netrw_altv")
 let g:netrw_altv= 0
endif
if !exists("g:netrw_maxfilenamelen")
 let g:netrw_maxfilenamelen= 32
endif
if !exists("g:netrw_dirhistmax")
 let g:netrw_dirhistmax= 10
endif
if !exists("g:NETRW_DIRHIST_CNT")
 let g:NETRW_DIRHIST_CNT= 0
endif

" BufEnter event ignored by decho when following variable is true
"  Has a side effect that doau BufReadPost doesn't work, so
"  files read by network transfer aren't appropriately highlighted.
"let g:decho_bufenter = 1	"Decho

" ---------------------------------------------------------------------
" Default values for global protocol variables {{{1
if !exists("g:netrw_rcp_cmd")
  let g:netrw_rcp_cmd	= "rcp"
endif
if !exists("g:netrw_ftp_cmd")
  let g:netrw_ftp_cmd	= "ftp"
endif
if !exists("g:netrw_scp_cmd")
  let g:netrw_scp_cmd	= "scp -q"
endif
if !exists("g:netrw_sftp_cmd")
  let g:netrw_sftp_cmd	= "sftp"
endif
if !exists("g:netrw_http_cmd")
 if executable("wget")
  let g:netrw_http_cmd	= "wget -q -O"
 elseif executable("fetch")
  let g:netrw_http_cmd	= "fetch -o"
 else
  let g:netrw_http_cmd	= ""
 endif
endif
if !exists("g:netrw_dav_cmd")
  let g:netrw_dav_cmd	= "cadaver"
endif
if !exists("g:netrw_rsync_cmd")
  let g:netrw_rsync_cmd	= "rsync"
endif
if !exists("g:netrw_fetch_cmd")
 if executable("fetch")
  let g:netrw_fetch_cmd	= "fetch -o"
 else
  let g:netrw_fetch_cmd	= ""
 endif
endif

if has("win32") || has("win95") || has("win64") || has("win16")
  \ && exists("g:netrw_use_nt_rcp")
  \ && g:netrw_use_nt_rcp
  \ && executable( $SystemRoot .'/system32/rcp.exe')
 let s:netrw_has_nt_rcp = 1
 let s:netrw_rcpmode    = '-b'
 else
 let s:netrw_has_nt_rcp = 0
 let s:netrw_rcpmode    = ''
endif

" ------------------------------------------------------------------------
" NetSavePosn: saves position of cursor on screen {{{1
fun! netrw#NetSavePosn()
"  call Dfunc("NetSavePosn()")
  " Save current line and column
  let w:netrw_winnr= winnr()
  let w:netrw_line = line(".")
  let w:netrw_col  = virtcol(".")

  " Save top-of-screen line
  norm! H0
  let w:netrw_hline= line(".")

  call netrw#NetRestorePosn()
"  call Dret("NetSavePosn : winnr=".w:netrw_winnr." line=".w:netrw_line." col=".w:netrw_col." hline=".w:netrw_hline)
endfun

" ------------------------------------------------------------------------
" NetRestorePosn: restores the cursor and file position as saved by NetSavePosn() {{{1
fun! netrw#NetRestorePosn()
"  call Dfunc("NetRestorePosn() winnr=".w:netrw_winnr." line=".w:netrw_line." col=".w:netrw_col." hline=".w:netrw_hline)
  let eikeep= &ei
  set ei=all

  " restore window
"  call Decho("restore window: exe silent! ".w:netrw_winnr."wincmd w")
  exe "silent! ".w:netrw_winnr."wincmd w"
"  if v:shell_error == 0
"   " as suggested by Bram M: redraw on no error
"   " allows protocol error messages to remain visible
"   redraw!
"  endif

  " restore top-of-screen line
"  call Decho("restore topofscreen: exe norm! ".w:netrw_hline."G0z")
  exe "norm! ".w:netrw_hline."G0z\<CR>"

  " restore position
"  call Decho("restore posn: exe norm! ".w:netrw_line."G0".w:netrw_col."|")
  exe "norm! ".w:netrw_line."G0".w:netrw_col."\<bar>"

  let &ei= eikeep
"  call Dret("NetRestorePosn")
endfun

" ------------------------------------------------------------------------
" NetRead: responsible for reading a file over the net {{{1
fun! netrw#NetRead(...)
"  call Dfunc("NetRead(a:1<".a:1.">)")
 
  " save options
  call s:NetOptionSave()
 
  " Special Exception: if a file is named "0r", then
  "		      "0r" will be used to read the
  "		      following files instead of "r"
  if	a:0 == 0
   let readcmd= "r"
   let ichoice= 0
  elseif a:1 == "0r"
   let readcmd = "0r"
   let ichoice = 2
  else
   let readcmd = "r"
   let ichoice = 1
  endif
 
  " get name of a temporary file and set up shell-quoting character
  let tmpfile= tempname()
  let tmpfile= substitute(tmpfile,'\','/','ge')
  if !isdirectory(substitute(tmpfile,'[^/]\+$','','e'))
   echohl Error | echo "***netrw*** your <".substitute(tmpfile,'[^/]\+$','','e')."> directory is missing!"
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"   call Dret("NetRead")
   return
  endif
 
"  call Decho("ichoice=".ichoice." readcmd<".readcmd.">")
  while ichoice <= a:0
 
   " attempt to repeat with previous host-file-etc
   if exists("b:netrw_lastfile") && a:0 == 0
"    call Decho("using b:netrw_lastfile<" . b:netrw_lastfile . ">")
    let choice = b:netrw_lastfile
    let ichoice= ichoice + 1
 
   else
    exe "let choice= a:" . ichoice
"    call Decho("no lastfile: choice<" . choice . ">")
 
    if match(choice,"?") == 0
     " give help
     echomsg 'NetRead Usage:'
     echomsg ':Nread machine:path                         uses rcp'
     echomsg ':Nread "machine path"                       uses ftp   with <.netrc>'
     echomsg ':Nread "machine id password path"           uses ftp'
     echomsg ':Nread dav://machine[:port]/path            uses cadaver'
     echomsg ':Nread fetch://machine/path                 uses fetch'
     echomsg ':Nread ftp://[user@]machine[:port]/path     uses ftp   autodetects <.netrc>'
     echomsg ':Nread http://[user@]machine/path           uses http  wget'
     echomsg ':Nread rcp://[user@]machine/path            uses rcp'
     echomsg ':Nread rsync://machine[:port]/path          uses rsync'
     echomsg ':Nread scp://[user@]machine[[:#]port]/path  uses scp'
     echomsg ':Nread sftp://[user@]machine[[:#]port]/path uses sftp'
     break

    elseif match(choice,"^\"") != -1
     " Reconstruct Choice if choice starts with '"'
"     call Decho("reconstructing choice")
     if match(choice,"\"$") != -1
      " case "..."
      let choice=strpart(choice,1,strlen(choice)-2)
     else
       "  case "... ... ..."
      let choice      = strpart(choice,1,strlen(choice)-1)
      let wholechoice = ""
 
      while match(choice,"\"$") == -1
       let wholechoice = wholechoice . " " . choice
       let ichoice     = ichoice + 1
       if ichoice > a:0
       	if !exists("g:netrw_quiet")
         echohl Error | echo "***netrw*** Unbalanced string in filename '". wholechoice ."'" | echohl None
         call inputsave()|call input("Press <cr> to continue")|call inputrestore()
	endif
"        call Dret("NetRead")
        return
       endif
       let choice= a:{ichoice}
      endwhile
      let choice= strpart(wholechoice,1,strlen(wholechoice)-1) . " " . strpart(choice,0,strlen(choice)-1)
     endif
    endif
   endif

"   call Decho("choice<" . choice . ">")
   let ichoice= ichoice + 1
 
   " fix up windows urls
   if has("win32") || has("win95") || has("win64") || has("win16")
    let choice = substitute(choice,'\\','/','ge')
"    call Decho("fixing up windows url to <".choice."> tmpfile<".tmpfile)

    exe 'lcd ' . fnamemodify(tmpfile,':h')
    let tmpfile = fnamemodify(tmpfile,':t')
   endif
 
   " Determine method of read (ftp, rcp, etc)
   call s:NetMethod(choice)
 
   " Check if NetBrowse() should be handling this request
"   call Decho("checking if netlist: choice<".choice."> netrw_list_cmd<".g:netrw_list_cmd.">")
   if choice =~ "^.*[\/]$"
    keepjumps call s:NetBrowse(choice)
"    call Dret("NetRead")
    return
   endif
 
   " ============
   " Perform Read
   " ============
 
   ".........................................
   " rcp:  NetRead Method #1
   if  b:netrw_method == 1 " read with rcp
"    call Decho("read via rcp (method #1)")
   " ER: noting done with g:netrw_uid yet?
   " ER: on Win2K" rcp machine[.user]:file tmpfile
   " ER: if machine contains '.' adding .user is required (use $USERNAME)
   " ER: the tmpfile is full path: rcp sees C:\... as host C
   if s:netrw_has_nt_rcp == 1
    if exists("g:netrw_uid") &&	( g:netrw_uid != "" )
     let uid_machine = g:netrw_machine .'.'. g:netrw_uid
    else
     " Any way needed it machine contains a '.'
     let uid_machine = g:netrw_machine .'.'. $USERNAME
    endif
   else
    if exists("g:netrw_uid") &&	( g:netrw_uid != "" )
     let uid_machine = g:netrw_uid .'@'. g:netrw_machine
    else
     let uid_machine = g:netrw_machine
    endif
   endif
"   call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".uid_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
   exe g:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".uid_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile
   let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
   let b:netrw_lastfile = choice
 
   ".........................................
   " ftp + <.netrc>:  NetRead Method #2
   elseif b:netrw_method  == 2		" read with ftp + <.netrc>
"    call Decho("read via ftp+.netrc (method #2)")
     let netrw_fname= b:netrw_fname
     new
     setlocal ff=unix
     exe "put ='".g:netrw_ftpmode."'"
     exe "put ='"."get ".netrw_fname." ".tmpfile."'"
     if exists("g:netrw_port") && g:netrw_port != ""
"      call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
      exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
     else
"      call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
      exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i ".g:netrw_machine
     endif
     " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
     if getline(1) !~ "^$" && !exists("g:netrw_quiet") && getline(1) !~ '^Trying '
      let debugkeep= &debug
      set debug=msg
      echohl Error | echo "***netrw*** ".getline(1) | echohl None
      call inputsave()|call input("Press <cr> to continue")|call inputrestore()
      let &debug= debugkeep
     endif
     bd!
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
     let b:netrw_lastfile = choice
 
   ".........................................
   " ftp + machine,id,passwd,filename:  NetRead Method #3
   elseif b:netrw_method == 3		" read with ftp + machine, id, passwd, and fname
    " Construct execution string (four lines) which will be passed through filter
"    call Decho("read via ftp+mipf (method #3)")
    let netrw_fname= b:netrw_fname
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     put ='open '.g:netrw_machine
    endif
 
    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     put =g:netrw_uid
     put =g:netrw_passwd
    else
     put ='user '.g:netrw_uid.' '.g:netrw_passwd
    endif
 
    if exists("g:netrw_ftpmode") && g:netrw_ftpmode != ""
     put =g:netrw_ftpmode
    endif
    put ='get '.netrw_fname.' '.tmpfile
 
    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
"    call Decho('performing ftp -i -n')
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
"     call Decho("error<".getline(1).">")
     if !exists("g:netrw_quiet")
      echohl Error | echo "***netrw*** ".getline(1) | echohl None
      call inputsave()|call input("Press <cr> to continue")|call inputrestore()
     endif
    endif
    bd!
    let result		= s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   " scp: NetRead Method #4
   elseif     b:netrw_method  == 4	" read with scp
"    call Decho("read via scp (method #4)")
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " -P ".g:netrw_port
    else
     let useport= ""
    endif
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
"     call Decho("executing: !".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
     exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile
    else
"     call Decho("executing: !".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
     exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile
    endif
    let result		= s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   elseif     b:netrw_method  == 5	" read with http (wget)
"    call Decho("read via http (method #5)")
    if g:netrw_http_cmd == ""
     if !exists("g:netrw_quiet")
      echohl Error | echo "***netrw*** neither wget nor fetch command is available" | echohl None
      call inputsave()|call input("Press <cr> to continue")|call inputrestore()
     endif
     exit
    endif
 
    if match(b:netrw_fname,"#") == -1
     " simple wget
"     call Decho("executing: !".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.escape(b:netrw_fname,' ?&')
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
 
    else
     " wget plus a jump to an in-page marker (ie. http://abc/def.html#aMarker)
     let netrw_html= substitute(b:netrw_fname,"#.*$","","")
     let netrw_tag = substitute(b:netrw_fname,"^.*#","","")
"     call Decho("netrw_html<".netrw_html.">")
"     call Decho("netrw_tag <".netrw_tag.">")
"     call Decho("executing: !".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_html)
     exe g:netrw_silentxfer."!".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_html
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
"     call Decho('<\s*a\s*name=\s*"'.netrw_tag.'"/')
     exe 'norm! 1G/<\s*a\s*name=\s*"'.netrw_tag.'"/'."\<CR>"
    endif
    let b:netrw_lastfile = choice
 
   ".........................................
   " cadaver: NetRead Method #6
   elseif     b:netrw_method  == 6	" read with cadaver
"    call Decho("read via cadaver (method #6)")
 
    " Construct execution string (four lines) which will be passed through filter
    let netrw_fname= b:netrw_fname
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     put ='open '.g:netrw_machine
    endif
    put ='user '.g:netrw_uid.' '.g:netrw_passwd
 
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
     put ='get '.netrw_fname.' '.cygtmpfile
    else
     put ='get '.netrw_fname.' '.tmpfile
    endif
    put ='quit'
 
    " perform cadaver operation:
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_dav_cmd)
    exe g:netrw_silentxfer."%!".g:netrw_dav_cmd
    bd!
    let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   " rsync: NetRead Method #7
   elseif     b:netrw_method  == 7	" read with rsync
"    call Decho("read via rsync (method #7)")
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
"     call Decho("executing: !".g:netrw_rsync_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
     exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile
    else
"     call Decho("executing: !".g:netrw_rsync_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
     exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile
    endif
    let result		= s:NetGetFile(readcmd,tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   " fetch: NetRead Method #8
   "    fetch://[user@]host[:http]/path
   elseif     b:netrw_method  == 8	" read with fetch
    if g:netrw_fetch_cmd == ""
     if !exists("g:netrw_quiet")
      echohl Error | echo "***netrw*** fetch command not available" | echohl None
      call inputsave()|call input("Press <cr> to continue")|call inputrestore()
     endif
     exit
    endif
    if exists("g:netrw_option") && g:netrw_option == ":http"
     let netrw_option= "http"
    else
     let netrw_option= "ftp"
    endif
"    call Decho("read via fetch for ".netrw_option)
 
    if exists("g:netrw_uid") && g:netrw_uid != "" && exists("g:netrw_passwd") && g:netrw_passwd != ""
"     call Decho("executing: !".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_uid.':'.g:netrw_passwd.'@'.g:netrw_machine."/".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_uid.':'.g:netrw_passwd.'@'.g:netrw_machine."/".escape(b:netrw_fname,' ?&')
    else
"     call Decho("executing: !".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_machine."/".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_machine."/".escape(b:netrw_fname,' ?&')
    endif
 
    let result		= s:NetGetFile(readcmd,tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   " sftp: NetRead Method #9
   elseif     b:netrw_method  == 9	" read with sftp
"    call Decho("read via sftp (method #4)")
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
"     call Decho("!".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
"     call Decho("executing: !".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
     exe "!".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile
    else
"     call Decho("executing: !".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
     exe g:netrw_silentxfer."!".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile
    endif
    let result		= s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   else " Complain
    echo "***warning*** unable to comply with your request<" . choice . ">"
   endif
  endwhile
 
  " cleanup
"  call Decho("cleanup")
  if exists("b:netrw_method")
   unlet b:netrw_method
   unlet b:netrw_fname
  endif
  call s:NetOptionRestore()

"  call Dret("NetRead")
endfun

" ------------------------------------------------------------------------
" NetGetFile: Function to read file "fname" with command "readcmd". {{{1
fun! s:NetGetFile(readcmd, fname, method)
"   call Dfunc("NetGetFile(readcmd<".a:readcmd.">,fname<".a:fname."> method<".a:method.">)")
 
  if exists("*NetReadFixup")
   " for the use of NetReadFixup (not otherwise used internally)
   let line2= line("$")
  endif
 
  " transform paths from / to \ for Windows (except for cygwin)
  if &term == "win32"
   if g:netrw_cygwin
    let fname= a:fname
"    call Decho("(win32 && cygwin) fname<".fname.">")
   else
    let fname= substitute(a:fname,'/','\\\\','ge')
"    call Decho("(win32 && !cygwin) fname<".fname.">")
   endif
  else
   let fname= a:fname
"   call Decho("(copied) fname<".fname.">")
  endif
 
  if a:readcmd[0] == '0'
  " get file into buffer

   " record remote filename
   let rfile= bufname(".")
"   call Decho("remotefile<".rfile.">")
"   call Dredir("ls!","starting buffer list")

   " rename the current buffer to the temp file (ie. fname)
   keepalt exe "file ".fname
"   call Dredir("ls!","after renaming current buffer to <".fname.">")

   " edit temporary file
   e
"   call Dredir("ls!","after editing temporary file")

   " rename buffer back to remote filename
   keepalt exe "file ".rfile
"   call Dredir("ls!","renaming buffer back to remote filename<".rfile.">")
   let line1 = 1
   let line2 = line("$")

  elseif filereadable(fname)
   " read file after current line
   let curline = line(".")
   let lastline= line("$")
"   call Decho("exe<".a:readcmd." ".v:cmdarg." ".fname.">  line#".curline)
   exe a:readcmd." ".v:cmdarg." ".fname
   let line1        = curline + 1
   let line2        = line("$") - lastline + 1
  else
   " not readable
"   call Dret("NetGetFile : fname<".fname."> not readable")
   return
  endif
 
  " User-provided (ie. optional) fix-it-up command
  if exists("*NetReadFixup")
"   call Decho("calling NetReadFixup(method<".a:method."> line1=".line1." line2=".line2.")")
   call NetReadFixup(a:method, line1, line2)
" else " Decho
"  call Decho("NetReadFixup() not called, doesn't exist")
  endif

  " update the Buffers menu
  if has("gui") && has("gui_running")
   silent! emenu Buffers.Refresh\ menu
  endif
 
"  call Decho("readcmd<".a:readcmd."> cmdarg<".v:cmdarg."> fname<".a:fname."> readable=".filereadable(a:fname))
 
 " insure that we have the right filetype and that its being displayed
  filetype detect
  redraw!
"  call Dret("NetGetFile")
endfun

" ------------------------------------------------------------------------
" NetWrite: responsible for writing a file over the net {{{1
fun! netrw#NetWrite(...) range
"  call Dfunc("NetWrite(a:0=".a:0.")")
 
  " option handling
  let mod= 0
  call s:NetOptionSave()
 
  " Get Temporary Filename
  let tmpfile= tempname()
  if !isdirectory(substitute(tmpfile,'[^/]\+$','','e'))
   echohl Error | echo "***netrw*** your ".substitute(tmpfile,'[^/]\+$','','e')." directory is missing!"
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"   call Dret("NetRead")
   return
  endif
 
  if a:0 == 0
   let ichoice = 0
  else
   let ichoice = 1
  endif
 
  " write (selected portion of) file to temporary
  silent exe a:firstline."," . a:lastline . "w! ".v:cmdarg." ".tmpfile
 
  while ichoice <= a:0
 
   " attempt to repeat with previous host-file-etc
   if exists("b:netrw_lastfile") && a:0 == 0
"    call Decho("using b:netrw_lastfile<" . b:netrw_lastfile . ">")
    let choice = b:netrw_lastfile
    let ichoice= ichoice + 1
   else
    exe "let choice= a:" . ichoice
 
    " Reconstruct Choice if choice starts with '"'
    if match(choice,"?") == 0
     echomsg 'NetWrite Usage:"'
     echomsg ':Nwrite machine:path                        uses rcp'
     echomsg ':Nwrite "machine path"                      uses ftp with <.netrc>'
     echomsg ':Nwrite "machine id password path"          uses ftp'
     echomsg ':Nwrite dav://[user@]machine/path           uses cadaver'
     echomsg ':Nwrite fetch://[user@]machine/path         uses fetch'
     echomsg ':Nwrite ftp://machine[#port]/path           uses ftp  (autodetects <.netrc>)'
     echomsg ':Nwrite rcp://machine/path                  uses rcp'
     echomsg ':Nwrite rsync://[user@]machine/path         uses rsync'
     echomsg ':Nwrite scp://[user@]machine[[:#]port]/path uses scp'
     echomsg ':Nwrite sftp://[user@]machine/path          uses sftp'
     break
 
    elseif match(choice,"^\"") != -1
     if match(choice,"\"$") != -1
       " case "..."
      let choice=strpart(choice,1,strlen(choice)-2)
     else
      "  case "... ... ..."
      let choice      = strpart(choice,1,strlen(choice)-1)
      let wholechoice = ""
 
      while match(choice,"\"$") == -1
       let wholechoice= wholechoice . " " . choice
       let ichoice    = ichoice + 1
       if choice > a:0
       	if !exists("g:netrw_quiet")
         echohl Error | echo "***netrw*** Unbalanced string in filename '". wholechoice ."'" | echohl None
         call inputsave()|call input("Press <cr> to continue")|call inputrestore()
	endif
"        call Dret("NetWrite")
        return
       endif
       let choice= a:{ichoice}
      endwhile
      let choice= strpart(wholechoice,1,strlen(wholechoice)-1) . " " . strpart(choice,0,strlen(choice)-1)
     endif
    endif
   endif
"   call Decho("choice<" . choice . ">")
   let ichoice= ichoice + 1
 
   " fix up windows urls
   if has("win32") || has("win95") || has("win64") || has("win16")
    let choice= substitute(choice,'\\','/','ge')
    "ER: see NetRead()
    exe 'lcd ' . fnamemodify(tmpfile,':h')
    let tmpfile = fnamemodify(tmpfile,':t')
   endif
 
   " Determine method of read (ftp, rcp, etc)
   call s:NetMethod(choice)
 
   " =============
   " Perform Write
   " =============
 
   ".........................................
   " rcp: NetWrite Method #1
   if  b:netrw_method == 1	" write with rcp
" Decho "write via rcp (method #1)"
    if s:netrw_has_nt_rcp == 1
     if exists("g:netrw_uid") &&  ( g:netrw_uid != "" )
      let uid_machine = g:netrw_machine .'.'. g:netrw_uid
     else
      let uid_machine = g:netrw_machine .'.'. $USERNAME
     endif
    else
     if exists("g:netrw_uid") &&  ( g:netrw_uid != "" )
      let uid_machine = g:netrw_uid .'@'. g:netrw_machine
     else
      let uid_machine = g:netrw_machine
     endif
    endif
"    call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".tmpfile." ".uid_machine.":".escape(b:netrw_fname,' ?&'))
    exe g:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".tmpfile." ".uid_machine.":".escape(b:netrw_fname,' ?&')
    let b:netrw_lastfile = choice
 
   ".........................................
   " ftp + <.netrc>: NetWrite Method #2
   elseif b:netrw_method == 2	" write with ftp + <.netrc>
    let netrw_fname = b:netrw_fname
    new
    setlocal ff=unix
    exe "put ='".g:netrw_ftpmode."'"
"    call Decho(" NetWrite: put ='".g:netrw_ftpmode."'")
    exe "put ='"."put ".tmpfile." ".netrw_fname."'"
"    call Decho("put ='"."put ".tmpfile." ".netrw_fname."'")
    if exists("g:netrw_port") && g:netrw_port != ""
"     call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
     exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
    else
"     call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
     exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i ".g:netrw_machine
    endif
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
     if !exists("g:netrw_quiet")
      echohl Error | echo "***netrw*** ".getline(1) | echohl None
      call inputsave()|call input("Press <cr> to continue")|call inputrestore()
     endif
     let mod=1
    endif
    bd!
    let b:netrw_lastfile = choice
 
   ".........................................
   " ftp + machine, id, passwd, filename: NetWrite Method #3
   elseif b:netrw_method == 3	" write with ftp + machine, id, passwd, and fname
    let netrw_fname= b:netrw_fname
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     put ='open '.g:netrw_machine
    endif
    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     put =g:netrw_uid
     put =g:netrw_passwd
    else
     put ='user '.g:netrw_uid.' '.g:netrw_passwd
    endif
    put ='put '.tmpfile.' '.netrw_fname
    " save choice/id/password for future use
    let b:netrw_lastfile = choice
 
    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
"    call Decho('performing ftp -i -n')
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
     if  !exists("g:netrw_quiet")
      echohl Error | echo "***netrw*** ".getline(1) | echohl None
      call inputsave()|call input("Press <cr> to continue")|call inputrestore()
     endif
     let mod=1
    endif
    bd!
 
   ".........................................
   " scp: NetWrite Method #4
   elseif     b:netrw_method == 4	" write with scp
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " -P ".g:netrw_port
    else
     let useport= ""
    endif
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
"     call Decho("executing: !".g:netrw_scp_cmd.useport." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')
    else
"     call Decho("executing: !".g:netrw_scp_cmd.useport." ".tmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".tmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')
    endif
    let b:netrw_lastfile = choice
 
   ".........................................
   " http: NetWrite Method #5
   elseif     b:netrw_method == 5
    if !exists("g:netrw_quiet")
     echohl Error | echo "***netrw*** currently <netrw.vim> does not support writing using http:" | echohl None
     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
    endif
 
   ".........................................
   " dav: NetWrite Method #6
   elseif     b:netrw_method == 6	" write with cadaver
"    call Decho("write via cadaver (method #6)")
 
    " Construct execution string (four lines) which will be passed through filter
    let netrw_fname= b:netrw_fname
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     put ='open '.g:netrw_machine
    endif
    put ='user '.g:netrw_uid.' '.g:netrw_passwd
 
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
     put ='put '.cygtmpfile.' '.netrw_fname
    else
     put ='put '.tmpfile.' '.netrw_fname
    endif
 
    " perform cadaver operation:
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_dav_cmd)
    exe g:netrw_silentxfer."%!".g:netrw_dav_cmd
    bd!
    let b:netrw_lastfile = choice
 
   ".........................................
   " rsync: NetWrite Method #7
   elseif     b:netrw_method == 7	" write with rsync
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
"     call Decho("executing: !".g:netrw_rsync_cmd." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')
    else
"     call Decho("executing: !".g:netrw_rsync_cmd." ".tmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".tmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')
    endif
    let b:netrw_lastfile = choice
 
   ".........................................
   " scp: NetWrite Method #9
   elseif     b:netrw_method == 9	" write with sftp
    let netrw_fname= b:netrw_fname
    if exists("g:netrw_uid") &&  ( g:netrw_uid != "" )
     let uid_machine = g:netrw_uid .'@'. g:netrw_machine
    else
     let uid_machine = g:netrw_machine
    endif
    new
    setlocal ff=unix
    put ='put '.tmpfile.' '.netrw_fname
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_sftp_cmd.' '.uid_machine)
    exe g:netrw_silentxfer."%!".g:netrw_sftp_cmd.' '.uid_machine
    bd!
    let b:netrw_lastfile= choice
 
   ".........................................
   else " Complain
    echo "***warning*** unable to comply with your request<" . choice . ">"
   endif
  endwhile
 
  " cleanup
"  call Decho("cleanup")
  let result=delete(tmpfile)
  call s:NetOptionRestore()
 
  if a:firstline == 1 && a:lastline == line("$")
   let &mod= mod	" usually equivalent to set nomod
  endif
 
"  call Dret("NetWrite")
endfun

" ===========================================
"  Remote Directory Browsing Support:    {{{1
" ===========================================

" NetBrowse: This function uses the command in g:netrw_list_cmd to get a list {{{2
"  of the contents of a remote directory.  It is assumed that the
"  g:netrw_list_cmd has a string, HOSTNAME, that needs to be substituted
"  with the requested remote hostname first.
fun! s:NetBrowse(dirname)
  if !exists("w:netrw_longlist")|let w:netrw_longlist= g:netrw_longlist|endif
"  call Dfunc("NetBrowse(dirname<".a:dirname.">) longlist=".w:netrw_longlist)

  if exists("s:netrw_skipbrowse")
   unlet s:netrw_skipbrowse
"   call Dret("NetBrowse")
   return
  endif

  call s:NetOptionSave()

  " sanity check
  if exists("b:netrw_method") && b:netrw_method =~ '[235]'
"   call Decho("b:netrw_method=".b:netrw_method)
   if !executable("ftp")
    if !exists("g:netrw_quiet")
     echohl Error | echo "***netrw*** this system doesn't support remote directory listing via ftp" | echohl None
     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
    endif
    call s:NetOptionRestore()
"    call Dret("NetBrowse")
    return
   endif
  elseif !exists("g:netrw_list_cmd") || g:netrw_list_cmd == ''
   if !exists("g:netrw_quiet")
    echohl Error | echo "***netrw*** this system doesn't support remote directory listing via ssh" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif

    call s:NetOptionRestore()
"   call Dret("NetBrowse")
   return
  endif

  " use buffer-oriented WinVars if buffer ones exist but window ones don't
  call s:UseBufWinVars()

  " make this buffer modifiable
  setlocal ma nonu nowrap

  " analyze a:dirname and g:netrw_list_cmd
  let dirpat  = '^\(\w\{-}\)://\(\w\+@\)\=\([^/]\+\)/\(.*\)$'
  let dirname = substitute(a:dirname,'\\','/','ge')
"  call Decho("dirpat<".dirpat.">")
  if dirname !~ dirpat
   if !exists("g:netrw_quiet")
    echohl Error | echo "***netrw*** netrw doesn't understand your dirname<".dirname.">" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
    call s:NetOptionRestore()
"   call Dret("NetBrowse : badly formatted dirname<".dirname.">")
   return
  endif

  let method  = substitute(dirname,dirpat,'\1','')
  let user    = substitute(dirname,dirpat,'\2','')
  let machine = substitute(dirname,dirpat,'\3','')
  let path    = substitute(dirname,dirpat,'\4','')
  let fname   = substitute(dirname,'^.*/\ze.','','')
"  call Decho("set up method <".method .">")
"  call Decho("set up user   <".user   .">")
"  call Decho("set up machine<".machine.">")
"  call Decho("set up path   <".path   .">")
"  call Decho("set up fname  <".fname  .">")

  if method == "ftp" || method == "http"
   let method  = "ftp"
   let listcmd = g:netrw_ftp_list_cmd
  else
   let listcmd = substitute(g:netrw_list_cmd,'\<HOSTNAME\>',user.machine,'')
  endif

  if exists("b:netrw_method")
"   call Decho("setting w:netrw_method<".b:netrw_method.">")
   let w:netrw_method= b:netrw_method
  endif

  " optionally sort by time (-t) or by size (-S)
  if listcmd == "dir" && g:netrw_sort_by =~ "^[ts]"
   echohl WarningMsg | echo "***netrw*** windows' ftp doesn't support time/size sorts (get cygwin, set g:netrw_cygwin)" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
  else
   if g:netrw_sort_by =~ "^t"
    let listcmd= listcmd."t"
   elseif g:netrw_sort_by =~ "^s"
    let listcmd= listcmd."S"
   endif
   " optionally sort in reverse
   if g:netrw_sort_direction =~ "^r" && listcmd == "dir"
    let listcmd= listcmd."r"
   endif
  endif

"  call Decho("set up listcmd<".listcmd.">")
  if fname =~ '@$' && fname !~ '^"'
"   call Decho("attempt transfer of symlink as file")
   call s:NetBrowse(substitute(dirname,'@$','','e'))
   redraw!
   call s:NetOptionRestore()
"   call Dret("NetBrowse : symlink")
   return

  elseif fname !~ '[\/]$' && fname !~ '^"'
   " looks like a regular file, attempt transfer
"   call Decho("attempt transfer as regular file<".dirname.">")

   " remove any filetype indicator from end of dirname, except for the
   " "this is a directory" indicator (/).  There shouldn't be one of those,
   " anyway.
   let path= substitute(path,'[*=@|]\r\=$','','e')
"   call Decho("new path<".path.">")

   " remote-read the requested file into current buffer
   enew!
   set ma
"   call Decho("exe file .method."://".user.machine."/".escape(path,s:netrw_cd_escape))
   exe "file ".method."://".user.machine."/".escape(path,s:netrw_cd_escape)
   exe "silent doau BufReadPre ".fname
   silent call netrw#NetRead(method."://".user.machine."/".path)
   exe "silent doau BufReadPost ".fname
   keepjumps 1d

   " save certain window-oriented variables into buffer-oriented variables
   call s:BufWinVars()
   call s:NetOptionRestore()
   setlocal nomod

"   call Dret("NetBrowse : file<".fname.">")
   return
  endif

  " ---------------------------------------------------------------------
  "  Perform Directory Listing:
"  call Decho("Perform directory listing...")
  " set up new buffer and map
  let bufname   = method.'://'.user.machine.'/'.path
  let bufnamenr = bufnr(bufname.'$')
"  call Decho("bufname<".bufname."> bufnamenr=".bufnamenr)
  if bufnamenr != -1
   " buffer already exists, switch to it!
"   call Decho("buffer already exists, switching to it")
   exe "b ".bufnamenr
   if line("$") >= 5
    call s:NetOptionRestore()
"    call Dret("NetBrowse")
    return
   endif
  else
"   call Decho("generate a new buffer")
   enew!
  endif

  " rename file to reflect where its from
  setlocal bt=nofile bh=wipe nobl noswf
  exe "setlocal ts=".g:netrw_maxfilenamelen
"  call Decho("exe file ".escape(bufname,s:netrw_cd_escape))
  exe 'file '.escape(bufname,s:netrw_cd_escape)
"  call Decho("renaming file to bufname<".bufname.">")
  setlocal bh=hide bt=nofile nobl nonu

  " save current directory on directory history list
  call <SID>NetBookmarkDir(3,expand("%"))

  " set up buffer-local mappings
"  call Decho("set up buffer-local mappings")
  nnoremap <buffer> <silent> <cr>	:call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> <c-l>	:call <SID>NetRefresh(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'../'))<cr>
  nnoremap <buffer> <silent> a		:let g:netrw_hide=(g:netrw_hide+1)%3<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  if w:netrw_longlist != 2
   nnoremap <buffer> <silent> b		:<c-u>call <SID>NetBookmarkDir(0,expand("%"))<cr>
   nnoremap <buffer> <silent> B		:<c-u>call <SID>NetBookmarkDir(1,expand("%"))<cr>
  endif
  nnoremap <buffer> <silent> Nb		:<c-u>call <SID>NetBookmarkDir(0,expand("%"))<cr>
  nnoremap <buffer> <silent> NB		:<c-u>call <SID>NetBookmarkDir(0,expand("%"))<cr>
  nnoremap <buffer> <silent> <c-h>	:call <SID>NetHideEdit(0)<cr>
  nnoremap <buffer> <silent> i		:call <SID>NetLongList(0)<cr>
  nnoremap <buffer> <silent> o		:call <SID>NetSplit(0)<cr>
  nnoremap <buffer> <silent> O		:call <SID>NetObtain()<cr>
  nnoremap <buffer> <silent> q		:<c-u>call <SID>NetBookmarkDir(2,expand("%"))<cr>
  nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  nnoremap <buffer> <silent> s		:call <SID>NetSaveWordPosn()<bar>let g:netrw_sort_by= (g:netrw_sort_by =~ 'n')? 'time' : (g:netrw_sort_by =~ 't')? 'size' : 'name'<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<bar>call <SID>NetRestoreWordPosn()<cr>
  nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(0)<cr>
  nnoremap <buffer> <silent> u		:<c-u>call <SID>NetBookmarkDir(4,expand("%"))<cr>
  nnoremap <buffer> <silent> U		:<c-u>call <SID>NetBookmarkDir(5,expand("%"))<cr>
  nnoremap <buffer> <silent> v		:call <SID>NetSplit(1)<cr>
  nnoremap <buffer> <silent> x		:call <SID>NetBrowseX(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()),1)<cr>
  nnoremap <buffer> <silent> <2-leftmouse> :call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))<cr>
  exe 'nnoremap <buffer> <silent> <del>	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> <del>	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'nnoremap <buffer> <silent> d	:call <SID>NetMakeDir("'.user.machine.'")<cr>'
  exe 'nnoremap <buffer> <silent> D	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> D	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'nnoremap <buffer> <silent> R	:call <SID>NetBrowseRename("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> R	:call <SID>NetBrowseRename("'.user.machine.'","'.path.'")<cr>'
  nnoremap <buffer> ?			:he netrw-browse-cmds<cr>
  setlocal ma nonu nowrap

  " Set up the banner
"  call Decho("set up the banner: sortby<".g:netrw_sort_by."> method<".method.">")
  keepjumps put ='\" ==========================================================================='
  keepjumps put ='\" Netrw Remote Directory Listing                                 (netrw '.g:loaded_netrw.')'
  keepjumps put ='\"   '.bufname
  let w:netrw_bannercnt = 7
  let sortby            = g:netrw_sort_by
  if g:netrw_sort_direction =~ "^r"
   let sortby           = sortby." reversed"
  endif

  if g:netrw_sort_by =~ "^n"
   " sorted by name
   let w:netrw_bannercnt= w:netrw_bannercnt + 1
   keepjumps put ='\"   Sorted by      '.sortby
   keepjumps put ='\"   Sort sequence: '.g:netrw_sort_sequence
  else
   " sorted by size or date
   keepjumps put ='\"   Sorted by '.sortby
  endif
  if g:netrw_list_hide != "" && g:netrw_hide
   if g:netrw_hide == 1
    keepjumps put ='\"   Hiding:        '.g:netrw_list_hide
   else
    keepjumps put ='\"   Showing:       '.g:netrw_list_hide
   endif
   let w:netrw_bannercnt= w:netrw_bannercnt + 1
  endif
  keepjumps put ='\"   Quick Help:    ?:help  -:go up dir  D:delete  R:rename  s:sort-by  x:exec'
  keepjumps put ='\" ==========================================================================='

  " remote read the requested directory listing
  " Use ftp if that was the file-transfer method selected, otherwise use ssh
  " Note that not all ftp servers honor the options for ls
  if method == "ftp"
   " use ftp to get remote file listing
"   call Decho("use ftp to get remote file listing")
   call s:NetBrowseFtpCmd(path,listcmd)
   keepjumps 1d

   if w:netrw_longlist == 0 || w:netrw_longlist == 2
    " shorten the listing
"    call Decho("generate short listing")
    exe "keepjumps ".w:netrw_bannercnt

    " cleanup
    if g:netrw_ftp_browse_reject != ""
     exe "silent! g/".g:netrw_ftp_browse_reject."/keepjumps d"
    endif

    " if there's no ../ listed, then put ./ and ../ in
    let line1= line(".")
    keepjumps 1
    silent keepjumps call search('^\.\.\/\%(\s\|$\)','W')
    let line2= line(".")
    if line2 == 0
     keepjumps put='../'
     keepjumps put='./'
    endif
    exe "keepjumps ".line1
    keepjumps norm! 0

    " more cleanup
    exe 'keepjumps silent! '.w:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2/e'
    exe "keepjumps silent! ".w:netrw_bannercnt.',$g/ -> /s# -> .*/$#/#e'
    exe "keepjumps silent! ".w:netrw_bannercnt.',$g/ -> /s# -> .*$#/#e'
   endif

  else
   " use ssh to get remote file listing
"   call Decho("use ssh to get remote file listing")
   let shq= &shq? &shq : ( &sxq? &sxq : "'")
"   call Decho("exe silent r! ".listcmd." '".shq.escape(path,s:netrw_cd_escape).shq."'")
   exe "silent r! ".listcmd." ".shq.escape(path,s:netrw_cd_escape).shq
   keepjumps 1d
   " cleanup
   if g:netrw_ftp_browse_reject != ""
    exe "silent! g/".g:netrw_ssh_browse_reject."/keepjumps d"
   endif
  endif

  " set up syntax highlighting
  if has("syntax")
   setlocal ft=netrwlist
   if !exists("g:syntax_on") || !g:syntax_on
    setlocal ft=
    " Ugly workaround -- when syntax highlighting is off and laststatus==2,
    " sometimes the laststatus highlight bleeds into the entire display.
    " Only seems to happen with remote browsing.  Weird.
    redraw
   endif
  endif

  " manipulate the directory listing (hide, sort)
  if line("$") >= w:netrw_bannercnt
   if g:netrw_hide && g:netrw_list_hide != ""
    call s:NetrwListHide()
   endif

   if w:netrw_longlist == 1
    " do a long listing; these substitutions need to be done prior to sorting
"    call Decho("manipulate long listing")

    if method == "ftp"
     " cleanup
     exe "keepjumps ".w:netrw_bannercnt
     while getline(".") =~ g:netrw_ftp_browse_reject
      keepjumps d
     endwhile
     " if there's no ../ listed, then put ./ and ../ in
     let line1= line(".")
     keepjumps 1
     silent keepjumps call search('^\.\.\/\%(\s\|$\)','W')
     let line2= line(".")
     if line2 == 0
      exe 'keepjumps '.w:netrw_bannercnt."put='./'"
      exe 'keepjumps '.w:netrw_bannercnt."put='../'"
     endif
    exe "keepjumps ".line1
    keepjumps norm! 0
    endif

    exe 'keepjumps silent '.w:netrw_bannercnt.',$s/ -> .*$//e'
    exe 'keepjumps silent '.w:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2\t\1/e'
    exe w:netrw_bannercnt
   endif

   if line("$") >= w:netrw_bannercnt
    if g:netrw_sort_by =~ "^n"
     call s:SetSort()
     if g:netrw_sort_direction =~ 'n'
      exe 'keepjumps silent '.w:netrw_bannercnt.',$sort'
     else
      exe 'keepjumps silent '.w:netrw_bannercnt.',$sort!'
     endif
     exe 'keepjumps silent '.w:netrw_bannercnt.',$s/^\d\{3}\///e'
    endif
    if w:netrw_longlist == 1
     " shorten the list to keep its width <= winwidth characters
     exe "keepjumps silent ".w:netrw_bannercnt.',$s/\t[-dstrwx]\+/\t/e'
    endif
   endif
  endif

  " cleanup any windows mess at end-of-line
  keepjumps silent! %s/\r$//e
  call s:NetrwWideListing()
  if line("$") >= w:netrw_bannercnt
   exe "keepjumps ".w:netrw_bannercnt
  endif

  call s:NetOptionRestore()
  setlocal nomod noma nonu

"  call Dret("NetBrowse")
  return
endfun

" ---------------------------------------------------------------------
" NetBrowseChgDir: {{{2
fun! s:NetBrowseChgDir(dirname,newdir)
"  call Dfunc("NetBrowseChgDir(dirname<".a:dirname."> newdir<".a:newdir.">)")

  let dirname= a:dirname
  let newdir = a:newdir

  if newdir !~ '[\/]$'
   " handling a file
   let dirname= dirname.newdir
"   call Decho("handling a file: dirname<".dirname.">")

  elseif newdir == './'
   " refresh the directory list
"   call Decho("refresh directory listing")
   setlocal ma nobl bh=hide
   %d

  elseif newdir == '../'
   " go up one directory
   let trailer= substitute(a:dirname,'^\(\w\+://\%(\w\+@\)\=\w\+/\)\(.*\)$','\2','')

   if trailer =~ '^\%(\.\./\)*$'
    " tack on a ../"
    let dirname= dirname.'../'

   else
    " strip off a directory name from dirname
    let dirname= substitute(dirname,'^\(.*/\)[^/]\+/','\1','')
   endif
"   call Decho("go up one dir: dirname<".dirname."> trailer<".trailer.">")

  else
   " go down one directory
   let dirname= dirname.newdir
"   call Decho("go down one dir: dirname<".dirname."> newdir<".newdir.">")
  endif

"  call Dret("NetBrowseChgDir <".dirname.">")
  return dirname
endfun

" ---------------------------------------------------------------------
"  NetGetWord: it gets the directory named under the cursor
fun! s:NetGetWord()
"  call Dfunc("NetGetWord() line#".line("."))
  call s:UseBufWinVars()

  " insure that w:netrw_longlist is set up
  if !exists("w:netrw_longlist")
   if exists("g:netrw_longlist")
    let w:netrw_longlist= g:netrw_longlist
   else
    let w:netrw_longlist= 0
   endif
  endif

  if exists("w:netrw_bannercnt") && line(".") < w:netrw_bannercnt
   " Active Banner support
"   call Decho("active banner handling")
   norm! 0
   let dirname= "./"
   let curline= getline(".")
   if curline =~ '"\s*Sorted by\s'
    norm s
    let s:netrw_skipbrowse= 1
    echo 'Pressing "s" also works'
   elseif curline =~ '"\s*Sort sequence:'
    let s:netrw_skipbrowse= 1
    echo 'Press "S" to edit sorting sequence'
   elseif curline =~ '"\s*Quick Help:'
    norm ?
    let s:netrw_skipbrowse= 1
    echo 'Pressing "?" also works'
   elseif curline =~ '"\s*\%(Hiding\|Showing\):'
    norm a
    let s:netrw_skipbrowse= 1
    echo 'Pressing "a" also works'
   elseif line("$") > w:netrw_bannercnt
    exe w:netrw_bannercnt
   endif

  elseif w:netrw_longlist == 0
"   call Decho("thin column handling")
   norm! 0
   let dirname= getline(".")

  elseif w:netrw_longlist == 1
"   call Decho("long column handling")
   norm! 0
   let dirname= substitute(getline("."),'^\(\%(\S\+\s\)*\S\+\).\{-}$','\1','e')

  else
"   call Decho("obtain word from wide listing")
   let dirname= getline(".")

   if !exists("b:netrw_cpf")
    let b:netrw_cpf= 0
    exe 'silent keepjumps '.w:netrw_bannercnt.',$g/^./if virtcol("$") > b:netrw_cpf|let b:netrw_cpf= virtcol("$")|endif'
"    call Decho("computed cpf")
   endif

   let filestart = (virtcol(".")/b:netrw_cpf)*b:netrw_cpf
"   call Decho("virtcol=".virtcol(".")." cpf=".b:netrw_cpf." bannercnt=".w:netrw_bannercnt." filestart=".filestart)
"   call Decho("1: dirname<".dirname.">")
   if filestart > 0|let dirname= substitute(dirname,'^.\{'.filestart.'}','','')|endif
"   call Decho("2: dirname<".dirname.">")
   let dirname   = substitute(dirname,'^\(.\{'.b:netrw_cpf.'}\).*$','\1','e')
"   call Decho("3: dirname<".dirname.">")
   let dirname   = substitute(dirname,'\s\+$','','e')
"   call Decho("4: dirname<".dirname.">")
  endif

"  call Dret("NetGetWord <".dirname.">")
  return dirname
endfun

" ---------------------------------------------------------------------
" NetBrowseRm: remove/delete a remote file or directory {{{2
fun! s:NetBrowseRm(usrhost,path) range
"  call Dfunc("NetBrowseRm(usrhost<".a:usrhost."> path<".a:path.">)")
"  call Decho("firstline=".a:firstline." lastline=".a:lastline)

  " preparation for removing multiple files/directories
  let ctr= a:firstline
  let all= 0

  " remove multiple files and directories
  while ctr <= a:lastline
   exe ctr

   norm! 0
   let rmfile= s:NetGetWord()
"   call Decho("rmfile<".rmfile.">")

   if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '[\/]$')
    " attempt to remove file
    if !all
     echohl Statement
     call inputsave()
     let ok= input("Confirm deletion of file<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
     call inputrestore()
     echohl NONE
     let ok= substitute(ok,'\[{y(es)},n(o),a(ll),q(uit)]\s*','','e')
     if ok =~ 'a\%[ll]'
      let all= 1
     endif
    endif

    if all || ok =~ 'y\%[es]' || ok == ""
     if exists("w:netrw_method") && (w:netrw_method == 2 || w:netrw_method == 3)
      silent! keepjumps .,$d
      call s:NetBrowseFtpCmd(a:path,"delete ".rmfile)
     else
      let netrw_rm_cmd= substitute(g:netrw_rm_cmd,'HOSTNAME',a:usrhost,'').' "'.escape(a:path.rmfile,s:netrw_cd_escape).'"'
"      call Decho("attempt to remove file: system(".netrw_rm_cmd.")")
      let ret= system(netrw_rm_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)
     endif
    elseif ok =~ 'q\%[uit]'
     break
    endif
  
   else
    " attempt to remove directory
    if !all
     call inputsave()
     let ok= input("Confirm deletion of directory<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
     call inputrestore()
     let ok= substitute(ok,'\[{y(es)},n(o),a(ll),q(uit)]\s*','','e')
     if ok =~ 'a\%[ll]'
      let all= 1
     endif
    endif

    if all || ok =~ 'y\%[es]' || ok == ""
     if exists("w:netrw_method") && (w:netrw_method == 2 || w:netrw_method == 3)
      call s:NetBrowseFtpCmd(a:path,"rmdir ".rmfile)
     else
      let rmfile         = a:path.rmfile
      let netrw_rmdir_cmd= substitute(g:netrw_rmdir_cmd,'HOSTNAME',a:usrhost,'').' '."'".'"'.rmfile.'"'."'"
"      call Decho("attempt to remove dir: system(".netrw_rmdir_cmd.")")
      let ret= system(netrw_rmdir_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)

      if v:shell_error != 0
       let netrw_rmf_cmd= substitute(g:netrw_rmf_cmd,'HOSTNAME',a:usrhost,'').' '.substitute(rmfile,'[\/]$','','e')
"       call Decho("2nd attempt to remove dir: system(".netrw_rmf_cmd.")")
       let ret= system(netrw_rmf_cmd)
"       call Decho("returned=".ret." errcode=".v:shell_error)
     
       if v:shell_error != 0 && !exists("g:netrw_quiet")
        echohl Error | echo "***netrw*** unable to remove directory<".rmfile."> -- is it empty?" | echohl None
        call inputsave()|call input("Press <cr> to continue")|call inputrestore()
       endif
      endif
     endif

    elseif ok =~ 'q\%[uit]'
     break
    endif
   endif

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")-1
"  call Decho("refresh the directory")
  call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))
  exe curline

"  call Dret("NetBrowseRm")
endfun

" ---------------------------------------------------------------------
" NetBrowseRename: rename a remote file or directory {{{2
fun! s:NetBrowseRename(usrhost,path) range
"  call Dfunc("NetBrowseRename(usrhost<".a:usrhost."> path<".a:path.">)")

  " preparation for removing multiple files/directories
  let ctr        = a:firstline
  let rename_cmd = substitute(g:netrw_rename_cmd,'\<HOSTNAME\>',a:usrhost,'')

  " attempt to rename files/directories
  while ctr <= a:lastline
   exe "keepjumps ".ctr

   norm! 0
   let oldname= s:NetGetWord()
"   call Decho("oldname<".oldname.">")

   call inputsave()
   let newname= input("Moving ".oldname." to : ",oldname)
   call inputrestore()

   if exists("w:netrw_method") && (w:netrw_method == 2 || w:netrw_method == 3)
    call s:NetBrowseFtpCmd(a:path,"rename ".oldname." ".newname)
   else
    let oldname= a:path.oldname
    let newname= a:path.newname
"    call Decho("system(rename_cmd".' "'.escape(oldname," ").'" "'.escape(newname,s:netrw_cd_escape).'"')
    let ret= system(rename_cmd.' "'.escape(oldname,s:netrw_cd_escape).'" "'.escape(newname,s:netrw_cd_escape).'"')
   endif

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")
  call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))
  exe "keepjumps ".curline
"  call Dret("NetBrowseRename")
endfun

" ---------------------------------------------------------------------
" NetRefresh: {{{2
fun! s:NetRefresh(dirname)
"  call Dfunc("NetRefresh(dirname<".a:dirname.">)")
  set ma
  %d
  call <SID>NetBrowse(dirname)
  redraw!
"  call Dret("NetRefresh")
endfun

" ---------------------------------------------------------------------
" NetSplit: mode {{{2
"           =0 : net   and o
"           =1 : net   and v
"           =2 : local and o
"           =3 : local and v
fun! s:NetSplit(mode)
"  call Dfunc("NetSplit(mode=".a:mode.")")

  call s:SaveWinVars()
  if a:mode == 0
   exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   call s:CopyWinVars()
   exe "norm! 0"
   call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))
  elseif a:mode ==1
   exe (g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v"
   call s:CopyWinVars()
   exe "norm! 0"
   call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))
  elseif a:mode ==2
   exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   call s:CopyWinVars()
   exe "norm! 0"
   call s:LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))
  else
   exe (g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v"
   call s:CopyWinVars()
   exe "norm! 0"
   call s:LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))
  endif

"  call Dret("NetSplit")
endfun

" ---------------------------------------------------------------------
" NetBrowseX:  allows users to write custom functions to operate on {{{2
"              files given their extension.  Passes 0=local, 1=remote
fun! s:NetBrowseX(fname,remote)
"  call Dfunc("NetBrowseX(".a:fname." remote=".a:remote.")")

  " set up the filename
  " (lower case the extension, make a local copy of a remote file)
  let exten= substitute(a:fname,'.*\.\(.\{-}\)','\1','e')
  if has("win32") || has("win95") || has("win64") || has("win16")
   let exten= substitute(exten,'^.*$','\L&\E','')
  endif
  let fname= escape(a:fname,"%#")
"  call Decho("fname<".fname."> after escape()")

  if a:remote == 1
   " create a local copy
   let fname= tempname().".".exten
"   call Decho("create a local copy of <".a:fname."> as <".fname.">")
   exe "keepjumps silent bot 1new ".a:fname
   set bh=delete
   exe "w! ".fname
   q
  endif
"  call Decho("exten<".exten."> "."NetrwFileHandler_".exten."():exists=".exists("*NetrwFileHandler_".exten))

  " set up redirection
  if &srr =~ "%s"
   let redir= substitute(&srr,"%s","/dev/null"."")
  else
   let redir= &srr . "/dev/null"
  endif
"  call Decho("redir:".redir.":")

  " execute the file handler
  if has("win32") || has("win64")
"   call Decho('exe silent !start rundll32 url.dll,FileProtocolHandler "'.escape(fname, '%#').'"')
   exe 'silent !start rundll32 url.dll,FileProtocolHandler "'.escape(fname, '%#').'"'
   let ret= v:shell_error

  elseif has("unix") && executable("kfmclient")
"   call Decho("exe silent !kfmclient exec '".escape(fname,'%#')."' ".redir)
   exe "silent !kfmclient exec '".escape(fname,'%#')."' ".redir
   let ret= v:shell_error

  elseif has("unix") && executable("gnome-open")
"   call Decho("exe silent !gnome-open '".escape(fname,'%#')."' ".redir)
   exe "silent !gnome-open '".escape(fname,'%#')."'".redir
   let ret= v:shell_error

  elseif exten != "" && exists("*NetrwFileHandler_".exten)
"   call Decho("let ret= NetrwFileHandler_".exten.'("'.fname.'")')
   exe "let ret= NetrwFileHandler_".exten.'("'.fname.'")'
  endif
  redraw!

  " cleanup: remove temporary file,
  "          delete current buffer if success with handler,
  "          return to prior buffer (directory listing)
  if a:remote == 1 && fname != a:fname
"   call Decho("deleting temporary file<".fname.">")
   call delete(fname)
  endif

  if a:remote == 1
   set bh=delete bt=nofile noswf
   exe "norm! \<c-o>"
   redraw!
  endif

"  call Dret("NetBrowseX")
endfun

" ---------------------------------------------------------------------
" NetBrowseFtpCmd: unfortunately, not all ftp servers honor options for ls {{{2
"  This function assumes that a long listing will be received.  Size, time,
"  and reverse sorts will be requested of the server but not otherwise
"  enforced here.
fun! s:NetBrowseFtpCmd(path,cmd)
"  call Dfunc("NetBrowseFtpCmd(path<".a:path."> cmd<".a:cmd.">) netrw_method=".w:netrw_method)

  " because WinXX ftp uses unix style input
  " curline is one more than the bannercnt in order to account
  " for the unwanted first blank line (doing a :put to an empty
  " buffer yields a blank first line)
  let ffkeep= &ff
  setlocal ma ff=unix
  let curline= w:netrw_bannercnt+1
  exe "silent! keepjumps ".curline.",$d"

  ".........................................
  if w:netrw_method == 2 || w:netrw_method == 5 
   " ftp + <.netrc>:  Method #2
   if a:path != ""
    put ='cd '.a:path
"    call Decho("ftp:  cd ".a:path)
   endif
   exe "put ='".a:cmd."'"
"   call Decho("ftp:  ".a:cmd)
"    redraw!|call inputsave()|call input("Pausing...")|call inputrestore()
   if exists("g:netrw_port") && g:netrw_port != ""
"    call Decho("exe ".g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
    exe g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
   else
"    call Decho("exe ".g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
    exe g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine
   endif

   ".........................................
  elseif w:netrw_method == 3
   " ftp + machine,id,passwd,filename:  Method #3
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     put ='open '.g:netrw_machine
    endif
 
    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     put =g:netrw_uid
     put =g:netrw_passwd
    else
     put ='user '.g:netrw_uid.' '.g:netrw_passwd
    endif
 
   if a:path != ""
    put ='cd '.a:path
   endif
   exe "put ='".a:cmd."'"
 
    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
"    call Decho("exe ".g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i -n"

   ".........................................
  else
    echo "***warning*** unable to comply with your request<" . choice . ">"
  endif

  " cleanup for Windows
  if has("win32") || has("win95") || has("win64") || has("win16")
   keepjumps silent!! %s/\r$//e
  endif
  if a:cmd == "dir"
   " infer directory/link based on the file permission string
   keepjumps silent! g/d\%([-r][-w][-x]\)\{3}/s@$@/@
   keepjumps silent! g/l\%([-r][-w][-x]\)\{3}/s/$/@/
   if w:netrw_longlist == 0 || w:netrw_longlist == 2
    exe "keepjumps silent! ".curline.',$s/^\%(\S\+\s\+\)\{8}//e'
   endif
  endif

  " ftp's ls doesn't seem to include ./ or ../
  if !search('^\.\/$','wn')
   exe 'keepjumps '.curline
   if a:path !~ '^$'
    put ='../'
   endif
   put ='./'
   exe 'keepjumps '.curline
  endif

  " restore settings
  let &ff= ffkeep
"  call Dret("NetBrowseFtpCmd")
endfun

" ---------------------------------------------------------------------
" NetrwListHide: uses [range]g~...~d to delete files that match comma {{{2
" separated patterns given in g:netrw_list_hide
fun! s:NetrwListHide()
"  call Dfunc("NetrwListHide() listhide<".g:netrw_list_hide.">")

  let listhide= g:netrw_list_hide
  while listhide != ""
   if listhide =~ ','
    let hide     = substitute(listhide,',.*$','','e')
    let listhide = substitute(listhide,'^.\{-},\(.*\)$','\1','e')
   else
    let hide     = listhide
    let listhide= ""
   endif

   " Prune the list by hiding any files which match
"   call Decho("pruning <".hide."> listhide<".listhide.">")
   if g:netrw_hide == 1
    exe 'keepjumps silent '.w:netrw_bannercnt.',$g~'.hide.'~d'
   elseif g:netrw_hide == 2
    exe 'keepjumps silent '.w:netrw_bannercnt.',$v~'.hide.'~d'
   endif
  endwhile

"  call Dret("NetrwListHide")
endfun

" ---------------------------------------------------------------------
" NetHideEdit: allows user to edit the file/directory hiding list
fun! s:NetHideEdit(mode)
"  call Dfunc("NetHideEdit(mode=".a:mode.")")

  call inputsave()
  let newhide= input("Edit Hiding List: ",g:netrw_list_hide)
  call inputrestore()

  " refresh the listing
  let g:netrw_list_hide= newhide
  if a:mode == 0
   silent call s:NetBrowse(s:NetBrowseChgDir(expand("%"),'./'))
  else
   silent call s:LocalRefresh(<SID>LocalBrowseChgDir(b:netrw_curdir,"./"))
  endif

"  call Dret("NetHideEdit")
endfun

" ---------------------------------------------------------------------
" NetSortSequence: allows user to edit the sorting sequence
fun! s:NetSortSequence(mode)
"  call Dfunc("NetSortSequence(mode=".a:mode.")")

  call inputsave()
  let newsortseq= input("Edit Sorting Sequence: ",g:netrw_sort_sequence)
  call inputrestore()

  " refresh the listing
  let g:netrw_sort_sequence= newsortseq
  if a:mode == 0
   silent call s:NetBrowse(s:NetBrowseChgDir(expand("%"),'./'))
  else
   silent call s:LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,"./"))
  endif

"  call Dret("NetSortSequence")
endfun

" ---------------------------------------------------------------------
"  NetLongList: {{{2
fun! s:NetLongList(mode)
"  call Dfunc("NetLongList(mode=".a:mode.") netrw_longlist=".w:netrw_longlist)
  let fname            = s:NetGetWord()
  let w:netrw_longlist = (w:netrw_longlist + 1) % 3
"  call Decho("fname<".fname.">")

  if w:netrw_longlist == 0
   " use one column listing
"   call Decho("use one column list")
   let g:netrw_list_cmd = substitute(g:netrw_list_cmd,' -l','','ge')

  elseif w:netrw_longlist == 1
   " use long list
"   call Decho("use long list")
   let g:netrw_list_cmd = g:netrw_list_cmd." -l"

  else
   " give wide list
"   call Decho("use wide list")
   let g:netrw_list_cmd = substitute(g:netrw_list_cmd,' -l','','ge')
  endif
  setlocal ma

  " clear buffer - this will cause NetBrowse/LocalBrowse to do a refresh
  %d

  " refresh the listing
  if a:mode == 0
   silent call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),"./"))
  else
   silent call s:LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,"./"))
  endif

  " keep cursor on the filename
  silent keepjumps $
  if fname =~ '/$'
   silent call search('\%(^\|\s\{2,}\)\zs'.escape(fname,'.\[]*$^').'\%(\s\{2,}\|$\)','bW')
  else
   silent call search('\%(^\|\s\{2,}\)\zs'.escape(fname,'.\[]*$^').'\%(\s\{2,}\|$\)','bW')
  endif

"  call Dret("NetLongList : w:netrw_longlist=".w:netrw_longlist)
endfun

" ---------------------------------------------------------------------
" NetrwWideListing: {{{2
fun! s:NetrwWideListing()
"  call Dfunc("NetrwWideListing()")

  if w:netrw_longlist == 2
   " look for longest filename (cpf=characters per filename)
   " cpf: characters per file
   " fpl: files per line
   " fpc: files per column
   set ma
   let b:netrw_cpf= 0
   if line("$") >= w:netrw_bannercnt
    exe 'silent keepjumps '.w:netrw_bannercnt.',$g/^./if virtcol("$") > b:netrw_cpf|let b:netrw_cpf= virtcol("$")|endif'
  else
"   call Dret("NetrwWideListing")
   return
  endif
"   call Decho("max file strlen+1=".b:netrw_cpf)
   let b:netrw_cpf= b:netrw_cpf + 1

   " determine qty files per line (fpl)
   let w:netrw_fpl= winwidth(0)/b:netrw_cpf
"   call Decho("fpl= ".winwidth(0)."/[b:netrw_cpf=".b:netrw_cpf.']='.w:netrw_fpl)

   " make wide display
   exe 'silent keepjumps '.w:netrw_bannercnt.',$s/^.*$/\=printf("%-'.b:netrw_cpf.'s",submatch(0))/'
   let fpc         = (line("$") - w:netrw_bannercnt + w:netrw_fpl)/w:netrw_fpl
   let newcolstart = w:netrw_bannercnt + fpc
   let newcolend   = newcolstart + fpc - 1
"   call Decho("bannercnt=".w:netrw_bannercnt." fpl=".w:netrw_fpl." fpc=".fpc." newcol[".newcolstart.",".newcolend."]")
   while line("$") >= newcolstart
    if newcolend > line("$") | let newcolend= line("$") | endif
    let newcolqty= newcolend - newcolstart
    exe newcolstart
    if newcolqty == 0
     exe "silent keepjumps norm! 0\<c-v>$hx".w:netrw_bannercnt."G$p"
    else
     exe "silent keepjumps norm! 0\<c-v>".newcolqty.'j$hx'.w:netrw_bannercnt.'G$p'
    endif
    exe "silent keepjumps ".newcolstart.','.newcolend.'d'
    exe w:netrw_bannercnt
   endwhile
   exe "silent keepjumps ".w:netrw_bannercnt.',$s/\s\+$//e'
   set noma nomod
  endif

"  call Dret("NetrwWideListing")
endfun

" ---------------------------------------------------------------------
" NetSaveWordPosn: used by the "s" command in both remote and local {{{2
" browsing.  Along with NetRestoreWordPosn(), it keeps the cursor on
" the same word even though the sorting has changed its order of appearance.
fun! s:NetSaveWordPosn()
"  call Dfunc("NetSaveWordPosn()")
  let s:netrw_saveword= '^'.escape(getline("."),s:netrw_cd_escape).'$'
"  call Dret("NetSaveWordPosn : saveword<".s:netrw_saveword.">")
endfun

" ---------------------------------------------------------------------
" NetRestoreWordPosn: used by the "s" command; see NetSaveWordPosn() above {{{2
fun! s:NetRestoreWordPosn()
"  call Dfunc("NetRestoreWordPosn()")
  silent! call search(s:netrw_saveword,'w')
"  call Dret("NetRestoreWordPosn")
endfun

" ---------------------------------------------------------------------
" NetMakeDir: this function makes a directory (both local and remote) {{{2
fun! s:NetMakeDir(usrhost)
"  call Dfunc("NetMakeDir(usrhost<".a:usrhost.">)")

  " get name of new directory from user.  A bare <CR> will skip.
  " if its currently a directory, also request will be skipped, but with
  " a message.
  call inputsave()
  let newdirname= input("Please give directory name: ")
  call inputrestore()
"  call Decho("newdirname<".newdirname.">")

  if newdirname == ""
"   call Dret("NetMakeDir : user aborted with bare <cr>")
   return
  endif

  if a:usrhost == ""

   " Local mkdir:
   " sanity checks
   let fullnewdir= b:netrw_curdir.'/'.newdirname
"   call Decho("fullnewdir<".fullnewdir.">")
   if isdirectory(fullnewdir)
    if !exists("g:netrw_quiet")
     echohl WarningMsg | echo "***netrw*** <".newdirname."> is already a directory!" | echohl None
     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
    endif
"    call Dret("NetMakeDir : directory<".newdirname."> exists previously")
    return
   endif
   if filereadable(fullnewdir)
    if !exists("g:netrw_quiet")
     echohl WarningMsg | echo "***netrw*** <".newdirname."> is already a file!" | echohl None
     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
    endif
"    call Dret("NetMakeDir : file<".newdirname."> exists previously")
    return
   endif

   " requested new local directory is neither a pre-existing file or
   " directory, so make it!
   if exists("*mkdir")
    call mkdir(fullnewdir,"p")
   else
    let netrw_origdir= s:NetGetcwd(1)
    exe 'cd '.b:netrw_curdir
"    call Decho("netrw_origdir<".netrw_origdir."> b:netrw_curdir<".b:netrw_curdir.">")
"    call Decho("exe silent! !".g:netrw_local_mkdir.' "'.newdirname.'"')
    exe "silent! !".g:netrw_local_mkdir.' "'.newdirname.'"'
    if !g:netrw_keepdir | exe 'keepjumps cd '.netrw_origdir | endif
   endif

   if v:shell_error == 0
    " refresh listing
"    call Decho("refresh listing")
    let linenum= line(".")
    norm! H0
    let hline  = line(".")
    set ma|norm! 2D
    call s:LocalBrowse(s:LocalBrowseChgDir(b:netrw_curdir,'./'))
    exe "norm! ".hline."G0z\<CR>"
    exe linenum
   elseif !exists("g:netrw_quiet")
    echohl Error | echo "***netrw*** unable to make directory<".newdirname.">" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
   redraw!

  else
   " Remote mkdir:
   let mkdircmd  = substitute(g:netrw_mkdir_cmd,'\<HOSTNAME\>',a:usrhost,'')
   let newdirname= "'".'"'.substitute(expand("%"),'^\%(.\{-}/\)\{3}\(.*\)$','\1','').newdirname.'"'."'"
"   call Decho("exe silent! !".mkdircmd." ".newdirname)
   exe "silent! !".mkdircmd." ".newdirname
   if v:shell_error == 0
    " refresh listing
    let linenum= line(".")
    norm! H0
    let hline  = line(".")
    call s:NetBrowse(s:NetBrowseChgDir(expand("%"),'./'))
    exe "norm! ".hline."G0z\<CR>"
    exe linenum
   elseif !exists("g:netrw_quiet")
    echohl Error | echo "***netrw*** unable to make directory<".newdirname.">" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
   redraw!
  endif
  
"  call Dret("NetMakeDir")
endfun

" ---------------------------------------------------------------------
"  NetBookmarkDir: {{{2
"    0: (user: <b>)   bookmark current directory
"    1: (user: <B>)   change to the bookmarked directory
"    2: (user: <q>)   list bookmarks
"    3: (LocalBrowse) record current directory history
"    4: (user: <u>)   go up   (previous) bookmark
"    5: (user: <U>)   go down (next)     bookmark
fun! s:NetBookmarkDir(chg,curdir)
"  call Dfunc("NetBookmarkDir(chg=".a:chg." curdir<".a:curdir.">) cnt=".v:count)
  if exists("w:netrw_bannercnt") && line(".") <= w:netrw_bannercnt
   " looks like a "b" was pressed while in the banner region
   if line("$") > w:netrw_bannercnt
    exe w:netrw_bannercnt
   endif
   echo ""
"  call Dret("NetBookmarkDir - ignoring")
   return
  endif

  if a:chg == 0
   " bookmark the current directory
   let g:NETRW_BOOKMARKDIR_{v:count}= a:curdir
   if !exists("g:NETRW_BOOKMARKMAX")
    let g:NETRW_BOOKMARKMAX= v:count
   elseif v:count > g:NETRW_BOOKMARKMAX
    let g:NETRW_BOOKMARKMAX= v:count
   endif
   echo "bookmarked the current directory"

  elseif a:chg == 1
   " change to the bookmarked directory
   if exists("g:NETRW_BOOKMARKDIR_{v:count}")
    exe "e ".g:NETRW_BOOKMARKDIR_{v:count}
   else
    echomsg "Sorry, bookmark#".v:count." doesn't exist!"
   endif

  elseif a:chg == 2
   " list user's bookmarks
   if exists("g:NETRW_BOOKMARKMAX")
"    call Decho("list bookmarks [0,".g:NETRW_BOOKMARKMAX."]")
    let cnt= 0
    while cnt <= g:NETRW_BOOKMARKMAX
     if exists("g:NETRW_BOOKMARKDIR_{cnt}")
"      call Decho("Netrw Bookmark#".cnt.": ".g:NETRW_BOOKMARKDIR_{cnt})
      echo "Netrw Bookmark#".cnt.": ".g:NETRW_BOOKMARKDIR_{cnt}
     endif
     let cnt= cnt + 1
    endwhile
   endif

   " list directory history
   let cnt     = g:NETRW_DIRHIST_CNT
   let first   = 1
   let histcnt = 0
   while ( first || cnt != g:NETRW_DIRHIST_CNT )
"    call Decho("first=".first." cnt=".cnt." dirhist_cnt=".g:NETRW_DIRHIST_CNT)
    let histcnt= histcnt + 1
    if exists("g:NETRW_DIRHIST_{cnt}")
"     call Decho("Netrw  History#".histcnt.": ".g:NETRW_DIRHIST_{cnt})
     echo "Netrw  History#".histcnt.": ".g:NETRW_DIRHIST_{cnt}
    endif
    let first = 0
    let cnt   = ( cnt - 1 ) % g:netrw_dirhistmax
    if cnt < 0
     let cnt= cnt + g:netrw_dirhistmax
    endif
   endwhile

  elseif a:chg == 3
   " saves most recently visited directories (when they differ)
   if !exists("g:NETRW_DIRHIST_0") || g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT} != a:curdir
    let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT + 1 ) % g:netrw_dirhistmax
    let g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}= substitute(a:curdir,'[/\\]$','','e')
"    call Decho("save dirhist#".g:NETRW_DIRHIST_CNT."<".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}.">")
   endif

  elseif a:chg == 4
   " u: change to the previous directory stored on the history list
   let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT - 1 ) % g:netrw_dirhistmax
   if g:NETRW_DIRHIST_CNT < 0
    let g:NETRW_DIRHIST_CNT= g:NETRW_DIRHIST_CNT + g:netrw_dirhistmax
   endif
   if exists("g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}")
"    call Decho("changedir u#".g:NETRW_DIRHIST_CNT."<".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}.">")
    exe "e ".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}
   else
    let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT + 1 ) % g:netrw_dirhistmax
    echo "Sorry, no predecessor directory exists yet"
   endif

  elseif a:chg == 5
   " U: change to the subsequent directory stored on the history list
   let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT + 1 ) % g:netrw_dirhistmax
   if exists("g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}")
"    call Decho("changedir U#".g:NETRW_DIRHIST_CNT."<".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}.">")
    exe "e ".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}
   else
    let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT - 1 ) % g:netrw_dirhistmax
    if g:NETRW_DIRHIST_CNT < 0
     let g:NETRW_DIRHIST_CNT= g:NETRW_DIRHIST_CNT + g:netrw_dirhistmax
    endif
    echo "Sorry, no successor directory exists yet"
   endif
  endif
"  call Dret("NetBookmarkDir")
endfun

" ---------------------------------------------------------------------
" NetObtain: obtain file under cursor (for remote browsing support) {{{2
fun! s:NetObtain()
  if !exists("s:netrw_users_stl")
   let s:netrw_users_stl= &stl
  endif
  let fname= expand("<cWORD>")
  exe 'set stl=%f\ %h%m%r%=Obtaining\ '.escape(fname,' ')
  redraw!

"  call Dfunc("NetObtain() method=".w:netrw_method)
  if exists("w:netrw_method") && w:netrw_method =~ '[235]'
   if executable("ftp")
    let curdir = expand("%")
    let path   = substitute(curdir,'ftp://[^/]\+/','','e')
    let curline= line(".")
    let endline= line("$")+1
    set ma
    keepjumps $

    ".........................................
    if w:netrw_method == 2
     " ftp + <.netrc>: Method #2
     if path != ""
      put ='cd '.path
"      call Decho("ftp:  cd ".path)
     endif
     put ='get '.fname
"     call Decho("ftp:  get ".fname)
     if exists("g:netrw_port") && g:netrw_port != ""
"      call Decho("exe ".g:netrw_silentxfer.endline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
      exe g:netrw_silentxfer.endline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
     else
"      call Decho("exe ".g:netrw_silentxfer.endline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
      exe g:netrw_silentxfer.endline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine
     endif

   ".........................................
  elseif w:netrw_method == 3
   " ftp + machine,id,passwd,filename: Method #3
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
"     call Decho('ftp:  open '.g:netrw_machine.' '.g:netrw_port)
    else
     put ='open '.g:netrw_machine
"     call Decho('ftp:  open '.g:netrw_machine
    endif
 
    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     put =g:netrw_uid
     put =g:netrw_passwd
"     call Decho('ftp:  g:netrw_uid')
"     call Decho('ftp:  g:netrw_passwd')
    else
     put ='user '.g:netrw_uid.' '.g:netrw_passwd
"     call Decho('user '.g:netrw_uid.' '.g:netrw_passwd)
    endif
 
   if a:path != ""
    put ='cd '.a:path
"    call Decho('cd '.a:path)
   endif
   exe "put ='".a:cmd."'"
"   call Decho("ftp:  ".a:cmd)
 
    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
"    call Decho("exe ".g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer.endline.",$!".g:netrw_ftp_cmd." -i -n"
   
    ".........................................
    else
      echo "***warning*** unable to comply with your request<" . choice . ">"
    endif
    " restore
    exe "silent! ".endline.",$d"
    exe "keepjumps ".curline
    set noma nomod
   else
    if !exists("g:netrw_quiet")
     echohl Error | echo "***netrw*** this system doesn't support ftp" | echohl None
     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
    endif
"    call Dret("NetObtain")
    return
   endif

  ".........................................
  else
   " scp: Method#4
   if exists("g:netrw_port") && g:netrw_port != ""
    let useport= " -P ".g:netrw_port
   else
    let useport= ""
   endif
   if g:netrw_cygwin == 1
    let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
"    call Decho("executing: !".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(fname,' ?&')." .")
    exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(fname,' ?&')." ."
   else
"    call Decho("executing: !".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(fname,' ?&')." .")
    exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(fname,' ?&')." ."
   endif
  endif

  " restore status line
  let &stl= s:netrw_users_stl
  redraw!

"  call Dret("NetObtain")
endfun

" ==========================================
"  Local Directory Browsing Support:    {{{1
" ==========================================

" ---------------------------------------------------------------------
" LocalBrowse: {{{2
fun! s:LocalBrowse(dirname)
  " unfortunate interaction -- debugging calls can't be used here;
  " the BufEnter event causes triggering when attempts to write to
  " the DBG buffer are made.
  if isdirectory(a:dirname)
   call netrw#DirBrowse(a:dirname)
  endif
  " not a directory, ignore it
endfun

" ---------------------------------------------------------------------
" DirBrowse: supports local file/directory browsing {{{2
fun! netrw#DirBrowse(dirname)
  if !exists("w:netrw_longlist")|let w:netrw_longlist= g:netrw_longlist|endif
"  call Dfunc("DirBrowse(dirname<".a:dirname.">) buf#".bufnr("%")." winnr=".winnr()." sortby=".g:netrw_sort_by)
"  call Dredir("ls!")

  if exists("s:netrw_skipbrowse")
   unlet s:netrw_skipbrowse
"   call Dret("DirBrowse")
   return
  endif
  call s:NetOptionSave()

  if v:version < 603
   if !exists("g:netrw_quiet")
    echohl Error | echo "***netrw*** vim version<".v:version."> too old for browsing with netrw" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
   call s:NetOptionRestore()
"   call Dret("DirBrowse : vim version<".v:version."> too old")
   return
  endif

  " use buffer-oriented WinVars if buffer ones exist but window ones don't
  call s:UseBufWinVars()

  " find buffer number of buffer named precisely the same as a:dirname
  let bufnum= bufnr(escape(a:dirname,'\'))
"  call Decho("findbuf: bufnum=".bufnum)
  if bufnum > 0 && bufname(bufnum) != a:dirname
   let ibuf= 1
   let buflast= bufnr("$")
   while bufname(ibuf) !~ '^'.a:dirname.'\=$' && ibuf <= buflast
"    call Decho("findbuf: ibuf=".ibuf. " bufname<".bufname(ibuf)."> dirname<".a:dirname.">")
    let ibuf= ibuf + 1
   endwhile
   if ibuf > buflast
    let bufnum= -1
   else
    let bufnum= ibuf
   endif
"   call Decho("findbuf: bufnum=".bufnum." (final)")
  endif

  " get cleared buffer
  if bufnum < 0 || !bufexists(bufnum)
   keepalt enew!
"   call Decho("enew buffer")
  else
   exe "keepalt b ".bufnum
   if exists("s:last_sort_by") && g:netrw_sort_by == s:last_sort_by
    if getline(2) =~ '^" Directory Listing '
     if !g:netrw_keepdir
"      call Decho("change directory: cd ".b:netrw_curdir)
      exe 'cd '.escape(b:netrw_curdir,s:netrw_cd_escape)
     endif
     call s:NetOptionRestore()
"     call Dret("DirBrowse : reusing buffer#".bufnum."<".a:dirname.">")
     return
    endif
   endif
  endif
  let s:last_sort_by= g:netrw_sort_by

  " get the new directory name
  if has("win32") || has("win95") || has("win64") || has("win16")
   let b:netrw_curdir= substitute(a:dirname,'\\','/','ge')
  else
   let b:netrw_curdir= a:dirname
  endif
  if b:netrw_curdir =~ '[/\\]$'
   let b:netrw_curdir= substitute(b:netrw_curdir,'[/\\]$','','e')
  endif
  if b:netrw_curdir == ''
   " under unix, when the root directory is encountered, the result
   " from the preceding substitute is an empty string.
   let b:netrw_curdir= '/'
  endif
"  call Decho("b:netrw_curdir<".b:netrw_curdir.">")

  " make netrw's idea of the current directory vim's if the user wishes
  if !g:netrw_keepdir
"   call Decho("change directory: cd ".b:netrw_curdir)
   try
    exe 'cd '.escape(b:netrw_curdir,s:netrw_cd_escape)
   catch /^Vim\%((\a\+)\)\=:E472/
    echohl Error | echo "***netrw*** unable to change directory to <".b:netrw_curdir."> (permissions?)" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
    if exists("w:netrw_prvdir")
     let b:netrw_curdir= w:netrw_prvdir
    else
     call s:NetOptionRestore()
"     call Dret("DirBrowse : reusing buffer#".bufnum."<".a:dirname.">")
     return
    endif
   endtry
  endif

  " change the name of the buffer to reflect the b:netrw_curdir
  exe 'silent! file '.escape(b:netrw_curdir,s:netrw_cd_escape)

  " make this buffer not-a-file, modifiable, not line-numbered, etc
  setlocal bh=hide bt=nofile nobl ma nonu
  keepalt silent! %d

  " ---------------------------
  "  Perform Directory Listing:

  " save current directory on directory history list
  call <SID>NetBookmarkDir(3,b:netrw_curdir)

  " set up all the maps
"  call Decho("Setting up local browser maps")
  nnoremap <buffer> <silent> <cr>	:call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> <c-l>	:set ma<bar>%d<bar>call <SID>LocalRefresh(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<bar>redraw!<cr>
  nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'../'))<cr>
  nnoremap <buffer> <silent> a		:let g:netrw_hide=(g:netrw_hide+1)%3<bar>exe "norm! 0"<bar>call <SID>LocalRefresh(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<cr>
  if w:netrw_longlist != 2
   nnoremap <buffer> <silent> b		:<c-u>call <SID>NetBookmarkDir(0,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> B		:<c-u>call <SID>NetBookmarkDir(1,b:netrw_curdir)<cr>
  endif
  nnoremap <buffer> <silent> Nb		:<c-u>call <SID>NetBookmarkDir(0,b:netrw_curdir)<cr>
  nnoremap <buffer> <silent> NB		:<c-u>call <SID>NetBookmarkDir(1,b:netrw_curdir)<cr>
  nnoremap <buffer> <silent> c		:exe "cd ".b:netrw_curdir<cr>
  nnoremap <buffer> <silent> d		:call <SID>NetMakeDir("")<cr>
  nnoremap <buffer> <silent> <c-h>	:call <SID>NetHideEdit(1)<cr>
  nnoremap <buffer> <silent> i		:call <SID>NetLongList(1)<cr>
  nnoremap <buffer> <silent> o		:call <SID>NetSplit(2)<cr>
  nnoremap <buffer> <silent> O		:call <SID>LocalObtain()<cr>
  nnoremap <buffer> <silent> p		:call <SID>LocalPreview(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord(),1))<cr>
  nnoremap <buffer> <silent> q		:<c-u>call <SID>NetBookmarkDir(2,b:netrw_curdir)<cr>
  nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>LocalRefresh(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<cr>
  nnoremap <buffer> <silent> s		:call <SID>NetSaveWordPosn()<bar>let g:netrw_sort_by= (g:netrw_sort_by =~ 'n')? 'time' : (g:netrw_sort_by =~ 't')? 'size' : 'name'<bar>exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<bar>call <SID>NetRestoreWordPosn()<cr>
  nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(1)<cr>
  nnoremap <buffer> <silent> u		:<c-u>call <SID>NetBookmarkDir(4,expand("%"))<cr>
  nnoremap <buffer> <silent> U		:<c-u>call <SID>NetBookmarkDir(5,expand("%"))<cr>
  nnoremap <buffer> <silent> v		:call <SID>NetSplit(3)<cr>
  nnoremap <buffer> <silent> x		:call <SID>NetBrowseX(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord(),0),0)"<cr>
  nnoremap <buffer> <silent> <2-leftmouse> :exe "call <SID>LocalRefresh(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))"<cr>
  nnoremap <buffer> <silent> <s-up>	:Pexplore<cr>
  nnoremap <buffer> <silent> <s-down>	:Nexplore<cr>
  exe 'nnoremap <buffer> <silent> <del>	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'vnoremap <buffer> <silent> <del>	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'nnoremap <buffer> <silent> D	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'vnoremap <buffer> <silent> D	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'nnoremap <buffer> <silent> R	:call <SID>LocalBrowseRename("'.b:netrw_curdir.'")<cr>'
  exe 'vnoremap <buffer> <silent> R	:call <SID>LocalBrowseRename("'.b:netrw_curdir.'")<cr>'
  exe 'nnoremap <buffer> <silent> <Leader>m :call <SID>NetMakeDir("")<cr>'
  nnoremap <buffer> ?			:he netrw-dir<cr>

  " Set up the banner
"  call Decho("set up banner")
  keepjumps put ='\" ============================================================================'
  keepjumps 1d
  keepjumps put ='\" Directory Listing                                              (netrw '.g:loaded_netrw.')'
  keepjumps put ='\"   '.b:netrw_curdir
  let w:netrw_bannercnt= 3

  let sortby= g:netrw_sort_by
  if g:netrw_sort_direction =~ "^r"
   let sortby= sortby." reversed"
  endif

  " Sorted by...
  if g:netrw_sort_by =~ "^n"
"   call Decho("directories will be sorted by name")
   " sorted by name
   keepjumps put ='\"   Sorted by      '.sortby
   keepjumps put ='\"   Sort sequence: '.g:netrw_sort_sequence
   let w:netrw_bannercnt= w:netrw_bannercnt + 2
  else
"   call Decho("directories will be sorted by size or date")
   " sorted by size or date
   keepjumps put ='\"   Sorted by '.sortby
   let w:netrw_bannercnt= w:netrw_bannercnt + 1
  endif

  " Hiding...  -or-  Showing...
  if g:netrw_list_hide != "" && g:netrw_hide
   if g:netrw_hide == 1
    keepjumps put ='\"   Hiding:        '.g:netrw_list_hide
   else
    keepjumps put ='\"   Showing:       '.g:netrw_list_hide
   endif
   let w:netrw_bannercnt= w:netrw_bannercnt + 1
  endif
  keepjumps put ='\"   Quick Help:    ?:help  -:go up dir  D:delete  R:rename  s:sort-by  x:exec'
  keepjumps put ='\" ============================================================================'
  let w:netrw_bannercnt= w:netrw_bannercnt + 2

  " bannercnt should index the line just after the banner
  let w:netrw_bannercnt= w:netrw_bannercnt + 1
"  call Decho("bannercnt=".w:netrw_bannercnt)

  " generate the requested directory listing
  call s:LocalBrowseList()

  " set up syntax highlighting
  if has("syntax")
   setlocal ft=netrwlist
   if !exists("g:syntax_on") || !g:syntax_on
    setlocal ft=
   endif
  endif

  " manipulate the directory listing (hide, sort)
  if line("$") >= w:netrw_bannercnt
   if g:netrw_hide && g:netrw_list_hide != ""
    call s:NetrwListHide()
   endif
   if line("$") >= w:netrw_bannercnt

    if g:netrw_sort_by =~ "^n"
     call s:SetSort()

     if g:netrw_sort_direction =~ 'n'
      exe 'keepjumps silent '.w:netrw_bannercnt.',$sort'
     else
      exe 'keepjumps silent '.w:netrw_bannercnt.',$sort!'
     endif
     exe 'keepjumps silent '.w:netrw_bannercnt.',$s/^\d\{3}\///e'

    else
     if g:netrw_sort_direction =~ 'n'
      exe 'keepjumps silent '.w:netrw_bannercnt.',$sort'
     else
      exe 'keepjumps silent '.w:netrw_bannercnt.',$sort!'
     endif
     exe 'keepjumps silent '.w:netrw_bannercnt.',$s/^\d\{-}\///e'
    endif

   endif
  endif

  call s:NetrwWideListing()
  if exists("w:netrw_bannercnt") && line("$") > w:netrw_bannercnt
   exe w:netrw_bannercnt
  endif

  " record previous current directory
  let w:netrw_prvdir= b:netrw_curdir

  " save certain window-oriented variables into buffer-oriented variables
  call s:BufWinVars()
  call s:NetOptionRestore()
  setlocal noma nomod nonu bh=hide nobl

"  call Dret("DirBrowse : file<".expand("%:p")."> bufname<".bufname("%").">")
endfun

" ---------------------------------------------------------------------
"  LocalBrowseList: does the job of "ls" for local directories {{{2
fun! s:LocalBrowseList()
"  call Dfunc("LocalBrowseList() b:netrw_curdir<".b:netrw_curdir.">")

  " get the list of files contained in the current directory
  let dirname    = escape(b:netrw_curdir,s:netrw_glob_escape)
  let dirnamelen = strlen(b:netrw_curdir)
  let filelist   = glob(dirname."/*")
"  call Decho("glob(dirname<".dirname.">,*)=".filelist)
  if filelist != ""
   let filelist= filelist."\n"
  endif
  let filelist= filelist.glob(dirname."/.*")
"  call Decho("glob(dirname<".dirname.">,.*)=".glob(dirname.".*"))

  " if the directory name includes a "$", and possibly other characters,
  " the glob() doesn't include "." and ".." entries.
  if filelist !~ '[\\/]\.[\\/]\=\(\n\|$\)'
"   call Decho("forcibly tacking on .")
   if filelist == ""
    let filelist= dirname."."
   else
    let filelist= filelist."\n".b:netrw_curdir."."
   endif
"  call Decho("filelist<".filelist.">")
  endif
  if filelist !~ '[\\/]\.\.[\\/]\=\(\n\|$\)'
"   call Decho("forcibly tacking on ..")
   let filelist= filelist."\n".b:netrw_curdir.".."
"  call Decho("filelist<".filelist.">")
  endif
  let filelist= substitute(filelist,'\n\{2,}','\n','ge')
  let filelist= substitute(filelist,'\','/','ge')

"  call Decho("dirname<".dirname.">")
"  call Decho("dirnamelen<".dirnamelen.">")
"  call Decho("filelist<".filelist.">")

  while filelist != ""
   if filelist =~ '\n'
    let filename = substitute(filelist,'\n.*$','','e')
    let filelist = substitute(filelist,'^.\{-}\n\(.*\)$','\1','e')
   else
    let filename = filelist
    let filelist = ""
   endif
   let pfile= filename
   if isdirectory(filename)
    let pfile= filename."/"
   endif
   if pfile =~ '//$'
    let pfile= substitute(pfile,'//$','/','e')
   endif
   let pfile= strpart(pfile,dirnamelen)
   let pfile= substitute(pfile,'^/','','e')
"   call Decho(" ")
"   call Decho("filename<".filename.">")
"   call Decho("pfile   <".pfile.">")

   if w:netrw_longlist == 1
    let sz   = getfsize(filename)
    let fsz  = strpart("               ",1,15-strlen(sz)).sz
    let pfile= pfile."\t".fsz." ".strftime(g:netrw_timefmt,getftime(filename))
"    call Decho("sz=".sz." fsz=".fsz)
   endif

   if     g:netrw_sort_by =~ "^t"
    " sort by time (handles time up to 1 quintillion seconds, US)
"    call Decho("getftime(".filename.")=".getftime(filename))
    let t  = getftime(filename)
    let ft = strpart("000000000000000000",1,18-strlen(t)).t
"    call Decho("exe keepjumps put ='".ft.'/'.filename."'")
    let ftpfile= ft.'/'.pfile
    keepjumps put=ftpfile

   elseif g:netrw_sort_by =~ "^s"
    " sort by size (handles file sizes up to 1 quintillion bytes, US)
"    call Decho("getfsize(".filename.")=".getfsize(filename))
    let sz   = getfsize(filename)
    let fsz  = strpart("000000000000000000",1,18-strlen(sz)).sz
"    call Decho("exe keepjumps put ='".fsz.'/'.filename."'")
    let fszpfile= fsz.'/'.pfile
    keepjumps put =fszpfile

   else 
    " sort by name
"    call Decho("exe keepjumps put ='".pfile."'")
    keepjumps put=pfile
   endif
  endwhile
  
  " cleanup any windows mess at end-of-line
  keepjumps silent! %s/\r$//e
  setlocal ts=32

"  call Dret("LocalBrowseList")
endfun

" ---------------------------------------------------------------------
"  LocalBrowseChgDir: constructs a new directory based on the current {{{2
"                     directory and a new directory name
fun! s:LocalBrowseChgDir(dirname,newdir,...)
"  call Dfunc("LocalBrowseChgDir(dirname<".a:dirname."> newdir<".a:newdir.">) a:0=".a:0)

  let dirname= substitute(a:dirname,'\\','','ge')
  let newdir = a:newdir

  if dirname !~ '[\/]$'
   " apparently vim is "recognizing" that it is in the home directory and
   " is removing the "/".  Bad idea, so I have to put it back.
   let dirname= dirname.'/'
"   call Decho("adjusting dirname<".dirname.">")
  endif

  if newdir !~ '[\/]$'
   " handling a file
   let dirname= dirname.newdir
"   call Decho("handling a file: dirname<".dirname.">")
   " this lets NetBrowseX avoid the edit
   if a:0 < 1
"    call Decho("dirname<".dirname."> netrw_cd_escape<".s:netrw_cd_escape.">")
"    call Decho("about to edit<".escape(dirname,s:netrw_cd_escape).">")
    exe "e! ".escape(dirname,s:netrw_cd_escape)
    set ma nomod
   endif

  elseif newdir == './'
   " refresh the directory list
"   call Decho("refresh directory listing")

  elseif newdir == '../'
   " go up one directory
   let dirname= substitute(dirname,'^\(.*/\)\([^/]\+[\/]$\)','\1','e')
"   call Decho("go up one dir: dirname<".dirname.">")

  else
   " go down one directory
   let dirname= dirname.newdir
"   call Decho("go down one dir: dirname<".dirname."> newdir<".newdir.">")
  endif

"  call Dret("LocalBrowseChgDir <".dirname.">")
  return dirname
endfun

" ---------------------------------------------------------------------
" LocalBrowseRm: {{{2
fun! s:LocalBrowseRm(path) range
"  call Dfunc("LocalBrowseRm(path<".a:path.">)")
"  call Decho("firstline=".a:firstline." lastline=".a:lastline)

  " preparation for removing multiple files/directories
  let ctr           = a:firstline
  let ret           = 0
  let all= 0

  " remove multiple files and directories
  while ctr <= a:lastline
   exe "keepjumps ".ctr

   " sanity checks
   if line(".") < w:netrw_bannercnt
    let ctr= ctr + 1
    continue
   endif
   let curword= s:NetGetWord()
   if curword == "./" || curword == "../"
    let ctr= ctr + 1
    continue
   endif

   norm! 0
   let rmfile= a:path."/".curword
"   call Decho("rmfile<".rmfile.">")

   if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '[\/]$')
    " attempt to remove file
    if !all
     echohl Statement
     call inputsave()
     let ok= input("Confirm deletion of file<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
     call inputrestore()
     echohl NONE
     let ok= substitute(ok,'\[{y(es)},n(o),a(ll),q(uit)]\s*','','e')
     if ok =~ 'a\%[ll]'
      let all= 1
     endif
    endif

    if all || ok =~ 'y\%[es]' || ok == ""
     let ret= delete(rmfile)
"     call Decho("errcode=".v:shell_error." ret=".ret)
    elseif ok =~ 'q\%[uit]'
     break
    endif
  
   else
    " attempt to remove directory
    if !all
     echohl Statement
     call inputsave()
     let ok= input("Confirm deletion of directory<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
     call inputrestore()
     let ok= substitute(ok,'\[{y(es)},n(o),a(ll),q(uit)]\s*','','e')
     if ok =~ 'a\%[ll]'
      let all= 1
     endif
    endif
    let rmfile= substitute(rmfile,'[\/]$','','e')

    if all || ok =~ 'y\%[es]' || ok == ""
"     call Decho("1st attempt: system(".g:netrw_local_rmdir.' "'.rmfile.'")')
     call system(g:netrw_local_rmdir.' "'.rmfile.'"')
"     call Decho("v:shell_error=".v:shell_error)

     if v:shell_error != 0
"      call Decho("2nd attempt to remove directory<".rmfile.">")
      let errcode= delete(rmfile)
"      call Decho("errcode=".errcode)

      if errcode != 0
       if has("unix")
"        call Decho("3rd attempt to remove directory<".rmfile.">")
call system("rm ".rmfile)
        if v:shell_error != 0 && !exists("g:netrw_quiet")
         echohl Error | echo "***netrw*** unable to remove directory<".rmfile."> -- is it empty?" | echohl None
         call inputsave()|call input("Press <cr> to continue")|call inputrestore()
endif
       elseif !exists("g:netrw_quiet")
        echohl Error | echo "***netrw*** unable to remove directory<".rmfile."> -- is it empty?" | echohl None
        call inputsave()|call input("Press <cr> to continue")|call inputrestore()
       endif
      endif
     endif

    elseif ok =~ 'q\%[uit]'
     break
    endif
   endif

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")
"  call Decho("refresh the directory")
  call s:LocalRefresh(s:LocalBrowseChgDir(b:netrw_curdir,'./'))
  exe curline

"  call Dret("LocalBrowseRm")
endfun

" ---------------------------------------------------------------------
" LocalBrowseRename: rename a remote file or directory {{{2
fun! s:LocalBrowseRename(path) range
"  call Dfunc("LocalBrowseRename(path<".a:path.">)")

  " preparation for removing multiple files/directories
  let ctr= a:firstline

  " attempt to rename files/directories
  while ctr <= a:lastline
   exe "keepjumps ".ctr

   " sanity checks
   if line(".") < w:netrw_bannercnt
    let ctr= ctr + 1
    continue
   endif
   let curword= s:NetGetWord()
   if curword == "./" || curword == "../"
    let ctr= ctr + 1
    continue
   endif

   norm! 0
   let oldname= a:path."/".curword
"   call Decho("oldname<".oldname.">")

   call inputsave()
   let newname= input("Moving ".oldname." to : ",substitute(oldname,'/*$','','e'))
   call inputrestore()

   let ret= rename(oldname,newname)
"   call Decho("renaming <".oldname."> to <".newname.">")

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")
"  call Decho("refresh the directory listing")
  call s:LocalRefresh(s:LocalBrowseChgDir(b:netrw_curdir,'./'))
  exe "keepjumps ".curline
"  call Dret("LocalBrowseRename")
endfun

" ---------------------------------------------------------------------
" LocalObtain: copy selected file to current working directory {{{2
fun! s:LocalObtain()
"  call Dfunc("LocalObtain()")
  if exists("b:netrw_curdir") && getcwd() != b:netrw_curdir
   let fname= expand("<cWORD>")
   let fcopy= readfile(b:netrw_curdir."/".fname,"b")
   call writefile(fcopy,getcwd()."/".fname,"b")
  elseif !exists("b:netrw_curdir")
   echohl Error | echo "***netrw*** local browsing directory doesn't exist!"
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
  else
   echohl Error | echo "***netrw*** local browsing directory and current directory are identical"
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
  endif
"  call Dret("LocalObtain")
endfun

" ---------------------------------------------------------------------
" LocalPreview: {{{2
fun! s:LocalPreview(path) range
"  call Dfunc("LocalPreview(path<".a:path.">)")
  if has("quickfix")
   if !isdirectory(a:path)
    exe "pedit ".a:path
   elseif !exists("g:netrw_quiet")
    echohl WarningMsg | echo "***netrw*** sorry, cannot preview a directory such as <".a:path.">" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
  elseif !exists("g:netrw_quiet")
   echohl WarningMsg | echo "***netrw*** sorry, to preview your vim needs the quickfix feature compiled in" | echohl None
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
  endif
"  call Dret("LocalPreview")
endfun

" ---------------------------------------------------------------------
" LocalRefresh: {{{2
fun! s:LocalRefresh(dirname)
"  call Dfunc("LocalRefresh(dirname<".a:dirname.">)")
  set ma
  %d
  call s:LocalBrowse(a:dirname)
  redraw!
"  call Dret("LocalRefresh")
endfun

" ---------------------------------------------------------------------
" Explore: launch the local browser in the directory of the current file {{{2
"          dosplit==0: the window will be split iff the current file has
"                      been modified
"          dosplit==1: the window will be split before running the local
"                      browser
fun! netrw#Explore(indx,dosplit,style,...)
"  call Dfunc("Explore(indx=".a:indx." dosplit=".a:dosplit." style=".a:style.")")

  " if dosplit or file has been modified
  if a:dosplit || &modified
   call <SID>SaveWinVars()

   if a:style == 0      " Explore, Sexplore
    exe g:netrw_winsize."wincmd s"
"    call Decho("style=0: Explore or Sexplore")

   elseif a:style == 1  "Explore!, Sexplore!
    exe g:netrw_winsize."wincmd v"
"    call Decho("style=1: Explore! or Sexplore!")

   elseif a:style == 2  " Hexplore
    exe "bel ".g:netrw_winsize."wincmd s"
"    call Decho("style=2: Hexplore")

   elseif a:style == 3  " Hexplore!
    exe "abo ".g:netrw_winsize."wincmd s"
"    call Decho("style=3: Hexplore!")

   elseif a:style == 4  " Vexplore
    exe "lefta ".g:netrw_winsize."wincmd v"
"    call Decho("style=4: Vexplore")

   elseif a:style == 5  " Vexplore!
    exe "rightb ".g:netrw_winsize."wincmd v"
"    call Decho("style=5: Vexplore!")
   endif
   call s:CopyWinVars()
  endif
  norm! 0

  if a:1 == "" && a:indx >= 0
   " Explore Hexplore Vexplore Sexplore
   let newdir= substitute(expand("%:p"),'^\(.*[/\\]\)[^/\\]*$','\1','e')
   if newdir =~ '^scp:' || newdir =~ '^ftp:'
"    call Decho("calling NetBrowse(newdir<".newdir.">)")
    call s:NetBrowse(newdir)
   else
"    call Decho("calling LocalBrowse(newdir<".newdir.">)")
    call s:LocalBrowse(newdir)
   endif

  elseif a:1 =~ '\*\*/' || a:indx < 0
   " Nexplore Pexplore -or-  Explore **/...

   if has("path_extra")
    if !exists("w:netrw_explore_indx")
     let w:netrw_explore_indx= 0
    endif
    let indx = a:indx
    if indx == -1
     let indx= w:netrw_explore_indx + 1
    elseif indx == -2
     let indx= w:netrw_explore_indx - 1
    else
     let w:netrw_explore_indx    = 0
     if !exists("b:netrw_curdir")
      let b:netrw_curdir= getcwd()
     endif
     let w:netrw_explore_list    = split(expand(b:netrw_curdir."/".a:1),'\n')
     let w:netrw_explore_listlen = len(w:netrw_explore_list)
     if w:netrw_explore_listlen == 1 && w:netrw_explore_list[0] =~ '\*\*\/'
      echohl WarningMsg | echo "***netrw*** no files matched" | echohl None
      call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"      call Dret("Explore")
      return
     endif
    endif

    " NetrwStatusLine support
    let w:netrw_explore_indx= indx
    if !exists("s:netrw_users_stl")
     let s:netrw_users_stl= &stl
    endif
    set stl=%f\ %h%m%r%=%{NetrwStatusLine()}
"    call Decho("explorelist<".join(w:netrw_explore_list,',')."> len=".w:netrw_explore_listlen)

    " sanity check
    if indx >= w:netrw_explore_listlen || indx < 0
     let indx= (indx < 0)? 0 : ( w:netrw_explore_listlen - 1 )
     echohl WarningMsg | echo "***netrw*** no more files match Explore pattern" | echohl None
     call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"     call Dret("Explore")
     return
    endif

    exe "let dirfile= w:netrw_explore_list[".indx."]"
"    call Decho("dirfile<".dirfile."> indx=".indx)
    let newdir= substitute(dirfile,'/[^/]*$','','e')
"    call Decho("newdir<".newdir.">")
"    call Decho("calling LocalBrowse(newdir<".newdir.">)")
    call s:LocalBrowse(newdir)
    call search(substitute(dirfile,"^.*/","",""),"W")
    let w:netrw_explore_mtchcnt = indx + 1
    let w:netrw_explore_bufnr   = bufnr(".")
    let w:netrw_explore_line    = line(".")
"    call Decho("explore: mtchcnt=".w:netrw_explore_mtchcnt." bufnr=".w:netrw_explore_bufnr." line#".w:netrw_explore_line)

   else
    if !exists("g:netrw_quiet")
     echohl WarningMsg | echo "***netrw*** your vim needs the +path_extra feature for Exploring with **!" | echohl None | echohl None
    endif
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif

  else
   let newdir= a:1
"   call Decho("calling LocalBrowse(newdir<".newdir.">)")
   call s:LocalBrowse(newdir)
  endif

"  call Dret("Explore")
endfun

" ---------------------------------------------------------------------
" NetrwStatusLine: {{{2
fun! NetrwStatusLine()
"  let g:stlmsg= "Xbufnr=".w:netrw_explore_bufnr." bufnr=".bufnr(".")." Xline#".w:netrw_explore_line." line#".line(".")
  if !exists("w:netrw_explore_bufnr") || w:netrw_explore_bufnr != bufnr(".") || !exists("w:netrw_explore_line") || w:netrw_explore_line != line(".") || !exists("w:netrw_explore_list")
   " restore user's status line
   let &stl= s:netrw_users_stl
   if exists("w:netrw_explore_bufnr")|unlet w:netrw_explore_bufnr|endif
   if exists("w:netrw_explore_line")|unlet w:netrw_explore_line|endif
   return ""
  else
   return "Match ".w:netrw_explore_mtchcnt." of ".w:netrw_explore_listlen
  endif
endfun

" ---------------------------------------------------------------------
" NetGetcwd: get the current directory. {{{2
"   Change backslashes to forward slashes, if any.
"   If doesc is true, escape certain troublesome characters
fun! s:NetGetcwd(doesc)
"  call Dfunc("NetGetcwd(doesc=".a:doesc.")")
  let curdir= substitute(getcwd(),'\\','/','ge')
  if curdir !~ '[\/]$'
   let curdir= curdir.'/'
  endif
  if a:doesc
   let curdir= escape(curdir,s:netrw_cd_escape)
  endif
"  call Dret("NetGetcwd <".curdir.">")
  return curdir
endfun

" ---------------------------------------------------------------------
" NetMethod:  determine method of transfer {{{1
"  method == 1: rcp
"	     2: ftp + <.netrc>
"	     3: ftp + machine, id, password, and [path]filename
"	     4: scp
"	     5: http (wget)
"	     6: cadaver
"	     7: rsync
"	     8: fetch
"	     9: sftp
fun! s:NetMethod(choice)  " globals: method machine id passwd fname
"   call Dfunc("NetMethod(a:choice<".a:choice.">)")
 
  " initialization
  let b:netrw_method  = 0
  let g:netrw_machine = ""
  let b:netrw_fname   = ""
  let g:netrw_port    = ""
 
  " Patterns:
  " mipf     : a:machine a:id password filename	     Use ftp
  " mf	    : a:machine filename		     Use ftp + <.netrc> or g:netrw_uid g:netrw_passwd
  " ftpurm   : ftp://[user@]host[[#:]port]/filename  Use ftp + <.netrc> or g:netrw_uid g:netrw_passwd
  " rcpurm   : rcp://[user@]host/filename	     Use rcp
  " rcphf    : [user@]host:filename		     Use rcp
  " scpurm   : scp://[user@]host[[#:]port]/filename  Use scp
  " httpurm  : http://[user@]host/filename	     Use wget
  " davurm   : [s]dav://host[:port]/path             Use cadaver
  " rsyncurm : rsync://host[:port]/path              Use rsync
  " fetchurm : fetch://[user@]host[:http]/filename   Use fetch (defaults to ftp, override for http)
  " sftpurm  : sftp://[user@]host/filename  Use scp
  let mipf     = '^\(\S\+\)\s\+\(\S\+\)\s\+\(\S\+\)\s\+\(\S\+\)$'
  let mf       = '^\(\S\+\)\s\+\(\S\+\)$'
  let ftpurm   = '^ftp://\(\([^/@]\{-}\)@\)\=\([^/#:]\{-}\)\([#:]\d\+\)\=/\(.*\)$'
  let rcpurm   = '^rcp://\%(\([^/@]\{-}\)@\)\=\([^/]\{-}\)/\(.*\)$'
  let rcphf    = '^\(\(\h\w*\)@\)\=\(\h\w*\):\([^@]\+\)$'
  let scpurm   = '^scp://\([^/]\{-}\)\([#:]\d\+\)\=/\(.*\)$'
  let httpurm  = '^http://\([^/]\{-}\)\(/.*\)\=$'
  let davurm   = '^s\=dav://\([^/]\+\)/\(.*/\)\([-_.~[:alnum:]]\+\)$'
  let rsyncurm = '^rsync://\([^/]\{-}\)/\(.*\)\=$'
  let fetchurm = '^fetch://\(\([^/@]\{-}\)@\)\=\([^/#:]\{-}\)\(:http\)\=/\(.*\)$'
  let sftpurm  = '^sftp://\([^/]\{-}\)/\(.*\)\=$'
 
"  call Decho("determine method:")
  " Determine Method
  " rcp://user@hostname/...path-to-file
  if match(a:choice,rcpurm) == 0
"   call Decho("rcp://...")
   let b:netrw_method  = 1
   let userid          = substitute(a:choice,rcpurm,'\1',"")
   let g:netrw_machine = substitute(a:choice,rcpurm,'\2',"")
   let b:netrw_fname   = substitute(a:choice,rcpurm,'\3',"")
   if userid != ""
    let g:netrw_uid= userid
   endif
 
  " scp://user@hostname/...path-to-file
  elseif match(a:choice,scpurm) == 0
"   call Decho("scp://...")
   let b:netrw_method  = 4
   let g:netrw_machine = substitute(a:choice,scpurm,'\1',"")
   let g:netrw_port    = substitute(a:choice,scpurm,'\2',"")
   let b:netrw_fname   = substitute(a:choice,scpurm,'\3',"")
 
  " http://user@hostname/...path-to-file
  elseif match(a:choice,httpurm) == 0
"   call Decho("http://...")
   let b:netrw_method = 5
   let g:netrw_machine= substitute(a:choice,httpurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,httpurm,'\2',"")
 
  " dav://hostname[:port]/..path-to-file..
  elseif match(a:choice,davurm) == 0
"   call Decho("dav://...")
   let b:netrw_method= 6
   if a:choice =~ '^s'
    let g:netrw_machine= 'https://'.substitute(a:choice,davurm,'\1/\2',"")
   else
    let g:netrw_machine= 'http://'.substitute(a:choice,davurm,'\1/\2',"")
   endif
   let b:netrw_fname  = substitute(a:choice,davurm,'\3',"")
 
  " rsync://user@hostname/...path-to-file
  elseif match(a:choice,rsyncurm) == 0
"   call Decho("rsync://...")
   let b:netrw_method = 7
   let g:netrw_machine= substitute(a:choice,rsyncurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,rsyncurm,'\2',"")
 
  " ftp://[user@]hostname[[:#]port]/...path-to-file
  elseif match(a:choice,ftpurm) == 0
"   call Decho("ftp://...")
   let userid	      = substitute(a:choice,ftpurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,ftpurm,'\3',"")
   let g:netrw_port   = substitute(a:choice,ftpurm,'\4',"")
   let b:netrw_fname  = substitute(a:choice,ftpurm,'\5',"")
   if userid != ""
    let g:netrw_uid= userid
   endif
   if exists("g:netrw_uid") && exists("g:netrw_passwd")
    let b:netrw_method = 3
   else
    if filereadable(expand("$HOME/.netrc")) && !exists("g:netrw_ignorenetrc")
     let b:netrw_method= 2
    else
     if !exists("g:netrw_uid") || g:netrw_uid == ""
      call NetUserPass()
     elseif !exists("g:netrw_passwd") || g:netrw_passwd == ""
      call NetUserPass(g:netrw_uid)
    " else just use current g:netrw_uid and g:netrw_passwd
     endif
     let b:netrw_method= 3
    endif
   endif
 
  elseif match(a:choice,fetchurm) == 0
"   call Decho("fetch://...")
   let b:netrw_method = 8
   let g:netrw_userid = substitute(a:choice,fetchurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,fetchurm,'\3',"")
   let b:netrw_option = substitute(a:choice,fetchurm,'\4',"")
   let b:netrw_fname  = substitute(a:choice,fetchurm,'\5',"")
 
  " Issue an ftp : "machine id password [path/]filename"
  elseif match(a:choice,mipf) == 0
"   call Decho("(ftp) host id pass file")
   let b:netrw_method  = 3
   let g:netrw_machine = substitute(a:choice,mipf,'\1',"")
   let g:netrw_uid     = substitute(a:choice,mipf,'\2',"")
   let g:netrw_passwd  = substitute(a:choice,mipf,'\3',"")
   let b:netrw_fname   = substitute(a:choice,mipf,'\4',"")
 
  " Issue an ftp: "hostname [path/]filename"
  elseif match(a:choice,mf) == 0
"   call Decho("(ftp) host file")
   if exists("g:netrw_uid") && exists("g:netrw_passwd")
    let b:netrw_method  = 3
    let g:netrw_machine = substitute(a:choice,mf,'\1',"")
    let b:netrw_fname   = substitute(a:choice,mf,'\2',"")
 
   elseif filereadable(expand("$HOME/.netrc"))
    let b:netrw_method  = 2
    let g:netrw_machine = substitute(a:choice,mf,'\1',"")
    let b:netrw_fname   = substitute(a:choice,mf,'\2',"")
   endif
 
  " sftp://user@hostname/...path-to-file
  elseif match(a:choice,sftpurm) == 0
"   call Decho("sftp://...")
   let b:netrw_method = 9
   let g:netrw_machine= substitute(a:choice,sftpurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,sftpurm,'\2',"")
 
  " Issue an rcp: hostname:filename"  (this one should be last)
  elseif match(a:choice,rcphf) == 0
"   call Decho("(rcp) [user@]host:file) rcphf<".rcphf.">")
   let b:netrw_method = 1
   let userid	     = substitute(a:choice,rcphf,'\2',"")
   let g:netrw_machine= substitute(a:choice,rcphf,'\3',"")
   let b:netrw_fname  = substitute(a:choice,rcphf,'\4',"")
"   call Decho('\1<'.substitute(a:choice,rcphf,'\1',"").">")
"   call Decho('\2<'.substitute(a:choice,rcphf,'\2',"").">")
"   call Decho('\3<'.substitute(a:choice,rcphf,'\3',"").">")
"   call Decho('\4<'.substitute(a:choice,rcphf,'\4',"").">")
   if userid != ""
    let g:netrw_uid= userid
   endif
   if has("win32") || has("win95") || has("win64") || has("win16")
    " don't let PCs try <.netrc>
    let b:netrw_method = 3
   endif
 
  else
   if !exists("g:netrw_quiet")
    echohl Error | echo "***netrw*** cannot determine method" | echohl None
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif
   let b:netrw_method  = -1
  endif

  " remove any leading [:#] from port number
  if g:netrw_port != ""
    let g:netrw_port = substitute(g:netrw_port,'[#:]\+','','')
  endif
 
"  call Decho("a:choice       <".a:choice.">")
"  call Decho("b:netrw_method <".b:netrw_method.">")
"  call Decho("g:netrw_machine<".g:netrw_machine.">")
"  call Decho("g:netrw_port   <".g:netrw_port.">")
"  if exists("g:netrw_uid")		"Decho
"   call Decho("g:netrw_uid    <".g:netrw_uid.">")
"  endif					"Decho
"  if exists("g:netrw_passwd")		"Decho
"   call Decho("g:netrw_passwd <".g:netrw_passwd.">")
"  endif					"Decho
"  call Decho("b:netrw_fname  <".b:netrw_fname.">")
"  call Dret("NetMethod")
endfun

" ------------------------------------------------------------------------
" NetUserPass: set username and password for subsequent ftp transfer {{{1
"   Usage:  :call NetUserPass()			-- will prompt for userid and password
"	    :call NetUserPass("uid")		-- will prompt for password
"	    :call NetUserPass("uid","password") -- sets global userid and password
fun! NetUserPass(...)

 " get/set userid
 if a:0 == 0
"  call Dfunc("NetUserPass(a:0<".a:0.">)")
  if !exists("g:netrw_uid") || g:netrw_uid == ""
   " via prompt
   let g:netrw_uid= input('Enter username: ')
  endif
 else	" from command line
"  call Dfunc("NetUserPass(a:1<".a:1.">) {")
  let g:netrw_uid= a:1
 endif

 " get password
 if a:0 <= 1 " via prompt
"  call Decho("a:0=".a:0." case <=1:")
  let g:netrw_passwd= inputsecret("Enter Password: ")
 else " from command line
"  call Decho("a:0=".a:0." case >1: a:2<".a:2.">")
  let g:netrw_passwd=a:2
 endif
"  call Dret("NetUserPass")
endfun

" ------------------------------------------------------------------------
" NetOptionSave: save options and set to "standard" form {{{1
fun! s:NetOptionSave()
"  call Dfunc("NetOptionSave()")
  if !exists("w:netoptionsave")
   let w:netoptionsave= 1
  else
"   call Dret("NetOptionSave : netoptionsave=".w:netoptionsave)
   return
  endif

  " Get Temporary Filename
  let w:aikeep   = &ai
  " record autochdir setting and then insure its unset (tnx to David Fishburn)
  if has("netbeans_intg") || has("sun_workshop")
   let w:acdkeep = &autochdir
   set noautochdir
  endif
  let w:cinkeep  = &cin
  let w:cinokeep = &cino
  let w:comkeep  = &com
  let w:cpokeep  = &cpo
  if !g:netrw_keepdir
   let w:dirkeep = getcwd()
  endif
  let w:gdkeep   = &gd
  let w:repkeep  = &report
  let w:twkeep   = &tw
  setlocal cino =
  setlocal com  =
  setlocal cpo -=aA
  setlocal nocin noai
  setlocal tw   =0
  setlocal report=10000
  if has("win32") && !has("win95")
   let w:swfkeep= &swf
   setlocal noswf
"  call Decho("setting w:swfkeep to <".&swf.">")
  endif

"  call Dret("NetOptionSave")
endfun

" ------------------------------------------------------------------------
" NetOptionRestore: restore options {{{1
fun! s:NetOptionRestore()
"  call Dfunc("NetOptionRestore()")
  if !exists("w:netoptionsave")
"   call Dret("NetOptionRestore : w:netoptionsave doesn't exist")
   return
  endif
  unlet w:netoptionsave
 
  let &ai	= w:aikeep
  if has("netbeans_intg") || has("sun_workshop")
   let &acd     = w:acdkeep
  endif
  let &cin	= w:cinkeep
  let &cino	= w:cinokeep
  let &com	= w:comkeep
  let &cpo	= w:cpokeep
  if exists("w:dirkeep")
   exe "lcd ".w:dirkeep
  endif
  let &gd	= w:gdkeep
  let &report   = w:repkeep
  let &tw	= w:twkeep
  if exists("w:swfkeep")
   if &directory == ""
    " user hasn't specified a swapfile directory;
    " netrw will temporarily make the swapfile
    " directory the current local one.
    let &directory   = getcwd()
    silent! let &swf = w:swfkeep
    set directory=
   else
    let &swf= w:swfkeep
   endif
   unlet w:swfkeep
  endif
  unlet w:aikeep
  unlet w:cinkeep
  unlet w:cinokeep
  unlet w:comkeep
  unlet w:cpokeep
  unlet w:gdkeep
  unlet w:repkeep
  unlet w:twkeep
  if exists("w:dirkeep")
   unlet w:dirkeep
  endif
 
"  call Dret("NetOptionRestore")
endfun

" ------------------------------------------------------------------------
" NetReadFixup: this sort of function is typically written by the user {{{1
"               to handle extra junk that their system's ftp dumps
"               into the transfer.  This function is provided as an
"               example and as a fix for a Windows 95 problem: in my
"               experience, win95's ftp always dumped four blank lines
"               at the end of the transfer.
if has("win95") && g:netrw_win95ftp
 fun! NetReadFixup(method, line1, line2)
"   call Dfunc("NetReadFixup(method<".a:method."> line1=".a:line1." line2=".a:line2.")")
   if method == 3   " ftp (no <.netrc>)
    let fourblanklines= line2 - 3
    silent fourblanklines.",".line2."g/^\s*/d"
   endif
"   call Dret("NetReadFixup")
 endfun
endif

" ---------------------------------------------------------------------
" NetSort: Piet Delport's BISort2() function, modified to take a range {{{1
if v:version < 700
 fun! s:NetSort() range
" "  call Dfunc("NetSort()")
 
  let i = a:firstline + 1
  while i <= a:lastline
    " find insertion point via binary search
    let i_val = getline(i)
    let lo    = a:firstline
    let hi    = i
    while lo < hi
     let mid     = (lo + hi) / 2
     let mid_val = getline(mid)
     if g:netrw_sort_direction =~ '^n'
      " normal sorting order
      if i_val < mid_val
       let hi = mid
      else
       let lo = mid + 1
       if i_val == mid_val | break | endif
      endif
     else
      " reverse sorting order
      if i_val > mid_val
       let hi = mid
      else
       let lo = mid + 1
       if i_val == mid_val | break | endif
      endif
     endif
    endwhile
    " do insert
    if lo < i
     exe 'keepjumps '.i.'d_'
     keepjumps call append(lo - 1, i_val)
    endif
    let i = i + 1
  endwhile
 
" "  call Dret("NetSort")
 endfun
endif

" ---------------------------------------------------------------------
" SetSort: sets up the sort based on the g:netrw_sort_sequence {{{1
"          What this function does is to compute a priority for the patterns
"          in the g:netrw_sort_sequence.  It applies a substitute to any
"          "files" that satisfy each pattern, putting the priority / in
"          front.  An "*" pattern handles the default priority.
fun! s:SetSort()
"  call Dfunc("SetSort() bannercnt=".w:netrw_bannercnt)
  if w:netrw_longlist == 1
   let seqlist  = substitute(g:netrw_sort_sequence,'\$','\\%(\t\\|\$\\)','ge')
  else
   let seqlist  = g:netrw_sort_sequence
  endif
  " sanity check -- insure that * appears somewhere
  if seqlist == ""
   let seqlist= '*'
  elseif seqlist !~ '\*'
   let seqlist= seqlist.',*'
  endif
  let priority = 1
  while seqlist != ""
   if seqlist =~ ','
    let seq     = substitute(seqlist,',.*$','','e')
    let seqlist = substitute(seqlist,'^.\{-},\(.*\)$','\1','e')
   else
    let seq     = seqlist
    let seqlist = ""
   endif
   let eseq= escape(seq,'/')
   if priority < 10
    let spriority= "00".priority.'\/'
   elseif priority < 100
    let spriority= "0".priority.'\/'
   else
    let spriority= priority.'\/'
   endif
"   call Decho("priority=".priority." spriority<".spriority."> seq<".seq."> seqlist<".seqlist.">")

   " sanity check
   if w:netrw_bannercnt > line("$")
    " apparently no files were left after a Hiding pattern was used
"    call Dret("SetSort : no files left after hiding")
    return
   endif
   if seq == '*'
    exe 'keepjumps silent '.w:netrw_bannercnt.',$v/^\d\{3}\//s/^/'.spriority.'/'
   else
    exe 'keepjumps silent '.w:netrw_bannercnt.',$g/'.eseq.'/s/^/'.spriority.'/'
   endif
   let priority = priority + 1
  endwhile

  exe 'keepjumps silent '.w:netrw_bannercnt.',$s/^\(\d\{3}\/\)\%(\d\{3}\/\)\+/\1/e'

"  call Dret("SetSort")
endfun

" ---------------------------------------------------------------------
" SaveWinVars: (used by Explore()) {{{1
fun! s:SaveWinVars()
"  call Dfunc("SaveWinVars()")
  if exists("w:netrw_bannercnt")      |let s:bannercnt       = w:netrw_bannercnt      |endif
  if exists("w:netrw_method")         |let s:method          = w:netrw_method         |endif
  if exists("w:netrw_prvdir")         |let s:prvdir          = w:netrw_prvdir         |endif
  if exists("w:netrw_explore_indx")   |let s:explore_indx    = w:netrw_explore_indx   |endif
  if exists("w:netrw_explore_listlen")|let s:explore_listlen = w:netrw_explore_listlen|endif
  if exists("w:netrw_explore_mtchcnt")|let s:explore_mtchcnt = w:netrw_explore_mtchcnt|endif
  if exists("w:netrw_explore_bufnr")  |let s:explore_bufnr   = w:netrw_explore_bufnr  |endif
  if exists("w:netrw_explore_line")   |let s:explore_line    = w:netrw_explore_line   |endif
  if exists("w:netrw_explore_list")   |let s:explore_list    = w:netrw_explore_list   |endif
"  call Dret("SaveWinVars")
endfun

" ---------------------------------------------------------------------
" CopyWinVars: (used by Explore()) {{{1
fun! s:CopyWinVars()
"  call Dfunc("CopyWinVars()")
  if exists("s:bannercnt")      |let w:netrw_bannercnt       = s:bannercnt      |unlet s:bannercnt      |endif
  if exists("s:method")         |let w:netrw_method          = s:method         |unlet s:method         |endif
  if exists("s:prvdir")         |let w:netrw_prvdir          = s:prvdir         |unlet s:prvdir         |endif
  if exists("s:explore_indx")   |let w:netrw_explore_indx    = s:explore_indx   |unlet s:explore_indx   |endif
  if exists("s:explore_listlen")|let w:netrw_explore_listlen = s:explore_listlen|unlet s:explore_listlen|endif
  if exists("s:explore_mtchcnt")|let w:netrw_explore_mtchcnt = s:explore_mtchcnt|unlet s:explore_mtchcnt|endif
  if exists("s:explore_bufnr")  |let w:netrw_explore_bufnr   = s:explore_bufnr  |unlet s:explore_bufnr  |endif
  if exists("s:explore_line")   |let w:netrw_explore_line    = s:explore_line   |unlet s:explore_line   |endif
  if exists("s:explore_list")   |let w:netrw_explore_list    = s:explore_list   |unlet s:explore_list   |endif
"  call Dret("CopyWinVars")
endfun

" ---------------------------------------------------------------------
" BufWinVars: (used by NetBrowse() and LocalBrowse()) {{{1
"   To allow separate windows to have their own activities, such as
"   Explore **/pattern, several variables have been made window-oriented.
"   However, when the user splits a browser window (ex: ctrl-w s), these
"   variables are not inherited by the new window.  BufWinVars() and
"   UseBufWinVars() get around that.
fun! s:BufWinVars()
"  call Dfunc("BufWinVars()")
  if exists("w:netrw_longlist")       |let b:netrw_longlist        = w:netrw_longlist       |endif
  if exists("w:netrw_bannercnt")      |let b:netrw_bannercnt       = w:netrw_bannercnt      |endif
  if exists("w:netrw_method")         |let b:netrw_method          = w:netrw_method         |endif
  if exists("w:netrw_prvdir")         |let b:netrw_prvdir          = w:netrw_prvdir         |endif
  if exists("w:netrw_explore_indx")   |let b:netrw_explore_indx    = w:netrw_explore_indx   |endif
  if exists("w:netrw_explore_listlen")|let b:netrw_explore_listlen = w:netrw_explore_listlen|endif
  if exists("w:netrw_explore_mtchcnt")|let b:netrw_explore_mtchcnt = w:netrw_explore_mtchcnt|endif
  if exists("w:netrw_explore_bufnr")  |let b:netrw_explore_bufnr   = w:netrw_explore_bufnr  |endif
  if exists("w:netrw_explore_line")   |let b:netrw_explore_line    = w:netrw_explore_line   |endif
  if exists("w:netrw_explore_list")   |let b:netrw_explore_list    = w:netrw_explore_list   |endif
"  call Dret("BufWinVars")
endfun

" ---------------------------------------------------------------------
" UseBufWinVars: (used by NetBrowse() and LocalBrowse() {{{1
"              Matching function to BufferWinVars()
fun! s:UseBufWinVars()
"  call Dfunc("UseBufWinVars()")
  if exists("b:netrw_longlist")        && !exists("w:netrw_longlist")       |let w:netrw_longlist        = b:netrw_longlist       |endif
  if exists("b:netrw_bannercnt")       && !exists("w:netrw_bannercnt")      |let w:netrw_bannercnt       = b:netrw_bannercnt      |endif
  if exists("b:netrw_method")          && !exists("w:netrw_method")         |let w:netrw_method          = b:netrw_method         |endif
  if exists("b:netrw_prvdir")          && !exists("w:netrw_prvdir")         |let w:netrw_prvdir          = b:netrw_prvdir         |endif
  if exists("b:netrw_explore_indx")    && !exists("w:netrw_explore_indx")   |let w:netrw_explore_indx    = b:netrw_explore_indx   |endif
  if exists("b:netrw_explore_listlen") && !exists("w:netrw_explore_listlen")|let w:netrw_explore_listlen = b:netrw_explore_listlen|endif
  if exists("b:netrw_explore_mtchcnt") && !exists("w:netrw_explore_mtchcnt")|let w:netrw_explore_mtchcnt = b:netrw_explore_mtchcnt|endif
  if exists("b:netrw_explore_bufnr")   && !exists("w:netrw_explore_bufnr")  |let w:netrw_explore_bufnr   = b:netrw_explore_bufnr  |endif
  if exists("b:netrw_explore_line")    && !exists("w:netrw_explore_line")   |let w:netrw_explore_line    = b:netrw_explore_line   |endif
  if exists("b:netrw_explore_list")    && !exists("w:netrw_explore_list")   |let w:netrw_explore_list    = b:netrw_explore_list   |endif
"  call Dret("UseBufWinVars")
endfun

" ------------------------------------------------------------------------
" Settings Restoration: {{{1
let &cpo= s:keepcpo
unlet s:keepcpo

" ------------------------------------------------------------------------
" Modelines: {{{1
" vim:ts=8 fdm=marker
