" netrw.vim: Handles file transfer and remote directory listing across a network
" Last Change:	Jun 24, 2004
" Maintainer:	Charles E. Campbell, Jr. PhD   <drchipNOSPAM at campbellfamily.biz>
" Version:	47f	NOT RELEASED
" License:	Vim License  (see vim's :help license)
"
"  But be doers of the word, and not only hearers, deluding your own selves
"  (James 1:22 RSV)
" =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

" Exit quickly when already loaded or when 'compatible' is set. {{{1
if exists("loaded_netrw") || &cp
  finish
endif
let loaded_netrw = "v47f	NOT RELEASED"
let s:save_cpo   = &cpo
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
 if has("win32")
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
  call Decho("ssh is not executable, can't do remote directory exploring")
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

if has("win32")
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
" Auto-detection for ftp://*, rcp://*, scp://*, sftp://*, http://*, dav://*,
" and rsync://*
" Should make file transfers across networks transparent.  Currently I haven't
" supported appends.  Hey, gotta leave something for a future <netrw.vim>!
if version >= 600
 augroup Network
  au!
  if has("win32")
   au BufReadCmd  file://*		exe "silent doau BufReadPre ".expand("<afile>")|exe 'e '.substitute(expand("<afile>"),"file:/*","","")|exe "silent doau BufReadPost ".expand("<afile>")
  else
   au BufReadCmd  file:///*		exe "silent doau BufReadPre ".expand("<afile>")|exe 'e /'.substitute(expand("<afile>"),"file:/*","","")|exe "silent doau BufReadPost ".expand("<afile>")
   au BufReadCmd  file://localhost/*	exe "silent doau BufReadPre ".expand("<afile>")|exe 'e /'.substitute(expand("<afile>"),"file:/*","","")|exe "silent doau BufReadPost ".expand("<afile>")
  endif
  au BufReadCmd  ftp://*,rcp://*,scp://*,http://*,dav://*,rsync://*,sftp://*	exe "silent doau BufReadPre ".expand("<afile>")|exe "Nread 0r ".expand("<afile>")|exe "silent doau BufReadPost ".expand("<afile>")
  au FileReadCmd ftp://*,rcp://*,scp://*,http://*,dav://*,rsync://*,sftp://*	exe "silent doau BufReadPre ".expand("<afile>")|exe "Nread "   .expand("<afile>")|exe "silent doau BufReadPost ".expand("<afile>")
  au BufWriteCmd ftp://*,rcp://*,scp://*,dav://*,rsync://*,sftp://*    		exe "Nwrite "  .expand("<afile>")|call <SID>NetRestorePosn()
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
  call Dfunc("NetSavePosn()")
  " Save current line and column
  let s:netrw_winnr= winnr()
  let s:netrw_line = line(".")
  let s:netrw_col  = virtcol(".")

  " Save top-of-screen line
  norm! H0
  let s:netrw_hline= line(".")

  call s:NetRestorePosn()
  call Dret("NetSavePosn : winnr=".s:netrw_winnr." line=".s:netrw_line." col=".s:netrw_col." hline=".s:netrw_hline)
endfun

" ------------------------------------------------------------------------
" NetRestorePosn: restores the cursor and file position as saved by NetSavePosn() {{{1
fun! <SID>NetRestorePosn()
  call Dfunc("NetRestorePosn() winnr=".s:netrw_winnr." line=".s:netrw_line." col=".s:netrw_col." hline=".s:netrw_hline)

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

  call Dret("NetRestorePosn")
endfun

