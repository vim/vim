" netrw.vim: Handles file transfer and remote directory listing across a network
" Last Change:	Aug 27, 2004
" Maintainer:	Charles E. Campbell, Jr. PhD   <drchipNOSPAM at campbellfamily.biz>
" Version:	47
" License:	Vim License  (see vim's :help license)
"
"  But be doers of the word, and not only hearers, deluding your own selves
"  (James 1:22 RSV)
" =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
" GetLatestVimScripts: 1075 1 :AutoInstall: netrw.vim

" ---------------------------------------------------------------------
" Prevent Reloading: {{{1
if exists("loaded_netrw") || &cp
  finish
endif
let loaded_netrw    = "v47"
let s:save_cpo      = &cpo
let loaded_explorer = 1
set cpo&vim

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
  let g:netrw_cygwin= 1
 else
  let g:netrw_cygwin= 0
 endif
endif
if !exists("g:netrw_list_cmd")
 if executable("ssh")
  " provide a default listing command
  let g:netrw_list_cmd= "ssh HOSTNAME ls -FLa"
 else
"  call Decho("ssh is not executable, can't do remote directory exploring")
  let g:netrw_list_cmd= ""
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
 let g:netrw_sort_sequence= '/$,*,\.bak$,\.o$,\.h$,\.info$,\.swp$,\.obj$'
endif
if !exists("g:netrw_sort_direction")
 " alternative: reverse  (z y x ...)
 let g:netrw_sort_direction= "normal"
endif
if !exists("g:netrw_longlist") || g:netrw_longlist == 0
 let g:netrw_longlist= 0
else
 let g:netrw_longlist= 1
 let g:netrw_list_cmd= "ssh HOSTNAME ls -FLa -lk"
endif
if !exists("g:netrw_timefmt")
 let g:netrw_timefmt= "%c"
endif
if !exists("g:netrw_local_rmdir")
 let g:netrw_local_rmdir= "rmdir"
endif
if !exists("g:netrw_local_rename")
 if g:netrw_cygwin
  let g:netrw_local_rename= "mv"
 elseif has("win32") || has("win95") || has("win64") || has("win16")
  let g:netrw_local_rename= "rename"
 elseif has("unix")
  let g:netrw_local_rename= "mv"
 endif
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
 let g:netrw_ftp_browse_reject='^total\s\+\d\+$\|^Trying\s\+\d\+.*$\|^KERBEROS_V\d rejected\|^Security extensions not'
endif
if !exists("g:netrw_keepdir")
 let g:netrw_keepdir= 0
endif
if !exists("s:netrw_cd_escape")
 if has("win32") || has("win95") || has("win64") || has("win16")
  let s:netrw_cd_escape="#% "
 else
  let s:netrw_cd_escape="*$%'\" ?`"
 endif
endif
if !exists("s:netrw_glob_escape")
 if has("win32") || has("win95") || has("win64") || has("win16")
  let s:netrw_glob_escape= ""
 else
  let s:netrw_glob_escape= '[]*?`{~'
 endif
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

" ---------------------------------------------------------------------
" Transparency Support: {{{1
 " File Explorer: {{{2
if version >= 600
 augroup FileExplorer
  au!
  au BufEnter * call <SID>LocalBrowse(expand("<afile>"))
 augroup END
 " Network Handler: {{{2
 augroup Network
  au!
  if has("win32") || has("win95") || has("win64") || has("win16")
   au BufReadCmd  file://*		exe "silent doau BufReadPre ".expand("<afile>")|exe 'e '.substitute(expand("<afile>"),"file:/*","","")|exe "silent doau BufReadPost ".expand("<afile>")
  else
   au BufReadCmd  file:///*		exe "silent doau BufReadPre ".expand("<afile>")|exe 'e /'.substitute(expand("<afile>"),"file:/*","","")|exe "silent doau BufReadPost ".expand("<afile>")
   au BufReadCmd  file://localhost/*	exe "silent doau BufReadPre ".expand("<afile>")|exe 'e /'.substitute(expand("<afile>"),"file:/*","","")|exe "silent doau BufReadPost ".expand("<afile>")
  endif
  au BufReadCmd  ftp://*,rcp://*,scp://*,http://*,dav://*,rsync://*,sftp://*	exe "silent doau BufReadPre ".expand("<afile>")|exe "Nread 0r ".expand("<afile>")|exe "silent doau BufReadPost ".expand("<afile>")
  au FileReadCmd ftp://*,rcp://*,scp://*,http://*,dav://*,rsync://*,sftp://*	exe "silent doau BufReadPre ".expand("<afile>")|exe "Nread "   .expand("<afile>")|exe "silent doau BufReadPost ".expand("<afile>")
  au BufWriteCmd ftp://*,rcp://*,scp://*,dav://*,rsync://*,sftp://*    		exe "silent doau BufWritePre ".expand("<afile>")|exe "Nwrite " .expand("<afile>")|exe "silent doau BufWritePost ".expand("<afile>")
 augroup END
endif

" ------------------------------------------------------------------------
" Commands: :Nread, :Nwrite, and :NetUserPass {{{1
com! -nargs=*		Nread		call <SID>NetSavePosn()<bar>call <SID>NetRead(<f-args>)<bar>call <SID>NetRestorePosn()
com! -range=% -nargs=*	Nwrite		call <SID>NetSavePosn()<bar><line1>,<line2>call <SID>NetWrite(<f-args>)<bar>call <SID>NetRestorePosn()
com! -nargs=*		NetUserPass	call NetUserPass(<f-args>)

" ------------------------------------------------------------------------
" NetSavePosn: saves position of cursor on screen {{{1
fun! s:NetSavePosn()
"  call Dfunc("NetSavePosn()")
  " Save current line and column
  let s:netrw_winnr= winnr()
  let s:netrw_line = line(".")
  let s:netrw_col  = virtcol(".")

  " Save top-of-screen line
  norm! H0
  let s:netrw_hline= line(".")

  call s:NetRestorePosn()
"  call Dret("NetSavePosn : winnr=".s:netrw_winnr." line=".s:netrw_line." col=".s:netrw_col." hline=".s:netrw_hline)
endfun

" ------------------------------------------------------------------------
" NetRestorePosn: restores the cursor and file position as saved by NetSavePosn() {{{1
fun! <SID>NetRestorePosn()
"  call Dfunc("NetRestorePosn() winnr=".s:netrw_winnr." line=".s:netrw_line." col=".s:netrw_col." hline=".s:netrw_hline)

  exe "silent! ".s:netrw_winnr."wincmd w"
  if v:shell_error == 0
   " as suggested by Bram M: redraw on no error
   " allows protocol error messages to remain visible
   redraw!
  endif
  " restore top-of-screen line
  exe "norm! ".s:netrw_hline."G0z\<CR>"
  " restore position
  exe "norm! ".s:netrw_line."G0".s:netrw_col."\<bar>"

"  call Dret("NetRestorePosn")
endfun

