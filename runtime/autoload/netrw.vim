" netrw.vim: Handles file transfer and remote directory listing across
"            AUTOLOAD SECTION
" Date:		May 05, 2007
" Version:	109
" Maintainer:	Charles E Campbell, Jr <NdrOchip@ScampbellPfamily.AbizM-NOSPAM>
" GetLatestVimScripts: 1075 1 :AutoInstall: netrw.vim
" Copyright:    Copyright (C) 1999-2007 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               netrw.vim, netrwPlugin.vim, and netrwSettings.vim are provided
"               *as is* and comes with no warranty of any kind, either
"               expressed or implied. By using this plugin, you agree that
"               in no event will the copyright holder be liable for any damages
"               resulting from the use of this software.
"               of this software.
" COMBAK: worked with tmpfile s:GetTempname() in NetRead() NetWrite()
"         !!NEEDS DEBUGGING && TESTING!!!
"redraw!|call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"
"  But be doers of the Word, and not only hearers, deluding your own selves {{{1
"  (James 1:22 RSV)
" =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
" Load Once: {{{1
if &cp || exists("g:loaded_netrw")
  finish
endif
if !exists("s:NOTE")
 let s:NOTE    = 0
 let s:WARNING = 1
 let s:ERROR   = 2
endif
let g:loaded_netrw = "v109"
if v:version < 700
 call netrw#ErrorMsg(s:WARNING,"you need vim version 7.0 or later for version ".g:loaded_netrw." of netrw",1)
 finish
endif
let s:keepcpo= &cpo
setlocal cpo&vim
"DechoTabOn
"call Decho("doing autoload/netrw.vim version ".g:loaded_netrw)

" ======================
"  Netrw Variables: {{{1
" ======================

" ---------------------------------------------------------------------
"  Netrw Constants: {{{2
if !exists("g:NETRW_BOOKMARKMAX")
 let g:NETRW_BOOKMARKMAX= 0
endif
if !exists("g:NETRW_DIRHIST_CNT")
 let g:NETRW_DIRHIST_CNT= 0
endif
if !exists("s:LONGLIST")
 let s:THINLIST = 0
 let s:LONGLIST = 1
 let s:WIDELIST = 2
 let s:TREELIST = 3
 let s:MAXLIST  = 4
endif

" ---------------------------------------------------------------------
" Default values for netrw's global protocol variables {{{2
if !exists("g:netrw_dav_cmd")
  let g:netrw_dav_cmd	= "cadaver"
endif
if !exists("g:netrw_fetch_cmd")
 if executable("fetch")
  let g:netrw_fetch_cmd	= "fetch -o"
 else
  let g:netrw_fetch_cmd	= ""
 endif
endif
if !exists("g:netrw_ftp_cmd")
  let g:netrw_ftp_cmd	= "ftp"
endif
if !exists("g:netrw_http_cmd")
 if executable("curl")
  let g:netrw_http_cmd	= "curl -o"
 elseif executable("wget")
  let g:netrw_http_cmd	= "wget -q -O"
 elseif executable("fetch")
  let g:netrw_http_cmd	= "fetch -o"
 else
  let g:netrw_http_cmd	= ""
 endif
endif
if !exists("g:netrw_rcp_cmd")
  let g:netrw_rcp_cmd	= "rcp"
endif
if !exists("g:netrw_rsync_cmd")
  let g:netrw_rsync_cmd	= "rsync"
endif
if !exists("g:netrw_scp_cmd")
  let g:netrw_scp_cmd	= "scp -q"
endif
if !exists("g:netrw_sftp_cmd")
  let g:netrw_sftp_cmd	= "sftp"
endif
if !exists("g:netrw_ssh_cmd")
 let g:netrw_ssh_cmd= "ssh"
endif

if (has("win32") || has("win95") || has("win64") || has("win16"))
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
" Default values for netrw's global variables {{{2
" Default values - a-c ---------- {{{3
if !exists("g:netrw_alto")
 let g:netrw_alto= &sb
endif
if !exists("g:netrw_altv")
 let g:netrw_altv= &spr
endif
if !exists("g:netrw_browse_split")
 let g:netrw_browse_split= 0
endif
if !exists("g:netrw_chgwin")
 let g:netrw_chgwin    = -1
endif
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
else
 let g:netrw_cygwin= 0
endif
" Default values - d-f ---------- {{{3
if !exists("g:NETRW_DIRHIST_CNT")
 let g:NETRW_DIRHIST_CNT= 0
endif
if !exists("g:netrw_dirhistmax")
 let g:netrw_dirhistmax= 10
endif
if !exists("g:netrw_ftp_browse_reject")
 let g:netrw_ftp_browse_reject='^total\s\+\d\+$\|^Trying\s\+\d\+.*$\|^KERBEROS_V\d rejected\|^Security extensions not\|No such file\|: connect to address [0-9a-fA-F:]*: No route to host$'
endif
if !exists("g:netrw_ftp_list_cmd")
 if has("unix") || (exists("g:netrw_cygwin") && g:netrw_cygwin)
  let g:netrw_ftp_list_cmd     = "ls -lF"
  let g:netrw_ftp_timelist_cmd = "ls -tlF"
  let g:netrw_ftp_sizelist_cmd = "ls -slF"
 else
  let g:netrw_ftp_list_cmd     = "dir"
  let g:netrw_ftp_timelist_cmd = "dir"
  let g:netrw_ftp_sizelist_cmd = "dir"
 endif
endif
if !exists("g:netrw_ftpmode")
 let g:netrw_ftpmode= "binary"
endif
" Default values - h-lh ---------- {{{3
if !exists("g:netrw_hide")
 let g:netrw_hide= 1
endif
if !exists("g:netrw_ignorenetrc")
 if &shell =~ '\c\<\%(cmd\|4nt\)\.exe$'
  let g:netrw_ignorenetrc= 1
 else
  let g:netrw_ignorenetrc= 0
 endif
endif
if !exists("g:netrw_keepdir")
 let g:netrw_keepdir= 1
endif
if !exists("g:netrw_list_cmd")
 if g:netrw_scp_cmd =~ '^pscp' && executable("pscp")
  " provide a 'pscp' listing command
  if (has("win32") || has("win95") || has("win64") || has("win16")) && filereadable("c:\\private.ppk")
   let g:netrw_scp_cmd ="pscp -i C:\\private.ppk"
  endif
  let g:netrw_list_cmd= g:netrw_scp_cmd." -ls USEPORT HOSTNAME:"
 elseif executable(g:netrw_ssh_cmd)
  " provide a default listing command
  let g:netrw_list_cmd= g:netrw_ssh_cmd." USEPORT HOSTNAME ls -FLa"
 else
"  call Decho(g:netrw_ssh_cmd." is not executable")
  let g:netrw_list_cmd= ""
 endif
endif
if !exists("g:netrw_list_hide")
 let g:netrw_list_hide= ""
endif
" Default values - lh-lz ---------- {{{3
if !exists("g:netrw_local_mkdir")
 let g:netrw_local_mkdir= "mkdir"
endif
if !exists("g:netrw_local_rmdir")
 let g:netrw_local_rmdir= "rmdir"
endif
if !exists("g:netrw_liststyle")
 let g:netrw_liststyle= s:THINLIST
endif
if g:netrw_liststyle < 0 || g:netrw_liststyle >= s:MAXLIST
 " sanity check
 let g:netrw_liststyle= s:THINLIST
endif
if g:netrw_liststyle == s:LONGLIST && g:netrw_scp_cmd !~ '^pscp'
 let g:netrw_list_cmd= g:netrw_list_cmd." -l"
endif
" Default values - m-r ---------- {{{3
if !exists("g:netrw_maxfilenamelen")
 let g:netrw_maxfilenamelen= 32
endif
if !exists("g:netrw_menu")
 let g:netrw_menu= 1
endif
if !exists("g:netrw_mkdir_cmd")
 let g:netrw_mkdir_cmd= g:netrw_ssh_cmd." USEPORT HOSTNAME mkdir"
endif
if !exists("g:netrw_scpport")
 let g:netrw_scpport= "-P"
endif
if !exists("g:netrw_sshport")
 let g:netrw_sshport= "-p"
endif
if !exists("g:netrw_rename_cmd")
 let g:netrw_rename_cmd= g:netrw_ssh_cmd." USEPORT HOSTNAME mv"
endif
if !exists("g:netrw_rm_cmd")
 let g:netrw_rm_cmd    = g:netrw_ssh_cmd." USEPORT HOSTNAME rm"
endif
if !exists("g:netrw_rmdir_cmd")
 let g:netrw_rmdir_cmd = g:netrw_ssh_cmd." USEPORT HOSTNAME rmdir"
endif
if !exists("g:netrw_rmf_cmd")
 let g:netrw_rmf_cmd    = g:netrw_ssh_cmd." USEPORT HOSTNAME rm -f"
endif
" Default values - s ---------- {{{3
if exists("g:netrw_silent") && g:netrw_silent != 0
 let g:netrw_silentxfer= "silent "
else
 let g:netrw_silentxfer= ""
endif
if !exists("g:netrw_fastbrowse")
 let g:netrw_fastbrowse= 1
endif
if !exists("g:netrw_shq")
 if exists("&shq") && &shq != ""
  let g:netrw_shq= &shq
 elseif has("win32") || has("win95") || has("win64") || has("win16")
  if g:netrw_cygwin
   let g:netrw_shq= "'"
  else
   let g:netrw_shq= '"'
  endif
 else
  let g:netrw_shq= "'"
 endif
" call Decho("g:netrw_shq<".g:netrw_shq.">")
endif
if !exists("g:netrw_sort_by")
 " alternatives: date size
 let g:netrw_sort_by= "name"
endif
if !exists("g:netrw_sort_direction")
 " alternative: reverse  (z y x ...)
 let g:netrw_sort_direction= "normal"
endif
if !exists("g:netrw_sort_sequence")
 let g:netrw_sort_sequence= '[\/]$,\.h$,\.c$,\.cpp$,\.[a-np-z]$,*,\.info$,\.swp$,\.o$\.obj$,\.bak$'
endif
if !exists("g:netrw_ssh_browse_reject")
  let g:netrw_ssh_browse_reject='^total\s\+\d\+$'
endif
if !has("patch192")
 if !exists("g:netrw_use_noswf")
  let g:netrw_use_noswf= 1
 endif
else
  let g:netrw_use_noswf= 0
endif
" Default values - t-w ---------- {{{3
if !exists("g:netrw_timefmt")
 let g:netrw_timefmt= "%c"
endif
if !exists("g:NetrwTopLvlMenu")
 let g:NetrwTopLvlMenu= "Netrw."
endif
if !exists("g:netrw_use_errorwindow")
 let g:netrw_use_errorwindow= 1
endif
if !exists("g:netrw_win95ftp")
 let g:netrw_win95ftp= 1
endif
if !exists("g:netrw_winsize")
 let g:netrw_winsize= ""
endif
" ---------------------------------------------------------------------
" Default values for netrw's script variables: {{{2
if !exists("s:netrw_cd_escape")
  let s:netrw_cd_escape="[]#*$%'\" ?`!&();<>\\"
endif
if !exists("g:netrw_fname_escape")
 let g:netrw_fname_escape= ' ?&;'
endif
if !exists("g:netrw_tmpfile_escape")
 let g:netrw_tmpfile_escape= ' ?&;'
endif
if !exists("s:netrw_glob_escape")
  let s:netrw_glob_escape= '[]*?`{~$'
endif

" BufEnter event ignored by decho when following variable is true
"  Has a side effect that doau BufReadPost doesn't work, so
"  files read by network transfer aren't appropriately highlighted.
"let g:decho_bufenter = 1	"Decho

" ==============================
"  Netrw Utility Functions: {{{1
" ==============================

" ------------------------------------------------------------------------
" NetSavePosn: saves position of cursor on screen {{{2
fun! netrw#NetSavePosn()
"  call Dfunc("netrw#NetSavePosn()")
  " Save current line and column
  let w:netrw_winnr= winnr()
  let w:netrw_line = line(".")
  let w:netrw_col  = virtcol(".")

  " Save top-of-screen line
  norm! H0
  let w:netrw_hline= line(".")

  call netrw#NetRestorePosn()
"  call Dret("netrw#NetSavePosn : winnr=".w:netrw_winnr." line=".w:netrw_line." col=".w:netrw_col." hline=".w:netrw_hline)
endfun

" ------------------------------------------------------------------------
" NetRestorePosn: restores the cursor and file position as saved by NetSavePosn() {{{2
fun! netrw#NetRestorePosn()
"  call Dfunc("netrw#NetRestorePosn() winnr=".(exists("w:netrw_winnr")? w:netrw_winnr : -1)." line=".(exists("w:netrw_line")? w:netrw_line : -1)." col=".(exists("w:netrw_col")? w:netrw_col : -1)." hline=".(exists("w:netrw_hline")? w:netrw_hline : -1))
  let eikeep= &ei
  set ei=all
  if expand("%") == "NetrwMessage"
   exe s:winBeforeErr."wincmd w"
  endif

  " restore window
  if exists("w:netrw_winnr")
"   call Decho("restore window: exe silent! ".w:netrw_winnr."wincmd w")
   exe "silent! ".w:netrw_winnr."wincmd w"
  endif
  if v:shell_error == 0
   " as suggested by Bram M: redraw on no error
   " allows protocol error messages to remain visible
   redraw!
  endif

  " restore top-of-screen line
  if exists("w:netrw_hline")
"   call Decho("restore topofscreen: exe norm! ".w:netrw_hline."G0z")
   exe "norm! ".w:netrw_hline."G0z\<CR>"
  endif

  " restore position
  if exists("w:netrw_line") && exists("w:netrw_col")
"   call Decho("restore posn: exe norm! ".w:netrw_line."G0".w:netrw_col."|")
   exe "norm! ".w:netrw_line."G0".w:netrw_col."\<bar>"
  endif

  let &ei= eikeep
"  call Dret("netrw#NetRestorePosn")
endfun

" ===============================
" NetOptionSave: save options and set to "standard" form {{{2
"DechoTabOn
fun! s:NetOptionSave()
"  call Dfunc("s:NetOptionSave() win#".winnr()." buf#".bufnr("."))
  if !exists("w:netrw_optionsave")
   let w:netrw_optionsave= 1
  else
"   call Dret("s:NetOptionSave : netoptionsave=".w:netrw_optionsave)
   return
  endif

  " Save current settings and current directory
  let s:yykeep          = @@
  if exists("&l:acd")
   let w:netrw_acdkeep  = &l:acd
  endif
  let w:netrw_aikeep    = &l:ai
  let w:netrw_awkeep    = &l:aw
  let w:netrw_cikeep    = &l:ci
  let w:netrw_cinkeep   = &l:cin
  let w:netrw_cinokeep  = &l:cino
  let w:netrw_comkeep   = &l:com
  let w:netrw_cpokeep   = &l:cpo
  if g:netrw_keepdir
   let w:netrw_dirkeep  = getcwd()
  endif
  let w:netrw_fokeep    = &l:fo           " formatoptions
  let w:netrw_gdkeep    = &l:gd           " gdefault
  let w:netrw_hidkeep   = &l:hidden
  let w:netrw_magickeep = &l:magic
  let w:netrw_repkeep   = &l:report
  let w:netrw_spellkeep = &l:spell
  let w:netrw_twkeep    = &l:tw           " textwidth
  let w:netrw_wigkeep   = &l:wig          " wildignore
  if has("win32") && !has("win95")
   let w:netrw_swfkeep= &l:swf            " swapfile
  endif
  call s:NetrwSafeOptions()
  if &go =~ 'a' | silent! let w:netrw_regstar = @* | endif
  silent! let w:netrw_regslash= @/

"  call Dret("s:NetOptionSave")
"  call Dret("s:NetOptionSave : win#".winnr()." buf#".bufnr("."))
endfun

" ------------------------------------------------------------------------
" NetOptionRestore: restore options {{{2
fun! s:NetOptionRestore()
"  call Dfunc("s:NetOptionRestore() win#".winnr()." buf#".bufnr("."))
  if !exists("w:netrw_optionsave")
"   call Dret("s:NetOptionRestore : w:netrw_optionsave doesn't exist")
   return
  endif
  unlet w:netrw_optionsave

  if exists("&acd")
   if exists("w:netrw_acdkeep") |let &l:acd    = w:netrw_acdkeep     |unlet w:netrw_acdkeep  |endif
  endif
  if exists("w:netrw_aikeep")   |let &l:ai     = w:netrw_aikeep      |unlet w:netrw_aikeep   |endif
  if exists("w:netrw_awkeep")   |let &l:aw     = w:netrw_awkeep      |unlet w:netrw_awkeep   |endif
  if exists("w:netrw_cikeep")   |let &l:ci     = w:netrw_cikeep      |unlet w:netrw_cikeep   |endif
  if exists("w:netrw_cinkeep")  |let &l:cin    = w:netrw_cinkeep     |unlet w:netrw_cinkeep  |endif
  if exists("w:netrw_cinokeep") |let &l:cino   = w:netrw_cinokeep    |unlet w:netrw_cinokeep |endif
  if exists("w:netrw_comkeep")  |let &l:com    = w:netrw_comkeep     |unlet w:netrw_comkeep  |endif
  if exists("w:netrw_cpokeep")  |let &l:cpo    = w:netrw_cpokeep     |unlet w:netrw_cpokeep  |endif
  if exists("w:netrw_dirkeep")  |exe "lcd ".w:netrw_dirkeep          |unlet w:netrw_dirkeep  |endif
  if exists("w:netrw_fokeep")   |let &l:fo     = w:netrw_fokeep      |unlet w:netrw_fokeep   |endif
  if exists("w:netrw_gdkeep")   |let &l:gd     = w:netrw_gdkeep      |unlet w:netrw_gdkeep   |endif
  if exists("w:netrw_hidkeep")  |let &l:hidden = w:netrw_hidkeep     |unlet w:netrw_hidkeep  |endif
  if exists("w:netrw_magic")    |let &l:magic  = w:netrw_magic       |unlet w:netrw_magic    |endif
  if exists("w:netrw_repkeep")  |let &l:report = w:netrw_repkeep     |unlet w:netrw_repkeep  |endif
  if exists("w:netrw_spellkeep")|let &l:spell  = w:netrw_spellkeep   |unlet w:netrw_spellkeep|endif
  if exists("w:netrw_twkeep")   |let &l:tw     = w:netrw_twkeep      |unlet w:netrw_twkeep   |endif
  if exists("w:netrw_wigkeep")  |let &l:wig    = w:netrw_wigkeep     |unlet w:netrw_wigkeep  |endif
  if exists("s:yykeep")         |let  @@       = s:yykeep            |unlet s:yykeep         |endif
  if exists("w:netrw_swfkeep")
   if &directory == ""
    " user hasn't specified a swapfile directory;
    " netrw will temporarily set the swapfile directory
    " to the current directory as returned by getcwd().
    let &l:directory   = getcwd()
    silent! let &l:swf = w:netrw_swfkeep
    setlocal directory=
    unlet w:netrw_swfkeep
   elseif &l:swf != w:netrw_swfkeep
    " following line causes a Press ENTER in windows -- can't seem to work around it!!! (COMBAK)
    silent! let &l:swf= w:netrw_swfkeep
    unlet w:netrw_swfkeep
   endif
  endif
  if exists("w:netrw_regstar") |silent! let @*= w:netrw_regstar |unlet w:netrw_regstar |endif
  if exists("w:netrw_regslash")|silent! let @/= w:netrw_regslash|unlet w:netrw_regslash|endif

"  call Dret("s:NetOptionRestore : win#".winnr()." buf#".bufnr("."))
endfun

" ---------------------------------------------------------------------
" NetrwSafeOptions: sets options to help netrw do its job {{{2
fun! s:NetrwSafeOptions()
"  call Dfunc("s:NetrwSafeOptions()")
  setlocal cino=
  setlocal com=
  setlocal cpo-=aA
  if exists("&acd")
   setlocal noacd nocin noai noci magic nospell nohid wig= noaw
   setlocal fo=nroql2
  else
   setlocal nocin noai noci magic nospell nohid wig= noaw
   setlocal fo=nroql2
  endif
  setlocal tw=0
  setlocal report=10000
  if g:netrw_use_noswf && has("win32") && !has("win95")
   setlocal noswf
  endif
"  call Dret("s:NetrwSafeOptions")
endfun

" ------------------------------------------------------------------------
"  Netrw Transfer Functions: {{{1
" ===============================

" ------------------------------------------------------------------------
" NetRead: responsible for reading a file over the net {{{2
"   mode: =0 read remote file and insert before current line
"         =1 read remote file and insert after current line
"         =2 replace with remote file
"         =3 obtain file, but leave in temporary format
fun! netrw#NetRead(mode,...)
"  call Dfunc("netrw#NetRead(mode=".a:mode.",...) a:0=".a:0." ".g:loaded_netrw)

  " save options {{{3
  call s:NetOptionSave()

  " interpret mode into a readcmd {{{3
  if     a:mode == 0 " read remote file before current line
   let readcmd = "0r"
  elseif a:mode == 1 " read file after current line
   let readcmd = "r"
  elseif a:mode == 2 " replace with remote file
   let readcmd = "%r"
  elseif a:mode == 3 " skip read of file (leave as temporary)
   let readcmd = "t"
  else
   exe a:mode
   let readcmd = "r"
  endif
  let ichoice = (a:0 == 0)? 0 : 1
"  call Decho("readcmd<".readcmd."> ichoice=".ichoice)

  " Get Temporary Filename {{{3
  let tmpfile= s:GetTempfile("")
  if tmpfile == ""
"   call Dret("netrw#NetRead : unable to get a tempfile!")
   return
  endif

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
     sleep 4
     break

    elseif match(choice,'^"') != -1
     " Reconstruct Choice if choice starts with '"'
"     call Decho("reconstructing choice")
     if match(choice,'"$') != -1
      " case "..."
      let choice=strpart(choice,1,strlen(choice)-2)
     else
       "  case "... ... ..."
      let choice      = strpart(choice,1,strlen(choice)-1)
      let wholechoice = ""

      while match(choice,'"$') == -1
       let wholechoice = wholechoice . " " . choice
       let ichoice     = ichoice + 1
       if ichoice > a:0
       	if !exists("g:netrw_quiet")
	 call netrw#ErrorMsg(s:ERROR,"Unbalanced string in filename '". wholechoice ."'",3)
	endif
"        call Dret("netrw#NetRead :2 getcwd<".getcwd().">")
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

   " Determine method of read (ftp, rcp, etc) {{{3
   call s:NetMethod(choice)
   let tmpfile= s:GetTempfile(b:netrw_fname) " apply correct suffix

   " Check if NetBrowse() should be handling this request
"   call Decho("checking if NetBrowse() should handle choice<".choice."> with netrw_list_cmd<".g:netrw_list_cmd.">")
   if choice =~ "^.*[\/]$" && b:netrw_method != 5 && choice !~ '^http://'
"    call Decho("yes, choice matches '^.*[\/]$'")
    keepjumps call s:NetBrowse(0,choice)
"    call Dret("netrw#NetRead :3 getcwd<".getcwd().">")
    return
   endif

   " ============
   " Perform Protocol-Based Read {{{3
   " ===========================
   if exists("g:netrw_silent") && g:netrw_silent == 0 && &ch >= 1
    echo "(netrw) Processing your read request..."
   endif

   ".........................................
   " rcp:  NetRead Method #1 {{{3
   if  b:netrw_method == 1 " read with rcp
"    call Decho("read via rcp (method #1)")
   " ER: nothing done with g:netrw_uid yet?
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
"   call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".uid_machine.":".escape(b:netrw_fname,' ?&;')." ".tmpfile)
   exe g:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".uid_machine.":".escape(b:netrw_fname,' ?&;')." ".tmpfile
   let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
   let b:netrw_lastfile = choice

   ".........................................
   " ftp + <.netrc>:  NetRead Method #2 {{{3
   elseif b:netrw_method  == 2		" read with ftp + <.netrc>