" ------------------------------------------------------------------------
" NetRead: responsible for reading a file over the net {{{1
fun! s:NetRead(...)
  call Dfunc("NetRead(a:1<".a:1.">)")
 
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
 
  " get name of a temporary file
  let tmpfile= tempname()
 
  call Decho("ichoice=".ichoice." readcmd<".readcmd.">")
  while ichoice <= a:0
 
   " attempt to repeat with previous host-file-etc
   if exists("b:netrw_lastfile") && a:0 == 0
    call Decho("using b:netrw_lastfile<" . b:netrw_lastfile . ">")
    let choice = b:netrw_lastfile
    let ichoice= ichoice + 1
 
   else
    exe "let choice= a:" . ichoice
    call Decho("no lastfile: choice<" . choice . ">")
 
    " Reconstruct Choice if choice starts with '"'
    if match(choice,"?") == 0
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
     call Decho("reconstructing choice")
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
        call Dret("NetRead")
        return
       endif
       let choice= a:{ichoice}
      endwhile
      let choice= strpart(wholechoice,1,strlen(wholechoice)-1) . " " . strpart(choice,0,strlen(choice)-1)
     endif
    endif
   endif

   call Decho("choice<" . choice . ">")
   let ichoice= ichoice + 1
 
   " fix up windows urls
   if has("win32")
    let choice = substitute(choice,'\\','/','ge')
    call Decho("fixing up windows url to <".choice."> tmpfile<".tmpfile)

    exe 'lcd ' . fnamemodify(tmpfile,':h')
    let tmpfile = fnamemodify(tmpfile,':t')
   endif
 
   " Determine method of read (ftp, rcp, etc)
   call s:NetMethod(choice)
 
   " Check if NetBrowse() should be handling this request
   call Decho("checking if netlist: choice<".choice."> netrw_list_cmd<".g:netrw_list_cmd.">")
   if choice =~ "^.*/$"
    if strlen(g:netrw_list_cmd) > 0
     keepjumps call s:NetBrowse(choice)
     call Dret("NetRead")
    else
     echoerr "sorry, can't do a remote listing; ssh isn't executable"
    endif
    return
   endif
 
   " ============
   " Perform Read
   " ============
 
   ".........................................
   " rcp:  NetRead Method #1
   if  b:netrw_method == 1 " read with rcp
    call Decho("read via rcp (method #1)")
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
   call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".uid_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
   exe g:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".uid_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile
   let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
   let b:netrw_lastfile = choice
 
   ".........................................
   " ftp + <.netrc>:  NetRead Method #2
   elseif b:netrw_method  == 2		" read with ftp + <.netrc>
    call Decho("read via ftp+.netrc (method #2)")
     let netrw_fname= b:netrw_fname
     new
     set ff=unix
     exe "put ='".g:netrw_ftpmode."'"
     exe "put ='get ".netrw_fname." ".tmpfile."'"
     if exists("g:netrw_port") && g:netrw_port != ""
      call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
      exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
     else
      call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
      exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i ".g:netrw_machine
     endif
     " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
     if getline(1) !~ "^$"
      echoerr getline(1)
     endif
     bd!
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
     let b:netrw_lastfile = choice
 
   ".........................................
   " ftp + machine,id,passwd,filename:  NetRead Method #3
   elseif b:netrw_method == 3		" read with ftp + machine, id, passwd, and fname
    " Construct execution string (four lines) which will be passed through filter
    call Decho("read via ftp+mipf (method #3)")
    let netrw_fname= b:netrw_fname
    new
    set ff=unix
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
    call Decho('performing ftp -i -n')
    norm 1Gdd
    call Decho("executing: %!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
     echoerr getline(1)
    endif
    bd!
    let result		= s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   " scp: NetRead Method #4
   elseif     b:netrw_method  == 4	" read with scp
    call Decho("read via scp (method #4)")
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " -P ".g:netrw_port
    else
     let useport= ""
    endif
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
     call Decho("executing: !".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
     exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile
    else
     call Decho("executing: !".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
     exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile
    endif
    let result		= s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   elseif     b:netrw_method  == 5	" read with http (wget)
    call Decho("read via http (method #5)")
    if g:netrw_http_cmd == ""
     echoerr "neither wget nor fetch command is available"
     exit
    endif
 
    if match(b:netrw_fname,"#") == -1
     " simple wget
     call Decho("executing: !".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.escape(b:netrw_fname,' ?&')
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
 
    else
     " wget plus a jump to an in-page marker (ie. http://abc/def.html#aMarker)
     let netrw_html= substitute(b:netrw_fname,"#.*$","","")
     let netrw_tag = substitute(b:netrw_fname,"^.*#","","")
     call Decho("netrw_html<".netrw_html.">")
     call Decho("netrw_tag <".netrw_tag.">")
     call Decho("executing: !".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_html)
     exe g:netrw_silentxfer."!".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_html
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
     call Decho('<\s*a\s*name=\s*"'.netrw_tag.'"/')
     exe 'norm! 1G/<\s*a\s*name=\s*"'.netrw_tag.'"/'."\<CR>"
    endif
    let b:netrw_lastfile = choice
 
   ".........................................
   " cadaver: NetRead Method #6
   elseif     b:netrw_method  == 6	" read with cadaver
    call Decho("read via cadaver (method #6)")
 
    " Construct execution string (four lines) which will be passed through filter
    let netrw_fname= b:netrw_fname
    new
    set ff=unix
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
    norm 1Gdd
    call Decho("executing: %!".g:netrw_dav_cmd)
    exe g:netrw_silentxfer."%!".g:netrw_dav_cmd
    bd!
    let result		= s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   " rsync: NetRead Method #7
   elseif     b:netrw_method  == 7	" read with rsync
    call Decho("read via rsync (method #7)")
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
     call Decho("executing: !".g:netrw_rsync_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
     exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile
    else
     call Decho("executing: !".g:netrw_rsync_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
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
    call Decho("read via fetch for ".netrw_option)
 
    if exists("g:netrw_uid") && g:netrw_uid != "" && exists("g:netrw_passwd") && g:netrw_passwd != ""
     call Decho("executing: !".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_uid.':'.g:netrw_passwd.'@'.g:netrw_machine."/".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_uid.':'.g:netrw_passwd.'@'.g:netrw_machine."/".escape(b:netrw_fname,' ?&')
    else
     call Decho("executing: !".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_machine."/".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_machine."/".escape(b:netrw_fname,' ?&')
    endif
 
    let result		= s:NetGetFile(readcmd,tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
 
   ".........................................
   " sftp: NetRead Method #9
   elseif     b:netrw_method  == 9	" read with sftp
    call Decho("read via sftp (method #4)")
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
     call Decho("!".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
     call Decho("executing: !".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile)
     exe "!".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".cygtmpfile
    else
     call Decho("executing: !".g:netrw_sftp_cmd." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')." ".tmpfile)
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
  call Decho("cleanup")
  if exists("b:netrw_method")
   unlet b:netrw_method
   unlet g:netrw_machine
   unlet b:netrw_fname
  endif
  call s:NetOptionRestore()
 
  call Dret("NetRead")
endfun
" end of NetRead

" ------------------------------------------------------------------------
" NetGetFile: Function to read file "fname" with command "readcmd". {{{1
fun! s:NetGetFile(readcmd, fname, method)
   call Dfunc("NetGetFile(readcmd<".a:readcmd.">,fname<".a:fname."> method<".a:method.">)")
 
  if exists("*NetReadFixup")
   " for the use of NetReadFixup (not otherwise used internally)
   let line2= line("$")
  endif
 
  " transform paths from / to \ for Windows, unless the shell is bash
  if &term == "win32"
   if &shell == "bash"
    let fname=a:fname
    call Decho("(win32 && bash) fname<".fname.">")
   else
    let fname=substitute(a:fname,'/','\\\\','ge')
    call Decho("(win32 && !bash) fname<".fname.">")
   endif
  else
   let fname= a:fname
   call Decho("(copied) fname<".fname.">")
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
    exe "f ".curfilename
    " the ":f newname" apparently leaves the temporary file as the alternate
    " file in the buffer list (see :ls!).  The following command wipes it out.
    exe bufnr("#")."bwipe!"
   else
    let oldul= &ul
    set ul=-1
    exe a:readcmd." ".v:cmdarg." ".fname
    if delline > 0
     " wipe out last line, which should be a blank line anyway
     $del
    endif
    let &ul= oldul
   endif
  elseif filereadable(fname)
   call Decho("exe<".a:readcmd." ".v:cmdarg." ".fname.">")
   exe a:readcmd." ".v:cmdarg." ".fname
  else
   call Dret("NetGetFile")
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
   call Decho("calling NetReadFixup(method<".a:method."> line1=".line1." line2=".line2.")")
   call NetReadFixup(a:method, line1, line2)
  endif
 
  call Decho("readcmd<".a:readcmd."> cmdarg<".v:cmdarg."> fname<".a:fname."> readable=".filereadable(a:fname))
 
 " insure that we have the right filetype and that its being displayed
  filetype detect
  redraw!
  call Dret("NetGetFile")
endfun

" ------------------------------------------------------------------------
" NetWrite: responsible for writing a file over the net {{{1
fun! s:NetWrite(...) range
  call Dfunc("NetWrite(a:0=".a:0.")")
 
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
    call Decho("using b:netrw_lastfile<" . b:netrw_lastfile . ">")
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
        call Dret("NetWrite")
        return
       endif
       let choice= a:{ichoice}
      endwhile
      let choice= strpart(wholechoice,1,strlen(wholechoice)-1) . " " . strpart(choice,0,strlen(choice)-1)
     endif
    endif
   endif
   call Decho("choice<" . choice . ">")
   let ichoice= ichoice + 1
 
   " fix up windows urls
   if has("win32")
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
 Decho "write via rcp (method #1)"
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
    call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".tmpfile." ".uid_machine.":".escape(b:netrw_fname,' ?&'))
    exe g:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".tmpfile." ".uid_machine.":".escape(b:netrw_fname,' ?&')
    let b:netrw_lastfile = choice
 
   ".........................................
   " ftp + <.netrc>: NetWrite Method #2
   elseif b:netrw_method == 2	" write with ftp + <.netrc>
    let netrw_fname = b:netrw_fname
    new
    set ff=unix
    exe "put ='".g:netrw_ftpmode."'"
    call Decho(" NetWrite: put ='".g:netrw_ftpmode."'")
    exe "put ='put ".tmpfile." ".netrw_fname."'"
    call Decho("put ='put ".tmpfile." ".netrw_fname."'")
    if exists("g:netrw_port") && g:netrw_port != ""
     call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
     exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
    else
     call Decho("executing: %!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
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
    set ff=unix
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
    call Decho('performing ftp -i -n')
    norm 1Gdd
    call Decho("executing: %!".g:netrw_ftp_cmd." -i -n")
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
     call Decho("executing: !".g:netrw_scp_cmd.useport." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')
    else
     call Decho("executing: !".g:netrw_scp_cmd.useport." ".tmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
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
    call Decho("write via cadaver (method #6)")
 
    " Construct execution string (four lines) which will be passed through filter
    let netrw_fname= b:netrw_fname
    new
    set ff=unix
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
    norm 1Gdd
    call Decho("executing: %!".g:netrw_dav_cmd)
    exe g:netrw_silentxfer."%!".g:netrw_dav_cmd
    bd!
    let b:netrw_lastfile = choice
 
   ".........................................
   " rsync: NetWrite Method #7
   elseif     b:netrw_method == 7	" write with rsync
    if g:netrw_cygwin == 1
     let cygtmpfile=substitute(tmpfile,'^\(\a\):','/cygdrive/\1/','e')
     call Decho("executing: !".g:netrw_rsync_cmd." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
     exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".cygtmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&')
    else
     call Decho("executing: !".g:netrw_rsync_cmd." ".tmpfile." ".g:netrw_machine.":".escape(b:netrw_fname,' ?&'))
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
    set ff=unix
    put ='put '.tmpfile.' '.netrw_fname
    norm 1Gdd
    call Decho("executing: %!".g:netrw_sftp_cmd.' '.uid_machine)
    exe g:netrw_silentxfer."%!".g:netrw_sftp_cmd.' '.uid_machine
    bd!
    let b:netrw_lastfile= choice
 
   ".........................................
   else " Complain
    echo "***warning*** unable to comply with your request<" . choice . ">"
   endif
  endwhile
 
  " cleanup
  call Decho("cleanup")
  let result=delete(tmpfile)
  call s:NetOptionRestore()
 
  if a:firstline == 1 && a:lastline == line("$")
   let &mod= mod	" usually equivalent to set nomod
  endif
 
  call Dret("NetWrite")
endfun
" end of NetWrite

" ------------------------------------------------------------------------
"  Remote Directory Browsing Support:    {{{1
" NetBrowse: This function uses the command in g:netrw_list_cmd to get a list {{{2
"  of the contents of a remote directory.  It is assumed that the
"  g:netrw_list_cmd has a string, HOSTNAME, that needs to be substituted
"  with the requested remote hostname first.
fun! <SID>NetBrowse(dirname)
  call Dfunc("NetBrowse(dirname<".a:dirname.">)")

  " sanity check
  if !exists("g:netrw_list_cmd") || g:netrw_list_cmd == ''
   echoerr "***netrw*** this system doesn't support remote directory listing"
   call Dret("NetBrowse 0")
   return 0
  endif

  " make this buffer modifiable
  setlocal ma

  " analyze a:dirname and g:netrw_list_cmd
  let dirpat  = '^\(\w\{-}\)://\(\w\+@\)\=\([^/]\+\)/\(.*\)$'
  if a:dirname !~ dirpat
   echoerr "NetBrowse: I don't understand your dirname<".a:dirname.">"
   call Dret("NetBrowse 0 : badly formatted dirname")
   return 0
  endif
  let method  = substitute(a:dirname,dirpat,'\1','')
  let user    = substitute(a:dirname,dirpat,'\2','')
  let machine = substitute(a:dirname,dirpat,'\3','')
  let path    = substitute(a:dirname,dirpat,'\4','')
  let fname   = substitute(a:dirname,'^.*/\ze.','','')
  call Decho("set up method <".method .">")
  call Decho("set up user   <".user   .">")
  call Decho("set up machine<".machine.">")
  call Decho("set up path   <".path   .">")
  call Decho("set up fname  <".fname  .">")

  let listcmd = substitute(g:netrw_list_cmd,'\<HOSTNAME\>',user.machine,'')
  call Decho("set up listcmd<".listcmd.">")

  if fname =~ '@$' && fname !~ '^"'
   call Decho("attempt transfer of symlink as file")
   call s:NetBrowse(substitute(a:dirname,'@$','','e'))
   redraw!
   call Dret("NetBrowse 0 : symlink")
   return 0

  elseif fname !~ '/$' && fname !~ '^"'
   " looks like a regular file, attempt transfer
   call Decho("attempt transfer as regular file<".a:dirname.">")

   " remove any filetype indicator from end of dirname, except for the
   " "this is a directory" indicator (/).  There shouldn't be one of those,
   " anyway.
   let path= substitute(path,'[*=@|]$','','e')
   call Decho("new path<".path.">")

   " remote-read the requested file into current buffer
   enew!
   exe "file ".method."://".user.machine."/".path
   exe "silent doau BufReadPre ".fname
   silent call s:NetRead(method."://".user.machine."/".path)
   exe "silent doau BufReadPost ".fname
   1d
   set nomod

   call Dret("NetBrowse 0 : file<".fname.">")
   return 0
  endif

  " ---------------------------------------------------------------------
  "  Perform Directory Listing:
  call Decho("Perform directory listing...")
  " set up new buffer and map
  let bufname   = method.'://'.user.machine.'/'.path
  let bufnamenr = bufnr(bufname)
  call Decho("bufname<".bufname."> bufnamenr=".bufnamenr)
  if bufnamenr != -1
   " buffer already exists, switch to it!
   exe "b ".bufnamenr
   if line("$") >= 5
    call Dret("NetBrowse 1")
    return 1
   endif
  else
   enew!
  endif
  setlocal bt=nofile bh=wipe nobl
  exe 'file '.bufname
  set bt=nowrite bh=hide nobl
  nnoremap <buffer> <silent> <cr>	:exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),expand("<cWORD>")))<cr>
  nnoremap <buffer> <silent> <c-l>	:exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'../'))<cr>
  nnoremap <buffer> <silent> a		:let g:netrw_list_showall=1<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))<cr>
  nnoremap <buffer> <silent> o		:exe g:netrw_winsize."wincmd s"<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),expand("<cWORD>")))<cr>
  nnoremap <buffer> <silent> v		:exe g:netrw_winsize."wincmd v"<bar>exe "norm! 0"<bar>call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),expand("<cWORD>")))<cr>
  nnoremap <buffer> <silent> x		:exe "norm! 0"<bar>call <SID>NetBrowseX(<SID>NetBrowseChgDir(expand("%"),expand("<cWORD>")))<cr>
  exe 'nnoremap <buffer> <silent> <del>	:exe "norm! 0"<bar>call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> <del>	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'nnoremap <buffer> <silent> D	:exe "norm! 0"<bar>call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'vnoremap <buffer> <silent> D	:call <SID>NetBrowseRm("'.user.machine.'","'.path.'")<cr>'
  exe 'nnoremap <buffer> <silent> R	:exe "norm! 0"<bar>call <SID>NetBrowseRename("'.user.machine.'","'.path.'")<cr>'
  nnoremap <buffer> ?		:he netrw-dir<cr>
  setlocal ma

  call Decho("executing: r! ".listcmd." '".path."'")
  keepjumps put ='\" =============================='
  keepjumps put ='\" Netrw Remote Directory Listing'
  keepjumps put ='\"   '.bufname
  let g:netrw_bannercnt= 5
  if g:netrw_list_hide != "" && !exists("g:netrw_list_showall")
   keepjumps put ='\"   Hiding: '.g:netrw_list_hide
   let g:netrw_bannercnt= g:netrw_bannercnt + 1
  endif
  keepjumps put ='\" =============================='
  exe "silent r! ".listcmd." '".path."'"
  keepjumps 1d
  set ft=netrwlist
  if line("$") >= g:netrw_bannercnt
   if !exists("g:netrw_list_showall")
    if g:netrw_list_hide != ""
     call s:NetrwListHide()
    endif
   else
    unlet g:netrw_list_showall
   endif
   exe 'keepjumps silent '.g:netrw_bannercnt.',$s/^\(.*\)\([/@]\)$/ \2\1/e'
   exe 'keepjumps silent '.g:netrw_bannercnt.',$call s:NetSort()'
   exe 'keepjumps silent '.g:netrw_bannercnt.',$s/^ \(.\)\(.*\)$/\2\1/e'
   exe 'keepjumps '.g:netrw_bannercnt
  endif
  let prvbuf= bufnr(bufname)
  if prvbuf != -1
   exe "silent! b ".prvbuf
  endif

  setlocal noma nomod

  call Dret("NetBrowse 1")
  return 1
endfun

" ---------------------------------------------------------------------
" NetBrowseChgDir: {{{2
fun! <SID>NetBrowseChgDir(dirname,newdir)
  call Dfunc("NetBrowseChgDir(dirname<".a:dirname."> newdir<".a:newdir.">)")

  let dirname= a:dirname
  let newdir = a:newdir

  if newdir !~ '/$'
   " handling a file
   let dirname= dirname.newdir
   call Decho("handling a file: dirname<".dirname.">")

  elseif newdir == './'
   " refresh the directory list
   call Decho("refresh directory listing")
   set ma
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
   call Decho("go up one dir: dirname<".dirname."> trailer<".trailer.">")

  else
   " go down one directory
   let dirname= dirname.newdir
   call Decho("go down one dir: dirname<".dirname."> newdir<".newdir.">")
  endif

  call Dret("NetBrowseChgDir <".dirname.">")
  return dirname
endfun

" ---------------------------------------------------------------------
" NetBrowseRm: remove a remote file or directory {{{2
fun! <SID>NetBrowseRm(usrhost,path) range
  call Dfunc("NetBrowseRm(usrhost<".a:usrhost."> path<".a:path.">)")
  call Decho("firstline=".a:firstline." lastline=".a:lastline)

  " preparation for removing multiple files/directories
  let ctr= a:firstline

  " remove multiple files and directories
  while ctr <= a:lastline
   exe ctr

   norm! 0
   let rmfile= a:path.expand("<cWORD>")
   call Decho("rmfile<".rmfile.">")

   if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '/$')
    " attempt to remove file
    let netrw_rm_cmd= substitute(g:netrw_rm_cmd,'HOSTNAME',a:usrhost,'').' '.rmfile
    call Decho("attempt to remove file: system(".netrw_rm_cmd.")")
    call inputsave()
    let ok= input("Confirm deletion of file<".rmfile."> ","y")
    call inputrestore()
    if ok == "y"
     let ret= system(netrw_rm_cmd)
     call Decho("returned=".ret." errcode=".v:shell_error)
    endif
  
   else
    " attempt to remove directory
    let netrw_rmdir_cmd= substitute(g:netrw_rmdir_cmd,'HOSTNAME',a:usrhost,'').' '.rmfile
    call Decho("attempt to remove dir: system(".netrw_rmdir_cmd.")")
    call inputsave()
    let ok= input("Confirm deletion of directory<".rmfile."> ","y")
    call inputrestore()

    if ok == "y"
     let ret= system(netrw_rmdir_cmd)
     call Decho("returned=".ret." errcode=".v:shell_error)

     if v:shell_error != 0
      let netrw_rmf_cmd= substitute(g:netrw_rmf_cmd,'HOSTNAME',a:usrhost,'').' '.substitute(rmfile,'/$','','e')
      call Decho("2nd attempt to remove dir: system(".netrw_rmf_cmd.")")
      let ret= system(netrw_rmf_cmd)
      call Decho("returned=".ret." errcode=".v:shell_error)

      if v:shell_error != 0
       echoerr "unable to remove directory<".rmfile."> -- is it empty?"
      endif
     endif
    endif
   endif

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")-1
  call Decho("refresh the directory")
  call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))
  exe curline

  call Dret("NetBrowseRm")