" ------------------------------------------------------------------------
" NetRead: responsible for reading a file over the net {{{1
fun! s:NetRead(...)
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
     echo 'NetRead Usage:'
     echo ':Nread machine:path                         uses rcp'
     echo ':Nread "machine path"                       uses ftp   with <.netrc>'
     echo ':Nread "machine id password path"           uses ftp'
     echo ':Nread dav://machine[:port]/path            uses cadaver'
     echo ':Nread fetch://machine/path                 uses fetch'
     echo ':Nread ftp://[user@]machine[:port]/path     uses ftp   autodetects <.netrc>'
     echo ':Nread http://[user@]machine/path           uses http  wget'
     echo ':Nread rcp://[user@]machine/path            uses rcp'
     echo ':Nread rsync://machine[:port]/path          uses rsync'
     echo ':Nread scp://[user@]machine[[:#]port]/path  uses scp'
     echo ':Nread sftp://[user@]machine[[:#]port]/path uses sftp'
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
        echoerr "Unbalanced string in filename '". wholechoice ."'"
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
   if choice =~ "^.*/$"
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
     if getline(1) !~ "^$"
      let debugkeep= &debug
      set debug=msg
      echoerr getline(1)
      exe "echomsg '".getline(1)."'"
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
     echoerr getline(1)
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
     echoerr "neither wget nor fetch command is available"
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
     echoerr "fetch command not available"
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
 
  " transform paths from / to \ for Windows, unless the shell is bash
  if &term == "win32"
   if &shell == "bash"
    let fname=a:fname
"    call Decho("(win32 && bash) fname<".fname.">")
   else
    let fname=substitute(a:fname,'/','\\\\','ge')
"    call Decho("(win32 && !bash) fname<".fname.">")
   endif
  else
   let fname= a:fname
"   call Decho("(copied) fname<".fname.">")
  endif
 
  " get the file, but disable undo when reading a new buffer
  if a:readcmd[0] == '0'
   let use_e_cmd = 0		" 1 when using ':edit'
   let delline   = 0		" 1 when have to delete empty last line
   if line("$") == 1 && getline(1) == ""
    " Now being asked to 0r a file into an empty file.
    " Safe to :e it instead, unless there is another window on the same buffer.
    let curbufnr  = bufnr("%")
    let use_e_cmd = 1
    let delline   = 1
    " Loop over all windows,
    " reset use_e_cmd when another one is editing the current buffer.
    let i = 1
    while 1
      if i != winnr() && winbufnr(i) == curbufnr
        let use_e_cmd = 0
        break
      endif
      let i = i + 1
      if winbufnr(i) < 0
        break
      endif
    endwhile
   endif
 
   if use_e_cmd > 0
    " ':edit' the temp file, wipe out the old buffer and rename the buffer
    let curfilename = expand("%")
 
    let binlocal = &l:bin
    let binglobal = &g:bin
    if binlocal
      setglobal bin		" Need to set 'bin' globally for ":e" command.
    endif
    silent exe "e! ".v:cmdarg." ".fname
    if binlocal && !binglobal
      setglobal nobin
      setlocal bin
    endif
 
    exe curbufnr . "bwipe!"
    exe "f ".escape(curfilename," ")
    " the ":f newname" apparently leaves the temporary file as the alternate
    " file in the buffer list (see :ls!).  The following command wipes it out.
    exe bufnr("#")."bwipe!"
   else
    let oldul= &ul
    setlocal ul=-1
    exe a:readcmd." ".v:cmdarg." ".escape(fname," ")
    if delline > 0
     " wipe out last line, which should be a blank line anyway
     $del
    endif
    let &ul= oldul
   endif
  elseif filereadable(fname)
"   call Decho("exe<".a:readcmd." ".v:cmdarg." ".fname.">")
   exe a:readcmd." ".v:cmdarg." ".fname
  else
"   call Dret("NetGetFile")
   return
  endif
 
  " User-provided (ie. optional) fix-it-up command
  if exists("*NetReadFixup")
   let line1= line(".")
   if a:readcmd == "r"
    let line2= line("$") - line2 + line1
   else
    let line2= line("$") - line2
   endif
"   call Decho("calling NetReadFixup(method<".a:method."> line1=".line1." line2=".line2.")")
   call NetReadFixup(a:method, line1, line2)
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
fun! s:NetWrite(...) range
"  call Dfunc("NetWrite(a:0=".a:0.")")
 
  " option handling
  let mod= 0
  call s:NetOptionSave()
 
  " Get Temporary Filename
  let tmpfile= tempname()
 
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
     echo 'NetWrite Usage:"'
     echo ':Nwrite machine:path                        uses rcp'
     echo ':Nwrite "machine path"                      uses ftp with <.netrc>'
     echo ':Nwrite "machine id password path"          uses ftp'
     echo ':Nwrite dav://[user@]machine/path           uses cadaver'
     echo ':Nwrite fetch://[user@]machine/path         uses fetch'
     echo ':Nwrite ftp://machine[#port]/path           uses ftp  (autodetects <.netrc>)'
     echo ':Nwrite rcp://machine/path                  uses rcp'
     echo ':Nwrite rsync://[user@]machine/path         uses rsync'
     echo ':Nwrite scp://[user@]machine[[:#]port]/path uses scp'
     echo ':Nwrite sftp://[user@]machine/path          uses sftp'
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
        echoerr "Unbalanced string in filename '". wholechoice ."'"
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
     echoerr getline(1)
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
     echoerr getline(1)
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
    echoerr "***warning*** currently <netrw.vim> does not support writing using http:"
 
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

" ------------------------------------------------------------------------
"  Browsing Support For Remote Directories And Files:    {{{1
" NetBrowse: This function uses the command in g:netrw_list_cmd to get a list {{{2
"  of the contents of a remote directory.  It is assumed that the
"  g:netrw_list_cmd has a string, HOSTNAME, that needs to be substituted
"  with the requested remote hostname first.
fun! <SID>NetBrowse(dirname)
"  call Dfunc("NetBrowse(dirname<".a:dirname.">) longlist=".g:netrw_longlist)

  " sanity check
  if exists("b:netrw_method") && (b:netrw_method =~ '[23]' && !executable("ftp"))
   echoerr "***netrw*** this system doesn't support remote directory listing via ftp"
"   call Dret("NetBrowse")
   return
  elseif !exists("g:netrw_list_cmd") || g:netrw_list_cmd == ''
   echoerr "***netrw*** this system doesn't support remote directory listing via ssh"
"   call Dret("NetBrowse")
   return
  endif

  " make this buffer modifiable
  setlocal ma

  " analyze a:dirname and g:netrw_list_cmd
  let dirpat  = '^\(\w\{-}\)://\(\w\+@\)\=\([^/]\+\)/\(.*\)$'
"  call Decho("dirpat<".dirpat.">")
  if a:dirname !~ dirpat
   echoerr "NetBrowse: I don't understand your dirname<".a:dirname.">"
"   call Dret("NetBrowse : badly formatted dirname<".a:dirname.">")
   return
  endif

  let method  = substitute(a:dirname,dirpat,'\1','')
  let user    = substitute(a:dirname,dirpat,'\2','')
  let machine = substitute(a:dirname,dirpat,'\3','')
  let path    = substitute(a:dirname,dirpat,'\4','')
  let fname   = substitute(a:dirname,'^.*/\ze.','','')