"     call Decho("read via ftp+.netrc (method #2)")
     let netrw_fname= b:netrw_fname
     new
     setlocal ff=unix
     exe "put ='".g:netrw_ftpmode."'"
"     call Decho("filter input: ".getline("."))
     if exists("g:netrw_ftpextracmd")
      exe "put ='".g:netrw_ftpextracmd."'"
"      call Decho("filter input: ".getline("."))
     endif
     exe "put ='".'get \"'.netrw_fname.'\" '.tmpfile."'"
"     call Decho("filter input: ".getline("."))
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
      setlocal debug=msg
      call netrw#ErrorMsg(s:ERROR,getline(1),4)
      let &debug= debugkeep
     endif
     bd!
     let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
     let b:netrw_lastfile = choice

   ".........................................
   " ftp + machine,id,passwd,filename:  NetRead Method #3 {{{3
   elseif b:netrw_method == 3		" read with ftp + machine, id, passwd, and fname
    " Construct execution string (four lines) which will be passed through filter
"    call Decho("read via ftp+mipf (method #3)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
"     call Decho("filter input: ".getline("."))
    else
     put ='open '.g:netrw_machine
"     call Decho("filter input: ".getline("."))
    endif

    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     put =g:netrw_uid
"     call Decho("filter input: ".getline("."))
     put ='\"'.g:netrw_passwd.'\"'
"     call Decho("filter input: ".getline("."))
    else
     put ='user \"'.g:netrw_uid.'\" \"'.g:netrw_passwd.'\"'
"     call Decho("filter input: ".getline("."))
    endif

    if exists("g:netrw_ftpmode") && g:netrw_ftpmode != ""
     put =g:netrw_ftpmode
"     call Decho("filter input: ".getline("."))
    endif
    if exists("g:netrw_ftpextracmd")
     exe "put ='".g:netrw_ftpextracmd."'"
"     call Decho("filter input: ".getline("."))
    endif
    put ='get \"'.netrw_fname.'\" '.tmpfile
"    call Decho("filter input: ".getline("."))

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
"     call Decho("error<".getline(1).">")
     if !exists("g:netrw_quiet")
      call netrw#ErrorMsg(s:ERROR,getline(1),5)
     endif
    endif
    bd!
    let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " scp: NetRead Method #4 {{{3
   elseif     b:netrw_method  == 4	" read with scp
"    call Decho("read via scp (method #4)")
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " ".g:netrw_scpport." ".g:netrw_port
    else
     let useport= ""
    endif
"    call  Decho("executing: !".g:netrw_scp_cmd.useport." '".g:netrw_machine.":".escape(b:netrw_fname,g:netrw_fname_escape)."' ".tmpfile)
    exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_shq.g:netrw_machine.":".escape(b:netrw_fname,g:netrw_fname_escape).g:netrw_shq." ".tmpfile
    let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " http: NetRead Method #5 (wget) {{{3
   elseif     b:netrw_method  == 5
"    call Decho("read via http (method #5)")
    if g:netrw_http_cmd == ""
     if !exists("g:netrw_quiet")
      call netrw#ErrorMsg(s:ERROR,"neither the wget nor the fetch command is available",6)
     endif
"     call Dret("netrw#NetRead :4 getcwd<".getcwd().">")
     return
    endif

    if match(b:netrw_fname,"#") == -1
     " simple wget
     let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
"     call Decho("executing: !".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_fname)
     exe g:netrw_silentxfer."!".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_fname
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)

    else
     " wget plus a jump to an in-page marker (ie. http://abc/def.html#aMarker)
     let netrw_html= substitute(netrw_fname,"#.*$","","")
     let netrw_tag = substitute(netrw_fname,"^.*#","","")
"     call Decho("netrw_html<".netrw_html.">")
"     call Decho("netrw_tag <".netrw_tag.">")
"     call Decho("executing: !".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_html)
     exe g:netrw_silentxfer."!".g:netrw_http_cmd." ".tmpfile." http://".g:netrw_machine.netrw_html
     let result = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
"     call Decho('<\s*a\s*name=\s*"'.netrw_tag.'"/')
     exe 'norm! 1G/<\s*a\s*name=\s*"'.netrw_tag.'"/'."\<CR>"
    endif
    let b:netrw_lastfile = choice
    setlocal ro

   ".........................................
   " cadaver: NetRead Method #6 {{{3
   elseif     b:netrw_method  == 6
"    call Decho("read via cadaver (method #6)")

    " Construct execution string (four lines) which will be passed through filter
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     put ='open '.g:netrw_machine
    endif
    put ='user '.g:netrw_uid.' '.g:netrw_passwd
    put ='get '.netrw_fname.' '.tmpfile
    put ='quit'

    " perform cadaver operation:
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_dav_cmd)
    exe g:netrw_silentxfer."%!".g:netrw_dav_cmd
    bd!
    let result           = s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " rsync: NetRead Method #7 {{{3
   elseif     b:netrw_method  == 7
"    call Decho("read via rsync (method #7)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
"    call Decho("executing: !".g:netrw_rsync_cmd." ".g:netrw_machine.":".netrw_fname." ".tmpfile)
    exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".g:netrw_machine.":".netrw_fname." ".tmpfile
    let result		= s:NetGetFile(readcmd,tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " fetch: NetRead Method #8 {{{3
   "    fetch://[user@]host[:http]/path
   elseif     b:netrw_method  == 8
"    call Decho("read via fetch (method #8)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    if g:netrw_fetch_cmd == ""
     if !exists("g:netrw_quiet")
      call netrw#ErrorMsg(s:ERROR,"fetch command not available",7)
     endif
"     call Dret("NetRead")
    endif
    if exists("g:netrw_option") && g:netrw_option == ":http"
     let netrw_option= "http"
    else
     let netrw_option= "ftp"
    endif
"    call Decho("read via fetch for ".netrw_option)

    if exists("g:netrw_uid") && g:netrw_uid != "" && exists("g:netrw_passwd") && g:netrw_passwd != ""
"     call Decho("executing: !".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_uid.':'.g:netrw_passwd.'@'.g:netrw_machine."/".netrw_fname)
     exe g:netrw_silentxfer."!".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_uid.':'.g:netrw_passwd.'@'.g:netrw_machine."/".netrw_fname
    else
"     call Decho("executing: !".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_machine."/".netrw_fname)
     exe g:netrw_silentxfer."!".g:netrw_fetch_cmd." ".tmpfile." ".netrw_option."://".g:netrw_machine."/".netrw_fname
    endif

    let result		= s:NetGetFile(readcmd,tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
    setlocal ro

   ".........................................
   " sftp: NetRead Method #9 {{{3
   elseif     b:netrw_method  == 9
"    call Decho("read via sftp (method #9)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
"    call Decho("executing: !".g:netrw_sftp_cmd." ".g:netrw_machine.":".netrw_fname." ".tmpfile)
    exe g:netrw_silentxfer."!".g:netrw_sftp_cmd." ".g:netrw_machine.":".netrw_fname." ".tmpfile
    let result		= s:NetGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " Complain {{{3
   else
    call netrw#ErrorMsg(s:WARNING,"unable to comply with your request<" . choice . ">",8)
   endif
  endwhile

  " cleanup {{{3
  if exists("b:netrw_method")
"   call Decho("cleanup b:netrw_method and b:netrw_fname")
   unlet b:netrw_method
   unlet b:netrw_fname
  endif
  if s:FileReadable(tmpfile) && tmpfile !~ '.tar.bz2$' && tmpfile !~ '.tar.gz$' && tmpfile !~ '.zip' && tmpfile !~ '.tar' && readcmd != 't'
"   call Decho("cleanup by deleting tmpfile<".tmpfile.">")
   call s:System("delete",tmpfile)
  endif
  call s:NetOptionRestore()

"  call Dret("netrw#NetRead :5 getcwd<".getcwd().">")
endfun

" ------------------------------------------------------------------------
" NetWrite: responsible for writing a file over the net {{{2
fun! netrw#NetWrite(...) range
"  call Dfunc("netrw#NetWrite(a:0=".a:0.") ".g:loaded_netrw)

  " option handling
  let mod= 0
  call s:NetOptionSave()

  " Get Temporary Filename {{{3
  let tmpfile= s:GetTempfile("")
  if tmpfile == ""
"   call Dret("netrw#NetWrite : unable to get a tempfile!")
   return
  endif

  if a:0 == 0
   let ichoice = 0
  else
   let ichoice = 1
  endif

  let curbufname= expand("%")
"  call Decho("curbufname<".curbufname.">")
  if &binary
   " For binary writes, always write entire file.
   " (line numbers don't really make sense for that).
   " Also supports the writing of tar and zip files.
"   call Decho("(write entire file) silent exe w! ".v:cmdarg." ".tmpfile)
   silent exe "w! ".v:cmdarg." ".tmpfile
  elseif g:netrw_cygwin
   " write (selected portion of) file to temporary
   let cygtmpfile= substitute(tmpfile,'/cygdrive/\(.\)','\1:','')
"   call Decho("(write selected portion) silent exe ".a:firstline."," . a:lastline . "w! ".v:cmdarg." ".cygtmpfile)
   silent exe a:firstline."," . a:lastline . "w! ".v:cmdarg." ".cygtmpfile
  else
   " write (selected portion of) file to temporary
"   call Decho("(write selected portion) silent exe ".a:firstline."," . a:lastline . "w! ".v:cmdarg." ".tmpfile)
   silent exe a:firstline."," . a:lastline . "w! ".v:cmdarg." ".tmpfile
  endif

  if curbufname == ""
   " if the file is [No Name], and one attempts to Nwrite it, the buffer takes
   " on the temporary file's name.  Deletion of the temporary file during
   " cleanup then causes an error message.
   0file!
  endif

  " While choice loop: {{{3
  while ichoice <= a:0

   " Process arguments: {{{4
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
     sleep 4
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
	 call netrw#ErrorMsg(s:ERROR,"Unbalanced string in filename '". wholechoice ."'",13)
	endif
"        call Dret("netrw#NetWrite")
        return
       endif
       let choice= a:{ichoice}
      endwhile
      let choice= strpart(wholechoice,1,strlen(wholechoice)-1) . " " . strpart(choice,0,strlen(choice)-1)
     endif
    endif
   endif
   let ichoice= ichoice + 1
"   call Decho("choice<" . choice . "> ichoice=".ichoice)

   " Determine method of write (ftp, rcp, etc) {{{4
   call s:NetMethod(choice)

   " =============
   " Perform Protocol-Based Write {{{4
   " ============================
   if exists("g:netrw_silent") && g:netrw_silent == 0 && &ch >= 1
    echo "(netrw) Processing your write request..."
   endif

   ".........................................
   " rcp: NetWrite Method #1 {{{4
   if  b:netrw_method == 1
"    call Decho("write via rcp (method #1)")
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
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
"    call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".g:netrw_shq.tmpfile.g:netrw_shq." ".uid_machine.":".netrw_fname)
    exe g:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".g:netrw_shq.tmpfile.g:netrw_shq." ".uid_machine.":".netrw_fname
    let b:netrw_lastfile = choice

   ".........................................
   " ftp + <.netrc>: NetWrite Method #2 {{{4
   elseif b:netrw_method == 2
"    call Decho("write via ftp+.netrc (method #2)")
    let netrw_fname= b:netrw_fname
    new
    setlocal ff=unix
    exe "put ='".g:netrw_ftpmode."'"
"    call Decho(" filter input: ".getline("."))
    if exists("g:netrw_ftpextracmd")
     exe "put ='".g:netrw_ftpextracmd."'"
"     call Decho("filter input: ".getline("."))
    endif
    exe "put ='".'put \"'.tmpfile.'\" \"'.netrw_fname.'\"'."'"
"    call Decho(" filter input: ".getline("."))
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
      call netrw#ErrorMsg(s:ERROR,getline(1),14)
     endif
     let mod=1
    endif
    bd!
    let b:netrw_lastfile = choice

   ".........................................
   " ftp + machine, id, passwd, filename: NetWrite Method #3 {{{4
   elseif b:netrw_method == 3
    " Construct execution string (four lines) which will be passed through filter
"    call Decho("read via ftp+mipf (method #3)")
    let netrw_fname= b:netrw_fname
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
"     call Decho("filter input: ".getline("."))
    else
     put ='open '.g:netrw_machine
"     call Decho("filter input: ".getline("."))
    endif
    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     put =g:netrw_uid
"     call Decho("filter input: ".getline("."))
     put ='\"'.g:netrw_passwd.'\"'
"     call Decho("filter input: ".getline("."))
    else
     put ='user \"'.g:netrw_uid.'\" \"'.g:netrw_passwd.'\"'
"     call Decho("filter input: ".getline("."))
    endif
    put ='put \"'.tmpfile.'\" \"'.netrw_fname.'\"'
"    call Decho("filter input: ".getline("."))
    " save choice/id/password for future use
    let b:netrw_lastfile = choice

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer."%!".g:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
     if  !exists("g:netrw_quiet")
      call netrw#ErrorMsg(s:ERROR,getline(1),15)
     endif
     let mod=1
    endif
    bd!

   ".........................................
   " scp: NetWrite Method #4 {{{4
   elseif     b:netrw_method == 4
"    call Decho("write via scp (method #4)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " ".g:netrw_scpport." ".g:netrw_port
    else
     let useport= ""
    endif
"    call Decho("exe ".g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_shq.tmpfile.g:netrw_shq." ".g:netrw_shq.g:netrw_machine.":".netrw_fname.g:netrw_shq)
    exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_shq.tmpfile.g:netrw_shq." ".g:netrw_shq.g:netrw_machine.":".netrw_fname.g:netrw_shq
    let b:netrw_lastfile = choice

   ".........................................
   " http: NetWrite Method #5 {{{4
   elseif     b:netrw_method == 5
"    call Decho("write via http (method #5)")
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:ERROR,"currently <netrw.vim> does not support writing using http:",16)
    endif

   ".........................................
   " dav: NetWrite Method #6 (cadaver) {{{4
   elseif     b:netrw_method == 6
"    call Decho("write via cadaver (method #6)")

    " Construct execution string (four lines) which will be passed through filter
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    new
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     put ='open '.g:netrw_machine
    endif
    put ='user '.g:netrw_uid.' '.g:netrw_passwd
    put ='put '.tmpfile.' '.netrw_fname

    " perform cadaver operation:
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_dav_cmd)
    exe g:netrw_silentxfer."%!".g:netrw_dav_cmd
    bd!
    let b:netrw_lastfile = choice

   ".........................................
   " rsync: NetWrite Method #7 {{{4
   elseif     b:netrw_method == 7
"    call Decho("write via rsync (method #7)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
"    call Decho("executing: !".g:netrw_rsync_cmd." ".tmpfile." ".g:netrw_machine.":".netrw_fname)
    exe g:netrw_silentxfer."!".g:netrw_rsync_cmd." ".tmpfile." ".g:netrw_machine.":".netrw_fname
    let b:netrw_lastfile = choice

   ".........................................
   " sftp: NetWrite Method #9 {{{4
   elseif     b:netrw_method == 9
"    call Decho("read via sftp (method #9)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    if exists("g:netrw_uid") &&  ( g:netrw_uid != "" )
     let uid_machine = g:netrw_uid .'@'. g:netrw_machine
    else
     let uid_machine = g:netrw_machine
    endif
    new
    setlocal ff=unix
    put ='put \"'.escape(tmpfile,'\').'\" '.netrw_fname
"    call Decho("filter input: ".getline("."))
    norm! 1Gdd
"    call Decho("executing: %!".g:netrw_sftp_cmd.' '.uid_machine)
    exe g:netrw_silentxfer."%!".g:netrw_sftp_cmd.' '.uid_machine
    bd!
    let b:netrw_lastfile= choice

   ".........................................
   " Complain {{{4
   else
    call netrw#ErrorMsg(s:WARNING,"unable to comply with your request<" . choice . ">",17)
   endif
  endwhile

  " Cleanup: {{{3
"  call Decho("cleanup")
  if s:FileReadable(tmpfile)
"   call Decho("tmpfile<".tmpfile."> readable, will now delete it")
   call s:System("delete",tmpfile)
  endif
  call s:NetOptionRestore()

  if a:firstline == 1 && a:lastline == line("$")
   " restore modifiability; usually equivalent to set nomod
   let &mod= mod
  endif

"  call Dret("netrw#NetWrite")
endfun

" ---------------------------------------------------------------------
" NetSource: source a remotely hosted vim script {{{2
" uses NetRead to get a copy of the file into a temporarily file,
"              then sources that file,
"              then removes that file.
fun! netrw#NetSource(...)
"  call Dfunc("netrw#NetSource() a:0=".a:0)
  if a:0 > 0 && a:1 == '?'
   " give help
   echomsg 'NetSource Usage:'
   echomsg ':Nsource dav://machine[:port]/path            uses cadaver'
   echomsg ':Nsource fetch://machine/path                 uses fetch'
   echomsg ':Nsource ftp://[user@]machine[:port]/path     uses ftp   autodetects <.netrc>'
   echomsg ':Nsource http://[user@]machine/path           uses http  wget'
   echomsg ':Nsource rcp://[user@]machine/path            uses rcp'
   echomsg ':Nsource rsync://machine[:port]/path          uses rsync'
   echomsg ':Nsource scp://[user@]machine[[:#]port]/path  uses scp'
   echomsg ':Nsource sftp://[user@]machine[[:#]port]/path uses sftp'
   sleep 4
  else
   let i= 1
   while i <= a:0
    call netrw#NetRead(3,a:{i})
"    call Decho("s:netread_tmpfile<".s:netrw_tmpfile.">")
    if s:FileReadable(s:netrw_tmpfile)
"     call Decho("exe so ".s:netrw_tmpfile)
     exe "so ".s:netrw_tmpfile
     call delete(s:netrw_tmpfile)
     unlet s:netrw_tmpfile
    else
     call netrw#ErrorMsg(s:ERROR,"unable to source <".a:{i}.">!",48)
    endif
    let i= i + 1
   endwhile
  endif
"  call Dret("netrw#NetSource")
endfun

" ===========================================
" NetGetFile: Function to read temporary file "tfile" with command "readcmd". {{{2
"    readcmd == %r : replace buffer with newly read file
"            == 0r : read file at top of buffer
"            == r  : read file after current line
"            == t  : leave file in temporary form (ie. don't read into buffer)
fun! s:NetGetFile(readcmd, tfile, method)
"  call Dfunc("NetGetFile(readcmd<".a:readcmd.">,tfile<".a:tfile."> method<".a:method.">)")

  " readcmd=='t': simply do nothing
  if a:readcmd == 't'
"   call Dret("NetGetFile : skip read of <".a:tfile.">")
   return
  endif

  " get name of remote filename (ie. url and all)
  let rfile= bufname("%")
"  call Decho("rfile<".rfile.">")

  if exists("*NetReadFixup")
   " for the use of NetReadFixup (not otherwise used internally)
   let line2= line("$")
  endif

  if a:readcmd[0] == '%'
  " get file into buffer
"   call Decho("get file into buffer")

   " rename the current buffer to the temp file (ie. tfile)
   if g:netrw_cygwin
    let tfile= substitute(a:tfile,'/cygdrive/\(.\)','\1:','')
   else
    let tfile= a:tfile
   endif
"   call Decho("keepalt exe file ".tfile)
   keepalt exe "silent! keepalt file ".tfile

   " edit temporary file (ie. read the temporary file in)
   if     rfile =~ '\.zip$'
"    call Decho("handling remote zip file with zip#Browse(tfile<".tfile.">)")
    call zip#Browse(tfile)
   elseif rfile =~ '\.tar$'
"    call Decho("handling remote tar file with tar#Browse(tfile<".tfile.">)")
    call tar#Browse(tfile)
   elseif rfile =~ '\.tar\.gz'
"    call Decho("handling remote gzip-compressed tar file")
    call tar#Browse(tfile)
   elseif rfile =~ '\.tar\.bz2'
"    call Decho("handling remote bz2-compressed tar file")
    call tar#Browse(tfile)
   else
"    call Decho("edit temporary file")
    e!
   endif

   " rename buffer back to remote filename
   exe "silent! keepalt file ".escape(rfile,' ')
   filetype detect
"   call Dredir("renamed buffer back to remote filename<".rfile."> : expand(%)<".expand("%").">","ls!")
   let line1 = 1
   let line2 = line("$")

  elseif s:FileReadable(a:tfile)
   " read file after current line
"   call Decho("read file<".a:tfile."> after current line")
   let curline = line(".")
   let lastline= line("$")
"   call Decho("exe<".a:readcmd." ".v:cmdarg." ".a:tfile.">  line#".curline)
   exe a:readcmd." ".v:cmdarg." ".a:tfile
   let line1= curline + 1
   let line2= line("$") - lastline + 1

  else
   " not readable
"   call Decho("tfile<".a:tfile."> not readable")
   call netrw#ErrorMsg(s:WARNING,"file <".a:tfile."> not readable",9)
"   call Dret("NetGetFile : tfile<".a:tfile."> not readable")
   return
  endif

  " User-provided (ie. optional) fix-it-up command
  if exists("*NetReadFixup")
"   call Decho("calling NetReadFixup(method<".a:method."> line1=".line1." line2=".line2.")")
   call NetReadFixup(a:method, line1, line2)
"  else " Decho
"   call Decho("NetReadFixup() not called, doesn't exist  (line1=".line1." line2=".line2.")")
  endif

  " update the Buffers menu
  if has("gui") && has("gui_running")
   silent! emenu Buffers.Refresh\ menu
  endif

"  call Decho("readcmd<".a:readcmd."> cmdarg<".v:cmdarg."> tfile<".a:tfile."> readable=".s:FileReadable(a:tfile))

 " make sure file is being displayed
  redraw!
"  call Dret("NetGetFile")
endfun

" ------------------------------------------------------------------------
" NetMethod:  determine method of transfer {{{2
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
  let g:netrw_choice  = a:choice

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
  let scpurm   = '^scp://\([^/#:]\+\)\%([#:]\(\d\+\)\)\=/\(.*\)$'
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
    if s:FileReadable(expand("$HOME/.netrc")) && !g:netrw_ignorenetrc
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

   elseif s:FileReadable(expand("$HOME/.netrc"))
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

  else
   if !exists("g:netrw_quiet")
    call netrw#ErrorMsg(s:WARNING,"cannot determine method",45)
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
"  call Dret("NetMethod : b:netrw_method=".b:netrw_method)
endfun

" ------------------------------------------------------------------------
" NetReadFixup: this sort of function is typically written by the user {{{2
"               to handle extra junk that their system's ftp dumps
"               into the transfer.  This function is provided as an
"               example and as a fix for a Windows 95 problem: in my
"               experience, win95's ftp always dumped four blank lines
"               at the end of the transfer.
if has("win95") && exists("g:netrw_win95ftp") && g:netrw_win95ftp
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
" NetUserPass: set username and password for subsequent ftp transfer {{{2
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

" ===========================================
"  Shared Browsing Support:    {{{1
" ===========================================

" ---------------------------------------------------------------------
" s:BrowserMaps: {{{2
fun! s:BrowserMaps(islocal)
"  call Dfunc("s:BrowserMaps(islocal=".a:islocal.") b:netrw_curdir<".b:netrw_curdir.">")
  if a:islocal
   nnoremap <buffer> <silent> <cr>	:call netrw#LocalBrowseCheck(<SID>NetBrowseChgDir(1,<SID>NetGetWord()))<cr>
   nnoremap <buffer> <silent> <leftmouse> <leftmouse>:call netrw#LocalBrowseCheck(<SID>NetBrowseChgDir(1,<SID>NetGetWord()))<cr>
   nnoremap <buffer> <silent> <c-l>	:call <SID>NetRefresh(1,<SID>NetBrowseChgDir(1,'./'))<cr>
   nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call netrw#LocalBrowseCheck(<SID>NetBrowseChgDir(1,'../'))<cr>
   nnoremap <buffer> <silent> a		:call <SID>NetHide(1)<cr>
   nnoremap <buffer> <silent> mb	:<c-u>call <SID>NetBookmarkDir(0,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> gb	:<c-u>call <SID>NetBookmarkDir(1,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> c		:exe "cd ".b:netrw_curdir<cr>
   nnoremap <buffer> <silent> C		:let g:netrw_chgwin= winnr()<cr>
   nnoremap <buffer> <silent> d		:call <SID>NetMakeDir("")<cr>
   nnoremap <buffer> <silent> <c-h>	:call <SID>NetHideEdit(1)<cr>
   nnoremap <buffer> <silent> i		:call <SID>NetListStyle(1)<cr>
   nnoremap <buffer> <silent> o		:call <SID>NetSplit(3)<cr>
   nnoremap <buffer> <silent> O		:call <SID>LocalObtain()<cr>
   nnoremap <buffer> <silent> p		:call <SID>NetPreview(<SID>NetBrowseChgDir(1,<SID>NetGetWord(),1))<cr>
   nnoremap <buffer> <silent> P		:call <SID>NetPrevWinOpen(1)<cr>
   nnoremap <buffer> <silent> q		:<c-u>call <SID>NetBookmarkDir(2,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetRefresh(1,<SID>NetBrowseChgDir(1,'./'))<cr>
   nnoremap <buffer> <silent> s		:call <SID>NetSortStyle(1)<cr>
   nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(1)<cr>
   nnoremap <buffer> <silent> t		:call <SID>NetSplit(4)<cr>
   nnoremap <buffer> <silent> u		:<c-u>call <SID>NetBookmarkDir(4,expand("%"))<cr>
   nnoremap <buffer> <silent> U		:<c-u>call <SID>NetBookmarkDir(5,expand("%"))<cr>
   nnoremap <buffer> <silent> v		:call <SID>NetSplit(5)<cr>
   nnoremap <buffer> <silent> x		:call netrw#NetBrowseX(<SID>NetBrowseChgDir(1,<SID>NetGetWord(),0),0)"<cr>
   if s:didstarstar || !mapcheck("<s-down>","n")
    nnoremap <buffer> <silent> <s-down>	:Nexplore<cr>
   endif
   if s:didstarstar || !mapcheck("<s-up>","n")
    nnoremap <buffer> <silent> <s-up>	:Pexplore<cr>
   endif
   exe 'nnoremap <buffer> <silent> <del>	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
   exe 'vnoremap <buffer> <silent> <del>	:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
   exe 'nnoremap <buffer> <silent> <rightmouse> <leftmouse>:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
   exe 'vnoremap <buffer> <silent> <rightmouse> <leftmouse>:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
   exe 'nnoremap <buffer> <silent> D		:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
   exe 'vnoremap <buffer> <silent> D		:call <SID>LocalBrowseRm("'.b:netrw_curdir.'")<cr>'
   exe 'nnoremap <buffer> <silent> R		:call <SID>LocalBrowseRename("'.b:netrw_curdir.'")<cr>'
   exe 'vnoremap <buffer> <silent> R		:call <SID>LocalBrowseRename("'.b:netrw_curdir.'")<cr>'
   exe 'nnoremap <buffer> <silent> <Leader>m	:call <SID>NetMakeDir("")<cr>'
   nnoremap <buffer> <F1>		:he netrw-dir<cr>

  else " remote
   call s:RemotePathAnalysis(b:netrw_curdir)
   nnoremap <buffer> <silent> <cr>	:call <SID>NetBrowse(0,<SID>NetBrowseChgDir(0,<SID>NetGetWord()))<cr>
   nnoremap <buffer> <silent> <leftmouse> <leftmouse>:call <SID>NetBrowse(0,<SID>NetBrowseChgDir(0,<SID>NetGetWord()))<cr>
   nnoremap <buffer> <silent> <c-l>	:call <SID>NetRefresh(0,<SID>NetBrowseChgDir(0,'./'))<cr>
   nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call <SID>NetBrowse(0,<SID>NetBrowseChgDir(0,'../'))<cr>
   nnoremap <buffer> <silent> a		:call <SID>NetHide(0)<cr>
   nnoremap <buffer> <silent> mb	:<c-u>call <SID>NetBookmarkDir(0,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> gb	:<c-u>call <SID>NetBookmarkDir(1,b:netrw_cur)<cr>
   nnoremap <buffer> <silent> C		:let g:netrw_chgwin= winnr()<cr>
   nnoremap <buffer> <silent> <c-h>	:call <SID>NetHideEdit(0)<cr>
   nnoremap <buffer> <silent> i		:call <SID>NetListStyle(0)<cr>
   nnoremap <buffer> <silent> o		:call <SID>NetSplit(0)<cr>
   nnoremap <buffer> <silent> O		:call netrw#NetObtain(0)<cr>
   vnoremap <buffer> <silent> O		:call netrw#NetObtain(1)<cr>
   nnoremap <buffer> <silent> p		:call <SID>NetPreview(<SID>NetBrowseChgDir(1,<SID>NetGetWord(),1))<cr>
   nnoremap <buffer> <silent> P		:call <SID>NetPrevWinOpen(0)<cr>
   nnoremap <buffer> <silent> q		:<c-u>call <SID>NetBookmarkDir(2,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetBrowse(0,<SID>NetBrowseChgDir(0,'./'))<cr>
   nnoremap <buffer> <silent> s		:call <SID>NetSortStyle(0)<cr>
   nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(0)<cr>
   nnoremap <buffer> <silent> t		:call <SID>NetSplit(1)<cr>
   nnoremap <buffer> <silent> u		:<c-u>call <SID>NetBookmarkDir(4,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> U		:<c-u>call <SID>NetBookmarkDir(5,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> v		:call <SID>NetSplit(2)<cr>
   nnoremap <buffer> <silent> x		:call netrw#NetBrowseX(<SID>NetBrowseChgDir(0,<SID>NetGetWord()),1)<cr>
   exe 'nnoremap <buffer> <silent> <del>	:call <SID>NetBrowseRm("'.s:user.s:machine.'","'.s:path.'")<cr>'
   exe 'vnoremap <buffer> <silent> <del>	:call <SID>NetBrowseRm("'.s:user.s:machine.'","'.s:path.'")<cr>'
   exe 'nnoremap <buffer> <silent> <rightmouse> <leftmouse>:call <SID>NetBrowseRm("'.s:user.s:machine.'","'.s:path.'")<cr>'
   exe 'vnoremap <buffer> <silent> <rightmouse> <leftmouse>:call <SID>NetBrowseRm("'.s:user.s:machine.'","'.s:path.'")<cr>'
   exe 'nnoremap <buffer> <silent> d	:call <SID>NetMakeDir("'.s:user.s:machine.'")<cr>'
   exe 'nnoremap <buffer> <silent> D	:call <SID>NetBrowseRm("'.s:user.s:machine.'","'.s:path.'")<cr>'
   exe 'vnoremap <buffer> <silent> D	:call <SID>NetBrowseRm("'.s:user.s:machine.'","'.s:path.'")<cr>'
   exe 'nnoremap <buffer> <silent> R	:call <SID>NetBrowseRename("'.s:user.s:machine.'","'.s:path.'")<cr>'
   exe 'vnoremap <buffer> <silent> R	:call <SID>NetBrowseRename("'.s:user.s:machine.'","'.s:path.'")<cr>'
   nnoremap <buffer> <F1>			:he netrw-browse-cmds<cr>
  endif
"  call Dret("s:BrowserMaps")
endfun

" ---------------------------------------------------------------------
" s:NetBrowse: This function uses the command in g:netrw_list_cmd to get a list {{{2
"  of the contents of a remote directory.  It is assumed that the
"  g:netrw_list_cmd has a string, USEPORT HOSTNAME, that needs to be substituted
"  with the requested remote hostname first.
fun! s:NetBrowse(islocal,dirname)
  if !exists("w:netrw_liststyle")|let w:netrw_liststyle= g:netrw_liststyle|endif
"  call Dfunc("NetBrowse(islocal=".a:islocal." dirname<".a:dirname.">) liststyle=".w:netrw_liststyle." ".g:loaded_netrw." buf#".bufnr("%")."<".bufname("%").">")
"  call Dredir("ls!")

  if exists("s:netrw_skipbrowse")
   unlet s:netrw_skipbrowse
"   call Dret("NetBrowse : s:netrw_skipbrowse=".s:netrw_skipbrowse)
   return
  endif

  call s:NetOptionSave()

  if a:islocal && exists("w:netrw_acdkeep") && w:netrw_acdkeep
"   call Decho("handle w:netrw_acdkeep:")
"   call Decho("cd ".escape(a:dirname,s:netrw_cd_escape)." (due to 'acd')")
   exe 'cd '.escape(a:dirname,s:netrw_cd_escape)
"   call Decho("getcwd<".getcwd().">")

  elseif !a:islocal && a:dirname !~ '[\/]$' && a:dirname !~ '^"'
   " looks like a regular file, attempt transfer
"   call Decho("attempt transfer as regular file<".a:dirname.">")

   " remove any filetype indicator from end of dirname, except for the {{{3
   " "this is a directory" indicator (/).
   " There shouldn't be one of those here, anyway.
   let path= substitute(a:dirname,'[*=@|]\r\=$','','e')
"   call Decho("new path<".path.">")
   call s:RemotePathAnalysis(a:dirname)

   " remote-read the requested file into current buffer {{{3
   mark '
   call s:NetrwEnew(a:dirname)
   let b:netrw_curdir= a:dirname
   call s:NetrwSafeOptions()
   setlocal ma noro
"   call Decho("exe silent! keepalt file ".s:method."://".s:user.s:machine."/".escape(s:path,s:netrw_cd_escape)." (bt=".&bt.")")
   exe "silent! keepalt file ".s:method."://".s:user.s:machine."/".escape(s:path,s:netrw_cd_escape)
   exe "silent keepalt doau BufReadPre ".s:fname
   silent call netrw#NetRead(2,s:method."://".s:user.s:machine."/".s:path)
   exe "silent keepalt doau BufReadPost ".s:fname

   " save certain window-oriented variables into buffer-oriented variables {{{3
   call s:SetBufWinVars()
   call s:NetOptionRestore()
   setlocal nomod nowrap

"   call Dret("NetBrowse : file<".s:fname.">")
   return
  endif

  " use buffer-oriented WinVars if buffer ones exist but window ones don't {{{3
  call s:UseBufWinVars()

  " set up some variables {{{3
  let b:netrw_browser_active = 1
  let dirname                = a:dirname
  let s:last_sort_by         = g:netrw_sort_by

  call s:NetMenu(1)                      " set up menu {{{3
  if s:NetGetBuffer(a:islocal,dirname)   " set up buffer {{{3
"   call Dret("NetBrowse : re-using buffer")
   return
  endif

  " set b:netrw_curdir to the new directory name {{{3
"  call Decho("set b:netrw_curdir to the new directory name:")
   let b:netrw_curdir= dirname
  if b:netrw_curdir =~ '[/\\]$'
   let b:netrw_curdir= substitute(b:netrw_curdir,'[/\\]$','','e')
  endif
  if b:netrw_curdir == ''
   if has("amiga")
    " On the Amiga, the empty string connotes the current directory
    let b:netrw_curdir= getcwd()
   else
    " under unix, when the root directory is encountered, the result
    " from the preceding substitute is an empty string.
    let b:netrw_curdir= '/'
   endif
  endif
  if !a:islocal && b:netrw_curdir !~ '/$'
   let b:netrw_curdir= b:netrw_curdir.'/'
  endif
"  call Decho("b:netrw_curdir<".b:netrw_curdir.">")

  " ------------
  " (local only) {{{3
  " ------------
  if a:islocal
"   call Decho("local only:")

   " Set up ShellCmdPost handling.  Append current buffer to browselist
   call s:LocalFastBrowser()

  " handle g:netrw_keepdir: set vim's current directory to netrw's notion of the current directory {{{3
   if !g:netrw_keepdir
"    call Decho("handle keepdir: (g:netrw_keepdir=".g:netrw_keepdir.")")
"    call Decho('exe cd '.escape(b:netrw_curdir,s:netrw_cd_escape))
    try
     exe 'cd '.escape(b:netrw_curdir,s:netrw_cd_escape)
    catch /^Vim\%((\a\+)\)\=:E472/
     call netrw#ErrorMsg(s:ERROR,"unable to change directory to <".b:netrw_curdir."> (permissions?)",33)
     if exists("w:netrw_prvdir")
      let b:netrw_curdir= w:netrw_prvdir
     else
      call s:NetOptionRestore()
      let b:netrw_curdir= dirname
"      call Dret("NetBrowse : reusing buffer#".(exists("bufnum")? bufnum : 'N/A')."<".dirname."> getcwd<".getcwd().">")
      return
     endif
    endtry
   endif

  " --------------------------------
  " remote handling: {{{3
  " --------------------------------
  else
"   call Decho("remote only:")

   " analyze a:dirname and g:netrw_list_cmd {{{4
"   call Decho("b:netrw_curdir<".(exists("b:netrw_curdir")? b:netrw_curdir : "doesn't exist")."> a:dirname<".a:dirname.">")
   if a:dirname == "NetrwTreeListing"
    let dirname= b:netrw_curdir
"    call Decho("(dirname was NetrwTreeListing) dirname<".dirname.">")
   elseif exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir")
    let dirname= substitute(b:netrw_curdir,'\\','/','g')
    if dirname !~ '/$'
     let dirname= dirname.'/'
    endif
    let b:netrw_curdir = dirname
"    call Decho("(liststyle is TREELIST) dirname<".dirname.">")
   else
    let dirname = substitute(a:dirname,'\\','/','g')
"    call Decho("(normal) dirname<".dirname.">")
   endif

   let dirpat  = '^\(\w\{-}\)://\(\w\+@\)\=\([^/]\+\)/\(.*\)$'
   if dirname !~ dirpat
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:ERROR,"netrw doesn't understand your dirname<".dirname.">",20)
    endif
     call s:NetOptionRestore()
"    call Dret("NetBrowse : badly formatted dirname<".dirname.">")
    return
   endif
   let b:netrw_curdir= dirname
"   call Decho("b:netrw_curdir<".b:netrw_curdir."> (remote)")
  endif  " (additional remote handling)

  " -----------------------
  " Directory Listing: {{{3
  " -----------------------
  setlocal noro ma
  call s:BrowserMaps(a:islocal)
  call s:PerformListing(a:islocal)

"  call Dret("NetBrowse")
  return
endfun

" ---------------------------------------------------------------------
" s:NetGetBuffer: {{{2
"   returns 0=cleared buffer
"           1=re-used buffer
fun! s:NetGetBuffer(islocal,dirname)
"  call Dfunc("s:NetGetBuffer(islocal=".a:islocal." dirname<".a:dirname.">)")

  " re-use buffer if possible {{{3
  if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
   " find NetrwTreeList buffer if there is one
   let dirname= "NetrwTreeListing"
   let bufnum = bufnr('\<NetrwTreeListing\>')
   if bufnum != -1
"    call Dret("s:NetGetBuffer : bufnum#".bufnum."<NetrwTreeListing>")
    return
   endif

  else
   " find buffer number of buffer named precisely the same as dirname {{{3
"   call Dredir("ls!")
   let dirname= a:dirname
"   call Decho("find buffer<".dirname.">'s number ")
   let bufnum= bufnr(escape(dirname,'\'))
"   call Decho("findbuf1: bufnum=bufnr('".escape(dirname,'\')."')=".bufnum." (initial)")
   let ibuf= 1
   if bufnum > 0 && bufname(bufnum) != dirname
    let buflast = bufnr("$")
"    call Decho("findbuf2: buflast=".buflast)
    while ibuf <= buflast
     let bname= bufname(ibuf)
"     call Decho("findbuf3: dirname<".dirname."> bufname(".ibuf.")<".bname.">")
     if bname != '' && bname !~ '/' && dirname =~ '/'.bname.'$' | break | endif
     if bname =~ '^'.dirname.'\=$' | break | endif
     let ibuf= ibuf + 1
    endwhile
    if ibuf > buflast
     let bufnum= -1
    else
     let bufnum= ibuf
    endif
"    call Decho("findbuf4: bufnum=".bufnum)
   endif
  endif

  " get enew buffer and name it -or- re-use buffer {{{3
  mark '
  if bufnum < 0 || !bufexists(bufnum)
"   call Decho("get enew buffer")
   call s:NetrwEnew(dirname)
   call s:NetrwSafeOptions()
   " name the buffer
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
"    call Decho('silent! keepalt file NetrwTreeListing')
    silent! keepalt file NetrwTreeListing
   else
"    call Decho('exe silent! keepalt file '.escape(dirname,s:netrw_cd_escape))
"    let v:errmsg= "" " Decho
    let escdirname= escape(dirname,s:netrw_cd_escape)
    exe 'silent! keepalt file '.escdirname
"    call Decho("errmsg<".v:errmsg."> bufnr(".escdirname.")=".bufnr(escdirname)."<".bufname(bufnr(escdirname)).">")
   endif
"   call Decho("named enew buffer#".bufnr("%")."<".bufname("%").">")

  else " Re-use the buffer

"   call Decho("re-use buffer:")
   let eikeep= &ei
   set ei=all
   if getline(2) =~ '^" Netrw Directory Listing'
"    call Decho("re-use buffer#".bufnum."<".((bufnum > 0)? bufname(bufnum) : "")."> using:  keepalt b ".bufnum)
    exe "keepalt b ".bufnum
   else
"    call Decho("reusing buffer#".bufnum."<".((bufnum > 0)? bufname(bufnum) : "")."> using:  b ".bufnum)
    exe "b ".bufnum
   endif
   let &ei= eikeep
   if line("$") <= 1
    call s:NetrwListSettings(a:islocal)
"    call Dret("s:NetGetBuffer 0 : re-using buffer#".bufnr("%").", but its empty, so refresh it")
    return 0
   elseif exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
"    call Decho("clear buffer<".expand("%")."> with :%d")
    silent %d
    call s:NetrwListSettings(a:islocal)
"    call Dret("s:NetGetBuffer 0 : re-using buffer#".bufnr("%").", but treelist mode always needs a refresh")
    return 0
   else
"    call Dret("s:NetGetBuffer 1 : buf#".bufnr("%"))
    return 1
   endif
  endif

  " do netrw settings: make this buffer not-a-file, modifiable, not line-numbered, etc {{{3
  "     fastbrowse  Local  Remote   Hiding a buffer implies it may be re-used (fast)
  "  slow   0         D      D      Deleting a buffer implies it will not be re-used (slow)
  "  med    1         D      H
  "  fast   2         H      H
  let fname= expand("%")
  call s:NetrwListSettings(a:islocal)
  exe "file ".escape(fname,' ')

  " delete all lines from buffer {{{3
"  call Decho("clear buffer<".expand("%")."> with :%d")
  keepalt silent! %d

"  call Dret("s:NetGetBuffer 0 : buf#".bufnr("%"))
  return 0
endfun

" ---------------------------------------------------------------------
" s:NetrwListSettings: {{{2
fun! s:NetrwListSettings(islocal)
"  call Dfunc("s:NetrwListSettings(islocal=".a:islocal.")")
  let fname= bufname("%")
  setlocal bt=nofile nobl ma nonu nowrap noro
  exe "file ".escape(fname,' ')
  if g:netrw_use_noswf
   setlocal noswf
  endif
"  call Dredir("ls!")
"  call Decho("exe setlocal ts=".g:netrw_maxfilenamelen)
  exe "setlocal ts=".g:netrw_maxfilenamelen
  if g:netrw_fastbrowse > a:islocal
   setlocal bh=hide
  else
   setlocal bh=delete
  endif
"  call Dret("s:NetrwListSettings")
endfun

" ---------------------------------------------------------------------
" s:PerformListing: {{{2
fun! s:PerformListing(islocal)
"  call Dfunc("s:PerformListing(islocal=".a:islocal.") buf(%)=".bufnr("%")."<".bufname("%").">")

"   if exists("g:netrw_silent") && g:netrw_silent == 0 && &ch >= 1	" Decho
"    call Decho("(netrw) Processing your browsing request...")
"   endif								" Decho

"   call Decho('w:netrw_liststyle='.(exists("w:netrw_liststyle")? w:netrw_liststyle : 'n/a'))
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict")
    " force a refresh for tree listings
"    call Decho("force refresh for treelisting: clear buffer<".expand("%")."> with :%d")
    setlocal ma noro
    keepjumps %d
   endif

  " save current directory on directory history list
  call s:NetBookmarkDir(3,b:netrw_curdir)

  " Set up the banner {{{3
"  call Decho("set up banner")
  keepjumps put ='\" ============================================================================'
  keepjumps put ='\" Netrw Directory Listing                                        (netrw '.g:loaded_netrw.')'
  keepjumps put ='\"   '.b:netrw_curdir
  keepjumps 1d
  let w:netrw_bannercnt= 3
  exe w:netrw_bannercnt

  let sortby= g:netrw_sort_by
  if g:netrw_sort_direction =~ "^r"
   let sortby= sortby." reversed"
  endif

  " Sorted by... {{{3
"  call Decho("handle specified sorting: g:netrw_sort_by<".g:netrw_sort_by.">")
  if g:netrw_sort_by =~ "^n"
"   call Decho("directories will be sorted by name")
   " sorted by name
   keepjumps put ='\"   Sorted by      '.sortby
   keepjumps put ='\"   Sort sequence: '.g:netrw_sort_sequence
   let w:netrw_bannercnt= w:netrw_bannercnt + 2
  else
"   call Decho("directories will be sorted by size or time")
   " sorted by size or date
   keepjumps put ='\"   Sorted by '.sortby
   let w:netrw_bannercnt= w:netrw_bannercnt + 1
  endif
  exe w:netrw_bannercnt

  " Hiding...  -or-  Showing... {{{3
"  call Decho("handle hiding/showing (g:netrw_hide=".g:netrw_list_hide." g:netrw_list_hide<".g:netrw_list_hide.">)")
  if g:netrw_list_hide != "" && g:netrw_hide
   if g:netrw_hide == 1
    keepjumps put ='\"   Hiding:        '.g:netrw_list_hide
   else
    keepjumps put ='\"   Showing:       '.g:netrw_list_hide
   endif
   let w:netrw_bannercnt= w:netrw_bannercnt + 1
  endif
  exe w:netrw_bannercnt
  keepjumps put ='\"   Quick Help: <F1>:help  -:go up dir  D:delete  R:rename  s:sort-by  x:exec'
  keepjumps put ='\" ============================================================================'
  let w:netrw_bannercnt= w:netrw_bannercnt + 2

  " bannercnt should index the line just after the banner
  let w:netrw_bannercnt= w:netrw_bannercnt + 1
  exe w:netrw_bannercnt
"  call Decho("bannercnt=".w:netrw_bannercnt." (should index line just after banner) line($)=".line("$"))

  " set up syntax highlighting {{{3
"  call Decho("set up syntax highlighting")
  if has("syntax")
   setlocal ft=netrw
   if !exists("g:syntax_on") || !g:syntax_on
    setlocal ft=
   endif
  endif

  " get list of files
  if a:islocal
   call s:LocalListing()
  else " remote
   call s:RemoteListing()
  endif
"  call Decho("w:netrw_bannercnt=".w:netrw_bannercnt." (banner complete)")

  " manipulate the directory listing (hide, sort) {{{3
  if line("$") >= w:netrw_bannercnt
"   call Decho("manipulate directory listing (hide)")
"   call Decho("g:netrw_hide=".g:netrw_hide." g:netrw_list_hide<".g:netrw_list_hide.">")
   if g:netrw_hide && g:netrw_list_hide != ""
    call s:NetListHide()
   endif
   if line("$") >= w:netrw_bannercnt
"    call Decho("manipulate directory listing (sort) : g:netrw_sort_by<".g:netrw_sort_by.">")

    if g:netrw_sort_by =~ "^n"
     " sort by name
     call s:SetSort()

     if w:netrw_bannercnt < line("$")
"      call Decho("g:netrw_sort_direction=".g:netrw_sort_direction." (bannercnt=".w:netrw_bannercnt.")")
      if g:netrw_sort_direction =~ 'n'
       " normal direction sorting
       exe 'silent keepjumps '.w:netrw_bannercnt.',$sort'
      else
       " reverse direction sorting
       exe 'silent keepjumps '.w:netrw_bannercnt.',$sort!'
      endif
     endif
     " remove priority pattern prefix
"     call Decho("remove priority pattern prefix")
     exe 'silent keepjumps '.w:netrw_bannercnt.',$s/^\d\{3}\///e'

    elseif a:islocal
     if w:netrw_bannercnt < line("$")
"      call Decho("g:netrw_sort_direction=".g:netrw_sort_direction)
      if g:netrw_sort_direction =~ 'n'
"       call Decho('exe silent keepjumps '.w:netrw_bannercnt.',$sort')
       exe 'silent keepjumps '.w:netrw_bannercnt.',$sort'
      else
"       call Decho('exe silent keepjumps '.w:netrw_bannercnt.',$sort!')
       exe 'silent keepjumps '.w:netrw_bannercnt.',$sort!'
      endif
     endif
     exe 'silent keepjumps '.w:netrw_bannercnt.',$s/^\d\{-}\///e'
    endif

   elseif g:netrw_sort_direction =~ 'r'
"    call Decho('reverse the sorted listing')
    exe 'silent keepjumps '.w:netrw_bannercnt.'g/^/m '.w:netrw_bannercnt
   endif
  endif

  " convert to wide/tree listing {{{3
"  call Decho("modify display if wide/tree listing style")
  call s:NetWideListing()
  call s:NetTreeListing(b:netrw_curdir)

  if exists("w:netrw_bannercnt") && line("$") > w:netrw_bannercnt
   " place cursor on the top-left corner of the file listing
"   call Decho("place cursor on top-left corner of file listing")
   exe 'silent '.w:netrw_bannercnt
   norm! 0
  endif

  " record previous current directory
  let w:netrw_prvdir= b:netrw_curdir
"  call Decho("record netrw_prvdir<".w:netrw_prvdir.">")

  " save certain window-oriented variables into buffer-oriented variables {{{3
  call s:SetBufWinVars()
  call s:NetOptionRestore()

  " set display to netrw display settings
"  call Decho("set display to netrw display settings (noma nomod etc)")
  setlocal noma nomod nonu nobl nowrap ro
  if exists("s:treecurpos")
   call setpos('.',s:treecurpos)
   unlet s:treecurpos
  endif

"  call Dret("s:PerformListing : curpos<".string(getpos(".")).">")
endfun

" ---------------------------------------------------------------------
"  s:NetBrowseChgDir: constructs a new directory based on the current {{{2
"                     directory and a new directory name
fun! s:NetBrowseChgDir(islocal,newdir,...)
"  call Dfunc("s:NetBrowseChgDir(islocal=".a:islocal."> newdir<".a:newdir.">) a:0=".a:0." curpos<".string(getpos("."))."> b:netrw_curdir<".(exists("b:netrw_curdir")? b:netrw_curdir : "").">")

  if !exists("b:netrw_curdir")
"   call Decho("(NetBrowseChgDir) b:netrw_curdir doesn't exist!")
   echoerr "(NetBrowseChgDir) b:netrw_curdir doesn't exist!"
"   call Dret("s:NetBrowseChgDir")
   return
  endif

  call netrw#NetSavePosn()
  let nbcd_curpos = getpos('.')
  let dirname     = substitute(b:netrw_curdir,'\\','/','ge')
  let newdir      = a:newdir

  " set up o/s-dependent directory recognition pattern
  if has("amiga")
   let dirpat= '[\/:]$'
  else
   let dirpat= '[\/]$'
  endif
"  call Decho("dirname<".dirname.">  dirpat<".dirpat.">")

  if dirname !~ dirpat
   " apparently vim is "recognizing" that it is in a directory and
   " is removing the trailing "/".  Bad idea, so I have to put it back.
   let dirname= dirname.'/'
"   call Decho("adjusting dirname<".dirname.">")
  endif

  if newdir !~ dirpat
   " handling a file
"   call Decho('case "handling a file": newdir<'.newdir.'> !~ dirpat<'.dirpat.">")
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict") && newdir !~ '^\(/\|\a:\)'
    let dirname= s:NetTreeDir().newdir
"    call Decho("tree listing")
   elseif newdir =~ '^\(/\|\a:\)'
    let dirname= newdir
   else
    let dirname= s:ComposePath(dirname,newdir)
   endif
"   call Decho("handling a file: dirname<".dirname."> (a:0=".a:0.")")
   " this lets NetBrowseX avoid the edit
   if a:0 < 1
"    call Decho("dirname<".dirname."> netrw_cd_escape<".s:netrw_cd_escape."> browse_split=".g:netrw_browse_split)
"    call Decho("about to edit<".escape(dirname,s:netrw_cd_escape).">  didsplit=".(exists("s:didsplit")? s:didsplit : "doesn't exist"))
    if !exists("s:didsplit")
     if     g:netrw_browse_split == 1
      new
      wincmd _
     elseif g:netrw_browse_split == 2
      rightb vert new
      wincmd |
     elseif g:netrw_browse_split == 3
      tabnew
     else
      " handling a file, didn't split, so remove menu
"      call Decho("handling a file+didn't split, so remove menu")
      call s:NetMenu(0)
      " optional change to window
      if g:netrw_chgwin >= 1 
       exe g:netrw_chgwin."wincmd w"
      endif
     endif
    endif
    " edit the file
    " its local only: LocalBrowseCheck() doesn't edit a file, but NetBrowse() will
    if a:islocal
"     call Decho("edit file: exe e! ".escape(dirname,s:netrw_cd_escape))
     exe "e! ".escape(dirname,s:netrw_cd_escape)
    endif
    setlocal ma nomod noro
   endif

  elseif newdir =~ '^/'
   " just go to the new directory spec
"   call Decho('case "just go to new directory spec": newdir<'.newdir.'>')
   let dirname= newdir

  elseif newdir == './'
   " refresh the directory list
"   call Decho('case "refresh directory listing": newdir == "./"')

  elseif newdir == '../'
   " go up one directory
"   call Decho('case "go up one directory": newdir == "../"')

   if w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict")
    " force a refresh
"    call Decho("clear buffer<".expand("%")."> with :%d")
    setlocal noro ma
    keepjumps %d
   endif

   if has("amiga")
    " amiga
"    call Decho('case "go up one directory": newdir == "../" and amiga')
    if a:islocal
     let dirname= substitute(dirname,'^\(.*[/:]\)\([^/]\+$\)','\1','')
     let dirname= substitute(dirname,'/$','','')
    else
     let dirname= substitute(dirname,'^\(.*[/:]\)\([^/]\+/$\)','\1','')
    endif
"    call Decho("amiga: dirname<".dirname."> (go up one dir)")

   else
    " unix or cygwin
"    call Decho('case "go up one directory": newdir == "../" and unix or cygwin')
    if a:islocal
     let dirname= substitute(dirname,'^\(.*\)/\([^/]\+\)/$','\1','')
     if dirname == ""
      let dirname= '/'
     endif
    else
     let dirname= substitute(dirname,'^\(\a\+://.\{-}/\{1,2}\)\(.\{-}\)\([^/]\+\)/$','\1\2','')
    endif
"    call Decho("unix: dirname<".dirname."> (go up one dir)")
   endif

  elseif w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict")
"   call Decho('case liststyle is TREELIST and w:netrw_treedict exists')
   " force a refresh (for TREELIST, wait for NetTreeDir() to force the refresh)
   setlocal noro ma
   if !(exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir"))
"    call Decho("clear buffer<".expand("%")."> with :%d")
    keepjumps %d
   endif
   let treedir      = s:NetTreeDir()
   let s:treecurpos = nbcd_curpos
   let haskey= 0
"   call Decho("w:netrw_treedict<".string(w:netrw_treedict).">")

   " search treedict for tree dir as-is
   if has_key(w:netrw_treedict,treedir)
"    call Decho('....searched for treedir<'.treedir.'> : found it!')
    let haskey= 1
   else
"    call Decho('....searched for treedir<'.treedir.'> : not found')
   endif

   " search treedict for treedir with a / appended
   if !haskey && treedir !~ '/$'
    if has_key(w:netrw_treedict,treedir."/")
     let treedir= treedir."/"
"     call Decho('....searched.for treedir<'.treedir.'> found it!')
     let haskey = 1
    else
"     call Decho('....searched for treedir<'.treedir.'/> : not found')
    endif
   endif

   " search treedict for treedir with any trailing / elided
   if !haskey && treedir =~ '/$'
    let treedir= substitute(treedir,'/$','','')
    if has_key(w:netrw_treedict,treedir)
"     call Decho('....searched.for treedir<'.treedir.'> found it!')
     let haskey = 1
    else
"     call Decho('....searched for treedir<'.treedir.'> : not found')
    endif
   endif

   if haskey
    " close tree listing for selected subdirectory
"    call Decho("closing selected subdirectory<".dirname.">")
    call remove(w:netrw_treedict,treedir)
"    call Decho("removed     entry<".dirname."> from treedict")
"    call Decho("yielding treedict<".string(w:netrw_treedict).">")
    let dirname= w:netrw_treetop
   else
    " go down one directory
    let dirname= substitute(treedir,'/*$','/','')
"    call Decho("go down one dir: treedir<".treedir.">")
   endif

  else
   " go down one directory
   let dirname= s:ComposePath(dirname,newdir)
"   call Decho("go down one dir: dirname<".dirname."> newdir<".newdir.">")
  endif

"  call Dret("s:NetBrowseChgDir <".dirname."> : curpos<".string(getpos(".")).">")
  return dirname
endfun

" ---------------------------------------------------------------------
" s:NetHide: this function is invoked by the "a" map for browsing {{{2
"          and switches the hiding mode
fun! s:NetHide(islocal)
"  call Dfunc("NetHide(islocal=".a:islocal.")")
   let g:netrw_hide=(g:netrw_hide+1)%3
   exe "norm! 0"
   if g:netrw_hide && g:netrw_list_hide == ""
    call netrw#ErrorMsg(s:WARNING,"your hiding list is empty!",49)
"    call Dret("NetHide")
    return
   endif
   call netrw#NetSavePosn()
   call s:NetRefresh(a:islocal,s:NetBrowseChgDir(a:islocal,'./'))
"  call Dret("NetHide")
endfun

" ---------------------------------------------------------------------

" ===========================================
" s:NetPreview: {{{2
fun! s:NetPreview(path) range
"  call Dfunc("NetPreview(path<".a:path.">)")
  if has("quickfix")
   if !isdirectory(a:path)
    exe "pedit ".escape(a:path,g:netrw_fname_escape)
   elseif !exists("g:netrw_quiet")
    call netrw#ErrorMsg(s:WARNING,"sorry, cannot preview a directory such as <".a:path.">",38)
   endif
  elseif !exists("g:netrw_quiet")
   call netrw#ErrorMsg(s:WARNING,"sorry, to preview your vim needs the quickfix feature compiled in",39)
  endif
"  call Dret("NetPreview")
endfun

" ---------------------------------------------------------------------
" s:NetSortStyle: change sorting style (name - time - size) and refresh display {{{2
fun! s:NetSortStyle(islocal)
"  call Dfunc("s:NetSortStyle(islocal=".a:islocal.") netrw_sort_by<".g:netrw_sort_by.">")
  call s:NetSaveWordPosn()

  let g:netrw_sort_by= (g:netrw_sort_by =~ 'n')? 'time' : (g:netrw_sort_by =~ 't')? 'size' : 'name'
  norm! 0
  call netrw#NetSavePosn()
  call s:NetRefresh(a:islocal,s:NetBrowseChgDir(a:islocal,'./'))

"  call Dret("s:NetSortStyle : netrw_sort_by<".g:netrw_sort_by.">")
endfun

" ---------------------------------------------------------------------
"  Remote Directory Browsing Support:    {{{1
" ===========================================

" ---------------------------------------------------------------------
" s:RemoteListing: {{{2
fun! s:RemoteListing()
"  call Dfunc("s:RemoteListing() b:netrw_curdir<".b:netrw_curdir.">)")

  call s:RemotePathAnalysis(b:netrw_curdir)

  " sanity check:
  if exists("b:netrw_method") && b:netrw_method =~ '[235]'
"   call Decho("b:netrw_method=".b:netrw_method)
   if !executable("ftp")
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:ERROR,"this system doesn't support remote directory listing via ftp",18)
    endif
    call s:NetOptionRestore()
"    call Dret("s:RemoteListing")
    return
   endif

  elseif !exists("g:netrw_list_cmd") || g:netrw_list_cmd == ''
   if !exists("g:netrw_quiet")
    if g:netrw_list_cmd == ""
     call netrw#ErrorMsg(s:ERROR,g:netrw_ssh_cmd." is not executable on your system",47)
    else
     call netrw#ErrorMsg(s:ERROR,"this system doesn't support remote directory listing via ".g:netrw_list_cmd,19)
    endif
   endif

   call s:NetOptionRestore()
"   call Dret("s:RemoteListing")
   return
  endif  " (remote handling sanity check)

  if exists("b:netrw_method")
"   call Decho("setting w:netrw_method<".b:netrw_method.">")
   let w:netrw_method= b:netrw_method
  endif

  if s:method == "ftp"
   " use ftp to get remote file listing
"   call Decho("use ftp to get remote file listing")
   let s:method  = "ftp"
   let listcmd = g:netrw_ftp_list_cmd
   if g:netrw_sort_by =~ '^t'
    let listcmd= g:netrw_ftp_timelist_cmd
   elseif g:netrw_sort_by =~ '^s'
    let listcmd= g:netrw_ftp_sizelist_cmd
   endif
"   call Decho("listcmd<".listcmd."> (using g:netrw_ftp_list_cmd)")
   call s:NetBrowseFtpCmd(s:path,listcmd)
"   exe "keepjumps ".w:netrw_bannercnt.',$g/^./call Decho("raw listing: ".getline("."))'

   if w:netrw_liststyle == s:THINLIST || w:netrw_liststyle == s:WIDELIST || w:netrw_liststyle == s:TREELIST
    " shorten the listing
"    call Decho("generate short listing")
    exe "keepjumps ".w:netrw_bannercnt

    " cleanup
    if g:netrw_ftp_browse_reject != ""
     exe "silent! g/".g:netrw_ftp_browse_reject."/keepjumps d"
    endif
    silent! keepjumps %s/\r$//e

    " if there's no ../ listed, then put ./ and ../ in
    let line1= line(".")
    exe "keepjumps ".w:netrw_bannercnt
    let line2= search('^\.\.\/\%(\s\|$\)','cnW')
    if line2 == 0
"     call Decho("netrw is putting ./ and ../ into listing")
     keepjumps put='../'
     keepjumps put='./'
    endif
    exe "keepjumps ".line1
    keepjumps norm! 0

"    call Decho("line1=".line1." line2=".line2." line(.)=".line("."))
    if search('^\d\{2}-\d\{2}-\d\{2}\s','n') " M$ ftp site cleanup
"     call Decho("M$ ftp cleanup")
     exe 'silent! keepjumps '.w:netrw_bannercnt.',$s/^\d\{2}-\d\{2}-\d\{2}\s\+\d\+:\d\+[AaPp][Mm]\s\+\%(<DIR>\|\d\+\)\s\+//'
    else " normal ftp cleanup
"     call Decho("normal ftp cleanup")
     exe 'silent! keepjumps '.w:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2/e'
     exe "silent! keepjumps ".w:netrw_bannercnt.',$g/ -> /s# -> .*/$#/#e'
     exe "silent! keepjumps ".w:netrw_bannercnt.',$g/ -> /s# -> .*$#/#e'
    endif
   endif

  else
   " use ssh to get remote file listing {{{3
"   call Decho("use ssh to get remote file listing: s:netrw_shq<".g:netrw_shq."> s:path<".s:path."> s:netrw_cd_escape<".s:netrw_cd_escape.">")
   let listcmd= s:MakeSshCmd(g:netrw_list_cmd)
"   call Decho("listcmd<".listcmd."> (using g:netrw_list_cmd)")
   if g:netrw_scp_cmd =~ '^pscp'
"    call Decho("1: exe silent r! ".listcmd.g:netrw_shq.s:path.g:netrw_shq)
    exe "silent r! ".listcmd.g:netrw_shq.s:path.g:netrw_shq
    " remove rubbish and adjust listing format of 'pscp' to 'ssh ls -FLa' like
    g/^Listing directory/d
    g/^d[-rwx][-rwx][-rwx]/s+$+/+e
    silent g/^l[-rwx][-rwx][-rwx]/s+$+@+e
    if g:netrw_liststyle != s:LONGLIST 
     g/^[dlsp-][-rwx][-rwx][-rwx]/s/^.*\s\(\S\+\)$/\1/e
    endif
   else
    if s:path == ""
"     call Decho("2: exe silent r! ".listcmd)
     exe "silent r! ".listcmd
    else
"     call Decho("3: exe silent r! ".listcmd." ".g:netrw_shq.s:path.g:netrw_shq)
     exe "silent r! ".listcmd." ".g:netrw_shq.s:path.g:netrw_shq
    endif
   endif

   " cleanup
   if g:netrw_ftp_browse_reject != ""
"    call Decho("(cleanup) exe silent! g/".g:netrw_ssh_browse_reject."/keepjumps d")
    exe "silent! g/".g:netrw_ssh_browse_reject."/keepjumps d"
   endif
  endif

  if w:netrw_liststyle == s:LONGLIST
   " do a long listing; these substitutions need to be done prior to sorting {{{3
"   call Decho("fix long listing:")

   if s:method == "ftp"
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
     if b:netrw_curdir != '/'
      exe 'keepjumps '.w:netrw_bannercnt."put='../'"
     endif
    endif
   exe "keepjumps ".line1
   keepjumps norm! 0
   endif

   if search('^\d\{2}-\d\{2}-\d\{2}\s','n') " M$ ftp site cleanup
"    call Decho("M$ ftp site listing cleanup")
    exe 'silent! keepjumps '.w:netrw_bannercnt.',$s/^\(\d\{2}-\d\{2}-\d\{2}\s\+\d\+:\d\+[AaPp][Mm]\s\+\%(<DIR>\|\d\+\)\s\+\)\(\w.*\)$/\2\t\1/'
   elseif exists("w:netrw_bannercnt") && w:netrw_bannercnt <= line("$")
"    call Decho("normal ftp site listing cleanup: bannercnt=".w:netrw_bannercnt." line($)=".line("$"))
    exe 'silent keepjumps '.w:netrw_bannercnt.',$s/ -> .*$//e'
    exe 'silent keepjumps '.w:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2\t\1/e'
    exe 'silent keepjumps '.w:netrw_bannercnt
   endif
  endif

"  if exists("w:netrw_bannercnt") && w:netrw_bannercnt <= line("$") " Decho
"   exe "keepjumps ".w:netrw_bannercnt.',$g/^./call Decho("listing: ".getline("."))'
"  endif " Decho
"  call Dret("s:RemoteListing")
endfun

" ---------------------------------------------------------------------
"  NetGetWord: it gets the directory named under the cursor {{{2
fun! s:NetGetWord()
"  call Dfunc("NetGetWord() line#".line(".")." liststyle=".g:netrw_liststyle." virtcol=".virtcol("."))
  call s:UseBufWinVars()

  " insure that w:netrw_liststyle is set up
  if !exists("w:netrw_liststyle")
   if exists("g:netrw_liststyle")
    let w:netrw_liststyle= g:netrw_liststyle
   else
    let w:netrw_liststyle= s:THINLIST
   endif
"   call Decho("w:netrw_liststyle=".w:netrw_liststyle)
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
    exe 'silent keepjumps '.w:netrw_bannercnt
   endif

  elseif w:netrw_liststyle == s:THINLIST
"   call Decho("thin column handling")
   norm! 0
   let dirname= getline(".")

  elseif w:netrw_liststyle == s:LONGLIST
"   call Decho("long column handling")
   norm! 0
   let dirname= substitute(getline("."),'^\(\%(\S\+ \)*\S\+\).\{-}$','\1','e')

  elseif w:netrw_liststyle == s:TREELIST
"   call Decho("treelist handling")
   let dirname= substitute(getline("."),'^\(| \)*','','e')

  else
"   call Decho("obtain word from wide listing")
   let dirname= getline(".")

   if !exists("b:netrw_cpf")
    let b:netrw_cpf= 0
    exe 'silent keepjumps '.w:netrw_bannercnt.',$g/^./if virtcol("$") > b:netrw_cpf|let b:netrw_cpf= virtcol("$")|endif'
"    call Decho("computed cpf")
   endif

"   call Decho("buf#".bufnr("%")."<".bufname("%").">")
   let filestart = (virtcol(".")/b:netrw_cpf)*b:netrw_cpf
"   call Decho("filestart= ([virtcol=".virtcol(".")."]/[b:netrw_cpf=".b:netrw_cpf."])*b:netrw_cpf=".filestart."  bannercnt=".w:netrw_bannercnt)
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
"  call Dfunc("NetBrowseRm(usrhost<".a:usrhost."> path<".a:path.">) virtcol=".virtcol("."))
"  call Decho("firstline=".a:firstline." lastline=".a:lastline)

  " preparation for removing multiple files/directories
  let ctr= a:firstline
  let all= 0

  " remove multiple files and directories
  while ctr <= a:lastline
   exe ctr

   let rmfile= s:NetGetWord()
"   call Decho("rmfile<".rmfile.">")

   if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '[\/]$')
    " attempt to remove file
"    call Decho("attempt to remove file")
    if !all
     echohl Statement
     call inputsave()
     let ok= input("Confirm deletion of file<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
     call inputrestore()
     echohl NONE
     if ok == ""
      let ok="no"
     endif
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
      let netrw_rm_cmd= s:MakeSshCmd(g:netrw_rm_cmd)
"      call Decho("attempt to remove file: system(".netrw_rm_cmd.")")
      let ret= s:System("system",netrw_rm_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)
     endif
    elseif ok =~ 'q\%[uit]'
     break
    endif

   else
    " attempt to remove directory
"    call Decho("attempt to remove directory")
    if !all
     call inputsave()
     let ok= input("Confirm deletion of directory<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
     call inputrestore()
     if ok == ""
      let ok="no"
     endif
     let ok= substitute(ok,'\[{y(es)},n(o),a(ll),q(uit)]\s*','','e')
     if ok =~ 'a\%[ll]'
      let all= 1
     endif
    endif

    if all || ok =~ 'y\%[es]' || ok == ""
     if exists("w:netrw_method") && (w:netrw_method == 2 || w:netrw_method == 3)
      call s:NetBrowseFtpCmd(a:path,"rmdir ".rmfile)
     else
      let rmfile          = substitute(a:path.rmfile,'/$','','')
      let netrw_rmdir_cmd = s:MakeSshCmd(g:netrw_rmdir_cmd).' '.rmfile
"      call Decho("attempt to remove dir: system(".netrw_rmdir_cmd.")")
      let ret= s:System("system",netrw_rmdir_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)

      if v:shell_error != 0
"       call Decho("v:shell_error not 0")
       let netrw_rmf_cmd= s:MakeSshCmd(g:netrw_rmf_cmd).' '.substitute(rmfile,'[\/]$','','e')
"       call Decho("2nd attempt to remove dir: system(".netrw_rmf_cmd.")")
       let ret= s:System("system",netrw_rmf_cmd)
"       call Decho("returned=".ret." errcode=".v:shell_error)

       if v:shell_error != 0 && !exists("g:netrw_quiet")
       	call netrw#ErrorMsg(s:ERROR,"unable to remove directory<".rmfile."> -- is it empty?",22)
       endif
      endif
     endif

    elseif ok =~ 'q\%[uit]'
     break
    endif
   endif

   let ctr= ctr + 1
  endwhile

  " refresh the (remote) directory listing
"  call Decho("refresh remote directory listing")
  call netrw#NetSavePosn()
  call s:NetRefresh(0,s:NetBrowseChgDir(0,'./'))

"  call Dret("NetBrowseRm")
endfun

" ---------------------------------------------------------------------
" NetBrowseRename: rename a remote file or directory {{{2
fun! s:NetBrowseRename(usrhost,path) range
"  call Dfunc("NetBrowseRename(usrhost<".a:usrhost."> path<".a:path.">)")

  " preparation for removing multiple files/directories
  let ctr        = a:firstline
  let rename_cmd = s:MakeSshCmd(g:netrw_rename_cmd)

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
    let ret= s:System("system",rename_cmd.' "'.escape(oldname,s:netrw_cd_escape).'" "'.escape(newname,s:netrw_cd_escape).'"')
   endif

   let ctr= ctr + 1
  endwhile

  " refresh the directory
  let curline= line(".")
  call s:NetBrowse(0,s:NetBrowseChgDir(0,'./'))
  exe "keepjumps ".curline
"  call Dret("NetBrowseRename")
endfun

" ---------------------------------------------------------------------
" NetRefresh: {{{2
fun! s:NetRefresh(islocal,dirname)
"  call Dfunc("NetRefresh(islocal<".a:islocal.">,dirname=".a:dirname.") hide=".g:netrw_hide." sortdir=".g:netrw_sort_direction)
  " at the current time (Mar 19, 2007) all calls to NetRefresh() call NetBrowseChgDir() first.
  " NetBrowseChgDir() may clear the display; hence a NetSavePosn() may not work if its placed here.
  " Also, NetBrowseChgDir() now does a NetSavePosn() itself.
  setlocal ma noro
"  call Decho("clear buffer<".expand("%")."> with :%d")
  %d
  if a:islocal
   call netrw#LocalBrowseCheck(a:dirname)
  else
   call s:NetBrowse(a:islocal,a:dirname)
  endif
  call netrw#NetRestorePosn()
  redraw!
"  call Dret("NetRefresh")
endfun

" ---------------------------------------------------------------------
" NetSplit: mode {{{2
"           =0 : net   and o
"           =1 : net   and t
"           =2 : net   and v
"           =3 : local and o
"           =4 : local and t
"           =5 : local and v
fun! s:NetSplit(mode)
"  call Dfunc("NetSplit(mode=".a:mode.") alto=".g:netrw_alto." altv=".g:netrw_altv)

  call s:SaveWinVars()

  if a:mode == 0
   " remote and o
   exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   let s:didsplit= 1
   call s:RestoreWinVars()
   call s:NetBrowse(0,s:NetBrowseChgDir(0,s:NetGetWord()))
   unlet s:didsplit

  elseif a:mode == 1
   " remote and t
   let cursorword  = s:NetGetWord()
   tabnew
   let s:didsplit= 1
   call s:RestoreWinVars()
   call s:NetBrowse(0,s:NetBrowseChgDir(0,cursorword))
   unlet s:didsplit

  elseif a:mode == 2
   " remote and v
   exe (g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v"
   let s:didsplit= 1
   call s:RestoreWinVars()
   call s:NetBrowse(0,s:NetBrowseChgDir(0,s:NetGetWord()))
   unlet s:didsplit

  elseif a:mode == 3
   " local and o
   exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   let s:didsplit= 1
   call s:RestoreWinVars()
   call netrw#LocalBrowseCheck(s:NetBrowseChgDir(1,s:NetGetWord()))
   unlet s:didsplit

  elseif a:mode == 4
   " local and t
   let netrw_curdir= b:netrw_curdir
   let cursorword  = s:NetGetWord()
   tabnew
   let b:netrw_curdir= netrw_curdir
   let s:didsplit= 1
   call s:RestoreWinVars()
   call netrw#LocalBrowseCheck(s:NetBrowseChgDir(1,cursorword))
   unlet s:didsplit

  elseif a:mode == 5
   " local and v
   exe (g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v"
   let s:didsplit= 1
   call s:RestoreWinVars()
   call netrw#LocalBrowseCheck(s:NetBrowseChgDir(1,s:NetGetWord()))
   unlet s:didsplit

  else
   call netrw#ErrorMsg(s:ERROR,"(NetSplit) unsupported mode=".a:mode,45)
  endif

"  call Dret("NetSplit")
endfun

" ---------------------------------------------------------------------
" NetBrowseX:  allows users to write custom functions to operate on {{{2
"              files given their extension.  Passes 0=local, 1=remote
fun! netrw#NetBrowseX(fname,remote)
"  call Dfunc("NetBrowseX(fname<".a:fname."> remote=".a:remote.")")

  " set up the filename
  " (lower case the extension, make a local copy of a remote file)
  let exten= substitute(a:fname,'.*\.\(.\{-}\)','\1','e')
  if has("win32") || has("win95") || has("win64") || has("win16")
   let exten= substitute(exten,'^.*$','\L&\E','')
  endif
  let fname= escape(a:fname,"%#")
"  call Decho("fname<".fname."> after escape()")

  " seems kde systems often have gnome-open due to dependencies, even though
  " gnome-open's subsidiary display tools are largely absent.  Kde systems
  " usually have "kdeinit" running, though...  (tnx Mikolaj Machowski)
  if !exists("s:haskdeinit")
   if has("unix")
    let s:haskdeinit= s:System("system",'ps -e') =~ 'kdeinit'
    if v:shell_error
     let s:haskdeinit = 0
    endif
   else
    let s:haskdeinit= 0
   endif
"   call Decho("setting s:haskdeinit=".s:haskdeinit)
  endif

  if a:remote == 1
   " create a local copy
   let fname= fnamemodify(tempname(),":t:r").".".exten
"   call Decho("a:remote=".a:remote.": create a local copy of <".a:fname."> as <".fname.">")
   exe "silent keepjumps bot 1new ".a:fname
   setlocal bh=delete
"   call Decho("exe w! ".fname)
   exe "w! ".fname
   q
  endif
"  call Decho("exten<".exten."> "."netrwFileHandlers#NFH_".exten."():exists=".exists("*netrwFileHandlers#NFH_".exten))

  " set up redirection
  if &srr =~ "%s"
   let redir= substitute(&srr,"%s","/dev/null","")
  else
   let redir= &srr . "/dev/null"
  endif
"  call Decho("redir{".redir."} srr{".&srr."}")

  " execute the file handler
  if exists("g:netrw_browsex_viewer") && g:netrw_browsex_viewer == '-'
"  call Decho("g:netrw_browsex_viewer<".g:netrw_browsex_viewer.">")
   let ret= netrwFileHandlers#Invoke(exten,fname)

  elseif exists("g:netrw_browsex_viewer") && executable(g:netrw_browsex_viewer)
"   call Decho("g:netrw_browsex_viewer<".g:netrw_browsex_viewer.">")
"   call Decho("exe silent !".g:netrw_browsex_viewer." '".escape(fname,'%#')."' ".redir)
   exe "silent !".g:netrw_browsex_viewer." '".escape(fname,'%#')."'".redir
   let ret= v:shell_error

  elseif has("win32") || has("win64")
"   call Decho('exe silent !start rundll32 url.dll,FileProtocolHandler "'.escape(fname, '%#').'"')
   exe 'silent !start rundll32 url.dll,FileProtocolHandler "'.escape(fname, '%#').'"'
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let ret= v:shell_error

  elseif has("unix") && executable("gnome-open") && !s:haskdeinit
"   call Decho("exe silent !gnome-open '".escape(fname,'%#')."' ".redir)
   exe "silent !gnome-open '".escape(fname,'%#')."'".redir
   let ret= v:shell_error

  elseif has("unix") && executable("kfmclient") && s:haskdeinit
"   call Decho("exe silent !kfmclient exec '".escape(fname,'%#')."' ".redir)
   exe "silent !kfmclient exec '".escape(fname,'%#')."' ".redir
   let ret= v:shell_error

  else
   " netrwFileHandlers#Invoke() always returns 0
   let ret= netrwFileHandlers#Invoke(exten,fname)
  endif

  " if unsuccessful, attempt netrwFileHandlers#Invoke()
  if ret
   let ret= netrwFileHandlers#Invoke(exten,fname)
  endif

  redraw!

  " cleanup: remove temporary file,
  "          delete current buffer if success with handler,
  "          return to prior buffer (directory listing)
  if a:remote == 1 && fname != a:fname
"   call Decho("deleting temporary file<".fname.">")
   call s:System("delete",fname)
  endif

  if a:remote == 1
   setlocal bh=delete bt=nofile
   if g:netrw_use_noswf
    setlocal noswf
   endif
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
fun! s:NetBrowseFtpCmd(path,listcmd)
"  call Dfunc("NetBrowseFtpCmd(path<".a:path."> listcmd<".a:listcmd.">) netrw_method=".w:netrw_method)
"  call Decho("line($)=".line("$")." bannercnt=".w:netrw_bannercnt)

  " because WinXX ftp uses unix style input
  let ffkeep= &ff
  setlocal ma ff=unix noro

  " clear off any older non-banner lines
  " note that w:netrw_bannercnt indexes the line after the banner
"  call Decho('exe silent! keepjumps '.w:netrw_bannercnt.",$d  (clear off old non-banner lines)")
  exe "silent! keepjumps ".w:netrw_bannercnt.",$d"

  ".........................................
  if w:netrw_method == 2 || w:netrw_method == 5 
   " ftp + <.netrc>:  Method #2
   if a:path != ""
    put ='cd \"'.a:path.'\"'
   endif
   if exists("g:netrw_ftpextracmd")
    exe "put ='".g:netrw_ftpextracmd."'"
"    call Decho("filter input: ".getline("."))
   endif
   exe "put ='".a:listcmd."'"
"   exe w:netrw_bannercnt.',$g/^./call Decho("ftp#".line(".").": ".getline("."))'
   if exists("g:netrw_port") && g:netrw_port != ""
"    call Decho("exe ".g:netrw_silentxfer.w:netrw_bannercnt.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port)
    exe g:netrw_silentxfer.w:netrw_bannercnt.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine." ".g:netrw_port
   else
"    call Decho("exe ".g:netrw_silentxfer.w:netrw_bannercnt.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine)
    exe g:netrw_silentxfer.w:netrw_bannercnt.",$!".g:netrw_ftp_cmd." -i ".g:netrw_machine
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
     put ='\"'.g:netrw_passwd.'\"'
    else
     put ='user \"'.g:netrw_uid.'\" \"'.g:netrw_passwd.'\"'
    endif

   if a:path != ""
    put ='cd \"'.a:path.'\"'
   endif
   if exists("g:netrw_ftpextracmd")
    exe "put ='".g:netrw_ftpextracmd."'"
"    call Decho("filter input: ".getline("."))
   endif
   exe "put ='".a:listcmd."'"

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
"    exe w:netrw_bannercnt.',$g/^./call Decho("ftp#".line(".").": ".getline("."))'
"    call Decho("exe ".g:netrw_silentxfer.w:netrw_bannercnt.",$!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer.w:netrw_bannercnt.",$!".g:netrw_ftp_cmd." -i -n"

   ".........................................
  else
   call netrw#ErrorMsg(s:WARNING,"unable to comply with your request<" . choice . ">",23)
  endif

  " cleanup for Windows
  if has("win32") || has("win95") || has("win64") || has("win16")
   silent! keepjumps %s/\r$//e
  endif
  if a:listcmd == "dir"
   " infer directory/link based on the file permission string
   silent! keepjumps g/d\%([-r][-w][-x]\)\{3}/s@$@/@
   silent! keepjumps g/l\%([-r][-w][-x]\)\{3}/s/$/@/
   if w:netrw_liststyle == s:THINLIST || w:netrw_liststyle == s:WIDELIST || w:netrw_liststyle == s:TREELIST
    exe "silent! keepjumps ".w:netrw_bannercnt.',$s/^\%(\S\+\s\+\)\{8}//e'
   endif
  endif

  " ftp's listing doesn't seem to include ./ or ../
  if !search('^\.\/$\|\s\.\/$','wn')
   exe 'keepjumps '.w:netrw_bannercnt
   put ='./'
  endif
  if !search('^\.\.\/$\|\s\.\.\/$','wn')
   exe 'keepjumps '.w:netrw_bannercnt
   put ='../'
  endif

  " restore settings
  let &ff= ffkeep
"  call Dret("NetBrowseFtpCmd")
endfun

" ---------------------------------------------------------------------
" NetListHide: uses [range]g~...~d to delete files that match comma {{{2
" separated patterns given in g:netrw_list_hide
fun! s:NetListHide()
"  call Dfunc("NetListHide() hide=".g:netrw_hide." listhide<".g:netrw_list_hide.">")

  " find a character not in the "hide" string to use as a separator for :g and :v commands
  " How-it-works: take the hiding command, convert it into a range.  Duplicate
  " characters don't matter.  Remove all such characters from the '/~...90'
  " string.  Use the first character left as a separator character.
  let listhide= g:netrw_list_hide
  let sep     = strpart(substitute('/~@#$%^&*{};:,<.>?|1234567890','['.escape(listhide,'-]^\').']','','ge'),1,1)
"  call Decho("sep=".sep)

  while listhide != ""
   if listhide =~ ','
    let hide     = substitute(listhide,',.*$','','e')
    let listhide = substitute(listhide,'^.\{-},\(.*\)$','\1','e')
   else
    let hide     = listhide
    let listhide= ""
   endif

   " Prune the list by hiding any files which match
   if g:netrw_hide == 1
"    call Decho("hiding<".hide."> listhide<".listhide.">")
    exe 'silent keepjumps '.w:netrw_bannercnt.',$g'.sep.hide.sep.'d'
   elseif g:netrw_hide == 2
"    call Decho("showing<".hide."> listhide<".listhide.">")
    exe 'silent keepjumps '.w:netrw_bannercnt.',$g'.sep.hide.sep.'s@^@ /-KEEP-/ @'
   endif
  endwhile
  if g:netrw_hide == 2
   exe 'silent keepjumps '.w:netrw_bannercnt.',$v@^ /-KEEP-/ @d'
   exe 'silent keepjumps '.w:netrw_bannercnt.',$s@^\%( /-KEEP-/ \)\+@@e'
  endif

"  call Dret("NetListHide")
endfun

" ---------------------------------------------------------------------
" NetHideEdit: allows user to edit the file/directory hiding list
fun! s:NetHideEdit(islocal)
"  call Dfunc("NetHideEdit(islocal=".a:islocal.")")

  " save current cursor position
  let s:nhe_curpos= getpos(".")

  " get new hiding list from user
  call inputsave()
  let newhide= input("Edit Hiding List: ",g:netrw_list_hide)
  call inputrestore()
  let g:netrw_list_hide= newhide
"  call Decho("new g:netrw_list_hide<".g:netrw_list_hide.">")

  " refresh the listing
  silent call s:NetRefresh(a:islocal,s:NetBrowseChgDir(a:islocal,"./"))

  " restore cursor position
  call setpos('.',s:nhe_curpos)
  unlet s:nhe_curpos

"  call Dret("NetHideEdit")
endfun

" ---------------------------------------------------------------------
" NetSortSequence: allows user to edit the sorting sequence
fun! s:NetSortSequence(islocal)
"  call Dfunc("NetSortSequence(islocal=".a:islocal.")")

  call inputsave()
  let newsortseq= input("Edit Sorting Sequence: ",g:netrw_sort_sequence)
  call inputrestore()

  " refresh the listing
  let g:netrw_sort_sequence= newsortseq
  call netrw#NetSavePosn()
  call s:NetRefresh(a:islocal,s:NetBrowseChgDir(a:islocal,'./'))

"  call Dret("NetSortSequence")
endfun

" ---------------------------------------------------------------------
"  NetListStyle: {{{2
"  islocal=0: remote browsing
"         =1: local browsing
fun! s:NetListStyle(islocal)
"  call Dfunc("NetListStyle(islocal=".a:islocal.") w:netrw_liststyle=".w:netrw_liststyle)
  let fname             = s:NetGetWord()
  if !exists("w:netrw_liststyle")|let w:netrw_liststyle= g:netrw_liststyle|endif
  let w:netrw_liststyle = (w:netrw_liststyle + 1) % s:MAXLIST
"  call Decho("fname<".fname.">")
"  call Decho("chgd w:netrw_liststyle to ".w:netrw_liststyle)
"  call Decho("b:netrw_curdir<".(exists("b:netrw_curdir")? b:netrw_curdir : "doesn't exist").">")

  if w:netrw_liststyle == s:THINLIST
   " use one column listing
"   call Decho("use one column list")
   let g:netrw_list_cmd = substitute(g:netrw_list_cmd,' -l','','ge')

  elseif w:netrw_liststyle == s:LONGLIST
   " use long list
"   call Decho("use long list")
   let g:netrw_list_cmd = g:netrw_list_cmd." -l"

  elseif w:netrw_liststyle == s:WIDELIST
   " give wide list
"   call Decho("use wide list")
   let g:netrw_list_cmd = substitute(g:netrw_list_cmd,' -l','','ge')

  elseif w:netrw_liststyle == s:TREELIST
"   call Decho("use tree list")
   let g:netrw_list_cmd = substitute(g:netrw_list_cmd,' -l','','ge')

  else
   call netrw#ErrorMsg(s:WARNING,"bad value for g:netrw_liststyle (=".w:netrw_liststyle.")",46)
   let g:netrw_liststyle = s:THINLIST
   let w:netrw_liststyle = g:netrw_liststyle
   let g:netrw_list_cmd  = substitute(g:netrw_list_cmd,' -l','','ge')
  endif
  setlocal ma noro

  " clear buffer - this will cause NetBrowse/LocalBrowseCheck to do a refresh
"  call Decho("clear buffer<".expand("%")."> with :%d")
  %d

  " refresh the listing
  call netrw#NetSavePosn()
  call s:NetRefresh(a:islocal,s:NetBrowseChgDir(a:islocal,'./'))

  " keep cursor on the filename
  silent keepjumps $
  let result= search('\%(^\%(|\+\s\)\=\|\s\{2,}\)\zs'.escape(fname,'.\[]*$^').'\%(\s\{2,}\|$\)','bc')
"  call Decho("search result=".result." w:netrw_bannercnt=".(exists("w:netrw_bannercnt")? w:netrw_bannercnt : 'N/A'))
  if result <= 0 && exists("w:netrw_bannercnt")
   exe w:netrw_bannercnt
  endif

"  call Dret("NetListStyle".(exists("w:netrw_liststyle")? ' : w:netrw_liststyle='.w:netrw_liststyle : ""))
endfun

" ---------------------------------------------------------------------
" NetWideListing: {{{2
fun! s:NetWideListing()

  if w:netrw_liststyle == s:WIDELIST
"   call Dfunc("NetWideListing() w:netrw_liststyle=".w:netrw_liststyle.' fo='.&fo.' l:fo='.&l:fo)
   " look for longest filename (cpf=characters per filename)
   " cpf: characters per file
   " fpl: files per line
   " fpc: files per column
   setlocal ma noro
   let b:netrw_cpf= 0
   if line("$") >= w:netrw_bannercnt
    exe 'silent keepjumps '.w:netrw_bannercnt.',$g/^./if virtcol("$") > b:netrw_cpf|let b:netrw_cpf= virtcol("$")|endif'
   else
"    call Dret("NetWideListing")
    return
   endif
"   call Decho("max file strlen+1=".b:netrw_cpf)
   let b:netrw_cpf= b:netrw_cpf + 1

   " determine qty files per line (fpl)
   let w:netrw_fpl= winwidth(0)/b:netrw_cpf
   if w:netrw_fpl <= 0
    let w:netrw_fpl= 1
   endif
"   call Decho("fpl= ".winwidth(0)."/[b:netrw_cpf=".b:netrw_cpf.']='.w:netrw_fpl)

   " make wide display
   exe 'silent keepjumps '.w:netrw_bannercnt.',$s/^.*$/\=escape(printf("%-'.b:netrw_cpf.'s",submatch(0)),"\\")/'
   let fpc         = (line("$") - w:netrw_bannercnt + w:netrw_fpl)/w:netrw_fpl
   let newcolstart = w:netrw_bannercnt + fpc
   let newcolend   = newcolstart + fpc - 1
"   call Decho("bannercnt=".w:netrw_bannercnt." fpl=".w:netrw_fpl." fpc=".fpc." newcol[".newcolstart.",".newcolend."]")
   silent! let keepregstar = @*
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
    exe 'silent keepjumps '.w:netrw_bannercnt
   endwhile
   silent! let @*= keepregstar
   exe "silent keepjumps ".w:netrw_bannercnt.',$s/\s\+$//e'
   setlocal noma nomod ro
"   call Dret("NetWideListing")
  endif

endfun

" ---------------------------------------------------------------------
" NetTreeDir: determine tree directory given current cursor position {{{2
" (full path directory with trailing slash returned)
fun! s:NetTreeDir()
"  call Dfunc("NetTreeDir() curline#".line(".")."<".getline(".")."> b:netrw_curdir<".b:netrw_curdir."> tab#".tabpagenr()." win#".winnr()." buf#".bufnr("%")."<".bufname("%").">")

  let treedir= b:netrw_curdir
"  call Decho("set initial treedir<".treedir.">")
  let s:treecurpos= getpos(".")

  if w:netrw_liststyle == s:TREELIST
"   call Decho("w:netrrw_liststyle is TREELIST:")
"   call Decho("line#".line(".")." getline(.)<".getline('.')."> treecurpos<".string(s:treecurpos).">")
   if getline('.') =~ '/$'
    let treedir= substitute(getline('.'),'^\%(| \)*\([^|].\{-}\)$','\1','e')
   else
    let treedir= ""
   endif

"   call Decho("treedir<".treedir.">")

   " detect user attempting to close treeroot
   if getline('.') !~ '|' && getline('.') != '..'
"    call Decho("user attempted to close treeroot")
    " now force a refresh
"    call Decho("clear buffer<".expand("%")."> with :%d")
    keepjumps %d
"    call Dret("NetTreeDir <".treedir."> : (side effect) s:treecurpos<".string(s:treecurpos).">")
    return b:netrw_curdir
   endif

   " elide all non-depth information
   let depth = substitute(getline('.'),'^\(\%(| \)*\)[^|].\{-}$','\1','e')
"   call Decho("depth<".depth."> 1st subst")

   " elide first depth
   let depth = substitute(depth,'^| ','','')
"   call Decho("depth<".depth."> 2nd subst")

   " construct treedir by searching backwards at correct depth
"   call Decho("constructing treedir<".treedir."> depth<".depth.">")
   while depth != "" && search('^'.depth.'[^|].\{-}/$','bW')
    let dirname= substitute(getline("."),'^\(| \)*','','e')
    let treedir= dirname.treedir
    let depth  = substitute(depth,'^| ','','')
"    call Decho("constructing treedir<".treedir.">: dirname<".dirname."> while depth<".depth.">")
   endwhile
   if w:netrw_treetop =~ '/$'
    let treedir= w:netrw_treetop.treedir
   else
    let treedir= w:netrw_treetop.'/'.treedir
   endif
"   call Decho("bufnr(.)=".bufnr(".")." line($)=".line("$")." line(.)=".line("."))
  endif
  let treedir= substitute(treedir,'//$','/','')

"  " now force a refresh
"  call Decho("clear buffer<".expand("%")."> with :%d")
"  setlocal ma noro
"  keepjumps %d

"  call Dret("NetTreeDir <".treedir."> : (side effect) s:treecurpos<".string(s:treecurpos).">")
  return treedir
endfun

" ---------------------------------------------------------------------
" NetTreeDisplay: recursive tree display {{{2
fun! s:NetTreeDisplay(dir,depth)
"  call Dfunc("NetTreeDisplay(dir<".a:dir."> depth<".a:depth.">)")

  " insure that there are no folds
  setlocal nofen

  " install ../ and shortdir
  if a:depth == ""
   call setline(line("$")+1,'../')
"   call Decho("setline#".line("$")." ../ (depth is zero)")
  endif
  if a:dir =~ '^\a\+://'
   if a:dir == w:netrw_treetop
    let shortdir= a:dir
   else
    let shortdir= substitute(a:dir,'^.*/\([^/]\+\)/$','\1/','e')
   endif
   call setline(line("$")+1,a:depth.shortdir)
  else
   let shortdir= substitute(a:dir,'^.*/','','e')
   call setline(line("$")+1,a:depth.shortdir.'/')
  endif
"  call Decho("setline#".line("$")." shortdir<".a:depth.shortdir.">")

  " append a / to dir if its missing one
  let dir= a:dir
  if dir !~ '/$'
   let dir= dir.'/'
  endif

  " display subtrees (if any)
  let depth= "| ".a:depth
"  call Decho("display subtrees with depth<".depth."> and current leaves")
  for entry in w:netrw_treedict[a:dir]
   let direntry= substitute(dir.entry,'/$','','e')
"   call Decho("dir<".dir."> entry<".entry."> direntry<".direntry.">")
   if entry =~ '/$' && has_key(w:netrw_treedict,direntry)
"    call Decho("<".direntry."> is a key in treedict - display subtree for it")
    call s:NetTreeDisplay(direntry,depth)
   elseif entry =~ '/$' && has_key(w:netrw_treedict,direntry.'/')
"    call Decho("<".direntry."/> is a key in treedict - display subtree for it")
    call s:NetTreeDisplay(direntry.'/',depth)
   else
"    call Decho("<".entry."> is not a key in treedict (no subtree)")
    call setline(line("$")+1,depth.entry)
   endif
  endfor
"  call Dret("NetTreeDisplay")
endfun

" ---------------------------------------------------------------------
" NetTreeListing: displays tree listing from treetop on down, using NetTreeDisplay() {{{2
fun! s:NetTreeListing(dirname)
  if w:netrw_liststyle == s:TREELIST
"   call Dfunc("NetTreeListing() bufname<".expand("%").">")
"   call Decho("curdir<".a:dirname.">")

   " update the treetop
"   call Decho("update the treetop")
   if !exists("w:netrw_treetop")
    let w:netrw_treetop= a:dirname
"    call Decho("w:netrw_treetop<".w:netrw_treetop."> (reusing)")
   elseif (w:netrw_treetop =~ ('^'.a:dirname) && strlen(a:dirname) < strlen(w:netrw_treetop)) || a:dirname !~ ('^'.w:netrw_treetop)
    let w:netrw_treetop= a:dirname
"    call Decho("w:netrw_treetop<".w:netrw_treetop."> (went up)")
   endif

   " insure that we have at least an empty treedict
   if !exists("w:netrw_treedict")
    let w:netrw_treedict= {}
   endif

   " update the directory listing for the current directory
"   call Decho("updating dictionary with ".a:dirname.":[..directory listing..]")
"   call Decho("bannercnt=".w:netrw_bannercnt." line($)=".line("$"))
   exe "silent! keepjumps ".w:netrw_bannercnt.',$g@^\.\.\=/$@d'
   let w:netrw_treedict[a:dirname]= getline(w:netrw_bannercnt,line("$"))
"   call Decho("treedict=".string(w:netrw_treedict))
   exe "silent! keepjumps ".w:netrw_bannercnt.",$d"

   " if past banner, record word
   if exists("w:netrw_bannercnt") && line(".") > w:netrw_bannercnt
    let fname= expand("<cword>")
   else
    let fname= ""
   endif

   " display from treetop on down
   call s:NetTreeDisplay(w:netrw_treetop,"")

   " place cursor
   if !exists("s:nbcd_curpos")
    if fname != ""
"     call Decho("(NetTreeListing) place cursor <".fname.">")
     call search('\<'.fname.'\>','cw')
    elseif exists("w:netrw_bannercnt")
     exe (w:netrw_bannercnt+1)
"     call Decho("(NetTreeListing) place cursor line#".(w:netrw_bannercnt+1))
    endif
   endif

"   call Dret("NetTreeListing : bufname<".expand("%").">")
  endif
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
     call netrw#ErrorMsg(s:WARNING,"<".newdirname."> is already a directory!",24)
    endif
"    call Dret("NetMakeDir : directory<".newdirname."> exists previously")
    return
   endif
   if s:FileReadable(fullnewdir)
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:WARNING,"<".newdirname."> is already a file!",25)
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
    exe 'keepjumps cd '.b:netrw_curdir
"    call Decho("netrw_origdir<".netrw_origdir.">: cd b:netrw_curdir<".b:netrw_curdir.">")
"    call Decho("exe silent! !".g:netrw_local_mkdir.' '.g:netrw_shq.newdirname.g:netrw_shq)
    exe "silent! !".g:netrw_local_mkdir.' '.g:netrw_shq.newdirname.g:netrw_shq
    if !g:netrw_keepdir | exe 'keepjumps cd '.netrw_origdir | endif
    if !g:netrw_keepdir
     exe 'keepjumps cd '.netrw_origdir
"     call Decho("netrw_keepdir=".g:netrw_keepdir.": cd ".netrw_origdir)
    endif
   endif

   if v:shell_error == 0
    " refresh listing
"    call Decho("refresh listing")
    call netrw#NetSavePosn()
    call s:NetRefresh(1,s:NetBrowseChgDir(1,'./'))
   elseif !exists("g:netrw_quiet")
    call netrw#ErrorMsg(s:ERROR,"unable to make directory<".newdirname.">",26)
   endif
   redraw!

  else
   " Remote mkdir:
   let mkdircmd  = s:MakeSshCmd(g:netrw_mkdir_cmd)
   let newdirname= substitute(b:netrw_curdir,'^\%(.\{-}/\)\{3}\(.*\)$','\1','').newdirname
"   call Decho("exe silent! !".mkdircmd." ".g:netrw_shq.newdirname.g:netrw_shq)
   exe "silent! !".mkdircmd." ".g:netrw_shq.newdirname.g:netrw_shq
   if v:shell_error == 0
    " refresh listing
    call netrw#NetSavePosn()
    call s:NetRefresh(0,s:NetBrowseChgDir(0,'./'))
   elseif !exists("g:netrw_quiet")
    call netrw#ErrorMsg(s:ERROR,"unable to make directory<".newdirname.">",27)
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
"    3: (browsing)    record current directory history
"    4: (user: <u>)   go up   (previous) bookmark
"    5: (user: <U>)   go down (next)     bookmark
fun! s:NetBookmarkDir(chg,curdir)
"  call Dfunc("NetBookmarkDir(chg=".a:chg." curdir<".a:curdir.">) cnt=".v:count." bookmarkcnt=".g:NETRW_BOOKMARKMAX." histcnt=".g:NETRW_DIRHIST_CNT." bookmax=".g:NETRW_BOOKMARKMAX." histmax=".g:netrw_dirhistmax)

  if a:chg == 0
   " bookmark the current directory
"   call Decho("(user: <b>) bookmark the current directory")
   if v:count > 0
    " handle bookmark# specified via the count
    let g:NETRW_BOOKMARKDIR_{v:count}= a:curdir
    if !exists("g:NETRW_BOOKMARKMAX")
     let g:NETRW_BOOKMARKMAX= v:count
    elseif v:count > g:NETRW_BOOKMARKMAX
     let g:NETRW_BOOKMARKMAX= v:count
    endif
   else
    " handle no count specified
    let g:NETRW_BOOKMARKMAX                       = g:NETRW_BOOKMARKMAX + 1
    let g:NETRW_BOOKMARKDIR_{g:NETRW_BOOKMARKMAX} = a:curdir
   endif
   echo "bookmarked the current directory"

  elseif a:chg == 1
   " change to the bookmarked directory
"   call Decho("(user: <B>) change to the bookmarked directory")
   if exists("g:NETRW_BOOKMARKDIR_{v:count}")
    exe "e ".g:NETRW_BOOKMARKDIR_{v:count}
   else
    echomsg "Sorry, bookmark#".v:count." doesn't exist!"
   endif

  elseif a:chg == 2
   redraw!
   let didwork= 0
   " list user's bookmarks
"   call Decho("(user: <q>) list user's bookmarks")
   if exists("g:NETRW_BOOKMARKMAX")
"    call Decho("list bookmarks [0,".g:NETRW_BOOKMARKMAX."]")
    let cnt= 0
    while cnt <= g:NETRW_BOOKMARKMAX
     if exists("g:NETRW_BOOKMARKDIR_{cnt}")
"      call Decho("Netrw Bookmark#".cnt.": ".g:NETRW_BOOKMARKDIR_{cnt})
      echo "Netrw Bookmark#".cnt.": ".g:NETRW_BOOKMARKDIR_{cnt}
      let didwork= 1
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
     let didwork= 1
    endif
    let first = 0
    let cnt   = ( cnt - 1 ) % g:netrw_dirhistmax
    if cnt < 0
     let cnt= cnt + g:netrw_dirhistmax
    endif
   endwhile
   if didwork
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif

  elseif a:chg == 3
   " saves most recently visited directories (when they differ)
"   call Decho("(browsing) record curdir history")
   if !exists("g:NETRW_DIRHIST_0") || g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT} != a:curdir
    let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT + 1 ) % g:netrw_dirhistmax
"    let g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}= substitute(a:curdir,'[/\\]$','','e')
    let g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}= a:curdir
"    call Decho("save dirhist#".g:NETRW_DIRHIST_CNT."<".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}.">")
   endif

  elseif a:chg == 4
   " u: change to the previous directory stored on the history list
"   call Decho("(user: <u>) chg to prev dir from history")
   let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT - 1 ) % g:netrw_dirhistmax
   if g:NETRW_DIRHIST_CNT < 0
    let g:NETRW_DIRHIST_CNT= g:NETRW_DIRHIST_CNT + g:netrw_dirhistmax
   endif
   if exists("g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}")
"    call Decho("changedir u#".g:NETRW_DIRHIST_CNT."<".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}.">")
    if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir")
     setlocal ma noro
     %d
     setlocal nomod
    endif
"    call Decho("exe e! ".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT})
    exe "e! ".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}
   else
    let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT + 1 ) % g:netrw_dirhistmax
    echo "Sorry, no predecessor directory exists yet"
   endif

  elseif a:chg == 5
   " U: change to the subsequent directory stored on the history list
"   call Decho("(user: <U>) chg to next dir from history")
   let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT + 1 ) % g:netrw_dirhistmax
   if exists("g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}")
"    call Decho("changedir U#".g:NETRW_DIRHIST_CNT."<".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}.">")
    if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir")
     setlocal ma noro
     %d
     setlocal nomod
    endif
"    call Decho("exe e! ".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT})
    exe "e! ".g:NETRW_DIRHIST_{g:NETRW_DIRHIST_CNT}
   else
    let g:NETRW_DIRHIST_CNT= ( g:NETRW_DIRHIST_CNT - 1 ) % g:netrw_dirhistmax
    if g:NETRW_DIRHIST_CNT < 0
     let g:NETRW_DIRHIST_CNT= g:NETRW_DIRHIST_CNT + g:netrw_dirhistmax
    endif
    echo "Sorry, no successor directory exists yet"
   endif
  endif
  call s:NetBookmarkMenu()
"  call Dret("NetBookmarkDir")
endfun

" ---------------------------------------------------------------------
" NetBookmarkMenu: {{{2
fun! s:NetBookmarkMenu()
  if !exists("s:netrw_menucnt")
   return
  endif
"  call Dfunc("NetBookmarkMenu() bookmarkcnt=".g:NETRW_BOOKMARKMAX." histcnt=".g:NETRW_DIRHIST_CNT." menucnt=".s:netrw_menucnt)
  if has("menu") && has("gui_running") && &go =~ 'm'
   if exists("g:NetrwTopLvlMenu")
    exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Bookmark'
   endif

   " show bookmarked places
   let cnt       = 0
   while cnt <= g:NETRW_BOOKMARKMAX
    if exists("g:NETRW_BOOKMARKDIR_{cnt}")
     let bmdir= escape(g:NETRW_BOOKMARKDIR_{cnt},'.')
"     call Decho('silent! menu '.g:NetrwMenuPriority.".2.".cnt." ".g:NetrwTopLvlMenu.'Bookmark.'.bmdir.'	:e '.g:NETRW_BOOKMARKDIR_{cnt})
     exe 'silent! menu '.g:NetrwMenuPriority.".2.".cnt." ".g:NetrwTopLvlMenu.'Bookmarks.'.bmdir.'	:e '.g:NETRW_BOOKMARKDIR_{cnt}."\<cr>"
    endif
    let cnt= cnt + 1
   endwhile

   " show directory browsing history
   let cnt     = g:NETRW_DIRHIST_CNT
   let first   = 1
   let histcnt = 0
   while ( first || cnt != g:NETRW_DIRHIST_CNT )
    let histcnt  = histcnt + 1
    let priority = g:NETRW_DIRHIST_CNT + histcnt
    if exists("g:NETRW_DIRHIST_{cnt}")
     let bmdir= escape(g:NETRW_DIRHIST_{cnt},'.')
"     call Decho('silent! menu '.g:NetrwMenuPriority.".3.".priority." ".g:NetrwTopLvlMenu.'History.'.bmdir.'	:e '.g:NETRW_DIRHIST_{cnt})
     exe 'silent! menu '.g:NetrwMenuPriority.".3.".priority." ".g:NetrwTopLvlMenu.'History.'.bmdir.'	:e '.g:NETRW_DIRHIST_{cnt}."\<cr>"
    endif
    let first = 0
    let cnt   = ( cnt - 1 ) % g:netrw_dirhistmax
    if cnt < 0
     let cnt= cnt + g:netrw_dirhistmax
    endif
   endwhile
  endif
"  call Dret("NetBookmarkMenu")
endfun

" ---------------------------------------------------------------------
" NetObtain: obtain file under cursor (for remote browsing support) {{{2
fun! netrw#NetObtain(vismode,...) range
"  call Dfunc("NetObtain(vismode=".a:vismode.") a:0=".a:0)

  if a:vismode == 0
   " normal mode
   let fname= expand("<cWORD>")
"   call Decho("no arguments, use <".fname.">")
  elseif a:vismode == 1
   " visual mode
   let keeprega = @a
   norm! gv"ay
   if g:netrw_liststyle == s:THINLIST
    " thin listing
    let filelist= split(@a,'\n')
   elseif g:netrw_liststyle == s:LONGLIST
    " long listing
    let filelist= split(substitute(@a,'\t.\{-}\n','\n','g'),'\n')
   else
    " wide listing
	let filelist = split(substitute(@a,'\s\{2,}','\n','g'),'\n')
	let filelist = map(filelist,'substitute(v:val,"^\\s\\+","","")')
	let filelist = map(filelist,'substitute(v:val,"\\s\\+$","","")')
   endif
"   call Decho("filelist<".string(filelist).">")
   let @a= keeprega
   for f in filelist
    if f != ""
     call netrw#NetObtain(2,f)
    endif
   endfor
"   call Dret("NetObtain : visual mode handler")
   return
  elseif a:vismode == 2
   " multiple file mode
   let fname= a:1
"   call Decho("visual mode handling: <".fname.">")
  endif

  " NetrwStatusLine support - for obtaining support
  call s:SetupNetrwStatusLine('%f %h%m%r%=%9*Obtaining '.fname)

  if exists("w:netrw_method") && w:netrw_method =~ '[235]'
"   call Decho("method=".w:netrw_method)
   if executable("ftp")
"    call Decho("ftp is executable, method=".w:netrw_method)
    let curdir = b:netrw_curdir
    let path   = substitute(curdir,'ftp://[^/]\+/','','e')
    let curline= line(".")
    let endline= line("$")+1
    setlocal ma noro
    keepjumps $
"    call Decho("getcwd<".getcwd().">")
"    call Decho("curdir<".curdir.">")
"    call Decho("path<".path.">")
"    call Decho("curline=".curline)
"    call Decho("endline=".endline)

    ".........................................
    if w:netrw_method == 2
     " ftp + <.netrc>: Method #2
     setlocal ff=unix
     if path != ""
      put ='cd '.path
"      call Decho("ftp:  cd ".path)
     endif
     put ='get '.fname
"     call Decho("ftp:  get ".fname)
     put ='quit'
"     call Decho("ftp:  quit")
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
"     call Decho('ftp:  open '.g:netrw_machine)
    endif

    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     put =g:netrw_uid
     put ='\"'.g:netrw_passwd.'\"'
"     call Decho('ftp:  g:netrw_uid')
"     call Decho('ftp:  g:netrw_passwd')
    else
     put ='user \"'.g:netrw_uid.'\" \"'.g:netrw_passwd.'\"'
"     call Decho('user '.g:netrw_uid.' '.g:netrw_passwd)
    endif

   if path != ""
    put ='cd '.path
"    call Decho('cd '.a:path)
   endif
   put ='get '.fname
"   call Decho("ftp:  get ".fname)
   put ='quit'
"   call Decho("ftp:  quit")

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
"    call Decho("exe ".g:netrw_silentxfer.curline.",$!".g:netrw_ftp_cmd." -i -n")
    exe g:netrw_silentxfer.endline.",$!".g:netrw_ftp_cmd." -i -n"

    ".........................................
    else
     call netrw#ErrorMsg(s:WARNING,"unable to comply with your request<" . choice . ">",28)
    endif
    " restore
    exe "silent! ".endline.",$d"
    exe "keepjumps ".curline
    setlocal noma nomod ro
   else
"    call Decho("ftp not executable")
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:ERROR,"this system doesn't support ftp",29)
    endif
    " restore status line
    let &stl        = s:netrw_users_stl
    let &laststatus = s:netrw_users_ls
    " restore NetMethod
    if exists("keep_netrw_method")
     call s:NetMethod(keep_netrw_choice)
     let w:netrw_method  = keep_netrw_wmethod
    endif
"    call Dret("NetObtain")
    return
   endif

  ".........................................
  else
   " scp: Method#4
"   call Decho("using scp")
   let curdir = b:netrw_curdir
   let path   = substitute(curdir,'scp://[^/]\+/','','e')
"   call Decho("path<".path.">")
   if exists("g:netrw_port") && g:netrw_port != ""
    let useport= " ".g:netrw_scpport." ".g:netrw_port
   else
    let useport= ""
   endif
"   call Decho("executing: !".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".path.escape(fname,' ?&')." .")
   exe g:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".g:netrw_machine.":".path.escape(fname,' ?&')." ."
   endif
  endif

  " restore status line
  let &stl        = s:netrw_users_stl
  let &laststatus = s:netrw_users_ls
  redraw!

  " restore NetMethod
  if exists("keep_netrw_method")
   call s:NetMethod(keep_netrw_choice)
   let w:netrw_method  = keep_netrw_wmethod
  endif

"  call Dret("NetObtain")
endfun

" ---------------------------------------------------------------------
" NetPrevWinOpen: open file/directory in previous window.  {{{2
"   If there's only one window, then the window will first be split.
fun! s:NetPrevWinOpen(islocal)
"  call Dfunc("NetPrevWinOpen(islocal=".a:islocal.")")

  " get last window number and the word currently under the cursor
  let lastwinnr = winnr("$")
  let curword   = s:NetGetWord()
"  call Decho("lastwinnr=".lastwinnr." curword<".curword.">")

  let didsplit  = 0
  if lastwinnr == 1
   " if only one window, open a new one first
"   call Decho("only one window, so open a new one (g:netrw_alto=".g:netrw_alto.")")
   exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   let didsplit  = 1

  else
   wincmd p
   " if the previous window's buffer has been changed (is modified),
   " and it doesn't appear in any other extant window, then ask the
   " user if s/he wants to abandon modifications therein.
   let bnr    = winbufnr(0)
   let bnrcnt = 0
   if &mod
    windo if winbufnr(0) == bnr | let bnrcnt=bnrcnt+1 | endif
"    call Decho("bnr=".bnr." bnrcnt=".bnrcnt)
    if bnrcnt == 1
     let bufname= bufname(winbufnr(winnr()))
     let choice= confirm("Save modified file<".bufname.">?","&Yes\n&No\n&Cancel")

     if choice == 1
      " Yes -- write file & then browse
      let v:errmsg= ""
      silent w
      if v:errmsg != ""
       call netrw#ErrorMsg(s:ERROR,"unable to write <".bufname.">!",30)
       if didsplit
       	q
       else
       	wincmd p
       endif
"       call Dret("NetPrevWinOpen : unable to write <".bufname.">")
       return
      endif

     elseif choice == 2
      " No -- don't worry about changed file, just browse anyway
      setlocal nomod
      call netrw#ErrorMsg(s:WARNING,bufname." changes abandoned",31)

     else
      " Cancel -- don't do this
      if didsplit
       q
      else
       wincmd p
      endif
"      call Dret("NetPrevWinOpen : cancelled")
      return
     endif
    endif
   endif
  endif

  if a:islocal
   call netrw#LocalBrowseCheck(s:NetBrowseChgDir(a:islocal,curword))
  else
   call s:NetBrowse(a:islocal,s:NetBrowseChgDir(a:islocal,curword))
  endif
"  call Dret("NetPrevWinOpen")
endfun

" ---------------------------------------------------------------------
" NetMenu: generates the menu for gvim and netrw {{{2
fun! s:NetMenu(domenu)

  if !exists("g:NetrwMenuPriority")
   let g:NetrwMenuPriority= 80
  endif

  if has("menu") && has("gui_running") && &go =~ 'm' && g:netrw_menu
"   call Dfunc("NetMenu(domenu=".a:domenu.")")

   if !exists("s:netrw_menu_enabled") && a:domenu
"    call Decho("initialize menu")
    let s:netrw_menu_enabled= 1
    exe 'silent! menu '.g:NetrwMenuPriority.'.1 '.g:NetrwTopLvlMenu.'Help<tab><F1>	<F1>'
    call s:NetBookmarkMenu() " provide some history!
    exe 'silent! menu '.g:NetrwMenuPriority.'.4 '.g:NetrwTopLvlMenu.'Go\ Up\ Directory<tab>-	-'
    exe 'silent! menu '.g:NetrwMenuPriority.'.5 '.g:NetrwTopLvlMenu.'Apply\ Special\ Viewer<tab>x	x'
    exe 'silent! menu '.g:NetrwMenuPriority.'.6 '.g:NetrwTopLvlMenu.'Bookmark\ Current\ Directory<tab>mb	mb'
    exe 'silent! menu '.g:NetrwMenuPriority.'.7 '.g:NetrwTopLvlMenu.'Goto\ Bookmarked\ Directory<tab>gb	gb'
    exe 'silent! menu '.g:NetrwMenuPriority.'.8 '.g:NetrwTopLvlMenu.'Change\ To\ Recently\ Used\ Directory<tab>u	u'
    exe 'silent! menu '.g:NetrwMenuPriority.'.9 '.g:NetrwTopLvlMenu.'Change\ To\ Subsequently\ Used\ Directory<tab>U	U'
    exe 'silent! menu '.g:NetrwMenuPriority.'.10 '.g:NetrwTopLvlMenu.'Delete\ File/Directory<tab>D	D'
    exe 'silent! menu '.g:NetrwMenuPriority.'.11 '.g:NetrwTopLvlMenu.'Edit\ File\ Hiding\ List<tab>'."<ctrl-h>	\<c-h>"
    exe 'silent! menu '.g:NetrwMenuPriority.'.12 '.g:NetrwTopLvlMenu.'Edit\ File/Directory<tab><cr>	'."\<cr>"
    exe 'silent! menu '.g:NetrwMenuPriority.'.13 '.g:NetrwTopLvlMenu.'Edit\ File/Directory,\ New\ Window<tab>o	o'
    exe 'silent! menu '.g:NetrwMenuPriority.'.14 '.g:NetrwTopLvlMenu.'Edit\ File/Directory,\ New\ Vertical\ Window<tab>v	v'
    exe 'silent! menu '.g:NetrwMenuPriority.'.15 '.g:NetrwTopLvlMenu.'List\ Bookmarks\ and\ History<tab>q	q'
    exe 'silent! menu '.g:NetrwMenuPriority.'.16 '.g:NetrwTopLvlMenu.'Listing\ Style\ (thin-long-wide)<tab>i	i'
    exe 'silent! menu '.g:NetrwMenuPriority.'.17 '.g:NetrwTopLvlMenu.'Make\ Subdirectory<tab>d	d'
    exe 'silent! menu '.g:NetrwMenuPriority.'.18 '.g:NetrwTopLvlMenu.'Normal-Hide-Show<tab>a	a'
    exe 'silent! menu '.g:NetrwMenuPriority.'.19 '.g:NetrwTopLvlMenu.'Obtain\ File<tab>O	O'
    exe 'silent! menu '.g:NetrwMenuPriority.'.20 '.g:NetrwTopLvlMenu.'Preview\ File/Directory<tab>p	p'
    exe 'silent! menu '.g:NetrwMenuPriority.'.21 '.g:NetrwTopLvlMenu.'Previous\ Window\ Browser<tab>P	P'
    exe 'silent! menu '.g:NetrwMenuPriority.'.22 '.g:NetrwTopLvlMenu.'Refresh\ Listing<tab>'."<ctrl-l>	\<c-l>"
    exe 'silent! menu '.g:NetrwMenuPriority.'.23 '.g:NetrwTopLvlMenu.'Rename\ File/Directory<tab>R	R'
    exe 'silent! menu '.g:NetrwMenuPriority.'.24 '.g:NetrwTopLvlMenu.'Reverse\ Sorting\ Order<tab>'."r	r"
    exe 'silent! menu '.g:NetrwMenuPriority.'.25 '.g:NetrwTopLvlMenu.'Select\ Sorting\ Style<tab>s	s'
    exe 'silent! menu '.g:NetrwMenuPriority.'.26 '.g:NetrwTopLvlMenu.'Sorting\ Sequence\ Edit<tab>S	S'
    exe 'silent! menu '.g:NetrwMenuPriority.'.27 '.g:NetrwTopLvlMenu.'Set\ Current\ Directory<tab>c	c'
    exe 'silent! menu '.g:NetrwMenuPriority.'.28 '.g:NetrwTopLvlMenu.'Settings/Options<tab>:NetrwSettings	'.":NetrwSettings\<cr>"
    let s:netrw_menucnt= 28

   elseif !a:domenu
    let s:netrwcnt = 0
    let curwin     = winnr()
    windo if getline(2) =~ "Netrw" | let s:netrwcnt= s:netrwcnt + 1 | endif
    exe curwin."wincmd w"
    
    if s:netrwcnt <= 1
"     call Decho("clear menus")
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Help'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Apply\ Special\ Viewer'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Bookmark\ Current\ Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Go\ Up\ Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Goto\ Bookmarked\ Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Change\ To\ Recently\ Used\ Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Change\ To\ Subsequently\ Used\ Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Delete\ File/Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Edit\ File/Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Edit\ File/Directory,\ New\ Window'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Edit\ File/Directory,\ New\ Vertical\ Window'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Edit\ File\ Hiding\ List'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Edit\ File'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Enter\ File/Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Enter\ File/Directory\ (vertical\ split)'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'List\ Bookmarks\ and\ History'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Listing\ Style\ (thin-long-wide)'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Make\ Subdirectory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Normal-Hide-Show'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Obtain\ File'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Preview\ File/Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Previous\ Window\ Browser'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Refresh\ Listing'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Rename\ File/Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Reverse\ Sorting\ Order'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Select\ Sorting\ Style'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Sorting\ Sequence\ Edit'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Set\ Current\ Directory'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Settings/Options'
     exe 'silent! unmenu '.g:NetrwTopLvlMenu.'Bookmarks'
     silent! unlet s:netrw_menu_enabled
    endif
   endif
"   call Dret("NetMenu")
  endif

endfun

" ==========================================
"  Local Directory Browsing Support:    {{{1
" ==========================================

" ---------------------------------------------------------------------
" LocalBrowseCheck: {{{2
fun! netrw#LocalBrowseCheck(dirname)
  " unfortunate interaction -- split window debugging can't be
"  " used here, must use DechoRemOn or DechoTabOn -- the BufEnter
  " event triggers another call to LocalBrowseCheck() when attempts
  " to write to the DBG buffer are made.
"  call Dfunc("LocalBrowseCheck(dirname<".a:dirname.">")
  if isdirectory(a:dirname)
   silent! call s:NetBrowse(1,a:dirname)
  endif
"  call Dret("LocalBrowseCheck")
  " not a directory, ignore it
endfun

" ---------------------------------------------------------------------
"  LocalListing: does the job of "ls" for local directories {{{2
fun! s:LocalListing()
"  call Dfunc("LocalListing() &ma=".&ma." &mod=".&mod." &ro=".&ro." buf(%)=".buf("%"))
"  if exists("b:netrw_curdir") |call Decho('b:netrw_curdir<'.b:netrw_curdir.">")  |else|call Decho("b:netrw_curdir doesn't exist") |endif
"  if exists("g:netrw_sort_by")|call Decho('g:netrw_sort_by<'.g:netrw_sort_by.">")|else|call Decho("g:netrw_sort_by doesn't exist")|endif

  " get the list of files contained in the current directory
  let dirname    = escape(b:netrw_curdir,s:netrw_glob_escape)
  let dirnamelen = strlen(b:netrw_curdir)
  let filelist   = glob(s:ComposePath(dirname,"*"))
"  call Decho("glob(dirname<".dirname."/*>)=".filelist)
  if filelist != ""
   let filelist= filelist."\n"
  endif
  let filelist= filelist.glob(s:ComposePath(dirname,".*"))
"  call Decho("glob(dirname<".dirname."/.*>)=".glob(dirname.".*"))

  " if the directory name includes a "$", and possibly other characters,
  " the glob() doesn't include "." and ".." entries.
  if filelist !~ '[\\/]\.[\\/]\=\(\n\|$\)'
"   call Decho("forcibly tacking on .")
   if filelist == ""
    let filelist= s:ComposePath(dirname,"./")
   else
    let filelist= filelist."\n".s:ComposePath(b:netrw_curdir,"./")
   endif
"  call Decho("filelist<".filelist.">")
  endif
  if filelist !~ '[\\/]\.\.[\\/]\=\(\n\|$\)'
"   call Decho("forcibly tacking on ..")
   let filelist= filelist."\n".s:ComposePath(b:netrw_curdir,"../")
"   call Decho("filelist<".filelist.">")
  endif
  if b:netrw_curdir == '/'
   " remove .. from filelist when current directory is root directory
   let filelist= substitute(filelist,'/\.\.\n','','')
"   call Decho("remove .. from filelist")
  endif
  let filelist= substitute(filelist,'\n\{2,}','\n','ge')
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   let filelist= substitute(filelist,'\','/','ge')
  else
   let filelist= substitute(filelist,'\','\\','ge')
  endif

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
   let pfile= substitute(pfile,'^[/\\]','','e')
"   call Decho(" ")
"   call Decho("filename<".filename.">")
"   call Decho("pfile   <".pfile.">")

   if w:netrw_liststyle == s:LONGLIST
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
    keepjumps silent! put=ftpfile

   elseif g:netrw_sort_by =~ "^s"
    " sort by size (handles file sizes up to 1 quintillion bytes, US)
"    call Decho("getfsize(".filename.")=".getfsize(filename))
    let sz   = getfsize(filename)
    let fsz  = strpart("000000000000000000",1,18-strlen(sz)).sz
"    call Decho("exe keepjumps put ='".fsz.'/'.filename."'")
    let fszpfile= fsz.'/'.pfile
    keepjumps silent! put =fszpfile

   else 
    " sort by name
"    call Decho("exe keepjumps put ='".pfile."'")
    keepjumps silent! put=pfile
   endif
  endwhile

  " cleanup any windows mess at end-of-line
  silent! keepjumps %s/\r$//e
  setlocal ts=32
"  call Decho("setlocal ts=32")

"  call Dret("LocalListing")
endfun

" ---------------------------------------------------------------------
" LocalBrowseShellCmdRefresh: this function is called after a user has {{{2
" performed any shell command.  The idea is to cause all local-browsing
" buffers to be refreshed after a user has executed some shell command,
" on the chance that s/he removed/created a file/directory with it.
fun! s:LocalBrowseShellCmdRefresh()
"  call Dfunc("LocalBrowseShellCmdRefresh() browselist=".string(s:netrw_browselist))
  " determine which buffers currently reside in a tab
  let itab       = 1
  let buftablist = []
  while itab <= tabpagenr("$")
   let buftablist= buftablist + tabpagebuflist()
   let itab= itab + 1
   tabn
  endwhile
"  call Decho("buftablist".string(buftablist))
  "  GO through all buffers on netrw_browselist (ie. just local-netrw buffers):
  "   | refresh any netrw window
  "   | wipe out any non-displaying netrw buffer
  let curwin = winnr()
  let ibl    = 0
  for ibuf in s:netrw_browselist
"   call Decho("bufwinnr(".ibuf.") index(buftablist,".ibuf.")=".index(buftablist,ibuf))
   if bufwinnr(ibuf) == -1 && index(buftablist,ibuf) == -1
"    call Decho("wiping  buf#".ibuf,"<".bufname(ibuf).">")
    exe "silent! bw ".ibuf
    call remove(s:netrw_browselist,ibl)
"    call Decho("browselist=".string(s:netrw_browselist))
    continue
   elseif index(tabpagebuflist(),ibuf) != -1
"    call Decho("refresh buf#".ibuf.'-> win#'.bufwinnr(ibuf))
    exe bufwinnr(ibuf)."wincmd w"
    call s:NetRefresh(1,s:NetBrowseChgDir(1,'./'))
   endif
   let ibl= ibl + 1
  endfor
  exe curwin."wincmd w"

"  call Dret("LocalBrowseShellCmdRefresh")
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
   let rmfile= s:ComposePath(a:path,curword)
"   call Decho("rmfile<".rmfile.">")

   if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '[\/]$')
    " attempt to remove file
    if !all
     echohl Statement
     call inputsave()
     let ok= input("Confirm deletion of file<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
     call inputrestore()
     echohl NONE
     if ok == ""
      let ok="no"
     endif
"     call Decho("response: ok<".ok.">")
     let ok= substitute(ok,'\[{y(es)},n(o),a(ll),q(uit)]\s*','','e')
"     call Decho("response: ok<".ok."> (after sub)")
     if ok =~ 'a\%[ll]'
      let all= 1
     endif
    endif

    if all || ok =~ 'y\%[es]' || ok == ""
     let ret= s:System("delete",rmfile)
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
     if ok == ""
      let ok="no"
     endif
     if ok =~ 'a\%[ll]'
      let all= 1
     endif
    endif
    let rmfile= substitute(rmfile,'[\/]$','','e')

    if all || ok =~ 'y\%[es]' || ok == ""
"     call Decho("1st attempt: system(".g:netrw_local_rmdir.' "'.rmfile.'")')
     call s:System("system",g:netrw_local_rmdir.' "'.rmfile.'"')
"     call Decho("v:shell_error=".v:shell_error)

     if v:shell_error != 0
"      call Decho("2nd attempt to remove directory<".rmfile.">")
      let errcode= s:System("delete",rmfile)
"      call Decho("errcode=".errcode)

      if errcode != 0
       if has("unix")
"        call Decho("3rd attempt to remove directory<".rmfile.">")
        call s:System("system","rm ".rmfile)
        if v:shell_error != 0 && !exists("g:netrw_quiet")
	 call netrw#ErrorMsg(s:ERROR,"unable to remove directory<".rmfile."> -- is it empty?",34)
endif
       elseif !exists("g:netrw_quiet")
       	call netrw#ErrorMsg(s:ERROR,"unable to remove directory<".rmfile."> -- is it empty?",35)
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
  call s:NetRefresh(1,s:NetBrowseChgDir(1,'./'))
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
   let oldname= s:ComposePath(a:path,curword)
"   call Decho("oldname<".oldname.">")

   call inputsave()
   let newname= input("Moving ".oldname." to : ",substitute(oldname,'/*$','','e'))
   call inputrestore()

   let ret= rename(oldname,newname)
"   call Decho("renaming <".oldname."> to <".newname.">")

   let ctr= ctr + 1
  endwhile

  " refresh the directory
"  call Decho("refresh the directory listing")
  call netrw#NetSavePosn()
  call s:NetRefresh(1,s:NetBrowseChgDir(1,'./'))
"  call Dret("LocalBrowseRename")
endfun

" ---------------------------------------------------------------------
" LocalFastBrowser: handles setting up/taking down fast browsing for the {{{2
"                   local browser
"     fastbrowse  Local  Remote   Hiding a buffer implies it may be re-used (fast)
"  slow   0         D      D      Deleting a buffer implies it will not be re-used (slow)
"  med    1         D      H
"  fast   2         H      H
fun! s:LocalFastBrowser()
"  call Dfunc("LocalFastBrowser() g:netrw_fastbrowse=".g:netrw_fastbrowse)

  " initialize browselist, a list of buffer numbers that the local browser has used
  if !exists("s:netrw_browselist")
"   call Decho("initialize s:netrw_browselist")
   let s:netrw_browselist= []
  endif

  " append current buffer to fastbrowse list
  if g:netrw_fastbrowse <= 1 && (empty(s:netrw_browselist) || bufnr("%") > s:netrw_browselist[-1])
"   call Decho("appendng current buffer to browselist")
   call add(s:netrw_browselist,bufnr("%"))
"   call Decho("browselist=".string(s:netrw_browselist))
  endif

  " enable autocmd events to handle refreshing/removing local browser buffers
  "    If local browse buffer is currently showing: refresh it
  "    If local browse buffer is currently hidden : wipe it
  if !exists("s:netrw_browser_shellcmd") && g:netrw_fastbrowse <= 1
"   call Decho("setting up local-browser shell command refresh")
   let s:netrw_browser_shellcmd= 1
   augroup AuNetrwShellCmd
    au!
    if (has("win32") || has("win95") || has("win64") || has("win16"))
     au ShellCmdPost *	call s:LocalBrowseShellCmdRefresh()
    else
     au ShellCmdPost,FocusGained *	call s:LocalBrowseShellCmdRefresh()
    endif
   augroup END
  endif

  " user must have changed fastbrowse to its fast setting, so remove
  " the associated autocmd events
  if g:netrw_fastbrowse > 1 && exists("s:netrw_browser_shellcmd")
"   call Decho("remove AuNetrwShellCmd autcmd group")
   unlet s:netrw_browser_shellcmd
   augroup AuNetrwShellCmd
    au!
   augroup END
   augroup! AuNetrwShellCmd
  endif

"  call Dret("LocalFastBrowser")
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
   call netrw#ErrorMsg(s:ERROR,"local browsing directory doesn't exist!",36)
  else
   call netrw#ErrorMsg(s:ERROR,"local browsing directory and current directory are identical",37)
  endif
"  call Dret("LocalObtain")
endfun

" ---------------------------------------------------------------------
" netrw#Explore: launch the local browser in the directory of the current file {{{2
"          dosplit==0: the window will be split iff the current file has
"                      been modified
"          dosplit==1: the window will be split before running the local
"                      browser
fun! netrw#Explore(indx,dosplit,style,...)
"  call Dfunc("netrw#Explore(indx=".a:indx." dosplit=".a:dosplit." style=".a:style.",a:1<".a:1.">) &modified=".&modified." a:0=".a:0)
  if !exists("b:netrw_curdir")
   let b:netrw_curdir= getcwd()
"   call Decho("set b:netrw_curdir<".b:netrw_curdir."> (used getcwd)")
  endif
  let curfile= b:netrw_curdir
"  call Decho("curfile<".curfile.">")

  " save registers
  silent! let keepregstar = @*
  silent! let keepregplus = @+
  silent! let keepregslash= @/

  " if dosplit or file has been modified
  if a:dosplit || &modified || a:style == 6
"   call Decho("case: dosplit=".a:dosplit." modified=".&modified." a:style=".a:style)
   call s:SaveWinVars()

   if a:style == 0      " Explore, Sexplore
"    call Decho("style=0: Explore or Sexplore")
    exe g:netrw_winsize."wincmd s"

   elseif a:style == 1  "Explore!, Sexplore!
"    call Decho("style=1: Explore! or Sexplore!")
    exe g:netrw_winsize."wincmd v"

   elseif a:style == 2  " Hexplore
"    call Decho("style=2: Hexplore")
    exe "bel ".g:netrw_winsize."wincmd s"

   elseif a:style == 3  " Hexplore!
"    call Decho("style=3: Hexplore!")
    exe "abo ".g:netrw_winsize."wincmd s"

   elseif a:style == 4  " Vexplore
"    call Decho("style=4: Vexplore")
    exe "lefta ".g:netrw_winsize."wincmd v"

   elseif a:style == 5  " Vexplore!
"    call Decho("style=5: Vexplore!")
    exe "rightb ".g:netrw_winsize."wincmd v"

   elseif a:style == 6  " Texplore
    call s:SaveBufVars()
"    call Decho("style  = 6: Texplore")
    tabnew
    call s:RestoreBufVars()
   endif
   call s:RestoreWinVars()
  endif
  norm! 0

  if a:0 > 0
"   call Decho("a:1<".a:1.">")
   if a:1 =~ '^\~' && (has("unix") || (exists("g:netrw_cygwin") && g:netrw_cygwin))
    let dirname= substitute(a:1,'\~',expand("$HOME"),'')
"    call Decho("using dirname<".dirname.">  (case: ~ && unix||cygwin)")
   elseif a:1 == '.'
    let dirname= exists("b:netrw_curdir")? b:netrw_curdir : getcwd()
    if dirname !~ '/$'
     let dirname= dirname."/"
    endif
"    call Decho("using dirname<".dirname.">  (case: ".(exists("b:netrw_curdir")? "b:netrw_curdir" : "getcwd()").")")
   elseif a:1 =~ '\$'
    let dirname= expand(a:1)
   else
    let dirname= a:1
"    call Decho("using dirname<".dirname.">")
   endif
  endif

  if dirname =~ '^\*/'
   " Explore */pattern
"   call Decho("case Explore */pattern")
   let pattern= substitute(dirname,'^\*/\(.*\)$','\1','')
"   call Decho("Explore */pat: dirname<".dirname."> -> pattern<".pattern.">")
   if &hls | let keepregslash= s:ExplorePatHls(pattern) | endif
  elseif dirname =~ '^\*\*//'
   " Explore **//pattern
"   call Decho("case Explore **//pattern")
   let pattern     = substitute(dirname,'^\*\*//','','')
   let starstarpat = 1
"   call Decho("Explore **//pat: dirname<".dirname."> -> pattern<".pattern.">")
  endif

  if dirname == "" && a:indx >= 0
   " Explore Hexplore Vexplore Sexplore
"   call Decho("case Explore Hexplore Vexplore Sexplore")
   let newdir= substitute(expand("%:p"),'^\(.*[/\\]\)[^/\\]*$','\1','e')
   if newdir =~ '^scp:' || newdir =~ '^ftp:'
"    call Decho("calling NetBrowse(0,newdir<".newdir.">)")
    call s:NetBrowse(0,newdir)
   else
    if newdir == ""|let newdir= getcwd()|endif
"    call Decho("calling LocalBrowseCheck(newdir<".newdir.">)")
    call netrw#LocalBrowseCheck(newdir)
   endif
   call search('\<'.substitute(curfile,'^.*/','','e').'\>','cW')

  elseif dirname =~ '^\*\*/' || a:indx < 0 || dirname =~ '^\*/'
   " Nexplore, Pexplore, Explore **/... , or Explore */pattern
"   call Decho("case Nexplore, Pexplore, <s-down>, <s-up>, Explore dirname<".dirname.">")
   if !mapcheck("<s-up>","n") && !mapcheck("<s-down>","n") && exists("b:netrw_curdir")
"    call Decho("set up <s-up> and <s-down> maps")
    let s:didstarstar= 1
    nnoremap <buffer> <silent> <s-up>	:Pexplore<cr>
    nnoremap <buffer> <silent> <s-down>	:Nexplore<cr>
   endif

   if has("path_extra")
"    call Decho("has path_extra")
    if !exists("w:netrw_explore_indx")
     let w:netrw_explore_indx= 0
    endif
    let indx = a:indx
"    call Decho("set indx= [a:indx=".indx."]")
"
    if indx == -1
     "Nexplore
"     call Decho("case Nexplore: (indx=".indx.")")
     if !exists("w:netrw_explore_list") " sanity check
      call netrw#ErrorMsg(s:WARNING,"using Nexplore or <s-down> improperly; see help for netrw-starstar",40)
      silent! let @* = keepregstar
      silent! let @+ = keepregstar
      silent! let @/ = keepregslash
"      call Dret("netrw#Explore")
      return
     endif
     let indx= w:netrw_explore_indx
     if indx < 0                        | let indx= 0                           | endif
     if indx >= w:netrw_explore_listlen | let indx= w:netrw_explore_listlen - 1 | endif
     let curfile= w:netrw_explore_list[indx]
"     call Decho("indx=".indx." curfile<".curfile.">")
     while indx < w:netrw_explore_listlen && curfile == w:netrw_explore_list[indx]
      let indx= indx + 1
"      call Decho("indx=".indx." (Nexplore while loop)")
     endwhile
     if indx >= w:netrw_explore_listlen | let indx= w:netrw_explore_listlen - 1 | endif
"     call Decho("Nexplore: indx= [w:netrw_explore_indx=".w:netrw_explore_indx."]=".indx)

    elseif indx == -2
     "Pexplore
"     call Decho("case Pexplore: (indx=".indx.")")
     if !exists("w:netrw_explore_list") " sanity check
      call netrw#ErrorMsg(s:WARNING,"using Pexplore or <s-up> improperly; see help for netrw-starstar",41)
      silent! let @* = keepregstar
      silent! let @+ = keepregstar
      silent! let @/ = keepregslash
"      call Dret("netrw#Explore")
      return
     endif
     let indx= w:netrw_explore_indx
     if indx < 0                        | let indx= 0                           | endif
     if indx >= w:netrw_explore_listlen | let indx= w:netrw_explore_listlen - 1 | endif
     let curfile= w:netrw_explore_list[indx]
"     call Decho("indx=".indx." curfile<".curfile.">")
     while indx >= 0 && curfile == w:netrw_explore_list[indx]
      let indx= indx - 1
"      call Decho("indx=".indx." (Pexplore while loop)")
     endwhile
     if indx < 0                        | let indx= 0                           | endif
"     call Decho("Pexplore: indx= [w:netrw_explore_indx=".w:netrw_explore_indx."]=".indx)

    else
     " Explore -- initialize
     " build list of files to Explore with Nexplore/Pexplore
"     call Decho("case Explore: initialize (indx=".indx.")")
     let w:netrw_explore_indx= 0
     if !exists("b:netrw_curdir")
      let b:netrw_curdir= getcwd()
     endif
"     call Decho("b:netrw_curdir<".b:netrw_curdir.">")

     if exists("pattern")
"      call Decho("pattern exists: building list pattern<".pattern."> cwd<".getcwd().">")
      if exists("starstarpat")
"       call Decho("starstarpat<".starstarpat.">")
       try
        exe "silent vimgrep /".pattern."/gj "."**/*"
       catch /^Vim\%((\a\+)\)\=:E480/
       	call netrw#ErrorMsg(s:WARNING,'no files matched pattern<'.pattern.'>',45)
        if &hls | let keepregslash= s:ExplorePatHls(pattern) | endif
        silent! let @* = keepregstar
        silent! let @+ = keepregstar
	silent! let @/ = keepregslash
"        call Dret("netrw#Explore : no files matched pattern")
        return
       endtry
       let s:netrw_curdir       = b:netrw_curdir
       let w:netrw_explore_list = getqflist()
       let w:netrw_explore_list = map(w:netrw_explore_list,'s:netrw_curdir."/".bufname(v:val.bufnr)')
      else
"       call Decho("no starstarpat")
       exe "vimgrep /".pattern."/gj ".b:netrw_curdir."/*"
       let w:netrw_explore_list = map(getqflist(),'bufname(v:val.bufnr)')
       if &hls | let keepregslash= s:ExplorePatHls(pattern) | endif
      endif
     else
"      call Decho("no pattern: building list based on ".b:netrw_curdir."/".dirname)
      let w:netrw_explore_list= split(expand(b:netrw_curdir."/".dirname),'\n')
      if &hls | let keepregslash= s:ExplorePatHls(dirname) | endif
     endif

     let w:netrw_explore_listlen = len(w:netrw_explore_list)
"     call Decho("w:netrw_explore_list<".string(w:netrw_explore_list)."> listlen=".w:netrw_explore_listlen)

     if w:netrw_explore_listlen == 0 || (w:netrw_explore_listlen == 1 && w:netrw_explore_list[0] =~ '\*\*\/')
      call netrw#ErrorMsg(s:WARNING,"no files matched",42)
      silent! let @* = keepregstar
      silent! let @+ = keepregstar
      silent! let @/ = keepregslash
"      call Dret("netrw#Explore : no files matched")
      return
     endif
    endif

    " NetrwStatusLine support - for exploring support
    let w:netrw_explore_indx= indx
"    call Decho("explorelist<".join(w:netrw_explore_list,',')."> len=".w:netrw_explore_listlen)

    " wrap the indx around, but issue a note
    if indx >= w:netrw_explore_listlen || indx < 0
"     call Decho("wrap indx (indx=".indx." listlen=".w:netrw_explore_listlen.")")
     let indx                = (indx < 0)? ( w:netrw_explore_listlen - 1 ) : 0
     let w:netrw_explore_indx= indx
     call netrw#ErrorMsg(s:NOTE,"no more files match Explore pattern",43)
     sleep 1
    endif

    exe "let dirfile= w:netrw_explore_list[".indx."]"
"    call Decho("dirfile=w:netrw_explore_list[indx=".indx."]= <".dirfile.">")
    let newdir= substitute(dirfile,'/[^/]*$','','e')
"    call Decho("newdir<".newdir.">")

"    call Decho("calling LocalBrowseCheck(newdir<".newdir.">)")
    call netrw#LocalBrowseCheck(newdir)
    if !exists("w:netrw_liststyle")
     let w:netrw_liststyle= g:netrw_liststyle
    endif
    if w:netrw_liststyle == s:THINLIST || w:netrw_liststyle == s:LONGLIST
     call search('^'.substitute(dirfile,"^.*/","","").'\>',"W")
    else
     call search('\<'.substitute(dirfile,"^.*/","","").'\>',"w")
    endif
    let w:netrw_explore_mtchcnt = indx + 1
    let w:netrw_explore_bufnr   = bufnr("%")
    let w:netrw_explore_line    = line(".")
    call s:SetupNetrwStatusLine('%f %h%m%r%=%9*%{NetrwStatusLine()}')
"    call Decho("explore: mtchcnt=".w:netrw_explore_mtchcnt." bufnr=".w:netrw_explore_bufnr." line#".w:netrw_explore_line)

   else
"    call Decho("vim does not have path_extra")
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:WARNING,"your vim needs the +path_extra feature for Exploring with **!",44)
    endif
    silent! let @* = keepregstar
    silent! let @+ = keepregstar
    silent! let @/ = keepregslash
"    call Dret("netrw#Explore : missing +path_extra")
    return
   endif

  else
"   call Decho("case Explore newdir<".dirname.">")
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && dirname =~ '/'
    silent! unlet w:netrw_treedict
    silent! unlet w:netrw_treetop
   endif
   let newdir= dirname
   if !exists("b:netrw_curdir")
    call netrw#LocalBrowseCheck(getcwd())
   else
    call netrw#LocalBrowseCheck(s:NetBrowseChgDir(1,newdir))
   endif
  endif

  silent! let @* = keepregstar
  silent! let @+ = keepregstar
  silent! let @/ = keepregslash
"  call Dret("netrw#Explore : @/<".@/.">")
endfun

" ---------------------------------------------------------------------
" s:ExplorePatHls: converts an Explore pattern into a regular expression search pattern {{{2
fun! s:ExplorePatHls(pattern)
"  call Dfunc("s:ExplorePatHls(pattern<".a:pattern.">)")
  let repat= substitute(a:pattern,'^**/\{1,2}','','')
"  call Decho("repat<".repat.">")
  let repat= escape(repat,'][.\')
"  call Decho("repat<".repat.">")
  let repat= '\<'.substitute(repat,'\*','\\(\\S\\+ \\)*\\S\\+','g').'\>'
"  call Dret("s:ExplorePatHls repat<".repat.">")
  return repat
endfun

" ---------------------------------------------------------------------
" SetupNetrwStatusLine: {{{2
fun! s:SetupNetrwStatusLine(statline)
"  call Dfunc("SetupNetrwStatusLine(statline<".a:statline.">)")

  if !exists("s:netrw_setup_statline")
   let s:netrw_setup_statline= 1
"   call Decho("do first-time status line setup")

   if !exists("s:netrw_users_stl")
    let s:netrw_users_stl= &stl
   endif
   if !exists("s:netrw_users_ls")
    let s:netrw_users_ls= &laststatus
   endif

   " set up User9 highlighting as needed
   let keepa= @a
   redir @a
   try
    hi User9
   catch /^Vim\%((\a\+)\)\=:E411/
    if &bg == "dark"
     hi User9 ctermfg=yellow ctermbg=blue guifg=yellow guibg=blue
    else
     hi User9 ctermbg=yellow ctermfg=blue guibg=yellow guifg=blue
    endif
   endtry
   redir END
   let @a= keepa
  endif

  " set up status line (may use User9 highlighting)
  " insure that windows have a statusline
  " make sure statusline is displayed
  let &stl=a:statline
  setlocal laststatus=2
"  call Decho("stl=".&stl)
  redraw!

"  call Dret("SetupNetrwStatusLine : stl=".&stl)
endfun

" ---------------------------------------------------------------------
" NetrwStatusLine: {{{2
fun! NetrwStatusLine()

" vvv NetrwStatusLine() debugging vvv
"  let g:stlmsg=""
"  if !exists("w:netrw_explore_bufnr")
"   let g:stlmsg="!X<explore_bufnr>"
"  elseif w:netrw_explore_bufnr != bufnr("%")
"   let g:stlmsg="explore_bufnr!=".bufnr("%")
"  endif
"  if !exists("w:netrw_explore_line")
"   let g:stlmsg=" !X<explore_line>"
"  elseif w:netrw_explore_line != line(".")
"   let g:stlmsg=" explore_line!={line(.)<".line(".").">"
"  endif
"  if !exists("w:netrw_explore_list")
"   let g:stlmsg=" !X<explore_list>"
"  endif
" ^^^ NetrwStatusLine() debugging ^^^

  if !exists("w:netrw_explore_bufnr") || w:netrw_explore_bufnr != bufnr("%") || !exists("w:netrw_explore_line") || w:netrw_explore_line != line(".") || !exists("w:netrw_explore_list")
   " restore user's status line
   let &stl        = s:netrw_users_stl
   let &laststatus = s:netrw_users_ls
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
" SetSort: sets up the sort based on the g:netrw_sort_sequence {{{2
"          What this function does is to compute a priority for the patterns
"          in the g:netrw_sort_sequence.  It applies a substitute to any
"          "files" that satisfy each pattern, putting the priority / in
"          front.  An "*" pattern handles the default priority.
fun! s:SetSort()
"  call Dfunc("SetSort() bannercnt=".w:netrw_bannercnt)
  if w:netrw_liststyle == s:LONGLIST
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
    exe 'silent keepjumps '.w:netrw_bannercnt.',$v/^\d\{3}\//s/^/'.spriority.'/'
   else
    exe 'silent keepjumps '.w:netrw_bannercnt.',$g/'.eseq.'/s/^/'.spriority.'/'
   endif
   let priority = priority + 1
  endwhile

  " Following line associated with priority -- items that satisfy a priority
  " pattern get prefixed by ###/ which permits easy sorting by priority.
  " Sometimes files can satisfy multiple priority patterns -- only the latest
  " priority pattern needs to be retained.  So, at this point, these excess
  " priority prefixes need to be removed, but not directories that happen to
  " be just digits themselves.
  exe 'silent keepjumps '.w:netrw_bannercnt.',$s/^\(\d\{3}\/\)\%(\d\{3}\/\)\+\ze./\1/e'

"  call Dret("SetSort")
endfun

" =====================================================================
" Support Functions: {{{1

" ---------------------------------------------------------------------
"  ComposePath: Appends a new part to a path taking different systems into consideration {{{2
fun! s:ComposePath(base,subdir)
"  call Dfunc("s:ComposePath(base<".a:base."> subdir<".a:subdir.">)")
  if(has("amiga"))
   let ec = a:base[strlen(a:base)-1]
   if ec != '/' && ec != ':'
    let ret = a:base . "/" . a:subdir
   else
    let ret = a:base . a:subdir
   endif
  elseif a:base =~ '^\a\+://'
   let urlbase = substitute(a:base,'^\(\a\+://.\{-}/\)\(.*\)$','\1','')
   let curpath = substitute(a:base,'^\(\a\+://.\{-}/\)\(.*\)$','\2','')
   let ret     = urlbase.curpath.a:subdir
"   call Decho("urlbase<".urlbase.">")
"   call Decho("curpath<".curpath.">")
"   call Decho("ret<".ret.">")
  else
   let ret = substitute(a:base."/".a:subdir,"//","/","g")
  endif
"  call Dret("s:ComposePath ".ret)
  return ret
endfun

" ---------------------------------------------------------------------
" netrw#ErrorMsg: {{{2
"   0=note     = s:NOTE
"   1=warning  = s:WARNING
"   2=error    = s:ERROR
"   Mar 19, 2007 : max errnum currently is 49
fun! netrw#ErrorMsg(level,msg,errnum)
"  call Dfunc("netrw#ErrorMsg(level=".a:level." msg<".a:msg."> errnum=".a:errnum.") g:netrw_use_errorwindow=".g:netrw_use_errorwindow)

  if a:level == 1
   let level= "**warning** (netrw) "
  elseif a:level == 2
   let level= "**error** (netrw) "
  else
   let level= "**note** (netrw) "
  endif

  if g:netrw_use_errorwindow
   " (default) netrw creates a one-line window to show error/warning
   " messages (reliably displayed)

   " record current window number for NetRestorePosn()'s benefit
   let s:winBeforeErr= winnr()
 
   " getting messages out reliably is just plain difficult!
   " This attempt splits the current window, creating a one line window.
   if bufexists("NetrwMessage") && bufwinnr("NetrwMessage") > 0
    exe bufwinnr("NetrwMessage")."wincmd w"
    set ma noro
    call setline(line("$")+1,level.a:msg)
    $
   else
    bo 1split
    enew
    setlocal bt=nofile
    file NetrwMessage
    call setline(line("$"),level.a:msg)
   endif
   if &fo !~ '[ta]'
    syn clear
    syn match netrwMesgNote	"^\*\*note\*\*"
    syn match netrwMesgWarning	"^\*\*warning\*\*"
    syn match netrwMesgError	"^\*\*error\*\*"
    hi link netrwMesgWarning WarningMsg
    hi link netrwMesgError   Error
   endif
   setlocal noma ro bh=wipe

  else
   " (optional) netrw will show messages using echomsg.  Even if the
   " message doesn't appear, at least it'll be recallable via :messages
   redraw!
   if a:level == s:WARNING
    echohl WarningMsg
   elseif a:level == s:ERROR
    echohl Error
   endif
   echomsg level.a:msg
"   call Decho("echomsg ***netrw*** ".a:msg)
   echohl None
  endif

"  call Dret("netrw#ErrorMsg")
endfun

" ---------------------------------------------------------------------
"  netrw#RFC2396: converts %xx into characters {{{2
fun! netrw#RFC2396(fname)
"  call Dfunc("netrw#RFC2396(fname<".a:fname.">)")
  let fname = escape(substitute(a:fname,'%\(\x\x\)','\=nr2char("0x".submatch(1))','ge')," \t")
"  call Dret("netrw#RFC2396 ".fname)
  return fname
endfun

" ---------------------------------------------------------------------
" s:FileReadable: o/s independent filereadable {{{2
fun! s:FileReadable(fname)
"  call Dfunc("s:FileReadable(fname<".a:fname.">)")

  if g:netrw_cygwin
   let ret= filereadable(substitute(a:fname,'/cygdrive/\(.\)','\1:/',''))
  else
   let ret= filereadable(a:fname)
  endif

"  call Dret("s:FileReadable ".ret)
  return ret
endfun

" ---------------------------------------------------------------------
"  s:GetTempfile: gets a tempname that'll work for various o/s's {{{2
"                 Places correct suffix on end of temporary filename,
"                 using the suffix provided with fname
fun! s:GetTempfile(fname)
"  call Dfunc("s:GetTempfile(fname<".a:fname.">)")

  if !exists("b:netrw_tmpfile")
   " get a brand new temporary filename
   let tmpfile= tempname()
"   call Decho("tmpfile<".tmpfile."> : from tempname()")
 
   let tmpfile= escape(substitute(tmpfile,'\','/','ge'),g:netrw_tmpfile_escape)
"   call Decho("tmpfile<".tmpfile."> : chgd any \\ -> /")
 
   " sanity check -- does the temporary file's directory exist?
   if !isdirectory(substitute(tmpfile,'[^/]\+$','','e'))
    call netrw#ErrorMsg(s:ERROR,"your <".substitute(tmpfile,'[^/]\+$','','e')."> directory is missing!",2)
"    call Dret("s:GetTempfile getcwd<".getcwd().">")
    return ""
   endif
 
   " let netrw#NetSource() know about the tmpfile
   let s:netrw_tmpfile= tmpfile " used by netrw#NetSource()
"   call Decho("tmpfile<".tmpfile."> s:netrw_tmpfile<".s:netrw_tmpfile.">")
 
   " o/s dependencies
   if g:netrw_cygwin == 1
    let tmpfile = substitute(tmpfile,'^\(\a\):','/cygdrive/\1','e')
   elseif has("win32") || has("win95") || has("win64") || has("win16")
    let tmpfile = substitute(tmpfile,'/','\\','g')
   else
    let tmpfile = tmpfile  
   endif
   let b:netrw_tmpfile= tmpfile
"   call Decho("o/s dependent fixed tempname<".tmpfile.">")
  else
   " re-use temporary filename
   let tmpfile= b:netrw_tmpfile
"   call Decho("tmpfile<".tmpfile."> re-using")
  endif

  " use fname's suffix for the temporary file
  if a:fname != ""
   if a:fname =~ '\.[^./]\+$'
"    call Decho("using fname<".a:fname.">'s suffix")
    if a:fname =~ '.tar.gz' || a:fname =~ '.tar.bz2'
     let suffix = ".tar".substitute(a:fname,'^.*\(\.[^./]\+\)$','\1','e')
    else
     let suffix = substitute(a:fname,'^.*\(\.[^./]\+\)$','\1','e')
    endif
    let suffix = escape(suffix,g:netrw_tmpfile_escape)
"    call Decho("suffix<".suffix.">")
    let tmpfile= substitute(tmpfile,'\.tmp$','','e')
"    call Decho("chgd tmpfile<".tmpfile."> (removed any .tmp suffix)")
    let tmpfile .= suffix
"    call Decho("chgd tmpfile<".tmpfile."> (added ".suffix." suffix) netrw_fname<".b:netrw_fname.">")
    let s:netrw_tmpfile= tmpfile " supports netrw#NetSource()
   endif
  endif

"  call Dret("s:GetTempfile <".tmpfile.">")
  return tmpfile
endfun  

" ---------------------------------------------------------------------
" s:MakeSshCmd: transforms input command using USEPORT HOSTNAME into {{{2
"               a correct command
fun! s:MakeSshCmd(sshcmd)
"  call Dfunc("s:MakeSshCmd(sshcmd<".a:sshcmd.">)")
  let sshcmd = substitute(a:sshcmd,'\<HOSTNAME\>',s:user.s:machine,'')
  if exists("g:netrw_port") && g:netrw_port != ""
   let sshcmd= substitute(sshcmd,"USEPORT",g:netrw_sshport.' '.g:netrw_port,'')
  elseif exists("s:port") && s:port != ""
   let sshcmd= substitute(sshcmd,"USEPORT",g:netrw_sshport.' '.s:port,'')
  else
   let sshcmd= substitute(sshcmd,"USEPORT ",'','')
  endif
"  call Dret("s:MakeSshCmd <".sshcmd.">")
  return sshcmd
endfun

" ---------------------------------------------------------------------
" s:NetrwEnew: opens a new buffer, passes netrw buffer variables through {{{2
fun! s:NetrwEnew(curdir)
"  call Dfunc("s:NetrwEnew(curdir<".a:curdir.">) buf#".bufnr("%")."<".bufname("%").">")

  " grab a function-local copy of buffer variables
  if exists("b:netrw_bannercnt")      |let netrw_bannercnt       = b:netrw_bannercnt      |endif
  if exists("b:netrw_browser_active") |let netrw_browser_active  = b:netrw_browser_active |endif
  if exists("b:netrw_cpf")            |let netrw_cpf             = b:netrw_cpf            |endif
  if exists("b:netrw_curdir")         |let netrw_curdir          = b:netrw_curdir         |endif
  if exists("b:netrw_explore_bufnr")  |let netrw_explore_bufnr   = b:netrw_explore_bufnr  |endif
  if exists("b:netrw_explore_indx")   |let netrw_explore_indx    = b:netrw_explore_indx   |endif
  if exists("b:netrw_explore_line")   |let netrw_explore_line    = b:netrw_explore_line   |endif
  if exists("b:netrw_explore_list")   |let netrw_explore_list    = b:netrw_explore_list   |endif
  if exists("b:netrw_explore_listlen")|let netrw_explore_listlen = b:netrw_explore_listlen|endif
  if exists("b:netrw_explore_mtchcnt")|let netrw_explore_mtchcnt = b:netrw_explore_mtchcnt|endif
  if exists("b:netrw_fname")          |let netrw_fname           = b:netrw_fname          |endif
  if exists("b:netrw_lastfile")       |let netrw_lastfile        = b:netrw_lastfile       |endif
  if exists("b:netrw_liststyle")      |let netrw_liststyle       = b:netrw_liststyle      |endif
  if exists("b:netrw_method")         |let netrw_method          = b:netrw_method         |endif
  if exists("b:netrw_option")         |let netrw_option          = b:netrw_option         |endif
  if exists("b:netrw_prvdir")         |let netrw_prvdir          = b:netrw_prvdir         |endif

  if getline(2) =~ '^" Netrw Directory Listing'
"   call Decho("generate a buffer with keepjumps keepalt enew! (1)")
   keepjumps keepalt enew!
  else
"   call Decho("generate a buffer with keepjumps enew! (2)")
   keepjumps enew!
  endif

  " copy function-local variables to buffer variable equivalents
  if exists("netrw_bannercnt")      |let b:netrw_bannercnt       = netrw_bannercnt      |endif
  if exists("netrw_browser_active") |let b:netrw_browser_active  = netrw_browser_active |endif
  if exists("netrw_cpf")            |let b:netrw_cpf             = netrw_cpf            |endif
  if exists("netrw_curdir")         |let b:netrw_curdir          = netrw_curdir         |endif
  if exists("netrw_explore_bufnr")  |let b:netrw_explore_bufnr   = netrw_explore_bufnr  |endif
  if exists("netrw_explore_indx")   |let b:netrw_explore_indx    = netrw_explore_indx   |endif
  if exists("netrw_explore_line")   |let b:netrw_explore_line    = netrw_explore_line   |endif
  if exists("netrw_explore_list")   |let b:netrw_explore_list    = netrw_explore_list   |endif
  if exists("netrw_explore_listlen")|let b:netrw_explore_listlen = netrw_explore_listlen|endif
  if exists("netrw_explore_mtchcnt")|let b:netrw_explore_mtchcnt = netrw_explore_mtchcnt|endif
  if exists("netrw_fname")          |let b:netrw_fname           = netrw_fname          |endif
  if exists("netrw_lastfile")       |let b:netrw_lastfile        = netrw_lastfile       |endif
  if exists("netrw_liststyle")      |let b:netrw_liststyle       = netrw_liststyle      |endif
  if exists("netrw_method")         |let b:netrw_method          = netrw_method         |endif
  if exists("netrw_option")         |let b:netrw_option          = netrw_option         |endif
  if exists("netrw_prvdir")         |let b:netrw_prvdir          = netrw_prvdir         |endif

  let b:netrw_curdir= a:curdir
  if b:netrw_curdir =~ '/$'
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
    file NetrwTreeListing
   else
    exe "silent! file ".b:netrw_curdir
   endif
  endif

"  call Dret("s:NetrwEnew : buf#".bufnr("%")."<".bufname("%").">")
endfun

" ------------------------------------------------------------------------
" s:RemotePathAnalysis: {{{2
fun! s:RemotePathAnalysis(dirname)
"  call Dfunc("s:RemotePathAnalysis()")

  let dirpat  = '^\(\w\{-}\)://\(\w\+@\)\=\([^/:#]\+\)\%([:#]\(\d\+\)\)\=/\(.*\)$'
  let s:method  = substitute(a:dirname,dirpat,'\1','')
  let s:user    = substitute(a:dirname,dirpat,'\2','')
  let s:machine = substitute(a:dirname,dirpat,'\3','')
  let s:port    = substitute(a:dirname,dirpat,'\4','')
  let s:path    = substitute(a:dirname,dirpat,'\5','')
  let s:fname   = substitute(a:dirname,'^.*/\ze.','','')

"  call Decho("set up s:method <".s:method .">")
"  call Decho("set up s:user   <".s:user   .">")
"  call Decho("set up s:machine<".s:machine.">")
"  call Decho("set up s:port   <".s:port.">")
"  call Decho("set up s:path   <".s:path   .">")
"  call Decho("set up s:fname  <".s:fname  .">")

"  call Dret("s:RemotePathAnalysis")
endfun

" ---------------------------------------------------------------------
" s:RestoreBufVars: {{{2
fun! s:RestoreBufVars()
"  call Dfunc("s:RestoreBufVars()")

  if exists("s:netrw_curdir")        |let b:netrw_curdir         = s:netrw_curdir        |endif
  if exists("s:netrw_lastfile")      |let b:netrw_lastfile       = s:netrw_lastfile      |endif
  if exists("s:netrw_method")        |let b:netrw_method         = s:netrw_method        |endif
  if exists("s:netrw_fname")         |let b:netrw_fname          = s:netrw_fname         |endif
  if exists("s:netrw_machine")       |let b:netrw_machine        = s:netrw_machine       |endif
  if exists("s:netrw_browser_active")|let b:netrw_browser_active = s:netrw_browser_active|endif

"  call Dret("s:RestoreBufVars")
endfun

" ---------------------------------------------------------------------
" s:RestoreWinVars: (used by Explore() and NetSplit()) {{{2
fun! s:RestoreWinVars()
"  call Dfunc("s:RestoreWinVars()")
  if exists("s:bannercnt")      |let w:netrw_bannercnt       = s:bannercnt      |unlet s:bannercnt      |endif
  if exists("s:col")            |let w:netrw_col             = s:col            |unlet s:col            |endif
  if exists("s:curdir")         |let w:netrw_curdir          = s:curdir         |unlet s:curdir         |endif
  if exists("s:explore_bufnr")  |let w:netrw_explore_bufnr   = s:explore_bufnr  |unlet s:explore_bufnr  |endif
  if exists("s:explore_indx")   |let w:netrw_explore_indx    = s:explore_indx   |unlet s:explore_indx   |endif
  if exists("s:explore_line")   |let w:netrw_explore_line    = s:explore_line   |unlet s:explore_line   |endif
  if exists("s:explore_listlen")|let w:netrw_explore_listlen = s:explore_listlen|unlet s:explore_listlen|endif
  if exists("s:explore_list")   |let w:netrw_explore_list    = s:explore_list   |unlet s:explore_list   |endif
  if exists("s:explore_mtchcnt")|let w:netrw_explore_mtchcnt = s:explore_mtchcnt|unlet s:explore_mtchcnt|endif
  if exists("s:fpl")            |let w:netrw_fpl             = s:fpl            |unlet s:fpl            |endif
  if exists("s:hline")          |let w:netrw_hline           = s:hline          |unlet s:hline          |endif
  if exists("s:line")           |let w:netrw_line            = s:line           |unlet s:line           |endif
  if exists("s:liststyle")      |let w:netrw_liststyle       = s:liststyle      |unlet s:liststyle      |endif
  if exists("s:method")         |let w:netrw_method          = s:method         |unlet s:method         |endif
  if exists("s:prvdir")         |let w:netrw_prvdir          = s:prvdir         |unlet s:prvdir         |endif
  if exists("s:treedict")       |let w:netrw_treedict        = s:treedict       |unlet s:treedict       |endif
  if exists("s:treetop")        |let w:netrw_treetop         = s:treetop        |unlet s:treetop        |endif
  if exists("s:winnr")          |let w:netrw_winnr           = s:winnr          |unlet s:winnr          |endif
"  call Dret("s:RestoreWinVars")
endfun

" ---------------------------------------------------------------------
" s:SaveBufVars: {{{2
fun! s:SaveBufVars()
"  call Dfunc("s:SaveBufVars()")

  if exists("b:netrw_curdir")        |let s:netrw_curdir         = b:netrw_curdir        |endif
  if exists("b:netrw_lastfile")      |let s:netrw_lastfile       = b:netrw_lastfile      |endif
  if exists("b:netrw_method")        |let s:netrw_method         = b:netrw_method        |endif
  if exists("b:netrw_fname")         |let s:netrw_fname          = b:netrw_fname         |endif
  if exists("b:netrw_machine")       |let s:netrw_machine        = b:netrw_machine       |endif
  if exists("b:netrw_browser_active")|let s:netrw_browser_active = b:netrw_browser_active|endif

"  call Dret("s:SaveBufVars")
endfun

" ---------------------------------------------------------------------
" s:SaveWinVars: (used by Explore() and NetSplit()) {{{2
fun! s:SaveWinVars()
"  call Dfunc("s:SaveWinVars()")
  if exists("w:netrw_bannercnt")      |let s:bannercnt       = w:netrw_bannercnt      |endif
  if exists("w:netrw_col")            |let s:col             = w:netrw_col            |endif
  if exists("w:netrw_curdir")         |let s:curdir          = w:netrw_curdir         |endif
  if exists("w:netrw_explore_bufnr")  |let s:explore_bufnr   = w:netrw_explore_bufnr  |endif
  if exists("w:netrw_explore_indx")   |let s:explore_indx    = w:netrw_explore_indx   |endif
  if exists("w:netrw_explore_line")   |let s:explore_line    = w:netrw_explore_line   |endif
  if exists("w:netrw_explore_listlen")|let s:explore_listlen = w:netrw_explore_listlen|endif
  if exists("w:netrw_explore_list")   |let s:explore_list    = w:netrw_explore_list   |endif
  if exists("w:netrw_explore_mtchcnt")|let s:explore_mtchcnt = w:netrw_explore_mtchcnt|endif
  if exists("w:netrw_fpl")            |let s:fpl             = w:netrw_fpl            |endif
  if exists("w:netrw_hline")          |let s:hline           = w:netrw_hline          |endif
  if exists("w:netrw_line")           |let s:line            = w:netrw_line           |endif
  if exists("w:netrw_liststyle")      |let s:liststyle       = w:netrw_liststyle      |endif
  if exists("w:netrw_method")         |let s:method          = w:netrw_method         |endif
  if exists("w:netrw_prvdir")         |let s:prvdir          = w:netrw_prvdir         |endif
  if exists("w:netrw_treedict")       |let s:treedict        = w:netrw_treedict       |endif
  if exists("w:netrw_treetop")        |let s:treetop         = w:netrw_treetop        |endif
  if exists("w:netrw_winnr")          |let s:winnr           = w:netrw_winnr          |endif
"  call Dret("s:SaveWinVars")
endfun

" ---------------------------------------------------------------------
" s:SetBufWinVars: (used by NetBrowse() and LocalBrowseCheck()) {{{2
"   To allow separate windows to have their own activities, such as
"   Explore **/pattern, several variables have been made window-oriented.
"   However, when the user splits a browser window (ex: ctrl-w s), these
"   variables are not inherited by the new window.  SetBufWinVars() and
"   UseBufWinVars() get around that.
fun! s:SetBufWinVars()
"  call Dfunc("s:SetBufWinVars()")
  if exists("w:netrw_liststyle")      |let b:netrw_liststyle      = w:netrw_liststyle      |endif
  if exists("w:netrw_bannercnt")      |let b:netrw_bannercnt      = w:netrw_bannercnt      |endif
  if exists("w:netrw_method")         |let b:netrw_method         = w:netrw_method         |endif
  if exists("w:netrw_prvdir")         |let b:netrw_prvdir         = w:netrw_prvdir         |endif
  if exists("w:netrw_explore_indx")   |let b:netrw_explore_indx   = w:netrw_explore_indx   |endif
  if exists("w:netrw_explore_listlen")|let b:netrw_explore_listlen= w:netrw_explore_listlen|endif
  if exists("w:netrw_explore_mtchcnt")|let b:netrw_explore_mtchcnt= w:netrw_explore_mtchcnt|endif
  if exists("w:netrw_explore_bufnr")  |let b:netrw_explore_bufnr  = w:netrw_explore_bufnr  |endif
  if exists("w:netrw_explore_line")   |let b:netrw_explore_line   = w:netrw_explore_line   |endif
  if exists("w:netrw_explore_list")   |let b:netrw_explore_list   = w:netrw_explore_list   |endif
"  call Dret("s:SetBufWinVars")
endfun

" ---------------------------------------------------------------------
" s:System: using Steve Hall's idea to insure that Windows paths stay {{{2
"              acceptable.  No effect on Unix paths.
"  Examples of use:  let result= s:System("system",path)
"                    let result= s:System("delete",path)
fun! s:System(cmd,path)
"  call Dfunc("s:System(cmd<".a:cmd."> path<".a:path.">)")

  let path = a:path
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   " system call prep
   " remove trailing slash (Win95)
   let path = substitute(path, '\(\\\|/\)$', '', 'g')
   " remove escaped spaces
   let path = substitute(path, '\ ', ' ', 'g')
   " convert slashes to backslashes
   let path = substitute(path, '/', '\', 'g')
   if exists("+shellslash")
    let sskeep= &shellslash
    setlocal noshellslash
    exe "let result= ".a:cmd."('".path."')"
    let &shellslash = sskeep
   else
    exe "let result= ".a:cmd."(".g:netrw_shq.path.g:netrw_shq.")"
   endif
  else
   exe "let result= ".a:cmd."('".path."')"
  endif

"  call Decho("result<".result.">")
"  call Dret("s:System")
  return result
endfun

" ---------------------------------------------------------------------
" s:UseBufWinVars: (used by NetBrowse() and LocalBrowseCheck() {{{2
"              Matching function to BufferWinVars()
fun! s:UseBufWinVars()
"  call Dfunc("s:UseBufWinVars()")
  if exists("b:netrw_liststyle")       && !exists("w:netrw_liststyle")      |let w:netrw_liststyle       = b:netrw_liststyle      |endif
  if exists("b:netrw_bannercnt")       && !exists("w:netrw_bannercnt")      |let w:netrw_bannercnt       = b:netrw_bannercnt      |endif
  if exists("b:netrw_method")          && !exists("w:netrw_method")         |let w:netrw_method          = b:netrw_method         |endif
  if exists("b:netrw_prvdir")          && !exists("w:netrw_prvdir")         |let w:netrw_prvdir          = b:netrw_prvdir         |endif
  if exists("b:netrw_explore_indx")    && !exists("w:netrw_explore_indx")   |let w:netrw_explore_indx    = b:netrw_explore_indx   |endif
  if exists("b:netrw_explore_listlen") && !exists("w:netrw_explore_listlen")|let w:netrw_explore_listlen = b:netrw_explore_listlen|endif
  if exists("b:netrw_explore_mtchcnt") && !exists("w:netrw_explore_mtchcnt")|let w:netrw_explore_mtchcnt = b:netrw_explore_mtchcnt|endif
  if exists("b:netrw_explore_bufnr")   && !exists("w:netrw_explore_bufnr")  |let w:netrw_explore_bufnr   = b:netrw_explore_bufnr  |endif
  if exists("b:netrw_explore_line")    && !exists("w:netrw_explore_line")   |let w:netrw_explore_line    = b:netrw_explore_line   |endif
  if exists("b:netrw_explore_list")    && !exists("w:netrw_explore_list")   |let w:netrw_explore_list    = b:netrw_explore_list   |endif
"  call Dret("s:UseBufWinVars")
endfun

" ---------------------------------------------------------------------
" Settings Restoration: {{{2
let &cpo= s:keepcpo
unlet s:keepcpo

" ------------------------------------------------------------------------
" Modelines: {{{1
" vim:ts=8 fdm=marker