endfun

" ---------------------------------------------------------------------
" NetBrowseRename: rename a remote file or directory {{{2
fun! <SID>NetBrowseRename(usrhost,path)
  call Dfunc("NetBrowseRename(usrhost<".a:usrhost."> path<".a:path.">)")

  " preparation for removing multiple files/directories
  let ctr        = a:firstline
  let rename_cmd = substitute(g:netrw_rename_cmd,'\<HOSTNAME\>',a:usrhost,'')

  " attempt to rename files/directories
  while ctr <= a:lastline
   exe ctr

   norm! 0
   let oldname= a:path.expand("<cWORD>")
   call Decho("oldname<".oldname.">")

   call inputsave()
   let newname= input("Moving ".a:usrhost."'s ".oldname." to : ",oldname)
   call inputrestore()

   let ret= system(rename_cmd." ".oldname." ".newname)
   call Decho("executing system(".rename_cmd." ".oldname." ".newname)

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")
  call <SID>NetBrowse(<SID>NetBrowseChgDir(expand("%"),'./'))
  exe curline
  call Dret("NetBrowseRename")
endfun

" ---------------------------------------------------------------------
" NetBrowseX:  allows users to write custom functions to operate on {{{2
"              files given their extension
fun! <SID>NetBrowseX(fname)
  call Dfunc("NetBrowseX(".a:fname.")")

  let exten= substitute(a:fname,'.*\.\(.\{-}\)','\1','e')
  call Decho("exten<".exten."> "."NetrwFileHandler_".exten."():exists=".exists("*NetrwFileHandler_".exten))
  if exten != "" && exists("*NetrwFileHandler_".exten)
   exe "call NetrwFileHandler_".exten.'("'.a:fname.'")'
  endif
  call Dret("NetBrowseX")