"  call Decho("set up method <".method .">")
"  call Decho("set up user   <".user   .">")
"  call Decho("set up machine<".machine.">")
"  call Decho("set up path   <".path   .">")
"  call Decho("set up fname  <".fname  .">")

  if method == "ftp"
   let listcmd = "-lF"
  else
   let listcmd = substitute(g:netrw_list_cmd,'\<HOSTNAME\>',user.machine,'')
  endif
  if exists("b:netrw_method")
"   call Decho("setting s:netrw_method<".b:netrw_method.">")
   let s:netrw_method= b:netrw_method
  endif

  " optionally sort by time (-t) or by size (-S)
  if g:netrw_sort_by =~ "^t"
   let listcmd= listcmd."t"
  elseif g:netrw_sort_by =~ "^s"
   let listcmd= listcmd."S"
  endif
  " optionally sort in reverse
  if g:netrw_sort_direction =~ "^r"
   let listcmd= listcmd."r"
  endif

"  call Decho("set up listcmd<".listcmd.">")
  if fname =~ '@$' && fname !~ '^"'
"   call Decho("attempt transfer of symlink as file")
   call s:NetBrowse(substitute(a:dirname,'@$','','e'))
   redraw!
"   call Dret("NetBrowse : symlink")
   return

  elseif fname !~ '/$' && fname !~ '^"'
   " looks like a regular file, attempt transfer
"   call Decho("attempt transfer as regular file<".a:dirname.">")

   " remove any filetype indicator from end of dirname, except for the
   " "this is a directory" indicator (/).  There shouldn't be one of those,
   " anyway.
   let path= substitute(path,'[*=@|]$','','e')
"   call Decho("new path<".path.">")

   " remote-read the requested file into current buffer
   enew!
   exe "file ".method."://".user.machine."/".escape(path,s:netrw_cd_escape)
   exe "silent doau BufReadPre ".fname
   silent call s:NetRead(method."://".user.machine."/".path)
   exe "silent doau BufReadPost ".fname
   keepjumps 1d
   setlocal nomod

"   call Dret("NetBrowse : file<".fname.">")
   return
  endif

  " ---------------------------------------------------------------------
  "  Perform Directory Listing:
"  call Decho("Perform directory listing...")
  " set up new buffer and map
  let bufname   = method.'://'.user.machine.'/'.path
  let bufnamenr = bufexists(bufname)
"  call Decho("bufname<".bufname."> bufnamenr=".bufnamenr)
  if bufnamenr != 0
   " buffer already exists, switch to it!
"   call Decho("buffer already exists, switching to it")
   exe "b ".bufnamenr
   if line("$") >= 5
"    call Dret("NetBrowse")
    return
   endif
  else
"   call Decho("generate a new buffer")
   enew!
  endif

  " rename file to reflect where its from
  setlocal ts=32 bt=nofile bh=wipe nobl
  exe 'file '.escape(bufname,s:netrw_cd_escape)
"  call Decho("renaming file to bufname<".bufname.">")
  setlocal bt=nowrite bh=hide nobl

  " set up buffer-local mappings