endfun

" ---------------------------------------------------------------------
" NetrwListHide: uses [range]g~...~d to delete files that match comma {{{2
" separated patterns given in g:netrw_list_hide
fun! <SID>NetrwListHide()
  call Dfunc("NetrwListHide() listhide<".g:netrw_list_hide.">")

  let listhide= g:netrw_list_hide
  while listhide != ""
   if listhide =~ ','
    let hide     = substitute(listhide,',.*$','','e')
    let listhide = substitute(listhide,'^.\{-},\(.*\)$','\1','e')
   else
    let hide     = listhide
    let listhide= ""
   endif
   call Decho("pruning <".hide."> listhide<".listhide.">")
   exe 'keepjumps silent '.g:netrw_bannercnt.',$g~'.hide.'~d'
  endwhile

  call Dret("NetrwListHide")
endfun

" ------------------------------------------------------------------------
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
   call Dfunc("NetMethod(a:choice<".a:choice.">)")
 
  " initialization
  let b:netrw_method  = 0
  let g:netrw_machine = ""
  let b:netrw_fname   = ""
  let g:netrw_port    = ""
 
  " Patterns:
  " mipf     : a:machine a:id password filename	    Use ftp
  " mf	    : a:machine filename		    Use ftp + <.netrc> or g:netrw_uid g:netrw_passwd
  " ftpurm   : ftp://[user@]host[[#:]port]/filename  Use ftp + <.netrc> or g:netrw_uid g:netrw_passwd
  " rcpurm   : rcp://[user@]host/filename	    Use rcp
  " rcphf    : [user@]host:filename		    Use rcp
  " scpurm   : scp://[user@]host[[#:]port]/filename  Use scp
  " httpurm  : http://[user@]host/filename	    Use wget
  " davurm   : dav://host[:port]/path                Use cadaver
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
  let davurm   = '^dav://\([^/]\{-}\)/\(.*\)\=$'
  let rsyncurm = '^rsync://\([^/]\{-}\)/\(.*\)\=$'
  let fetchurm = '^fetch://\(\([^/@]\{-}\)@\)\=\([^/#:]\{-}\)\(:http\)\=/\(.*\)$'
  let sftpurm  = '^sftp://\([^/]\{-}\)/\(.*\)\=$'
 
  call Decho("determine method:")
  " Determine Method
  " rcp://user@hostname/...path-to-file
  if match(a:choice,rcpurm) == 0
   call Decho("rcp://...")
   let b:netrw_method = 1
   let userid	     = substitute(a:choice,rcpurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,rcpurm,'\3',"")
   let b:netrw_fname  = substitute(a:choice,rcpurm,'\4',"")
   if userid != ""
    let g:netrw_uid= userid
   endif
 
  " scp://user@hostname/...path-to-file
  elseif match(a:choice,scpurm) == 0
   call Decho("scp://...")
   let b:netrw_method = 4
   let g:netrw_machine= substitute(a:choice,scpurm,'\1',"")
   let b:netrw_port   = substitute(a:choice,scpurm,'\2',"")
   let b:netrw_fname  = substitute(a:choice,scpurm,'\3',"")
 
  " http://user@hostname/...path-to-file
  elseif match(a:choice,httpurm) == 0
   call Decho("http://...")
   let b:netrw_method = 5
   let g:netrw_machine= substitute(a:choice,httpurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,httpurm,'\2',"")
 
  " dav://hostname[:port]/..path-to-file..
  elseif match(a:choice,davurm) == 0
   call Decho("dav://...")
   let b:netrw_method= 6
   let g:netrw_machine= substitute(a:choice,davurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,davurm,'\2',"")
 
  " rsync://user@hostname/...path-to-file
  elseif match(a:choice,rsyncurm) == 0
   call Decho("rsync://...")
   let b:netrw_method = 7
   let g:netrw_machine= substitute(a:choice,rsyncurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,rsyncurm,'\2',"")
 
  " ftp://[user@]hostname[[:#]port]/...path-to-file
  elseif match(a:choice,ftpurm) == 0
   call Decho("ftp://...")
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
   call Decho("fetch://...")
   let b:netrw_method = 8
   let g:netrw_userid = substitute(a:choice,fetchurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,fetchurm,'\3',"")
   let b:netrw_option = substitute(a:choice,fetchurm,'\4',"")
   let b:netrw_fname  = substitute(a:choice,fetchurm,'\5',"")
 
  " Issue an ftp : "machine id password [path/]filename"
  elseif match(a:choice,mipf) == 0
   call Decho("(ftp) host id pass file")
   let b:netrw_method  = 3
   let g:netrw_machine = substitute(a:choice,mipf,'\1',"")
   let g:netrw_uid     = substitute(a:choice,mipf,'\2',"")
   let g:netrw_passwd  = substitute(a:choice,mipf,'\3',"")
   let b:netrw_fname   = substitute(a:choice,mipf,'\4',"")
 
  " Issue an ftp: "hostname [path/]filename"
  elseif match(a:choice,mf) == 0
   call Decho("(ftp) host file")
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
   call Decho("sftp://...")
   let b:netrw_method = 9
   let g:netrw_machine= substitute(a:choice,sftpurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,sftpurm,'\2',"")
 
  " Issue an rcp: hostname:filename"  (this one should be last)
  elseif match(a:choice,rcphf) == 0
   call Decho("(rcp) [user@]host:file) rcphf<".rcphf.">")
   let b:netrw_method = 1
   let userid	     = substitute(a:choice,rcphf,'\2',"")
   let g:netrw_machine= substitute(a:choice,rcphf,'\3',"")
   let b:netrw_fname  = substitute(a:choice,rcphf,'\4',"")
   call Decho('\1<'.substitute(a:choice,rcphf,'\1',"").">")
   call Decho('\2<'.substitute(a:choice,rcphf,'\2',"").">")
   call Decho('\3<'.substitute(a:choice,rcphf,'\3',"").">")
   call Decho('\4<'.substitute(a:choice,rcphf,'\4',"").">")
   if userid != ""
    let g:netrw_uid= userid
   endif
   if has("win32")
    " don't let PCs try <.netrc>
    let b:netrw_method = 3
   endif
 
  else
   echoerr "***error*** cannot determine method"
   let b:netrw_method  = -1
  endif
 
  call Decho("a:choice       <".a:choice.">")
  call Decho("b:netrw_method <".b:netrw_method.">")
  call Decho("g:netrw_machine<".g:netrw_machine.">")
  call Decho("g:netrw_port   <".g:netrw_port.">")
  if exists("g:netrw_uid")		"Decho
   call Decho("g:netrw_uid    <".g:netrw_uid.">")
  endif					"Decho
  if exists("g:netrw_passwd")		"Decho
   call Decho("g:netrw_passwd <".g:netrw_passwd.">")
  endif					"Decho
  call Decho("b:netrw_fname  <".b:netrw_fname.">")
  call Dret("NetMethod")