"  call Decho("set up buffer-local mappings")
  nnoremap <buffer> <silent> <cr>	:exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> <c-l>	:exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'../'))<cr>
  nnoremap <buffer> <silent> a		:let g:netrw_hide=!g:netrw_hide<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  nnoremap <buffer> <silent> b		:<c-u>call <SID>NetBookmarkDir(0,expand("%"))<cr>
  nnoremap <buffer> <silent> B		:<c-u>call <SID>NetBookmarkDir(1,expand("%"))<cr>
  nnoremap <buffer> <silent> <c-h>	:call <SID>NetHideEdit(0)<cr>
  nnoremap <buffer> <silent> i		:call <SID>NetLongList(0)<cr>
  nnoremap <buffer> <silent> o		:exe g:netrw_winsize."wincmd s"<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> q		:<c-u>call <SID>NetBookmarkDir(2,expand("%"))<cr>
  nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  nnoremap <buffer> <silent> s		:call <SID>NetSaveWordPosn()<bar>let g:netrw_sort_by= (g:netrw_sort_by =~ 'n')? 'time' : (g:netrw_sort_by =~ 't')? 'size' : 'name'<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<bar>call <SID>NetRestoreWordPosn()<cr>
  nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(0)<cr>
  nnoremap <buffer> <silent> v		:exe g:netrw_winsize."wincmd v"<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> x		:exe "norm! 0"<bar>call <SID>NetBrowseX(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()),1)<cr>
  nnoremap <buffer> <silent> <2-leftmouse> :exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),<SID>NetGetWord()))<cr>
  exe 'nnoremap <buffer> <silent> <del>	:exe "norm! 0"<bar>call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> <del>	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'nnoremap <buffer> <silent> d	:call <SID>NetMakeDir("'.user.machine.'")<cr>'
  exe 'nnoremap <buffer> <silent> D	:exe "norm! 0"<bar>call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> D	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'nnoremap <buffer> <silent> R	:exe "norm! 0"<bar>call <SID>NetBrowseRename("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> R	:call <SID>NetBrowseRename("'.user.machine.'","'.path.'")<cr>'
  nnoremap <buffer> ?			:he netrw-browse-cmds<cr>
  setlocal ma

  " Set up the banner
"  call Decho("set up the banner: sortby<".g:netrw_sort_by."> method<".method.">")
  keepjumps put ='\" ==========================================================================='
  keepjumps put ='\" Netrw Remote Directory Listing'
  keepjumps put ='\"   '.bufname
  let s:netrw_bannercnt= 7
  let sortby= g:netrw_sort_by
  if g:netrw_sort_direction =~ "^r"
   let sortby= sortby." reversed"
  endif

  if g:netrw_sort_by =~ "^n"
   " sorted by name
   let s:netrw_bannercnt= s:netrw_bannercnt + 1
   keepjumps put ='\"   Sorted by      '.sortby
   keepjumps put ='\"   Sort sequence: '.g:netrw_sort_sequence
  else
   " sorted by size or date
   keepjumps put ='\"   Sorted by '.sortby
  endif
  if g:netrw_list_hide != "" && g:netrw_hide
   keepjumps put ='\"   Hiding: '.g:netrw_list_hide
   let s:netrw_bannercnt= s:netrw_bannercnt + 1
  endif
  keepjumps put ='\"   Quick Help:    ?:help  -:go up dir  D:delete  R:rename  s:sort-by  x:exec'
  keepjumps put ='\" ==========================================================================='

  " remote read the requested directory listing
  " Use ftp if that was the file-transfer method selected, otherwise use ssh
  " Note that not all ftp servers honor the options for ls
  if method == "ftp"
   call NetBrowseFtpCmd(path,"ls ".listcmd)
   keepjumps 1d

   if !g:netrw_longlist
"    call Decho("generate short listing")
    " shorten the listing
    exe "keepjumps ".s:netrw_bannercnt
    " cleanup
    while getline(".") =~ g:netrw_ftp_browse_reject
     keepjumps d
    endwhile
    keepjumps put='../'
    keepjumps put='./'
    exe 'keepjumps silent '.s:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2/e'
    exe "keepjumps silent ".s:netrw_bannercnt.',$g/ -> /s# -> .*/$#/#'
    exe "keepjumps silent ".s:netrw_bannercnt.',$g/ -> /s# -> .*$#/#'
   endif

  else
"   call Decho("use ssh")
   let shq= &shq? &shq : ( &sxq? &sxq : "'")
"   call Decho("exe silent r! ".listcmd." ".shq.escape(path,s:netrw_cd_escape).shq)
   exe "silent r! ".listcmd." ".shq.escape(path,s:netrw_cd_escape).shq
   keepjumps 1d
  endif

  " manipulate the directory listing (hide, sort)
  setlocal ft=netrwlist
  if line("$") >= s:netrw_bannercnt
   if g:netrw_hide && g:netrw_list_hide != ""
    call s:NetrwListHide()
   endif

   if g:netrw_longlist
    " do a long listing; these substitutions need to be done prior to sorting
"    call Decho("manipulate long listing")

    if method == "ftp"
     " cleanup
     exe "keepjumps ".s:netrw_bannercnt
     while getline(".") =~ '^total\s\+\d\+$' || getline(".") =~ 'Trying\s\+\d\+.*$'
      keepjumps d
     endwhile
     exe 'keepjumps '.s:netrw_bannercnt."put='./'"
     exe 'keepjumps '.s:netrw_bannercnt."put='../'"
    endif

    exe 'keepjumps silent '.s:netrw_bannercnt.',$s/ -> .*$//e'
    exe 'keepjumps silent '.s:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2\t\1/e'
    exe s:netrw_bannercnt
   endif

   if g:netrw_sort_by =~ "^n"
    call s:SetSort()
    exe 'keepjumps silent '.s:netrw_bannercnt.',$call s:NetSort()'
    exe 'keepjumps silent '.s:netrw_bannercnt.',$s/^\d\{3}\///e'
   endif
   if g:netrw_longlist
    " shorten the list to keep its width <= 80 characters
    exe "keepjumps silent ".s:netrw_bannercnt.',$s/\t[-dstrwx]\+/\t/e'
   endif
  endif
  setlocal nomod
  setlocal noma

"  call Dret("NetBrowse")
  return
endfun

" ---------------------------------------------------------------------
" NetBrowseChgDir: {{{2
fun! <SID>NetBrowseChgDir(dirname,newdir)
"  call Dfunc("NetBrowseChgDir(dirname<".a:dirname."> newdir<".a:newdir.">)")

  let dirname= a:dirname
  let newdir = a:newdir

  if newdir !~ '/$'
   " handling a file
   let dirname= dirname.newdir
"   call Decho("handling a file: dirname<".dirname.">")

  elseif newdir == './'
   " refresh the directory list
"   call Decho("refresh directory listing")
   setlocal ma
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
fun! <SID>NetGetWord()
"  call Dfunc("NetGetWord() line#".line("."))
  let dirname= getline(".")
  if dirname =~ '\t'
   let dirname= substitute(dirname,'\t.*$','','e')
  endif
"  call Dret("NetGetWord <".dirname.">")
  return dirname
endfun

" ---------------------------------------------------------------------
" NetBrowseRm: remove/delete a remote file or directory {{{2
fun! <SID>NetBrowseRm(usrhost,path) range
"  call Dfunc("NetBrowseRm(usrhost<".a:usrhost."> path<".a:path.">)")
"  call Decho("firstline=".a:firstline." lastline=".a:lastline)

  " preparation for removing multiple files/directories
  let ctr= a:firstline

  " remove multiple files and directories
  while ctr <= a:lastline
   exe ctr

   norm! 0
   let rmfile= s:NetGetWord()
"   call Decho("rmfile<".rmfile.">")

   if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '/$')
    " attempt to remove file
    call inputsave()
    let ok= input("Confirm deletion of file<".rmfile."> ","y")
    call inputrestore()

    if ok == "y"
     if exists("s:netrw_method") && (s:netrw_method == 2 || s:netrw_method == 3)
      silent! keepjumps .,$d
      call NetBrowseFtpCmd(a:path,"delete ".rmfile)
     else
      let netrw_rm_cmd= substitute(g:netrw_rm_cmd,'HOSTNAME',a:usrhost,'').' "'.escape(a:path.rmfile,s:netrw_cd_escape).'"'
"      call Decho("attempt to remove file: system(".netrw_rm_cmd.")")
      let ret= system(netrw_rm_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)
     endif
    endif
  
   else
    " attempt to remove directory
    call inputsave()
    let ok= input("Confirm deletion of directory<".rmfile."> ","y")
    call inputrestore()

    if ok == "y"
     if exists("s:netrw_method") && (s:netrw_method == 2 || s:netrw_method == 3)
      call NetBrowseFtpCmd(a:path,"rmdir ".rmfile)
     else
      let rmfile         = a:path.rmfile
      let netrw_rmdir_cmd= substitute(g:netrw_rmdir_cmd,'HOSTNAME',a:usrhost,'').' '."'".'"'.rmfile.'"'."'"
"      call Decho("attempt to remove dir: system(".netrw_rmdir_cmd.")")
      let ret= system(netrw_rmdir_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)

      if v:shell_error != 0
       let netrw_rmf_cmd= substitute(g:netrw_rmf_cmd,'HOSTNAME',a:usrhost,'').' '.substitute(rmfile,'/$','','e')
"       call Decho("2nd attempt to remove dir: system(".netrw_rmf_cmd.")")
       let ret= system(netrw_rmf_cmd)
"       call Decho("returned=".ret." errcode=".v:shell_error)
     
       if v:shell_error != 0
        echoerr "unable to remove directory<".rmfile."> -- is it empty?"
       endif
      endif
     endif
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
fun! <SID>NetBrowseRename(usrhost,path) range
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

   if exists("s:netrw_method") && (s:netrw_method == 2 || s:netrw_method == 3)
    call NetBrowseFtpCmd(a:path,"rename ".oldname." ".newname)
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
" NetBrowseX:  allows users to write custom functions to operate on {{{2
"              files given their extension.  Passes 0=local, 1=remote
fun! <SID>NetBrowseX(fname,remote)
"  call Dfunc("NetBrowseX(".a:fname." remote=".a:remote.")")

  let exten= substitute(a:fname,'.*\.\(.\{-}\)','\1','e')
  if has("win32") || has("win95") || has("win64") || has("win16")
   let exten= substitute(exten,'^.*$','\L&\E','')
  endif
"  call Decho("exten<".exten."> "."NetrwFileHandler_".exten."():exists=".exists("*NetrwFileHandler_".exten))
  if exten != "" && exists("*NetrwFileHandler_".exten)

   let fname= a:fname
   if a:remote == 1
    " create a local copy
    let fname= tempname().".".exten