endfun
" end of NetMethod

" ------------------------------------------------------------------------
" NetUserPass: set username and password for subsequent ftp transfer {{{1
"   Usage:  :call NetUserPass()			-- will prompt for userid and password
"	    :call NetUserPass("uid")		-- will prompt for password
"	    :call NetUserPass("uid","password") -- sets global userid and password
fun! NetUserPass(...)

 " get/set userid
 if a:0 == 0
  call Dfunc("NetUserPass(a:0<".a:0.">)")
  if !exists("g:netrw_uid") || g:netrw_uid == ""
   " via prompt
   let g:netrw_uid= input('Enter username: ')
  endif
 else	" from command line
  call Dfunc("NetUserPass(a:1<".a:1.">) {")
  let g:netrw_uid= a:1
 endif

 " get password
 if a:0 <= 1 " via prompt
  call Decho("a:0=".a:0." case <=1:")
  let g:netrw_passwd= inputsecret("Enter Password: ")
 else " from command line
  call Decho("a:0=".a:0." case >1: a:2<".a:2.">")
  let g:netrw_passwd=a:2
 endif
  call Dret("NetUserPass")
endfun
" end NetUserPass

" ------------------------------------------------------------------------
" NetOptionSave: save options and set to "standard" form {{{1
fun!s:NetOptionSave()
  call Dfunc("NetOptionSave()")

  " Get Temporary Filename
  let s:aikeep	= &ai
  let s:cinkeep	= &cin
  let s:cinokeep = &cino
  let s:comkeep	= &com
  let s:cpokeep	= &cpo
  let s:dirkeep	= getcwd()
  let s:gdkeep	= &gd
  let s:twkeep	= &tw
  set cino =
  set com  =
  set cpo -=aA
  set nocin noai
  set tw   =0
  if has("win32") && !has("win95")
   let s:swfkeep= &swf
   set noswf
  call Decho("setting s:swfkeep to <".&swf.">")
  endif

  call Dret("NetOptionSave")