"    call Decho("create a local copy of <".a:fname."> as <".fname.">")
    exe "keepjumps silent bot 1new ".a:fname
    let eikeep= &ei
    set ei=all bh=delete
    exe "w! ".fname
    let &ei= eikeep
    q
   endif

   exe "let ret= NetrwFileHandler_".exten.'("'.fname.'")'
    redraw!

   " cleanup: remove temporary file,
   "          delete current buffer if success with handler,
   "          return to prior buffer (directory listing)
   if a:remote == 1 && fname != a:fname
"    call Decho("deleting temporary file<".fname.">")
    call delete(fname)
   endif
   if ret != 0
    let eikeep= &ei
    set ei=all bh=delete bt=nofile
    exe "norm! \<c-o>"
    let &ei= eikeep
    redraw!
   endif
  endif

"  call Dret("NetBrowseX")
endfun

" ---------------------------------------------------------------------
" NetBrowseFtpCmd: unfortunately, not all ftp servers honor options for ls {{{2
"  This function assumes that a long listing will be received.  Size, time,
"  and reverse sorts will be requested of the server but not otherwise
"  enforced here.
fun! NetBrowseFtpCmd(path,cmd)
"  call Dfunc("NetBrowseFtpCmd(path<".a:path."> cmd<".a:cmd.">) netrw_method=".s:netrw_method)

  " because WinXX ftp uses unix style input
  " curline is one more than the bannercnt in order to account
  " for the unwanted first blank line (doing a :put to an empty
  " buffer yields a blank first line)
  let ffkeep= &ff
  setlocal ma
  setlocal ff=unix
  let curline= s:netrw_bannercnt+1
  exe "silent! keepjumps ".curline.",$d"

   ".........................................
  if s:netrw_method == 2
   " ftp + <.netrc>:  Method #2
   if a:path != ""
    put ='cd '.a:path
   endif
   exe "put ='".a:cmd."'"

   if exists("g:netrw_port") && g:netrw_port != ""
"    call Decho("exe ".g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
    exe g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
   else
"    call Decho("exe ".g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
    exe g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine
   endif

   ".........................................
  elseif s:netrw_method == 3
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

  " restore settings
  let &ff= ffkeep
"  call Dret("NetBrowseFtpCmd")
endfun

" ---------------------------------------------------------------------
" NetrwListHide: uses [range]g~...~d to delete files that match comma {{{2
" separated patterns given in g:netrw_list_hide
fun! <SID>NetrwListHide()
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
"   call Decho("pruning <".hide."> listhide<".listhide.">")
   exe 'keepjumps silent '.s:netrw_bannercnt.',$g~'.hide.'~d'
  endwhile

"  call Dret("NetrwListHide")
endfun

" ---------------------------------------------------------------------
" NetHideEdit: allows user to edit the file/directory hiding list
fun! <SID>NetHideEdit(mode)
"  call Dfunc("NetHideEdit(mode=".a:mode.")")

  call inputsave()
  let newhide= input("Edit Hiding List: ",g:netrw_list_hide)
  call inputrestore()

  " refresh the listing
  let g:netrw_list_hide= newhide
  if a:mode == 0
   silent call s:NetBrowse(s:NetBrowseChgDir(expand("%"),'./'))
  else
   silent call s:LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,"./"))
  endif

"  call Dret("NetHideEdit")
endfun

" ---------------------------------------------------------------------
" NetSortSequence: allows user to edit the sorting sequence
fun! <SID>NetSortSequence(mode)
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
fun! <SID>NetLongList(mode)
"  call Dfunc("NetLongList(mode=".a:mode.") netrw_longlist=".g:netrw_longlist)
  call s:NetSavePosn()

  if g:netrw_longlist != 0
   " turn long listing off
   let g:netrw_longlist = 0
   let g:netrw_list_cmd = substitute(g:netrw_list_cmd,' -l','','ge')

  else
   " turn long listing on
   let g:netrw_longlist = 1
   let g:netrw_list_cmd = g:netrw_list_cmd." -l"
  endif

  " refresh the listing
  if a:mode == 0
   silent call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),"./"))
  else
   silent call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,"./"))
  endif

  call s:NetRestorePosn()
"  call Dret("NetLongList : g:netrw_longlist=".g:netrw_longlist)
endfun

" ---------------------------------------------------------------------
" NetSaveWordPosn: used by the "s" command in both remote and local
" browsing.  Along with NetRestoreWordPosn(), it keeps the cursor on
" the same word even though the sorting has changed its order of appearance.
fun! s:NetSaveWordPosn()
"  call Dfunc("NetSaveWordPosn()")
  let s:netrw_saveword= '^'.escape(getline("."),s:netrw_cd_escape).'$'
"  call Dret("NetSaveWordPosn : saveword<".s:netrw_saveword.">")
endfun

" ---------------------------------------------------------------------
" NetRestoreWordPosn: used by the "s" command; see NetSaveWordPosn() above
fun! s:NetRestoreWordPosn()
"  call Dfunc("NetRestoreWordPosn()")
  silent! call search(s:netrw_saveword,'w')
"  call Dret("NetRestoreWordPosn")
endfun

" ---------------------------------------------------------------------
" NetMakeDir: this function makes a directory (both local and remote)
fun! <SID>NetMakeDir(usrhost)
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
    echoerr "***warning*** <".newdirname."> is already a directory!"
"    call Dret("NetMakeDir : directory<".newdirname."> exists previously")
    return
   endif
   if filereadable(fullnewdir)
    echoerr "***warning*** <".newdirname."> is already a file!"
"    call Dret("NetMakeDir : file<".newdirname."> exists previously")
    return
   endif
   let netrw_origdir= s:NetGetcwd(1)
   exe 'cd '.b:netrw_curdir
"   call Decho("netrw_origdir<".netrw_origdir."> b:netrw_curdir<".b:netrw_curdir.">")
"   call Decho("exe silent! !".g:netrw_local_mkdir.' "'.newdirname.'"')
   exe "silent! !".g:netrw_local_mkdir.' "'.newdirname.'"'
   if g:netrw_keepdir | exe 'keepjumps cd '.netrw_origdir | endif

   if v:shell_error == 0
    " refresh listing
"    call Decho("refresh listing")
    let linenum= line(".")
    norm! H0
    let hline  = line(".")
    call s:LocalBrowse(s:LocalBrowseChgDir(b:netrw_curdir,'./'))
    exe "norm! ".hline."G0z\<CR>"
    exe linenum
   else
    echoerr "***warning*** unable to make directory<".newdirname.">"
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
   else
    echoerr "***warning*** unable to make directory<".newdirname.">"
   endif
   redraw!
  endif
  
"  call Dret("NetMakeDir")
endfun