endfun

" ------------------------------------------------------------------------
" NetOptionRestore: restore options {{{1
fun! s:NetOptionRestore()
  call Dfunc("NetOptionRestore()")
 
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
 
  call Dret("NetOptionRestore")
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
   call Dfunc("NetReadFixup(method<".a:method."> line1=".a:line1." line2=".a:line2.")")
   if method == 3   " ftp (no <.netrc>)
    let fourblanklines= line2 - 3
    silent fourblanklines.",".line2."g/^\s*/d"
   endif
   call Dret("NetReadFixup")
 endfun
endif

" ---------------------------------------------------------------------
" NetSort: Piet Delport's BISort2() function, modified to take a range
fun! <SID>NetSort() range
  let i = a:firstline + 1
  while i <= a:lastline
    " find insertion point via binary search
    let i_val = getline(i)
    let lo    = a:firstline
    let hi    = i
    while lo < hi
      let mid     = (lo + hi) / 2
      let mid_val = getline(mid)
      if i_val < mid_val
        let hi = mid
      else
        let lo = mid + 1
        if i_val == mid_val | break | endif
      endif
    endwhile
    " do insert
    if lo < i
      exe 'keepjumps '.i.'d_'
      keepjumps call append(lo - 1, i_val)
    endif
    let i = i + 1
  endwhile
endfun

" ------------------------------------------------------------------------
" Restore {{{1
let &cpo= s:save_cpo
unlet s:save_cpo
" vim:ts=8 fdm=marker