" ---------------------------------------------------------------------
"  NetBookmarkDir:
"    0: bookmark the current directory
"    1: change to the bookmarked directory
fun! <SID>NetBookmarkDir(chg,curdir)
"  call Dfunc("NetBookmarkDir(chg=".a:chg." curdir<".a:curdir.">) cnt=".v:count)

  if a:chg == 0
   " bookmark the current directory
   let s:netrw_bookmarkdir_{v:count}= a:curdir
   if !exists("s:bookmarkmax")
    let s:bookmarkmax= v:count
   elseif v:count > s:bookmarkmax
    let s:bookmarkmax= v:count
   endif

  elseif a:chg == 1
   " change to the bookmarked directory
   if exists("s:netrw_bookmarkdir_{v:count}")
    exe "e ".s:netrw_bookmarkdir_{v:count}
   else
    echomsg "Sorry, bookmark#".v:count." doesn't exist!"
   endif

  elseif exists("s:bookmarkmax")
   " list bookmarks
"   call Decho("list bookmarks [0,".s:bookmarkmax."]")
   let cnt= 0
   while cnt <= s:bookmarkmax
    if exists("s:netrw_bookmarkdir_{cnt}")
     echo "Netrw Bookmark#".cnt.": ".s:netrw_bookmarkdir_{cnt}
"     call Decho("Netrw Bookmark#".cnt.": ".s:netrw_bookmarkdir_{cnt})
    endif
    let cnt= cnt + 1
   endwhile
  endif
"  call Dret("NetBookmarkDir")
endfun


" ---------------------------------------------------------------------
"  Browsing Support For Local Directories And Files:    {{{1

" ---------------------------------------------------------------------
" LocalBrowse: supports local file/directory browsing {{{2
fun! <SID>LocalBrowse(dirname)

"  let dirname= (a:dirname == "")? expand("%:p") : a:dirname
  if !isdirectory(a:dirname)
   " not a directory, ignore it
   return
  endif

  " unfortunate interaction -- when putting debugging calls
  " above one can no longer enter the DBG buffer.
"  call Dfunc("LocalBrowse(dirname<".a:dirname.">) buf#".bufnr("%")." winnr=".winnr())
"  call Decho("winbufnr1=".winbufnr(1)." winbufnr2=".winbufnr(2)." winbufnr3=".winbufnr(3))
"  call Dredir("ls!")

  if v:version < 603
   echoerr "vim version<".v:version."> too old for browsing with netrw"
"   call Dret("LocalBrowse : vim version<".v:version."> too old")
   return
  endif

  " record and change current directory
  let netrw_origdir= s:NetGetcwd(1)
  exe 'cd '.escape(substitute(a:dirname,'\\','/','ge'),s:netrw_cd_escape)
  let dirname= s:NetGetcwd(0)
"  call Decho("dirname<".dirname."> buf#".bufnr("%")." winnr=".winnr())

  " make this buffer modifiable
  setlocal ma

  " ---------------------------
  "  Perform Directory Listing:
"  call Decho("Perform directory listing...")
  " set up new buffer and map
  " remove the trailing "/"
  let dirnamens= substitute(dirname,'/$','','e')
  let dirnamenr= bufnr(dirnamens.'$')
"  call Decho("dirnamenr= bufnr(".dirnamens.")=".dirnamenr)

  if dirnamenr != 0 && bufname(dirnamenr) != dirnamens
   " try keeping the trailing slash
   let dirnamenr = bufnr(dirname.'$')
"   call Decho("retry: dirnamenr= bufnr(".dirname.")=".dirnamenr)
  endif
"  call Decho("bufnr(dirname<".dirname.">)=".dirnamenr)

  if dirnamenr != -1
   " buffer already exists (hidden), so switch to it!
"   call Decho("buffer already exists: dirnamenr=".dirnamenr." dirname<".dirname."> pre-exists")
"   call Dredir("ls!")
   exe "b ".dirnamenr
   exe 'silent! cd '.escape(substitute(a:dirname,'\\','/','ge'),s:netrw_cd_escape)
"   call Decho("changed directory to<".dirname.">")
   if a:dirname != "." && line("$") >= 5
"    call Dret("LocalBrowse : buffer already exists with info, #".dirnamenr)
    if g:netrw_keepdir | exe 'keepjumps cd '.netrw_origdir | endif
    return
   endif
"   call Decho("buffer already exists, but needs listing (buf#".dirnamenr.")")
   setlocal ma
   keepjumps %d
   if expand("%:p") != dirname
    exe 'silent! file '.escape(dirname,s:netrw_cd_escape)
"    call Decho("renamed file to<".escape(dirname,' #').">")
   endif
  else
"   call Decho("generate new buffer named<".escape(dirname,' #').">")
   silent! enew!
   exe 'silent! file '.substitute(escape(dirname,s:netrw_cd_escape),'/$','','e')
"   call Decho("renamed file to<".escape(dirname,s:netrw_cd_escape).">")
  endif
  " set standard browser options on buffer
  setlocal ts=32 bt=nowrite bh=hide nobl

  " set up all the maps
"  call Decho("Setting up local browser maps")
  let b:netrw_curdir= s:NetGetcwd(1)
  nnoremap <buffer> <silent> <cr>	:exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> <c-l>	:exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<cr>
  nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'../'))<cr>
  nnoremap <buffer> <silent> a		:let g:netrw_hide=!g:netrw_hide<bar>exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<cr>
  nnoremap <buffer> <silent> b		:<c-u>call <SID>NetBookmarkDir(0,b:netrw_curdir)<cr>
  nnoremap <buffer> <silent> B		:<c-u>call <SID>NetBookmarkDir(1,b:netrw_curdir)<cr>
  nnoremap <buffer> <silent> c		:exe "cd ".b:netrw_curdir<cr>
  nnoremap <buffer> <silent> d		:call <SID>NetMakeDir("")<cr>
  nnoremap <buffer> <silent> <c-h>	:call <SID>NetHideEdit(1)<cr>
  nnoremap <buffer> <silent> i		:call <SID>NetLongList(1)<cr>
  nnoremap <buffer> <silent> o		:exe g:netrw_winsize."wincmd s"<bar>exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> q		:<c-u>call <SID>NetBookmarkDir(2,b:netrw_curdir)<cr>
  nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<cr>
  nnoremap <buffer> <silent> s		:call <SID>NetSaveWordPosn()<bar>let g:netrw_sort_by= (g:netrw_sort_by =~ 'n')? 'time' : (g:netrw_sort_by =~ 't')? 'size' : 'name'<bar>exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))<bar>call <SID>NetRestoreWordPosn()<cr>
  nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(1)<cr>
  nnoremap <buffer> <silent> v		:exe g:netrw_winsize."wincmd v"<bar>exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))<cr>
  nnoremap <buffer> <silent> x		:exe "norm! 0"<bar>call <SID>NetBrowseX(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord(),0),0)<cr>
  nnoremap <buffer> <silent> <2-leftmouse> :exe "norm! 0"<bar>call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,<SID>NetGetWord()))<cr>
  exe 'nnoremap <buffer> <silent> <del>	:exe "norm! 0"<bar>call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'vnoremap <buffer> <silent> <del>	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'nnoremap <buffer> <silent> D	:exe "norm! 0"<bar>call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'vnoremap <buffer> <silent> D	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
  exe 'nnoremap <buffer> <silent> R	:exe "norm! 0"<bar>call <SID>LocalBrowseRename("'.b:netrw_curdir.'")<cr>'
  exe 'vnoremap <buffer> <silent> R	:call <SID>LocalBrowseRename("'.b:netrw_curdir.'")<cr>'
  exe 'nnoremap <buffer> <silent> <Leader>m :call <SID>NetMakeDir("")<cr>'
  nnoremap <buffer> ?			:he netrw-dir<cr>

  " Set up the banner
"  call Decho("set up banner")
  keepjumps put ='\" ============================================================================'
  keepjumps 1d
  keepjumps put ='\" Directory Listing                                              (netrw '.g:loaded_netrw.')'
  keepjumps put ='\"   '.dirname
  let s:netrw_bannercnt= 7

  let sortby= g:netrw_sort_by
  if g:netrw_sort_direction =~ "^r"
   let sortby= sortby." reversed"
  endif

  if g:netrw_sort_by =~ "^n"
"   call Decho("directories will be sorted by name")
   " sorted by name
   let s:netrw_bannercnt= s:netrw_bannercnt + 1
   keepjumps put ='\"   Sorted by      '.sortby
   keepjumps put ='\"   Sort sequence: '.g:netrw_sort_sequence
  else
"   call Decho("directories will be sorted by size or date")
   " sorted by size or date
   keepjumps put ='\"   Sorted by '.sortby
  endif
  if g:netrw_list_hide != "" && g:netrw_hide
   keepjumps put ='\"   Hiding: '.g:netrw_list_hide
   let s:netrw_bannercnt= s:netrw_bannercnt + 1
  endif
  keepjumps put ='\"   Quick Help:    ?:help  -:go up dir  D:delete  R:rename  s:sort-by  x:exec'
  keepjumps put ='\" ============================================================================'

  " generate the requested directory listing
  call LocalBrowseList(dirname)

  " manipulate the directory listing (hide, sort)
  setlocal ft=netrwlist
  if line("$") >= s:netrw_bannercnt
   if g:netrw_hide && g:netrw_list_hide != ""
    call s:NetrwListHide()
   endif
   if g:netrw_sort_by =~ "^n"
    call s:SetSort()
    exe 'keepjumps silent '.s:netrw_bannercnt.',$call s:NetSort()'
    exe 'keepjumps silent '.s:netrw_bannercnt.',$s/^\d\{3}\///e'
   else
    exe 'keepjumps silent '.s:netrw_bannercnt.',$call s:NetSort()'
    exe 'keepjumps silent '.s:netrw_bannercnt.',$s/^\d\{-}\///e'
   endif
  endif
  exe s:netrw_bannercnt

  setlocal noma nomod
  if g:netrw_keepdir | exe 'keepjumps cd '.netrw_origdir | endif

"  call Dret("LocalBrowse : file<".expand("%:p").">")
endfun

" ---------------------------------------------------------------------
"  LocalBrowseList: does the job of "ls" for local directories {{{2
fun! LocalBrowseList(dirname)
"  call Dfunc("LocalBrowseList(dirname<".a:dirname.">)")

  let dirname    = escape(a:dirname,s:netrw_glob_escape)
  let dirnamelen = strlen(a:dirname)
  let filelist   = glob(dirname."*")
  if filelist != ""
   let filelist= filelist."\n"
  endif
  let filelist= filelist.glob(dirname.".*")
"  call Decho("dirname<".dirname.">")
"  call Decho("dirnamelen<".dirnamelen.">")
"  call Decho("filelist<".filelist.">")

  while filelist != ""
   if filelist =~ '\n'
    let file     = substitute(filelist,'\n.*$','','e')
    let filelist = substitute(filelist,'^.\{-}\n\(.*\)$','\1','e')
   else
    let file     = filelist
    let filelist= ""
   endif
   let pfile= file
   if isdirectory(file)
    let pfile= file."/"
   endif
   let pfile= strpart(pfile,dirnamelen)
   if g:netrw_longlist
    let sz   = getfsize(file)
    let fsz  = strpart("               ",1,15-strlen(sz)).sz
    let pfile= pfile."\t".fsz." ".strftime(g:netrw_timefmt,getftime(file))
   endif

   if     g:netrw_sort_by =~ "^t"
    " sort by time (handles time up to 1 quintillion seconds, US)
"    call Decho("getftime(".file.")=".getftime(file))
    let t  = getftime(file)
    let ft = strpart("000000000000000000",1,18-strlen(t)).t
"    call Decho("exe keepjumps put ='".ft.'/'.file."'")
    let ftpfile= ft.'/'.pfile
    keepjumps put=ftpfile

   elseif g:netrw_sort_by =~ "^s"
    " sort by size (handles file sizes up to 1 quintillion bytes, US)
"    call Decho("getfsize(".file.")=".getfsize(file))
    let sz   = getfsize(file)
    let fsz  = strpart("000000000000000000",1,18-strlen(sz)).sz
"    call Decho("exe keepjumps put ='".fsz.'/'.file."'")
    let fszpfile= fsz.'/'.pfile
    keepjumps put =fszpfile

   else 
    " sort by name
"    call Decho("exe keepjumps put ='".pfile."'")
    keepjumps put=pfile
   endif
  endwhile

"  call Dret("LocalBrowseList")
endfun

" ---------------------------------------------------------------------
"  LocalBrowseChgDir: constructs a new directory based on the current {{{2
"                     directory and a new directory name
fun! <SID>LocalBrowseChgDir(dirname,newdir,...)
"  call Dfunc("LocalBrowseChgDir(dirname<".a:dirname."> newdir<".a:newdir.">) a:0=".a:0)

  let dirname= substitute(a:dirname,'\\','','ge')
  let newdir = a:newdir

  if dirname !~ '/$'
   " apparently vim is "recognizing" that it is in the home directory and
   " is removing the "/".  Bad idea, so I have to put it back.
   let dirname= dirname.'/'
"   call Decho("adjusting dirname<".dirname.">")
  endif

  if newdir !~ '/$'
   " handling a file
   let dirname= dirname.newdir
"   call Decho("handling a file: dirname<".dirname.">")
   " this lets NetBrowseX avoid the edit
   if a:0 < 1
    exe "e ".escape(dirname,s:netrw_cd_escape)
   endif

  elseif newdir == './'
   " refresh the directory list
"   call Decho("refresh directory listing")
   setlocal ma
   %d

  elseif newdir == '../'
   " go up one directory
   let dirname= substitute(dirname,'^\(.*/\)\([^/]\+/$\)','\1','e')
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
" LocalBrowseRm:
fun! <SID>LocalBrowseRm(path) range
"  call Dfunc("LocalBrowseRm(path<".a:path.">)")
"  call Decho("firstline=".a:firstline." lastline=".a:lastline)

  " preparation for removing multiple files/directories
  let ctr           = a:firstline
  let ret           = 0
  let netrw_origdir = s:NetGetcwd(1)
  exe 'cd '.b:netrw_curdir

  " remove multiple files and directories
  while ctr <= a:lastline
   exe "keepjumps ".ctr

   " sanity checks
   if line(".") < s:netrw_bannercnt
    let ctr= ctr + 1
    continue
   endif
   let curword= s:NetGetWord()
   if curword == "./" || curword == "../"
    let ctr= ctr + 1
    continue
   endif

   norm! 0
   let rmfile= a:path.curword
"   call Decho("rmfile<".rmfile.">")

   if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '/$')
    " attempt to remove file
    call inputsave()
    let ok= input("Confirm deletion of file<".rmfile."> ","y")
    call inputrestore()
    if ok == "y"
     let ret= delete(rmfile)
"     call Decho("errcode=".v:shell_error." ret=".ret)
    endif
  
   else
    " attempt to remove directory
    call inputsave()
    let ok= input("Confirm deletion of directory<".rmfile."> ","y")
    call inputrestore()
    let rmfile= substitute(rmfile,'/$','','e')

    if ok == "y"
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
        if v:shell_error != 0
         echoerr "unable to remove directory<".rmfile."> -- is it empty?"
	endif
       else
        echoerr "unable to remove directory<".rmfile."> -- is it empty?"
       endif
      endif
     endif
    endif
   endif

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")
  if g:netrw_keepdir | exe 'keepjumps cd '.netrw_origdir | endif
"  call Decho("refresh the directory")
  call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))
  exe curline

"  call Dret("LocalBrowseRm")
endfun

" ---------------------------------------------------------------------
" LocalBrowseRename: rename a remote file or directory {{{2
fun! <SID>LocalBrowseRename(path) range
"  call Dfunc("LocalBrowseRename(path<".a:path.">)")

  " preparation for removing multiple files/directories
  let ctr= a:firstline

  " attempt to rename files/directories
  while ctr <= a:lastline
   exe "keepjumps ".ctr

   " sanity checks
   if line(".") < s:netrw_bannercnt
    let ctr= ctr + 1
    continue
   endif
   let curword= s:NetGetWord()
   if curword == "./" || curword == "../"
    let ctr= ctr + 1
    continue
   endif

   norm! 0
   let oldname= a:path.curword
"   call Decho("oldname<".oldname.">")

   call inputsave()
   let newname= input("Moving ".oldname." to : ",substitute(oldname,'/*$','','e'))
   call inputrestore()

   let ret= system(g:netrw_local_rename.' "'.oldname.'" "'.newname.'"')
"   call Decho("executing system(".g:netrw_local_rename." ".oldname." ".newname)

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")
"  call Decho("refresh the directory listing")
  call <SID>LocalBrowse(<SID>LocalBrowseChgDir(b:netrw_curdir,'./'))
  exe "keepjumps ".curline
"  call Dret("LocalBrowseRename")
endfun

" ---------------------------------------------------------------------
" NetGetcwd: get the current directory.
"   Change backslashes to forward slashes, if any.
"   If doesc is true, escape certain troublesome characters
fun! <SID>NetGetcwd(doesc)
"  call Dfunc("NetGetcwd(doesc=".a:doesc.")")
  let curdir= substitute(getcwd(),'\\','/','ge')
  if curdir !~ '/$'
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
  let rcpurm   = '^rcp://\(\([^/@]\{-}\)@\)\=\([^/]\{-}\)/\(.*\)$'
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
   let b:netrw_method = 1
   let userid	     = substitute(a:choice,rcpurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,rcpurm,'\3',"")
   let b:netrw_fname  = substitute(a:choice,rcpurm,'\4',"")
   if userid != ""
    let g:netrw_uid= userid
   endif
 
  " scp://user@hostname/...path-to-file
  elseif match(a:choice,scpurm) == 0
"   call Decho("scp://...")
   let b:netrw_method = 4
   let g:netrw_machine= substitute(a:choice,scpurm,'\1',"")
   let b:netrw_port   = substitute(a:choice,scpurm,'\2',"")
   let b:netrw_fname  = substitute(a:choice,scpurm,'\3',"")
 
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
   let userid	     = substitute(a:choice,ftpurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,ftpurm,'\3',"")
   let g:netrw_port   = substitute(a:choice,ftpurm,'\4',"")
   let b:netrw_fname  = substitute(a:choice,ftpurm,'\5',"")
   if g:netrw_port != ""
     let g:netrw_port = substitute(g:netrw_port,"[#:]","","")
   endif
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
   echoerr "***error*** cannot determine method"
   let b:netrw_method  = -1
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
fun!s:NetOptionSave()
"  call Dfunc("NetOptionSave()")

  " Get Temporary Filename
  let s:aikeep	= &ai
  let s:cinkeep	= &cin
  let s:cinokeep = &cino
  let s:comkeep	= &com
  let s:cpokeep	= &cpo
  let s:dirkeep	= getcwd()
  let s:gdkeep	= &gd
  let s:twkeep	= &tw
  setlocal cino =
  setlocal com  =
  setlocal cpo -=aA
  setlocal nocin noai
  setlocal tw   =0
  if has("win32") && !has("win95")
   let s:swfkeep= &swf
   setlocal noswf
"  call Decho("setting s:swfkeep to <".&swf.">")
  endif

"  call Dret("NetOptionSave")
endfun

" ------------------------------------------------------------------------
" NetOptionRestore: restore options {{{1
fun! s:NetOptionRestore()
"  call Dfunc("NetOptionRestore()")
 
  let &ai	= s:aikeep
  let &cin	= s:cinkeep
  let &cino	= s:cinokeep
  let &com	= s:comkeep
  let &cpo	= s:cpokeep
  exe "lcd ".s:dirkeep
  let &gd	= s:gdkeep
  let &tw	= s:twkeep
  if exists("s:swfkeep")
   let &swf= s:swfkeep
   unlet s:swfkeep
  endif
  unlet s:aikeep
  unlet s:cinkeep
  unlet s:cinokeep
  unlet s:comkeep
  unlet s:cpokeep
  unlet s:gdkeep
  unlet s:twkeep
  unlet s:dirkeep
 
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
fun! <SID>NetSort() range
"  call Dfunc("NetSort()")

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

"  call Dret("NetSort")
endfun

" ---------------------------------------------------------------------
" SetSort: sets up the sort based on the g:netrw_sort_sequence {{{1
"          What this function does is to compute a priority for the patterns
"          in the g:netrw_sort_sequence.  It applies a substitute to any
"          "files" that satisfy each pattern, putting the priority / in
"          front.  An "*" pattern handles the default priority.
fun! <SID>SetSort()
"  call Dfunc("SetSort() bannercnt=".s:netrw_bannercnt)
  if g:netrw_longlist
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

   if seq == '*'
    exe 'keepjumps silent '.s:netrw_bannercnt.',$v/^\d\{3}\//s/^/'.spriority.'/'
   else
    exe 'keepjumps silent '.s:netrw_bannercnt.',$g/'.eseq.'/s/^/'.spriority.'/'
   endif
   let priority = priority + 1
  endwhile

  exe 'keepjumps silent '.s:netrw_bannercnt.',$s/^\(\d\{3}\/\)\%(\d\{3}\/\)\+/\1/e'

"  call Dret("SetSort")
endfun

" ------------------------------------------------------------------------
" Restore {{{1
let &cpo= s:save_cpo
unlet s:save_cpo
" vim:ts=8 fdm=marker
