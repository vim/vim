" netrw.vim: Handles file transfer and remote directory listing across
"            AUTOLOAD SECTION
" Date:		Apr 01, 2011
" Version:	141
" Maintainer:	Charles E Campbell, Jr <NdrOchip@ScampbellPfamily.AbizM-NOSPAM>
" GetLatestVimScripts: 1075 1 :AutoInstall: netrw.vim
" Copyright:    Copyright (C) 1999-2010 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               netrw.vim, netrwPlugin.vim, and netrwSettings.vim are provided
"               *as is* and come with no warranty of any kind, either
"               expressed or implied. By using this plugin, you agree that
"               in no event will the copyright holder be liable for any damages
"               resulting from the use of this software.
"redraw!|call DechoSep()|call inputsave()|call input("Press <cr> to continue")|call inputrestore()
"
"  But be doers of the Word, and not only hearers, deluding your own selves {{{1
"  (James 1:22 RSV)
" =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
" Load Once: {{{1
if &cp || exists("g:loaded_netrw")
  finish
endif
let g:loaded_netrw = "v141"
if v:version < 702
 echohl WarningMsg
 echo "***warning*** this version of netrw needs vim 7.2"
 echohl Normal
 finish
endif
if !exists("s:NOTE")
 let s:NOTE    = 0
 let s:WARNING = 1
 let s:ERROR   = 2
endif

" sanity checks
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
" NetrwInit: initializes variables if they haven't been defined {{{2
"            Loosely,  varname = value.
fun s:NetrwInit(varname,value)
  if !exists(a:varname)
   if type(a:value) == 0
    exe "let ".a:varname."=".a:value
   elseif type(a:value) == 1
    exe "let ".a:varname."="."'".a:value."'"
   else
    exe "let ".a:varname."=".a:value
   endif
  endif
endfun

" ---------------------------------------------------------------------
"  Netrw Constants: {{{2
call s:NetrwInit("g:netrw_dirhist_cnt",0)
if !exists("s:LONGLIST")
 call s:NetrwInit("s:THINLIST",0)
 call s:NetrwInit("s:LONGLIST",1)
 call s:NetrwInit("s:WIDELIST",2)
 call s:NetrwInit("s:TREELIST",3)
 call s:NetrwInit("s:MAXLIST" ,4)
endif

" ---------------------------------------------------------------------
" Default values for netrw's global protocol variables {{{2
call s:NetrwInit("g:netrw_use_errorwindow",1)

if !exists("g:netrw_dav_cmd")
 if executable("cadaver")
  let g:netrw_dav_cmd	= "cadaver"
 elseif executable("curl")
  let g:netrw_dav_cmd	= "curl"
 else
  let g:netrw_dav_cmd   = ""
 endif
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
let s:netrw_ftp_cmd= g:netrw_ftp_cmd
if !exists("g:netrw_http_cmd")
 if executable("elinks")
  let g:netrw_http_cmd = "elinks"
  call s:NetrwInit("g:netrw_http_xcmd","-source >")
 elseif executable("links")
  let g:netrw_http_cmd = "links"
  call s:NetrwInit("g:netrw_http_xcmd","-source >")
 elseif executable("curl")
  let g:netrw_http_cmd	= "curl"
  call s:NetrwInit("g:netrw_http_xcmd","-o")
 elseif executable("wget")
  let g:netrw_http_cmd	= "wget"
  call s:NetrwInit("g:netrw_http_xcmd","-q -O")
 elseif executable("fetch")
  let g:netrw_http_cmd	= "fetch"
  call s:NetrwInit("g:netrw_http_xcmd","-o")
 else
  let g:netrw_http_cmd	= ""
 endif
endif
call s:NetrwInit("g:netrw_rcp_cmd"  , "rcp")
call s:NetrwInit("g:netrw_rsync_cmd", "rsync")
call s:NetrwInit("g:netrw_scp_cmd"  , "scp -q")
call s:NetrwInit("g:netrw_sftp_cmd" , "sftp")
call s:NetrwInit("g:netrw_ssh_cmd"  , "ssh")

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
" Cygwin Detection ------- {{{3
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
endif
" Default values - a-c ---------- {{{3
call s:NetrwInit("g:netrw_alto"        , &sb)
call s:NetrwInit("g:netrw_altv"        , &spr)
call s:NetrwInit("g:netrw_banner"      , 1)
call s:NetrwInit("g:netrw_browse_split", 0)
call s:NetrwInit("g:netrw_bufsettings" , "noma nomod nonu nobl nowrap ro")
call s:NetrwInit("g:netrw_chgwin"      , -1)
call s:NetrwInit("g:netrw_compress"    , "gzip")
call s:NetrwInit("g:netrw_ctags"       , "ctags")
if exists("g:netrw_cursorline") && !exists("g:netrw_cursor")
 call netrw#ErrorMsg(s:NOTE,'g:netrw_cursorline is deprecated; use g:netrw_cursor instead',77)
 let g:netrw_cursor= g:netrw_cursorline
endif
call s:NetrwInit("g:netrw_cursor"      , 2)
let s:netrw_usercul = &cursorline
let s:netrw_usercuc = &cursorcolumn
" Default values - d-g ---------- {{{3
call s:NetrwInit("g:netrw_dirhist_cnt"      , 0)
call s:NetrwInit("g:netrw_decompress"       , '{ ".gz" : "gunzip", ".bz2" : "bunzip2", ".zip" : "unzip", ".tar" : "tar -xf", ".xz" : "unxz" }')
call s:NetrwInit("g:netrw_dirhistmax"       , 10)
call s:NetrwInit("g:netrw_fastbrowse"       , 1)
call s:NetrwInit("g:netrw_ftp_browse_reject", '^total\s\+\d\+$\|^Trying\s\+\d\+.*$\|^KERBEROS_V\d rejected\|^Security extensions not\|No such file\|: connect to address [0-9a-fA-F:]*: No route to host$')
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
call s:NetrwInit("g:netrw_ftpmode",'binary')
" Default values - h-lh ---------- {{{3
call s:NetrwInit("g:netrw_hide",1)
if !exists("g:netrw_ignorenetrc")
 if &shell =~ '\c\<\%(cmd\|4nt\)\.exe$'
  let g:netrw_ignorenetrc= 1
 else
  let g:netrw_ignorenetrc= 0
 endif
endif
call s:NetrwInit("g:netrw_keepdir",1)
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
call s:NetrwInit("g:netrw_list_hide","")
" Default values - lh-lz ---------- {{{3
if !exists("g:netrw_localcopycmd")
 if has("win32") || has("win95") || has("win64") || has("win16")
  if g:netrw_cygwin
   let g:netrw_localcopycmd= "cp"
  else
   let g:netrw_localcopycmd= "copy"
  endif
 elseif has("unix") || has("macunix")
  let g:netrw_localcopycmd= "cp"
 else
  let g:netrw_localcopycmd= ""
 endif
endif
call s:NetrwInit("g:netrw_local_mkdir","mkdir")
if !exists("g:netrw_localmovecmd")
 if has("win32") || has("win95") || has("win64") || has("win16")
  if g:netrw_cygwin
   let g:netrw_localmovecmd= "mv"
  else
   let g:netrw_localmovecmd= "move"
  endif
 elseif has("unix") || has("macunix")
  let g:netrw_localmovecmd= "mv"
 else
  let g:netrw_localmovecmd= ""
 endif
endif
call s:NetrwInit("g:netrw_local_rmdir", "rmdir")
call s:NetrwInit("g:netrw_liststyle"  , s:THINLIST)
" sanity checks
if g:netrw_liststyle < 0 || g:netrw_liststyle >= s:MAXLIST
 let g:netrw_liststyle= s:THINLIST
endif
if g:netrw_liststyle == s:LONGLIST && g:netrw_scp_cmd !~ '^pscp'
 let g:netrw_list_cmd= g:netrw_list_cmd." -l"
endif
" Default values - m-r ---------- {{{3
call s:NetrwInit("g:netrw_markfileesc"   , '*./[\~')
call s:NetrwInit("g:netrw_maxfilenamelen", 32)
call s:NetrwInit("g:netrw_menu"          , 1)
call s:NetrwInit("g:netrw_mkdir_cmd"     , g:netrw_ssh_cmd." USEPORT HOSTNAME mkdir")
call s:NetrwInit("g:netrw_mousemaps"     , (exists("&mouse") && &mouse =~ '[anh]'))
call s:NetrwInit("g:netrw_retmap"        , 0)
if has("unix") || (exists("g:netrw_cygwin") && g:netrw_cygwin)
 call s:NetrwInit("g:netrw_chgperm"       , "chmod PERM FILENAME")
elseif has("win32") || has("win95") || has("win64") || has("win16")
 call s:NetrwInit("g:netrw_chgperm"       , "cacls FILENAME /e /p PERM")
else
 call s:NetrwInit("g:netrw_chgperm"       , "chmod PERM FILENAME")
endif
call s:NetrwInit("g:netrw_preview"       , 0)
call s:NetrwInit("g:netrw_scpport"       , "-P")
call s:NetrwInit("g:netrw_sshport"       , "-p")
call s:NetrwInit("g:netrw_rename_cmd"    , g:netrw_ssh_cmd." USEPORT HOSTNAME mv")
call s:NetrwInit("g:netrw_rm_cmd"        , g:netrw_ssh_cmd." USEPORT HOSTNAME rm")
call s:NetrwInit("g:netrw_rmdir_cmd"     , g:netrw_ssh_cmd." USEPORT HOSTNAME rmdir")
call s:NetrwInit("g:netrw_rmf_cmd"       , g:netrw_ssh_cmd." USEPORT HOSTNAME rm -f")
" Default values - s ---------- {{{3
" g:netrw_sepchr: picking a character that doesn't appear in filenames that can be used to separate priority from filename
call s:NetrwInit("g:netrw_sepchr"        , (&enc == "euc-jp")? "\<Char-0x01>" : "\<Char-0xff>")
call s:NetrwInit("s:netrw_silentxfer"    , (exists("g:netrw_silent") && g:netrw_silent != 0)? "sil keepj " : "keepj ")
call s:NetrwInit("g:netrw_sort_by"       , "name") " alternatives: date                                      , size
call s:NetrwInit("g:netrw_sort_options"  , "")
call s:NetrwInit("g:netrw_sort_direction", "normal") " alternative: reverse  (z y x ...)
if !exists("g:netrw_sort_sequence")
 if has("unix")
  let g:netrw_sort_sequence= '[\/]$,\<core\%(\.\d\+\)\=\>,\.h$,\.c$,\.cpp$,*,\.o$,\.obj$,\.info$,\.swp$,\.bak$,\~$'
 else
  let g:netrw_sort_sequence= '[\/]$,\.h$,\.c$,\.cpp$,*,\.o$,\.obj$,\.info$,\.swp$,\.bak$,\~$'
 endif
endif
call s:NetrwInit("g:netrw_special_syntax"   , 0)
call s:NetrwInit("g:netrw_ssh_browse_reject", '^total\s\+\d\+$')
call s:NetrwInit("g:netrw_use_noswf"        , 0)
" Default values - t-w ---------- {{{3
call s:NetrwInit("g:netrw_timefmt","%c")
call s:NetrwInit("g:netrw_xstrlen",0)
call s:NetrwInit("g:NetrwTopLvlMenu","Netrw.")
call s:NetrwInit("g:netrw_win95ftp",1)
call s:NetrwInit("g:netrw_winsize",25)
" ---------------------------------------------------------------------
" Default values for netrw's script variables: {{{2
call s:NetrwInit("g:netrw_fname_escape",' ?&;%')
if has("win32") || has("win95") || has("win64") || has("win16")
 call s:NetrwInit("g:netrw_glob_escape",'[]*?`{$')
else
 call s:NetrwInit("g:netrw_glob_escape",'[]*?`{~$\')
endif
call s:NetrwInit("g:netrw_menu_escape",'./&? \')
call s:NetrwInit("g:netrw_tmpfile_escape",' &;')
call s:NetrwInit("s:netrw_map_escape","<|\n\r\\\<C-V>\"")

" BufEnter event ignored by decho when following variable is true
"  Has a side effect that doau BufReadPost doesn't work, so
"  files read by network transfer aren't appropriately highlighted.
"let g:decho_bufenter = 1	"Decho

" ======================
"  Netrw Initialization: {{{1
" ======================
if v:version >= 700 && has("balloon_eval") && &beval == 0
 let &l:bexpr= "netrw#NetrwBalloonHelp()"
 set beval
endif

" ==============================
"  Netrw Utility Functions: {{{1
" ==============================

" ---------------------------------------------------------------------
" netrw#NetrwBalloonHelp: {{{2
if v:version >= 700 && has("balloon_eval") && &beval == 1
  fun! netrw#NetrwBalloonHelp()
    if !exists("w:netrw_bannercnt") || v:beval_lnum >= w:netrw_bannercnt
     let mesg= ""
    elseif     v:beval_text == "Netrw" || v:beval_text == "Directory" || v:beval_text == "Listing"
     let mesg = "i: thin-long-wide-tree  gh: quick hide/unhide of dot-files   qf: quick file info"
    elseif     getline(v:beval_lnum) =~ '^"\s*/'
     let mesg = "<cr>: edit/enter   o: edit/enter in horiz window   t: edit/enter in new tab   v:edit/enter in vert window"
    elseif     v:beval_text == "Sorted" || v:beval_text == "by"
     let mesg = 's: sort by name, time, or file size   r: reverse sorting order   mt: mark target'
    elseif v:beval_text == "Sort"   || v:beval_text == "sequence"
     let mesg = "S: edit sorting sequence"
    elseif v:beval_text == "Hiding" || v:beval_text == "Showing"
     let mesg = "a: hiding-showing-all   ctrl-h: editing hiding list   mh: hide/show by suffix"
    elseif v:beval_text == "Quick" || v:beval_text == "Help"
     let mesg = "Help: press <F1>"
    elseif v:beval_text == "Copy/Move" || v:beval_text == "Tgt"
     let mesg = "mt: mark target   mc: copy marked file to target   mm: move marked file to target"
    else
     let mesg= ""
    endif
    return mesg
  endfun
endif

" ------------------------------------------------------------------------
" s:NetrwOptionSave: save options and set to "standard" form {{{2
"  06/08/07 : removed call to NetrwSafeOptions(), either placed
"             immediately after NetrwOptionSave() calls in NetRead
"             and NetWrite, or after the s:NetrwEnew() call in
"             NetrwBrowse.
"             vt: normally its "w:" or "s:" (a variable type)
fun! s:NetrwOptionSave(vt)
"  call Dfunc("s:NetrwOptionSave(vt<".a:vt.">) win#".winnr()." buf#".bufnr("%")."<".bufname(bufnr("%")).">"." winnr($)=".winnr("$"))

"  call Decho(a:vt."netrw_optionsave".(exists("{a:vt}netrw_optionsave")? ("=".{a:vt}netrw_optionsave) : " doesn't exist"))
  if !exists("{a:vt}netrw_optionsave")
   let {a:vt}netrw_optionsave= 1
  else
"   call Dret("s:NetrwOptionSave : options already saved")
   return
  endif
"  call Decho("fo=".&fo.(exists("&acd")? " acd=".&acd : " acd doesn't exist")." diff=".&l:diff)

  " Save current settings and current directory
  let s:yykeep          = @@
  if exists("&l:acd")
   let {a:vt}netrw_acdkeep  = &l:acd
  endif
  let {a:vt}netrw_aikeep    = &l:ai
  let {a:vt}netrw_awkeep    = &l:aw
  let {a:vt}netrw_bombkeep  = &l:bomb
  let {a:vt}netrw_cikeep    = &l:ci
  let {a:vt}netrw_cinkeep   = &l:cin
  let {a:vt}netrw_cinokeep  = &l:cino
  let {a:vt}netrw_comkeep   = &l:com
  let {a:vt}netrw_cpokeep   = &l:cpo
  let {a:vt}netrw_diffkeep  = &l:diff
  if g:netrw_keepdir
   let {a:vt}netrw_dirkeep  = getcwd()
  endif
  let {a:vt}netrw_fokeep    = &l:fo           " formatoptions
  let {a:vt}netrw_gdkeep    = &l:gd           " gdefault
  let {a:vt}netrw_hidkeep   = &l:hidden
  let {a:vt}netrw_imkeep    = &l:im
  let {a:vt}netrw_magickeep = &l:magic
  let {a:vt}netrw_repkeep   = &l:report
  let {a:vt}netrw_selkeep   = &l:sel
  let {a:vt}netrw_spellkeep = &l:spell
  let {a:vt}netrw_twkeep    = &l:tw           " textwidth
  let {a:vt}netrw_wigkeep   = &l:wig          " wildignore
  if has("win32") && !has("win95")
   let {a:vt}netrw_swfkeep  = &l:swf          " swapfile
  endif
  if &go =~# 'a' | sil! let {a:vt}netrw_regstar = @* | endif
  sil! let {a:vt}netrw_regslash= @/

"  call Dret("s:NetrwOptionSave : win#".winnr()." buf#".bufnr("%"))
endfun

" ------------------------------------------------------------------------
" s:NetrwOptionRestore: restore options {{{2
fun! s:NetrwOptionRestore(vt)
"  call Dfunc("s:NetrwOptionRestore(vt<".a:vt.">) win#".winnr()." buf#".bufnr("%")." winnr($)=".winnr("$"))
  if !exists("{a:vt}netrw_optionsave")
"   call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"   call Dret("s:NetrwOptionRestore : ".a:vt."netrw_optionsave doesn't exist")
   return
  endif
  unlet {a:vt}netrw_optionsave

  if exists("&acd")
   if exists("{a:vt}netrw_acdkeep")
"    call Decho("g:netrw_keepdir=".g:netrw_keepdir.": getcwd<".getcwd()."> acd=".&acd)
    let curdir = getcwd()
    let &l:acd = {a:vt}netrw_acdkeep
    unlet {a:vt}netrw_acdkeep
    if &l:acd
"     call Decho("exe keepjumps lcd ".fnameescape(curdir))  " NOTE: was g:netrw_fname_escape for some reason
     try
      if !exists("&l:acd") && !&l:acd
       exe 'keepj lcd '.fnameescape(curdir)
      endif
     catch /^Vim\%((\a\+)\)\=:E472/
      call netrw#ErrorMsg(s:ERROR,"unable to change directory to <".curdir."> (permissions?)",61)
     endtry
    endif
   endif
  endif
  if exists("{a:vt}netrw_aikeep")   |let &l:ai     = {a:vt}netrw_aikeep      |unlet {a:vt}netrw_aikeep   |endif
  if exists("{a:vt}netrw_awkeep")   |let &l:aw     = {a:vt}netrw_awkeep      |unlet {a:vt}netrw_awkeep   |endif
  if exists("{a:vt}netrw_bombkeep") |let &l:bomb   = {a:vt}netrw_bombkeep    |unlet {a:vt}netrw_bombkeep |endif
  if exists("{a:vt}netrw_cikeep")   |let &l:ci     = {a:vt}netrw_cikeep      |unlet {a:vt}netrw_cikeep   |endif
  if exists("{a:vt}netrw_cinkeep")  |let &l:cin    = {a:vt}netrw_cinkeep     |unlet {a:vt}netrw_cinkeep  |endif
  if exists("{a:vt}netrw_cinokeep") |let &l:cino   = {a:vt}netrw_cinokeep    |unlet {a:vt}netrw_cinokeep |endif
  if exists("{a:vt}netrw_comkeep")  |let &l:com    = {a:vt}netrw_comkeep     |unlet {a:vt}netrw_comkeep  |endif
  if exists("{a:vt}netrw_cpokeep")  |let &l:cpo    = {a:vt}netrw_cpokeep     |unlet {a:vt}netrw_cpokeep  |endif
  if exists("{a:vt}netrw_diffkeep") |let &l:diff   = {a:vt}netrw_diffkeep    |unlet {a:vt}netrw_diffkeep |endif
  if exists("{a:vt}netrw_dirkeep") && isdirectory({a:vt}netrw_dirkeep) && g:netrw_keepdir
   let dirkeep = substitute({a:vt}netrw_dirkeep,'\\','/','g')
   if exists("{a:vt}netrw_dirkeep")  |exe "keepjumps lcd ".fnameescape(dirkeep)|unlet {a:vt}netrw_dirkeep  |endif
  endif
  if exists("{a:vt}netrw_fokeep")   |let &l:fo     = {a:vt}netrw_fokeep      |unlet {a:vt}netrw_fokeep   |endif
  if exists("{a:vt}netrw_gdkeep")   |let &l:gd     = {a:vt}netrw_gdkeep      |unlet {a:vt}netrw_gdkeep   |endif
  if exists("{a:vt}netrw_hidkeep")  |let &l:hidden = {a:vt}netrw_hidkeep     |unlet {a:vt}netrw_hidkeep  |endif
  if exists("{a:vt}netrw_imkeep")   |let &l:im     = {a:vt}netrw_imkeep      |unlet {a:vt}netrw_imkeep   |endif
  if exists("{a:vt}netrw_magic")    |let &l:magic  = {a:vt}netrw_magic       |unlet {a:vt}netrw_magic    |endif
  if exists("{a:vt}netrw_repkeep")  |let &l:report = {a:vt}netrw_repkeep     |unlet {a:vt}netrw_repkeep  |endif
  if exists("{a:vt}netrw_selkeep")  |let &l:sel    = {a:vt}netrw_selkeep     |unlet {a:vt}netrw_selkeep  |endif
  if exists("{a:vt}netrw_spellkeep")|let &l:spell  = {a:vt}netrw_spellkeep   |unlet {a:vt}netrw_spellkeep|endif
  if exists("{a:vt}netrw_twkeep")   |let &l:tw     = {a:vt}netrw_twkeep      |unlet {a:vt}netrw_twkeep   |endif
  if exists("{a:vt}netrw_wigkeep")  |let &l:wig    = {a:vt}netrw_wigkeep     |unlet {a:vt}netrw_wigkeep  |endif
  if exists("s:yykeep")             |let  @@       = s:yykeep                |unlet s:yykeep             |endif
  if exists("{a:vt}netrw_swfkeep")
   if &directory == ""
    " user hasn't specified a swapfile directory;
    " netrw will temporarily set the swapfile directory
    " to the current directory as returned by getcwd().
    let &l:directory   = getcwd()
    sil! let &l:swf = {a:vt}netrw_swfkeep
    setlocal directory=
    unlet {a:vt}netrw_swfkeep
   elseif &l:swf != {a:vt}netrw_swfkeep
    " following line causes a Press ENTER in windows -- can't seem to work around it!!!
    sil! let &l:swf= {a:vt}netrw_swfkeep
    unlet {a:vt}netrw_swfkeep
   endif
  endif
  if exists("{a:vt}netrw_regstar") |sil! let @*= {a:vt}netrw_regstar |unlet {a:vt}netrw_regstar |endif
  if exists("{a:vt}netrw_regslash")|sil! let @/= {a:vt}netrw_regslash|unlet {a:vt}netrw_regslash|endif

"  call Decho("g:netrw_keepdir=".g:netrw_keepdir.": getcwd<".getcwd()."> acd=".&acd)
"  call Decho("fo=".&fo.(exists("&acd")? " acd=".&acd : " acd doesn't exist"))
"  call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"  call Decho("diff=".&l:diff." win#".winnr()." w:netrw_diffkeep=".(exists("w:netrw_diffkeep")? w:netrw_diffkeep : "doesn't exist"))
"  call Dret("s:NetrwOptionRestore : win#".winnr()." buf#".bufnr("%"))
endfun

" ---------------------------------------------------------------------
" s:NetrwSafeOptions: sets options to help netrw do its job {{{2
fun! s:NetrwSafeOptions()
"  call Dfunc("s:NetrwSafeOptions() win#".winnr()." buf#".bufnr("%")."<".bufname(bufnr("%"))."> winnr($)=".winnr("$"))
"  call Decho("win#".winnr()."'s ft=".&ft)
  setlocal cino=
  setlocal com=
  setlocal cpo-=aA
  if exists("&acd") | setlocal noacd | endif
  setlocal nocin noai nobomb noci magic nospell nohid wig= noaw noim
  setlocal fo=nroql2
  setlocal tw=0
  setlocal report=10000
  setlocal isk+=@ isk+=* isk+=/
  setlocal sel=inclusive
  if g:netrw_use_noswf && has("win32") && !has("win95")
   setlocal noswf
  endif
  call s:NetrwCursor()

  " allow the user to override safe options
"  call Decho("ft<".&ft."> ei=".&ei)
  if &ft == "netrw"
"   call Decho("do any netrw FileType autocmds")
   sil keepalt keepj doau FileType netrw
  endif

"  call Decho("fo=".&fo.(exists("&acd")? " acd=".&acd : " acd doesn't exist"))
"  call Dret("s:NetrwSafeOptions")
endfun

" ---------------------------------------------------------------------
" netrw#NetrwClean: remove netrw {{{2
" supports :NetrwClean  -- remove netrw from first directory on runtimepath
"          :NetrwClean! -- remove netrw from all directories on runtimepath
fun! netrw#NetrwClean(sys)
"  call Dfunc("netrw#NetrwClean(sys=".a:sys.")")

  if a:sys
   let choice= confirm("Remove personal and system copies of netrw?","&Yes\n&No")
  else
   let choice= confirm("Remove personal copy of netrw?","&Yes\n&No")
  endif
"  call Decho("choice=".choice)
  let diddel= 0
  let diddir= ""

  if choice == 1
   for dir in split(&rtp,',')
    if filereadable(dir."/plugin/netrwPlugin.vim")
"     call Decho("removing netrw-related files from ".dir)
     if s:NetrwDelete(dir."/plugin/netrwPlugin.vim")        |call netrw#ErrorMsg(1,"unable to remove ".dir."/plugin/netrwPlugin.vim",55)        |endif
     if s:NetrwDelete(dir."/autoload/netrwFileHandlers.vim")|call netrw#ErrorMsg(1,"unable to remove ".dir."/autoload/netrwFileHandlers.vim",55)|endif
     if s:NetrwDelete(dir."/autoload/netrwSettings.vim")    |call netrw#ErrorMsg(1,"unable to remove ".dir."/autoload/netrwSettings.vim",55)    |endif
     if s:NetrwDelete(dir."/autoload/netrw.vim")            |call netrw#ErrorMsg(1,"unable to remove ".dir."/autoload/netrw.vim",55)            |endif
     if s:NetrwDelete(dir."/syntax/netrw.vim")              |call netrw#ErrorMsg(1,"unable to remove ".dir."/syntax/netrw.vim",55)              |endif
     if s:NetrwDelete(dir."/syntax/netrwlist.vim")          |call netrw#ErrorMsg(1,"unable to remove ".dir."/syntax/netrwlist.vim",55)          |endif
     let diddir= dir
     let diddel= diddel + 1
     if !a:sys|break|endif
    endif
   endfor
  endif

   echohl WarningMsg
  if diddel == 0
   echomsg "netrw is either not installed or not removable"
  elseif diddel == 1
   echomsg "removed one copy of netrw from <".diddir.">"
  else
   echomsg "removed ".diddel." copies of netrw"
  endif
   echohl None

"  call Dret("netrw#NetrwClean")
endfun

" ---------------------------------------------------------------------
" netrw#Nread: {{{2
fun! netrw#Nread(mode,fname)
"  call Dfunc("netrw#Nread(mode=".a:mode." fname<".a:fname.">)")
  call netrw#NetrwSavePosn()
  call netrw#NetRead(a:mode,a:fname)
  call netrw#NetrwRestorePosn()
"  call Dret("netrw#Nread")
endfun

" ------------------------------------------------------------------------
"  Netrw Transfer Functions: {{{1
" ===============================

" ------------------------------------------------------------------------
" netrw#NetRead: responsible for reading a file over the net {{{2
"   mode: =0 read remote file and insert before current line
"         =1 read remote file and insert after current line
"         =2 replace with remote file
"         =3 obtain file, but leave in temporary format
fun! netrw#NetRead(mode,...)
"  call Dfunc("netrw#NetRead(mode=".a:mode.",...) a:0=".a:0." ".g:loaded_netrw)

  " NetRead: save options {{{3
  call s:NetrwOptionSave("w:")
  call s:NetrwSafeOptions()
  call s:RestoreCursorline()

  " NetRead: interpret mode into a readcmd {{{3
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

  " NetRead: get temporary filename {{{3
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
      let choice= strpart(choice,1,strlen(choice)-2)
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

   " NetRead: Determine method of read (ftp, rcp, etc) {{{3
   call s:NetrwMethod(choice)
   if !exists("b:netrw_method") || b:netrw_method < 0
"    call Dfunc("netrw#NetRead : unsupported method")
    return
   endif
   let tmpfile= s:GetTempfile(b:netrw_fname) " apply correct suffix

   " Check if NetrwBrowse() should be handling this request
"   call Decho("checking if NetrwBrowse() should handle choice<".choice."> with netrw_list_cmd<".g:netrw_list_cmd.">")
   if choice =~ "^.*[\/]$" && b:netrw_method != 5 && choice !~ '^http://'
"    call Decho("yes, choice matches '^.*[\/]$'")
    keepj call s:NetrwBrowse(0,choice)
"    call Dret("netrw#NetRead :3 getcwd<".getcwd().">")
    return
   endif

   " ============
   " NetRead: Perform Protocol-Based Read {{{3
   " ===========================
   if exists("g:netrw_silent") && g:netrw_silent == 0 && &ch >= 1
    echo "(netrw) Processing your read request..."
   endif

   ".........................................
   " NetRead: (rcp)  NetRead Method #1 {{{3
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
"   call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".shellescape(uid_machine.":".b:netrw_fname,1)." ".shellescape(tmpfile,1))
   exe s:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".shellescape(uid_machine.":".b:netrw_fname,1)." ".shellescape(tmpfile,1)
   let result           = s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)
   let b:netrw_lastfile = choice

   ".........................................
   " NetRead: (ftp + <.netrc>)  NetRead Method #2 {{{3
   elseif b:netrw_method  == 2		" read with ftp + <.netrc>
"     call Decho("read via ftp+.netrc (method #2)")
     let netrw_fname= b:netrw_fname
     keepj call s:SaveBufVars()|new|keepj call s:RestoreBufVars()
     let filtbuf= bufnr("%")
     setlocal ff=unix
     keepj put =g:netrw_ftpmode
"     call Decho("filter input: ".getline(line("$")))
     if exists("g:netrw_ftpextracmd")
      keepj put =g:netrw_ftpextracmd
"      call Decho("filter input: ".getline(line("$")))
     endif
     call setline(line("$")+1,'get "'.netrw_fname.'" '.tmpfile)
"     call Decho("filter input: ".getline(line("$")))
     if exists("g:netrw_port") && g:netrw_port != ""
"      call Decho("executing: %!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1))
      exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1)
     else
"      call Decho("executing: %!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1))
      exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)
     endif
     " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
     if getline(1) !~ "^$" && !exists("g:netrw_quiet") && getline(1) !~ '^Trying '
      let debugkeep = &debug
      setlocal debug=msg
      keepj call netrw#ErrorMsg(s:ERROR,getline(1),4)
      let &debug    = debugkeep
     endif
     call s:SaveBufVars()
     bd!
     if bufname("%") == "" && getline("$") == "" && line('$') == 1
      " needed when one sources a file in a nolbl setting window via ftp
      q!
     endif
     call s:RestoreBufVars()
     let result           = s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)
     let b:netrw_lastfile = choice

   ".........................................
   " NetRead: (ftp + machine,id,passwd,filename)  NetRead Method #3 {{{3
   elseif b:netrw_method == 3		" read with ftp + machine, id, passwd, and fname
    " Construct execution string (four lines) which will be passed through filter
"    call Decho("read via ftp+mipf (method #3)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    keepj call s:SaveBufVars()|new|keepj call s:RestoreBufVars()
    let filtbuf= bufnr("%")
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     keepj put ='open '.g:netrw_machine.' '.g:netrw_port
"     call Decho("filter input: ".getline('.'))
    else
     keepj put ='open '.g:netrw_machine
"     call Decho("filter input: ".getline('.'))
    endif

    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     keepj put =g:netrw_uid
"     call Decho("filter input: ".getline('.'))
     keepj put ='\"'.s:netrw_passwd.'\"'
"     call Decho("filter input: ".getline('.'))
    else
     keepj put ='user \"'.g:netrw_uid.'\" \"'.s:netrw_passwd.'\"'
"     call Decho("filter input: ".getline('.'))
    endif

    if exists("g:netrw_ftpmode") && g:netrw_ftpmode != ""
     keepj put =g:netrw_ftpmode
"     call Decho("filter input: ".getline('.'))
    endif
    if exists("g:netrw_ftpextracmd")
     keepj put =g:netrw_ftpextracmd
"     call Decho("filter input: ".getline('.'))
    endif
    keepj put ='get \"'.netrw_fname.'\" '.tmpfile
"    call Decho("filter input: ".getline('.'))

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
    keepj norm! 1Gdd
"    call Decho("executing: %!".s:netrw_ftp_cmd." -i -n")
    exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
"     call Decho("error<".getline(1).">")
     if !exists("g:netrw_quiet")
      call netrw#ErrorMsg(s:ERROR,getline(1),5)
     endif
    endif
    call s:SaveBufVars()|bd!|call s:RestoreBufVars()
    let result           = s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " NetRead: (scp) NetRead Method #4 {{{3
   elseif     b:netrw_method  == 4	" read with scp
"    call Decho("read via scp (method #4)")
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " ".g:netrw_scpport." ".g:netrw_port
    else
     let useport= ""
    endif
"    call Decho("exe ".s:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)." ".shellescape(tmpfile,1))
    exe s:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)." ".shellescape(tmpfile,1)
    let result           = s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " NetRead: (http) NetRead Method #5 (wget) {{{3
   elseif     b:netrw_method  == 5
"    call Decho("read via http (method #5)")
    if g:netrw_http_cmd == ""
     if !exists("g:netrw_quiet")
      call netrw#ErrorMsg(s:ERROR,"neither the wget nor the fetch command is available",6)
     endif
"     call Dret("netrw#NetRead :4 getcwd<".getcwd().">")
     return
    endif

    if match(b:netrw_fname,"#") == -1 || exists("g:netrw_http_xcmd")
     " using g:netrw_http_cmd (usually elinks, links, curl, wget, or fetch)
"     call Decho('using '.g:netrw_http_cmd.' (# not in b:netrw_fname<'.b:netrw_fname.">)")
     if exists("g:netrw_http_xcmd")
"      call Decho("exe ".s:netrw_silentxfer."!".g:netrw_http_cmd." ".shellescape("http://".g:netrw_machine.b:netrw_fname,1)." ".g:netrw_http_xcmd." ".shellescape(tmpfile,1))
      exe s:netrw_silentxfer."!".g:netrw_http_cmd." ".shellescape("http://".g:netrw_machine.b:netrw_fname,1)." ".g:netrw_http_xcmd." ".shellescape(tmpfile,1)
     else
"      call Decho("exe ".s:netrw_silentxfer."!".g:netrw_http_cmd." ".shellescape(tmpfile,1)." ".shellescape("http://".g:netrw_machine.b:netrw_fname,1))
      exe s:netrw_silentxfer."!".g:netrw_http_cmd." ".shellescape(tmpfile,1)." ".shellescape("http://".g:netrw_machine.b:netrw_fname,1)
     endif
     let result = s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)

    else
     " wget/curl/fetch plus a jump to an in-page marker (ie. http://abc/def.html#aMarker)
"     call Decho("wget/curl plus jump (# in b:netrw_fname<".b:netrw_fname.">)")
     let netrw_html= substitute(b:netrw_fname,"#.*$","","")
     let netrw_tag = substitute(b:netrw_fname,"^.*#","","")
"     call Decho("netrw_html<".netrw_html.">")
"     call Decho("netrw_tag <".netrw_tag.">")
"     call Decho("exe ".s:netrw_silentxfer."!".g:netrw_http_cmd." ".shellescape(tmpfile,1)." ".shellescape("http://".g:netrw_machine.netrw_html,1))
     exe s:netrw_silentxfer."!".g:netrw_http_cmd." ".shellescape(tmpfile,1)." ".shellescape("http://".g:netrw_machine.netrw_html,1)
     let result = s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)
"     call Decho('<\s*a\s*name=\s*"'.netrw_tag.'"/')
     exe 'keepj norm! 1G/<\s*a\s*name=\s*"'.netrw_tag.'"/'."\<CR>"
    endif
    let b:netrw_lastfile = choice
    setlocal ro

   ".........................................
   " NetRead: (dav) NetRead Method #6 {{{3
   elseif     b:netrw_method  == 6
"    call Decho("read via cadaver (method #6)")

    if !executable(g:netrw_dav_cmd)
     call netrw#ErrorMsg(s:ERROR,g:netrw_dav_cmd." is not executable",73)
"     call Dret("netrw#NetRead : ".g:netrw_dav_cmd." not executable")
     return
    endif
    if g:netrw_dav_cmd =~ "curl"
"     call Decho("exe ".s:netrw_silentxfer."!".g:netrw_dav_cmd." ".shellescape("dav://".g:netrw_machine.b:netrw_fname,1)." ".shellescape(tmpfile,1))
     exe s:netrw_silentxfer."!".g:netrw_dav_cmd." ".shellescape("dav://".g:netrw_machine.b:netrw_fname,1)." ".shellescape(tmpfile,1)
    else
     " Construct execution string (four lines) which will be passed through filter
     let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
     new
     setlocal ff=unix
     if exists("g:netrw_port") && g:netrw_port != ""
      keepj put ='open '.g:netrw_machine.' '.g:netrw_port
     else
      keepj put ='open '.g:netrw_machine
     endif
     keepj put ='user '.g:netrw_uid.' '.s:netrw_passwd
     keepj put ='get '.netrw_fname.' '.tmpfile
     keepj put ='quit'

     " perform cadaver operation:
     keepj norm! 1Gdd
"    call Decho("executing: %!".g:netrw_dav_cmd)
     exe s:netrw_silentxfer."%!".g:netrw_dav_cmd
     bd!
    endif
    let result           = s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " NetRead: (rsync) NetRead Method #7 {{{3
   elseif     b:netrw_method  == 7
"    call Decho("read via rsync (method #7)")
"    call Decho("exe ".s:netrw_silentxfer."!".g:netrw_rsync_cmd." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)." ".shellescape(tmpfile,1))
    exe s:netrw_silentxfer."!".g:netrw_rsync_cmd." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)." ".shellescape(tmpfile,1)
    let result		 = s:NetrwGetFile(readcmd,tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " NetRead: (fetch) NetRead Method #8 {{{3
   "    fetch://[user@]host[:http]/path
   elseif     b:netrw_method  == 8
"    call Decho("read via fetch (method #8)")
    if g:netrw_fetch_cmd == ""
     if !exists("g:netrw_quiet")
      keepj call netrw#ErrorMsg(s:ERROR,"fetch command not available",7)
     endif
"     call Dret("NetRead")
    endif
    if exists("g:netrw_option") && g:netrw_option == ":http"
     let netrw_option= "http"
    else
     let netrw_option= "ftp"
    endif
"    call Decho("read via fetch for ".netrw_option)

    if exists("g:netrw_uid") && g:netrw_uid != "" && exists("s:netrw_passwd") && s:netrw_passwd != ""
"     call Decho("exe ".s:netrw_silentxfer."!".g:netrw_fetch_cmd." ".shellescape(tmpfile,1)." ".shellescape(netrw_option."://".g:netrw_uid.':'.s:netrw_passwd.'@'.g:netrw_machine."/".b:netrw_fname,1))
     exe s:netrw_silentxfer."!".g:netrw_fetch_cmd." ".shellescape(tmpfile,1)." ".shellescape(netrw_option."://".g:netrw_uid.':'.s:netrw_passwd.'@'.g:netrw_machine."/".b:netrw_fname,1)
    else
"     call Decho("exe ".s:netrw_silentxfer."!".g:netrw_fetch_cmd." ".shellescape(tmpfile,1)." ".shellescape(netrw_option."://".g:netrw_machine."/".b:netrw_fname,1))
     exe s:netrw_silentxfer."!".g:netrw_fetch_cmd." ".shellescape(tmpfile,1)." ".shellescape(netrw_option."://".g:netrw_machine."/".b:netrw_fname,1)
    endif

    let result		= s:NetrwGetFile(readcmd,tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice
    setlocal ro

   ".........................................
   " NetRead: (sftp) NetRead Method #9 {{{3
   elseif     b:netrw_method  == 9
"    call Decho("read via sftp (method #9)")
"    call Decho("exe ".s:netrw_silentxfer."!".g:netrw_sftp_cmd." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)." ".tmpfile)
    exe s:netrw_silentxfer."!".g:netrw_sftp_cmd." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)." ".tmpfile
    let result		= s:NetrwGetFile(readcmd, tmpfile, b:netrw_method)
    let b:netrw_lastfile = choice

   ".........................................
   " NetRead: Complain {{{3
   else
    call netrw#ErrorMsg(s:WARNING,"unable to comply with your request<" . choice . ">",8)
   endif
  endwhile

  " NetRead: cleanup {{{3
  if exists("b:netrw_method")
"   call Decho("cleanup b:netrw_method and b:netrw_fname")
   unlet b:netrw_method
   unlet b:netrw_fname
  endif
  if s:FileReadable(tmpfile) && tmpfile !~ '.tar.bz2$' && tmpfile !~ '.tar.gz$' && tmpfile !~ '.zip' && tmpfile !~ '.tar' && readcmd != 't' && tmpfile !~ '.tar.xz$' && tmpfile !~ '.txz'
"   call Decho("cleanup by deleting tmpfile<".tmpfile.">")
   keepj call s:NetrwDelete(tmpfile)
  endif
  keepj call s:NetrwOptionRestore("w:")

"  call Dret("netrw#NetRead :5 getcwd<".getcwd().">")
endfun

" ------------------------------------------------------------------------
" netrw#NetWrite: responsible for writing a file over the net {{{2
fun! netrw#NetWrite(...) range
"  call Dfunc("netrw#NetWrite(a:0=".a:0.") ".g:loaded_netrw)

  " NetWrite: option handling {{{3
  let mod= 0
  call s:NetrwOptionSave("w:")
  call s:NetrwSafeOptions()

  " NetWrite: Get Temporary Filename {{{3
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
"   call Decho("(write entire file) sil exe w! ".fnameescape(v:cmdarg)." ".fnameescape(tmpfile))
   exe "sil keepj w! ".fnameescape(v:cmdarg)." ".fnameescape(tmpfile)
  elseif g:netrw_cygwin
   " write (selected portion of) file to temporary
   let cygtmpfile= substitute(tmpfile,'/cygdrive/\(.\)','\1:','')
"   call Decho("(write selected portion) sil exe ".a:firstline."," . a:lastline . "w! ".fnameescape(v:cmdarg)." ".fnameescape(cygtmpfile))
   exe "sil keepj ".a:firstline."," . a:lastline . "w! ".fnameescape(v:cmdarg)." ".fnameescape(cygtmpfile)
  else
   " write (selected portion of) file to temporary
"   call Decho("(write selected portion) sil exe ".a:firstline."," . a:lastline . "w! ".fnameescape(v:cmdarg)." ".fnameescape(tmpfile))
   exe "sil keepj ".a:firstline."," . a:lastline . "w! ".fnameescape(v:cmdarg)." ".fnameescape(tmpfile)
  endif

  if curbufname == ""
   " if the file is [No Name], and one attempts to Nwrite it, the buffer takes
   " on the temporary file's name.  Deletion of the temporary file during
   " cleanup then causes an error message.
   0file!
  endif

  " NetWrite: while choice loop: {{{3
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
   keepj call s:NetrwMethod(choice)
   if !exists("b:netrw_method") || b:netrw_method < 0
"    call Dfunc("netrw#NetWrite : unsupported method")
    return
   endif

   " =============
   " NetWrite: Perform Protocol-Based Write {{{3
   " ============================
   if exists("g:netrw_silent") && g:netrw_silent == 0 && &ch >= 1
    echo "(netrw) Processing your write request..."
"    call Decho("(netrw) Processing your write request...")
   endif

   ".........................................
   " NetWrite: (rcp) NetWrite Method #1 {{{3
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
"    call Decho("executing: !".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".shellescape(tmpfile,1)." ".shellescape(uid_machine.":".b:netrw_fname,1))
    exe s:netrw_silentxfer."!".g:netrw_rcp_cmd." ".s:netrw_rcpmode." ".shellescape(tmpfile,1)." ".shellescape(uid_machine.":".b:netrw_fname,1)
    let b:netrw_lastfile = choice

   ".........................................
   " NetWrite: (ftp + <.netrc>) NetWrite Method #2 {{{3
   elseif b:netrw_method == 2
"    call Decho("write via ftp+.netrc (method #2)")
    let netrw_fname = b:netrw_fname

    " formerly just a "new...bd!", that changed the window sizes when equalalways.  Using enew workaround instead
    let bhkeep      = &l:bh
    let curbuf      = bufnr("%")
    setlocal bh=hide
    enew

"    call Decho("filter input window#".winnr())
    setlocal ff=unix
    keepj put =g:netrw_ftpmode
"    call Decho("filter input: ".getline('$'))
    if exists("g:netrw_ftpextracmd")
     keepj put =g:netrw_ftpextracmd
"     call Decho("filter input: ".getline("$"))
    endif
    keepj call setline(line("$")+1,'put "'.tmpfile.'" "'.netrw_fname.'"')
"    call Decho("filter input: ".getline("$"))
    if exists("g:netrw_port") && g:netrw_port != ""
"     call Decho("executing: %!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1))
     exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1)
    else
"     call Decho("filter input window#".winnr())
"     call Decho("executing: %!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1))
     exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)
    endif
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
     if !exists("g:netrw_quiet")
      keepj call netrw#ErrorMsg(s:ERROR,getline(1),14)
     endif
     let mod=1
    endif

    " remove enew buffer (quietly)
    let filtbuf= bufnr("%")
    exe curbuf."b!"
    let &l:bh            = bhkeep
    exe filtbuf."bw!"

    let b:netrw_lastfile = choice

   ".........................................
   " NetWrite: (ftp + machine, id, passwd, filename) NetWrite Method #3 {{{3
   elseif b:netrw_method == 3
    " Construct execution string (three or more lines) which will be passed through filter
"    call Decho("read via ftp+mipf (method #3)")
    let netrw_fname = b:netrw_fname
    let bhkeep      = &l:bh

    " formerly just a "new...bd!", that changed the window sizes when equalalways.  Using enew workaround instead
    let curbuf      = bufnr("%")
    setlocal bh=hide
    enew
    setlocal ff=unix

    if exists("g:netrw_port") && g:netrw_port != ""
     keepj put ='open '.g:netrw_machine.' '.g:netrw_port
"     call Decho("filter input: ".getline('.'))
    else
     keepj put ='open '.g:netrw_machine
"     call Decho("filter input: ".getline('.'))
    endif
    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     keepj put =g:netrw_uid
"     call Decho("filter input: ".getline('.'))
     keepj put ='\"'.s:netrw_passwd.'\"'
"     call Decho("filter input: ".getline('.'))
    else
     keepj put ='user \"'.g:netrw_uid.'\" \"'.s:netrw_passwd.'\"'
"     call Decho("filter input: ".getline('.'))
    endif
    keepj put =g:netrw_ftpmode
"    call Decho("filter input: ".getline('$'))
    if exists("g:netrw_ftpextracmd")
     keepj put =g:netrw_ftpextracmd
"     call Decho("filter input: ".getline("$"))
    endif
    keepj put ='put \"'.tmpfile.'\" \"'.netrw_fname.'\"'
"    call Decho("filter input: ".getline('.'))
    " save choice/id/password for future use
    let b:netrw_lastfile = choice

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
    keepj norm! 1Gdd
"    call Decho("executing: %!".s:netrw_ftp_cmd." -i -n")
    exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
     if  !exists("g:netrw_quiet")
      call netrw#ErrorMsg(s:ERROR,getline(1),15)
     endif
     let mod=1
    endif

    " remove enew buffer (quietly)
    let filtbuf= bufnr("%")
    exe curbuf."b!"
    let &l:bh= bhkeep
    exe filtbuf."bw!"

   ".........................................
   " NetWrite: (scp) NetWrite Method #4 {{{3
   elseif     b:netrw_method == 4
"    call Decho("write via scp (method #4)")
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " ".g:netrw_scpport." ".fnameescape(g:netrw_port)
    else
     let useport= ""
    endif
"    call Decho("exe ".s:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".shellescape(tmpfile,1)." ".shellescape(g:netrw_machine.":".b:netrw_fname,1))
    exe s:netrw_silentxfer."!".g:netrw_scp_cmd.useport." ".shellescape(tmpfile,1)." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)
    let b:netrw_lastfile = choice

   ".........................................
   " NetWrite: (http) NetWrite Method #5 {{{3
   elseif     b:netrw_method == 5
"    call Decho("write via http (method #5)")
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:ERROR,"currently <netrw.vim> does not support writing using http:",16)
    endif

   ".........................................
   " NetWrite: (dav) NetWrite Method #6 (cadaver) {{{3
   elseif     b:netrw_method == 6
"    call Decho("write via cadaver (method #6)")

    " Construct execution string (four lines) which will be passed through filter
    let netrw_fname = escape(b:netrw_fname,g:netrw_fname_escape)
    let bhkeep      = &l:bh

    " formerly just a "new...bd!", that changed the window sizes when equalalways.  Using enew workaround instead
    let curbuf      = bufnr("%")
    setlocal bh=hide
    enew

    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     keepj put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     keepj put ='open '.g:netrw_machine
    endif
    if exists("g:netrw_uid") && exists("s:netrw_passwd")
     keepj put ='user '.g:netrw_uid.' '.s:netrw_passwd
    endif
    keepj put ='put '.tmpfile.' '.netrw_fname

    " perform cadaver operation:
    keepj norm! 1Gdd
"    call Decho("executing: %!".g:netrw_dav_cmd)
    exe s:netrw_silentxfer."%!".g:netrw_dav_cmd

    " remove enew buffer (quietly)
    let filtbuf= bufnr("%")
    exe curbuf."b!"
    let &l:bh            = bhkeep
    exe filtbuf."bw!"

    let b:netrw_lastfile = choice

   ".........................................
   " NetWrite: (rsync) NetWrite Method #7 {{{3
   elseif     b:netrw_method == 7
"    call Decho("write via rsync (method #7)")
"    call Decho("executing: !".g:netrw_rsync_cmd." ".shellescape(tmpfile,1)." ".shellescape(g:netrw_machine.":".b:netrw_fname,1))
    exe s:netrw_silentxfer."!".g:netrw_rsync_cmd." ".shellescape(tmpfile,1)." ".shellescape(g:netrw_machine.":".b:netrw_fname,1)
    let b:netrw_lastfile = choice

   ".........................................
   " NetWrite: (sftp) NetWrite Method #9 {{{3
   elseif     b:netrw_method == 9
"    call Decho("read via sftp (method #9)")
    let netrw_fname= escape(b:netrw_fname,g:netrw_fname_escape)
    if exists("g:netrw_uid") &&  ( g:netrw_uid != "" )
     let uid_machine = g:netrw_uid .'@'. g:netrw_machine
    else
     let uid_machine = g:netrw_machine
    endif

    " formerly just a "new...bd!", that changed the window sizes when equalalways.  Using enew workaround instead
    let bhkeep = &l:bh
    let curbuf = bufnr("%")
    setlocal bh=hide
    enew

    setlocal ff=unix
    call setline(1,'put "'.escape(tmpfile,'\').'" '.netrw_fname)
"    call Decho("filter input: ".getline('.'))
"    call Decho("executing: %!".g:netrw_sftp_cmd.' '.shellescape(uid_machine,1))
    exe s:netrw_silentxfer."%!".g:netrw_sftp_cmd.' '.shellescape(uid_machine,1)
    let filtbuf= bufnr("%")
    exe curbuf."b!"
    let &l:bh            = bhkeep
    exe filtbuf."bw!"
    let b:netrw_lastfile = choice

   ".........................................
   " NetWrite: Complain {{{3
   else
    call netrw#ErrorMsg(s:WARNING,"unable to comply with your request<" . choice . ">",17)
    let leavemod= 1
   endif
  endwhile

  " NetWrite: Cleanup: {{{3
"  call Decho("cleanup")
  if s:FileReadable(tmpfile)
"   call Decho("tmpfile<".tmpfile."> readable, will now delete it")
   call s:NetrwDelete(tmpfile)
  endif
  call s:NetrwOptionRestore("w:")

  if a:firstline == 1 && a:lastline == line("$")
   " restore modifiability; usually equivalent to set nomod
   let &mod= mod
  elseif !exists("leavemod")
   " indicate that the buffer has not been modified since last written
   set nomod
  endif

"  call Dret("netrw#NetWrite")
endfun

" ---------------------------------------------------------------------
" netrw#NetSource: source a remotely hosted vim script {{{2
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
"    call Decho("(netrw#NetSource) s:netread_tmpfile<".s:netrw_tmpfile.">")
    if s:FileReadable(s:netrw_tmpfile)
"     call Decho("(netrw#NetSource) exe so ".fnameescape(s:netrw_tmpfile))
     exe "so ".fnameescape(s:netrw_tmpfile)
"     call Decho("(netrw#NetSource) delete(".s:netrw_tmpfile.")")
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
" s:NetrwGetFile: Function to read temporary file "tfile" with command "readcmd". {{{2
"    readcmd == %r : replace buffer with newly read file
"            == 0r : read file at top of buffer
"            == r  : read file after current line
"            == t  : leave file in temporary form (ie. don't read into buffer)
fun! s:NetrwGetFile(readcmd, tfile, method)
"  call Dfunc("NetrwGetFile(readcmd<".a:readcmd.">,tfile<".a:tfile."> method<".a:method.">)")

  " readcmd=='t': simply do nothing
  if a:readcmd == 't'
"   call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"   call Dret("NetrwGetFile : skip read of <".a:tfile.">")
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
"   call Decho("exe sil! keepalt file ".fnameescape(tfile))
   exe "sil! keepalt file ".fnameescape(tfile)

   " edit temporary file (ie. read the temporary file in)
   if     rfile =~ '\.zip$'
"    call Decho("handling remote zip file with zip#Browse(tfile<".tfile.">)")
    call zip#Browse(tfile)
   elseif rfile =~ '\.tar$'
"    call Decho("handling remote tar file with tar#Browse(tfile<".tfile.">)")
    call tar#Browse(tfile)
   elseif rfile =~ '\.tar\.gz$'
"    call Decho("handling remote gzip-compressed tar file")
    call tar#Browse(tfile)
   elseif rfile =~ '\.tar\.bz2$'
"    call Decho("handling remote bz2-compressed tar file")
    call tar#Browse(tfile)
   elseif rfile =~ '\.tar\.xz$'
"    call Decho("handling remote xz-compressed tar file")
    call tar#Browse(tfile)
   elseif rfile =~ '\.txz$'
"    call Decho("handling remote xz-compressed tar file (.txz)")
    call tar#Browse(tfile)
   else
"    call Decho("edit temporary file")
    e!
   endif

   " rename buffer back to remote filename
"   call Decho("exe sil! keepalt file ".fnameescape(rfile))
   exe "sil! keepj keepalt file ".fnameescape(rfile)
   filetype detect
"   call Dredir("renamed buffer back to remote filename<".rfile."> : expand(%)<".expand("%").">","ls!")
   let line1 = 1
   let line2 = line("$")

  elseif s:FileReadable(a:tfile)
   " read file after current line
"   call Decho("read file<".a:tfile."> after current line")
   let curline = line(".")
   let lastline= line("$")
"   call Decho("exe<".a:readcmd." ".fnameescape(v:cmdarg)." ".fnameescape(a:tfile).">  line#".curline)
   exe "keepj ".a:readcmd." ".fnameescape(v:cmdarg)." ".fnameescape(a:tfile)
   let line1= curline + 1
   let line2= line("$") - lastline + 1

  else
   " not readable
"   call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"   call Decho("tfile<".a:tfile."> not readable")
   keepj call netrw#ErrorMsg(s:WARNING,"file <".a:tfile."> not readable",9)
"   call Dret("NetrwGetFile : tfile<".a:tfile."> not readable")
   return
  endif

  " User-provided (ie. optional) fix-it-up command
  if exists("*NetReadFixup")
"   call Decho("calling NetReadFixup(method<".a:method."> line1=".line1." line2=".line2.")")
   keepj call NetReadFixup(a:method, line1, line2)
"  else " Decho
"   call Decho("NetReadFixup() not called, doesn't exist  (line1=".line1." line2=".line2.")")
  endif

  if has("gui") && has("menu") && has("gui_running") && &go =~# 'm' && g:netrw_menu
   " update the Buffers menu
   keepj call s:UpdateBuffersMenu()
  endif

"  call Decho("readcmd<".a:readcmd."> cmdarg<".v:cmdarg."> tfile<".a:tfile."> readable=".s:FileReadable(a:tfile))

 " make sure file is being displayed
"  redraw!

"  call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"  call Dret("NetrwGetFile")
endfun

" ------------------------------------------------------------------------
" s:NetrwMethod:  determine method of transfer {{{2
" Input:
"   choice = url   [protocol:]//[userid@]hostname[:port]/[path-to-file]
" Output:
"  b:netrw_method= 1: rcp                                             
"                  2: ftp + <.netrc>                                  
"	           3: ftp + machine, id, password, and [path]filename 
"	           4: scp                                             
"	           5: http (wget)                                     
"	           6: dav
"	           7: rsync                                           
"	           8: fetch                                           
"	           9: sftp                                            
"  g:netrw_machine= hostname
"  b:netrw_fname  = filename
"  g:netrw_port   = optional port number (for ftp)
"  g:netrw_choice = copy of input url (choice)
fun! s:NetrwMethod(choice)
"   call Dfunc("NetrwMethod(a:choice<".a:choice.">)")

   " record current g:netrw_machine, if any
   " curmachine used if protocol == ftp and no .netrc
   if exists("g:netrw_machine")
    let curmachine= g:netrw_machine
"    call Decho("curmachine<".curmachine.">")
   else
    let curmachine= "N O T A HOST"
   endif
   if exists("g:netrw_port")
    let netrw_port= g:netrw_port
   endif

   " insure that netrw_ftp_cmd starts off every method determination
   " with the current g:netrw_ftp_cmd
   let s:netrw_ftp_cmd= g:netrw_ftp_cmd

  " initialization
  let b:netrw_method  = 0
  let g:netrw_machine = ""
  let b:netrw_fname   = ""
  let g:netrw_port    = ""
  let g:netrw_choice  = a:choice

  " Patterns:
  " mipf     : a:machine a:id password filename	     Use ftp
  " mf	    : a:machine filename		     Use ftp + <.netrc> or g:netrw_uid s:netrw_passwd
  " ftpurm   : ftp://[user@]host[[#:]port]/filename  Use ftp + <.netrc> or g:netrw_uid s:netrw_passwd
  " rcpurm   : rcp://[user@]host/filename	     Use rcp
  " rcphf    : [user@]host:filename		     Use rcp
  " scpurm   : scp://[user@]host[[#:]port]/filename  Use scp
  " httpurm  : http://[user@]host/filename	     Use wget
  " davurm   : dav[s]://host[:port]/path             Use cadaver/curl
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
  let davurm   = '^davs\=://\([^/]\+\)/\(.*/\)\([-_.~[:alnum:]]\+\)$'
  let rsyncurm = '^rsync://\([^/]\{-}\)/\(.*\)\=$'
  let fetchurm = '^fetch://\(\([^/@]\{-}\)@\)\=\([^/#:]\{-}\)\(:http\)\=/\(.*\)$'
  let sftpurm  = '^sftp://\([^/]\{-}\)/\(.*\)\=$'

"  call Decho("determine method:")
  " Determine Method
  " Method#1: rcp://user@hostname/...path-to-file {{{3
  if match(a:choice,rcpurm) == 0
"   call Decho("rcp://...")
   let b:netrw_method  = 1
   let userid          = substitute(a:choice,rcpurm,'\1',"")
   let g:netrw_machine = substitute(a:choice,rcpurm,'\2',"")
   let b:netrw_fname   = substitute(a:choice,rcpurm,'\3',"")
   if userid != ""
    let g:netrw_uid= userid
   endif

  " Method#4: scp://user@hostname/...path-to-file {{{3
  elseif match(a:choice,scpurm) == 0
"   call Decho("scp://...")
   let b:netrw_method  = 4
   let g:netrw_machine = substitute(a:choice,scpurm,'\1',"")
   let g:netrw_port    = substitute(a:choice,scpurm,'\2',"")
   let b:netrw_fname   = substitute(a:choice,scpurm,'\3',"")

  " Method#5: http://user@hostname/...path-to-file {{{3
  elseif match(a:choice,httpurm) == 0
"   call Decho("http://...")
   let b:netrw_method = 5
   let g:netrw_machine= substitute(a:choice,httpurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,httpurm,'\2',"")

  " Method#6: dav://hostname[:port]/..path-to-file.. {{{3
  elseif match(a:choice,davurm) == 0
"   call Decho("dav://...")
   let b:netrw_method= 6
   if a:choice =~ '^s'
    let g:netrw_machine= 'https://'.substitute(a:choice,davurm,'\1/\2',"")
   else
    let g:netrw_machine= 'http://'.substitute(a:choice,davurm,'\1/\2',"")
   endif
   let b:netrw_fname  = substitute(a:choice,davurm,'\3',"")

   " Method#7: rsync://user@hostname/...path-to-file {{{3
  elseif match(a:choice,rsyncurm) == 0
"   call Decho("rsync://...")
   let b:netrw_method = 7
   let g:netrw_machine= substitute(a:choice,rsyncurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,rsyncurm,'\2',"")

   " Methods 2,3: ftp://[user@]hostname[[:#]port]/...path-to-file {{{3
  elseif match(a:choice,ftpurm) == 0
"   call Decho("ftp://...")
   let userid	      = substitute(a:choice,ftpurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,ftpurm,'\3',"")
   let g:netrw_port   = substitute(a:choice,ftpurm,'\4',"")
   let b:netrw_fname  = substitute(a:choice,ftpurm,'\5',"")
"   call Decho("g:netrw_machine<".g:netrw_machine.">")
   if userid != ""
    let g:netrw_uid= userid
   endif
   if curmachine != g:netrw_machine
    if exists("s:netrw_passwd")
     " if there's a change in hostname, require password re-entry
     unlet s:netrw_passwd
    endif
    if exists("netrw_port")
     unlet netrw_port
    endif
   endif
   if exists("g:netrw_uid") && exists("s:netrw_passwd")
    let b:netrw_method = 3
   else
    if (has("win32") || has("win95") || has("win64") || has("win16")) && s:netrw_ftp_cmd =~ '-[sS]:'
"     call Decho("has -s: : s:netrw_ftp_cmd<".s:netrw_ftp_cmd.">")
"     call Decho("          g:netrw_ftp_cmd<".g:netrw_ftp_cmd.">")
     if g:netrw_ftp_cmd =~ '-[sS]:\S*MACHINE\>'
      let machine        = substitute(g:netrw_machine,'\([^.]\+\)\.\S*','\1','')
      let s:netrw_ftp_cmd= substitute(g:netrw_ftp_cmd,'\<MACHINE\>',machine.".ftp",'')
"      call Decho("s:netrw_ftp_cmd<".s:netrw_ftp_cmd.">")
     endif
     let b:netrw_method= 2
    elseif s:FileReadable(expand("$HOME/.netrc")) && !g:netrw_ignorenetrc
"     call Decho("using <".expand("$HOME/.netrc")."> (readable)")
     let b:netrw_method= 2
    else
     if !exists("g:netrw_uid") || g:netrw_uid == ""
      call NetUserPass()
     elseif !exists("s:netrw_passwd") || s:netrw_passwd == ""
      call NetUserPass(g:netrw_uid)
    " else just use current g:netrw_uid and s:netrw_passwd
     endif
     let b:netrw_method= 3
    endif
   endif

  " Method#8: fetch {{{3
  elseif match(a:choice,fetchurm) == 0
"   call Decho("fetch://...")
   let b:netrw_method = 8
   let g:netrw_userid = substitute(a:choice,fetchurm,'\2',"")
   let g:netrw_machine= substitute(a:choice,fetchurm,'\3',"")
   let b:netrw_option = substitute(a:choice,fetchurm,'\4',"")
   let b:netrw_fname  = substitute(a:choice,fetchurm,'\5',"")

   " Method#3: Issue an ftp : "machine id password [path/]filename" {{{3
  elseif match(a:choice,mipf) == 0
"   call Decho("(ftp) host id pass file")
   let b:netrw_method  = 3
   let g:netrw_machine = substitute(a:choice,mipf,'\1',"")
   let g:netrw_uid     = substitute(a:choice,mipf,'\2',"")
   let s:netrw_passwd  = substitute(a:choice,mipf,'\3',"")
   let b:netrw_fname   = substitute(a:choice,mipf,'\4',"")

  " Method#3: Issue an ftp: "hostname [path/]filename" {{{3
  elseif match(a:choice,mf) == 0
"   call Decho("(ftp) host file")
   if exists("g:netrw_uid") && exists("s:netrw_passwd")
    let b:netrw_method  = 3
    let g:netrw_machine = substitute(a:choice,mf,'\1',"")
    let b:netrw_fname   = substitute(a:choice,mf,'\2',"")

   elseif s:FileReadable(expand("$HOME/.netrc"))
    let b:netrw_method  = 2
    let g:netrw_machine = substitute(a:choice,mf,'\1',"")
    let b:netrw_fname   = substitute(a:choice,mf,'\2',"")
   endif

  " Method#9: sftp://user@hostname/...path-to-file {{{3
  elseif match(a:choice,sftpurm) == 0
"   call Decho("sftp://...")
   let b:netrw_method = 9
   let g:netrw_machine= substitute(a:choice,sftpurm,'\1',"")
   let b:netrw_fname  = substitute(a:choice,sftpurm,'\2',"")

  " Method#1: Issue an rcp: hostname:filename"  (this one should be last) {{{3
  elseif match(a:choice,rcphf) == 0
"   call Decho("(rcp) [user@]host:file) rcphf<".rcphf.">")
   let b:netrw_method  = 1
   let userid          = substitute(a:choice,rcphf,'\2',"")
   let g:netrw_machine = substitute(a:choice,rcphf,'\3',"")
   let b:netrw_fname   = substitute(a:choice,rcphf,'\4',"")
"   call Decho('\1<'.substitute(a:choice,rcphf,'\1',"").">")
"   call Decho('\2<'.substitute(a:choice,rcphf,'\2',"").">")
"   call Decho('\3<'.substitute(a:choice,rcphf,'\3',"").">")
"   call Decho('\4<'.substitute(a:choice,rcphf,'\4',"").">")
   if userid != ""
    let g:netrw_uid= userid
   endif

  " Cannot Determine Method {{{3
  else
   if !exists("g:netrw_quiet")
    call netrw#ErrorMsg(s:WARNING,"cannot determine method (format: protocol://[user@]hostname[:port]/[path])",45)
   endif
   let b:netrw_method  = -1
  endif
  "}}}3

  if g:netrw_port != ""
   " remove any leading [:#] from port number
   let g:netrw_port = substitute(g:netrw_port,'[#:]\+','','')
  elseif exists("netrw_port")
   " retain port number as implicit for subsequent ftp operations
   let g:netrw_port= netrw_port
  endif

"  call Decho("a:choice       <".a:choice.">")
"  call Decho("b:netrw_method <".b:netrw_method.">")
"  call Decho("g:netrw_machine<".g:netrw_machine.">")
"  call Decho("g:netrw_port   <".g:netrw_port.">")
"  if exists("g:netrw_uid")		"Decho
"   call Decho("g:netrw_uid    <".g:netrw_uid.">")
"  endif					"Decho
"  if exists("s:netrw_passwd")		"Decho
"   call Decho("s:netrw_passwd <".s:netrw_passwd.">")
"  endif					"Decho
"  call Decho("b:netrw_fname  <".b:netrw_fname.">")
"  call Dret("NetrwMethod : b:netrw_method=".b:netrw_method." g:netrw_port=".g:netrw_port)
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

   " sanity checks -- attempt to convert inputs to integers
   let method = a:method + 0
   let line1  = a:line1 + 0
   let line2  = a:line2 + 0
   if type(method) != 0 || type(line1) != 0 || type(line2) != 0 || method < 0 || line1 <= 0 || line2 <= 0
"    call Dret("NetReadFixup")
    return
   endif

   if method == 3   " ftp (no <.netrc>)
    let fourblanklines= line2 - 3
    if fourblanklines >= line1
     exe "sil keepj ".fourblanklines.",".line2."g/^\s*$/d"
     call histdel("/",-1)
    endif
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
"  call Dfunc("NetUserPass(a:1<".a:1.">)")
  let g:netrw_uid= a:1
 endif

 " get password
 if a:0 <= 1 " via prompt
"  call Decho("a:0=".a:0." case <=1:")
  let s:netrw_passwd= inputsecret("Enter Password: ")
 else " from command line
"  call Decho("a:0=".a:0." case >1: a:2<".a:2.">")
  let s:netrw_passwd=a:2
 endif

"  call Dret("NetUserPass")
endfun

" ===========================================
"  Shared Browsing Support:    {{{1
" ===========================================

" ---------------------------------------------------------------------
" s:NetrwMaps: {{{2
fun! s:NetrwMaps(islocal)
"  call Dfunc("s:NetrwMaps(islocal=".a:islocal.") b:netrw_curdir<".b:netrw_curdir.">")
  if a:islocal
"   call Decho("make local maps")
   inoremap <buffer> <silent> a		<c-o>:call <SID>NetrwHide(1)<cr>
   inoremap <buffer> <silent> c		<c-o>:exe "keepjumps lcd ".fnameescape(b:netrw_curdir)<cr>
   inoremap <buffer> <silent> C		<c-o>:let g:netrw_chgwin= winnr()<cr>
   inoremap <buffer> <silent> %		<c-o>:call <SID>NetrwOpenFile(1)<cr>
   inoremap <buffer> <silent> -		<c-o>:exe "norm! 0"<bar>call netrw#LocalBrowseCheck(<SID>NetrwBrowseChgDir(1,'../'))<cr>
   inoremap <buffer> <silent> <cr>	<c-o>:call netrw#LocalBrowseCheck(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord()))<cr>
   inoremap <buffer> <silent> d		<c-o>:call <SID>NetrwMakeDir("")<cr>
   inoremap <buffer> <silent> gb	<c-o>:<c-u>call <SID>NetrwBookHistHandler(1,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> gh	<c-o>:<c-u>call <SID>NetrwHidden(1)<cr>
   inoremap <buffer> <silent> gp	<c-o>:<c-u>call <SID>NetrwChgPerm(1,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> I		<c-o>:call <SID>NetrwBannerCtrl(1)<cr>
   inoremap <buffer> <silent> i		<c-o>:call <SID>NetrwListStyle(1)<cr>
   inoremap <buffer> <silent> mb	<c-o>:<c-u>call <SID>NetrwBookHistHandler(0,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> mB	<c-o>:<c-u>call <SID>NetrwBookHistHandler(6,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> mc	<c-o>:<c-u>call <SID>NetrwMarkFileCopy(1)<cr>
   inoremap <buffer> <silent> md	<c-o>:<c-u>call <SID>NetrwMarkFileDiff(1)<cr>
   inoremap <buffer> <silent> me	<c-o>:<c-u>call <SID>NetrwMarkFileEdit(1)<cr>
   inoremap <buffer> <silent> mf	<c-o>:<c-u>call <SID>NetrwMarkFile(1,<SID>NetrwGetWord())<cr>
   inoremap <buffer> <silent> mg	<c-o>:<c-u>call <SID>NetrwMarkFileGrep(1)<cr>
   inoremap <buffer> <silent> mh	<c-o>:<c-u>call <SID>NetrwMarkHideSfx(1)<cr>
   inoremap <buffer> <silent> mm	<c-o>:<c-u>call <SID>NetrwMarkFileMove(1)<cr>
   inoremap <buffer> <silent> mp	<c-o>:<c-u>call <SID>NetrwMarkFilePrint(1)<cr>
   inoremap <buffer> <silent> mr	<c-o>:<c-u>call <SID>NetrwMarkFileRegexp(1)<cr>
   inoremap <buffer> <silent> ms	<c-o>:<c-u>call <SID>NetrwMarkFileSource(1)<cr>
   inoremap <buffer> <silent> mT	<c-o>:<c-u>call <SID>NetrwMarkFileTag(1)<cr>
   inoremap <buffer> <silent> mt	<c-o>:<c-u>call <SID>NetrwMarkFileTgt(1)<cr>
   inoremap <buffer> <silent> mu	<c-o>:<c-u>call <SID>NetrwUnMarkFile(1)<cr>
   inoremap <buffer> <silent> mx	<c-o>:<c-u>call <SID>NetrwMarkFileExe(1)<cr>
   inoremap <buffer> <silent> mz	<c-o>:<c-u>call <SID>NetrwMarkFileCompress(1)<cr>
   inoremap <buffer> <silent> O		<c-o>:call <SID>NetrwObtain(1)<cr>
   inoremap <buffer> <silent> o		<c-o>:call <SID>NetrwSplit(3)<cr>
   inoremap <buffer> <silent> p		<c-o>:call <SID>NetrwPreview(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord(),1))<cr>
   inoremap <buffer> <silent> P		<c-o>:call <SID>NetrwPrevWinOpen(1)<cr>
   inoremap <buffer> <silent> qb	<c-o>:<c-u>call <SID>NetrwBookHistHandler(2,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> qf	<c-o>:<c-u>call <SID>NetrwFileInfo(1,<SID>NetrwGetWord())<cr>
   inoremap <buffer> <silent> r		<c-o>:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetrwRefresh(1,<SID>NetrwBrowseChgDir(1,'./'))<cr>
   inoremap <buffer> <silent> s		<c-o>:call <SID>NetrwSortStyle(1)<cr>
   inoremap <buffer> <silent> S		<c-o>:call <SID>NetSortSequence(1)<cr>
   inoremap <buffer> <silent> T		<c-o>:call <SID>NetrwSplit(4)<bar>norm! gT<cr>
   inoremap <buffer> <silent> t		<c-o>:call <SID>NetrwSplit(4)<cr>
   inoremap <buffer> <silent> u		<c-o>:<c-u>call <SID>NetrwBookHistHandler(4,expand("%"))<cr>
   inoremap <buffer> <silent> U		<c-o>:<c-u>call <SID>NetrwBookHistHandler(5,expand("%"))<cr>
   inoremap <buffer> <silent> v		<c-o>:call <SID>NetrwSplit(5)<cr>
   inoremap <buffer> <silent> x		<c-o>:call netrw#NetrwBrowseX(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord(),0),0)"<cr>
   nnoremap <buffer> <silent> a		:call <SID>NetrwHide(1)<cr>
   nnoremap <buffer> <silent> %		:call <SID>NetrwOpenFile(1)<cr>
   nnoremap <buffer> <silent> c		:exe "keepjumps lcd ".fnameescape(b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> C		:let g:netrw_chgwin= winnr()<cr>
   nnoremap <buffer> <silent> <cr>	:call netrw#LocalBrowseCheck(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord()))<cr>
   nnoremap <buffer> <silent> d		:call <SID>NetrwMakeDir("")<cr>
   nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call netrw#LocalBrowseCheck(<SID>NetrwBrowseChgDir(1,'../'))<cr>
   nnoremap <buffer> <silent> gb	:<c-u>call <SID>NetrwBookHistHandler(1,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> gd	:<c-u>call <SID>NetrwForceChgDir(1,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> gf	:<c-u>call <SID>NetrwForceFile(1,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> gh	:<c-u>call <SID>NetrwHidden(1)<cr>
   nnoremap <buffer> <silent> gp	:<c-u>call <SID>NetrwChgPerm(1,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> I		:call <SID>NetrwBannerCtrl(1)<cr>
   nnoremap <buffer> <silent> i		:call <SID>NetrwListStyle(1)<cr>
   nnoremap <buffer> <silent> mb	:<c-u>call <SID>NetrwBookHistHandler(0,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> mB	:<c-u>call <SID>NetrwBookHistHandler(6,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> mc	:<c-u>call <SID>NetrwMarkFileCopy(1)<cr>
   nnoremap <buffer> <silent> md	:<c-u>call <SID>NetrwMarkFileDiff(1)<cr>
   nnoremap <buffer> <silent> me	:<c-u>call <SID>NetrwMarkFileEdit(1)<cr>
   nnoremap <buffer> <silent> mf	:<c-u>call <SID>NetrwMarkFile(1,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> mg	:<c-u>call <SID>NetrwMarkFileGrep(1)<cr>
   nnoremap <buffer> <silent> mh	:<c-u>call <SID>NetrwMarkHideSfx(1)<cr>
   nnoremap <buffer> <silent> mm	:<c-u>call <SID>NetrwMarkFileMove(1)<cr>
   nnoremap <buffer> <silent> mp	:<c-u>call <SID>NetrwMarkFilePrint(1)<cr>
   nnoremap <buffer> <silent> mr	:<c-u>call <SID>NetrwMarkFileRegexp(1)<cr>
   nnoremap <buffer> <silent> ms	:<c-u>call <SID>NetrwMarkFileSource(1)<cr>
   nnoremap <buffer> <silent> mT	:<c-u>call <SID>NetrwMarkFileTag(1)<cr>
   nnoremap <buffer> <silent> mt	:<c-u>call <SID>NetrwMarkFileTgt(1)<cr>
   nnoremap <buffer> <silent> mu	:<c-u>call <SID>NetrwUnMarkFile(1)<cr>
   nnoremap <buffer> <silent> mx	:<c-u>call <SID>NetrwMarkFileExe(1)<cr>
   nnoremap <buffer> <silent> mz	:<c-u>call <SID>NetrwMarkFileCompress(1)<cr>
   nnoremap <buffer> <silent> O		:call <SID>NetrwObtain(1)<cr>
   nnoremap <buffer> <silent> o		:call <SID>NetrwSplit(3)<cr>
   nnoremap <buffer> <silent> p		:call <SID>NetrwPreview(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord(),1))<cr>
   nnoremap <buffer> <silent> P		:call <SID>NetrwPrevWinOpen(1)<cr>
   nnoremap <buffer> <silent> qb	:<c-u>call <SID>NetrwBookHistHandler(2,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> qf	:<c-u>call <SID>NetrwFileInfo(1,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetrwRefresh(1,<SID>NetrwBrowseChgDir(1,'./'))<cr>
   nnoremap <buffer> <silent> s		:call <SID>NetrwSortStyle(1)<cr>
   nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(1)<cr>
   nnoremap <buffer> <silent> T		:call <SID>NetrwSplit(4)<bar>norm! gT<cr>
   nnoremap <buffer> <silent> t		:call <SID>NetrwSplit(4)<cr>
   nnoremap <buffer> <silent> u		:<c-u>call <SID>NetrwBookHistHandler(4,expand("%"))<cr>
   nnoremap <buffer> <silent> U		:<c-u>call <SID>NetrwBookHistHandler(5,expand("%"))<cr>
   nnoremap <buffer> <silent> v		:call <SID>NetrwSplit(5)<cr>
   nnoremap <buffer> <silent> x		:call netrw#NetrwBrowseX(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord(),0),0)"<cr>
   if !hasmapto('<Plug>NetrwHideEdit')
    nmap <buffer> <unique> <c-h> <Plug>NetrwHideEdit
    imap <buffer> <unique> <c-h> <Plug>NetrwHideEdit
   endif
   nnoremap <buffer> <silent> <Plug>NetrwHideEdit	:call <SID>NetrwHideEdit(1)<cr>
   if !hasmapto('<Plug>NetrwRefresh')
    nmap <buffer> <unique> <c-l> <Plug>NetrwRefresh
    imap <buffer> <unique> <c-l> <Plug>NetrwRefresh
   endif
   nnoremap <buffer> <silent> <Plug>NetrwRefresh		:call <SID>NetrwRefresh(1,<SID>NetrwBrowseChgDir(1,'./'))<cr>
   if s:didstarstar || !mapcheck("<s-down>","n")
    nnoremap <buffer> <silent> <s-down>	:Nexplore<cr>
    inoremap <buffer> <silent> <s-down>	:Nexplore<cr>
   endif
   if s:didstarstar || !mapcheck("<s-up>","n")
    nnoremap <buffer> <silent> <s-up>	:Pexplore<cr>
    inoremap <buffer> <silent> <s-up>	:Pexplore<cr>
   endif
   let mapsafecurdir = escape(b:netrw_curdir, s:netrw_map_escape)
   if g:netrw_mousemaps == 1
    nnoremap <buffer> <silent> <leftmouse>   <leftmouse>:call <SID>NetrwLeftmouse(1)<cr>
    nnoremap <buffer> <silent> <middlemouse> <leftmouse>:call <SID>NetrwPrevWinOpen(1)<cr>
    nnoremap <buffer> <silent> <s-leftmouse> <leftmouse>:call <SID>NetrwMarkFile(1,<SID>NetrwGetWord())<cr>
    nmap     <buffer> <silent> <2-leftmouse> -
    exe 'nnoremap <buffer> <silent> <rightmouse>  <leftmouse>:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
    exe 'vnoremap <buffer> <silent> <rightmouse>  <leftmouse>:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
    inoremap <buffer> <silent> <leftmouse>   <c-o><leftmouse><c-o>:call <SID>NetrwLeftmouse(1)<cr>
    inoremap <buffer> <silent> <middlemouse> <c-o><leftmouse><c-o>:call <SID>NetrwPrevWinOpen(1)<cr>
    inoremap <buffer> <silent> <s-leftmouse> <c-o><leftmouse><c-o>:call <SID>NetrwMarkFile(1,<SID>NetrwGetWord())<cr>
    exe 'inoremap <buffer> <silent> <rightmouse>  <c-o><leftmouse><c-o>:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
   endif
   exe 'nnoremap <buffer> <silent> <del>	:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
   exe 'nnoremap <buffer> <silent> D		:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
   exe 'nnoremap <buffer> <silent> R		:call <SID>NetrwLocalRename("'.mapsafecurdir.'")<cr>'
   exe 'nnoremap <buffer> <silent> <Leader>m	:call <SID>NetrwMakeDir("")<cr>'
   exe 'vnoremap <buffer> <silent> <del>	:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
   exe 'vnoremap <buffer> <silent> D		:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
   exe 'vnoremap <buffer> <silent> R		:call <SID>NetrwLocalRename("'.mapsafecurdir.'")<cr>'
   exe 'inoremap <buffer> <silent> <del>	<c-o>:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
   exe 'inoremap <buffer> <silent> D		<c-o>:call <SID>NetrwLocalRm("'.mapsafecurdir.'")<cr>'
   exe 'inoremap <buffer> <silent> R		<c-o>:call <SID>NetrwLocalRename("'.mapsafecurdir.'")<cr>'
   exe 'inoremap <buffer> <silent> <Leader>m	<c-o>:call <SID>NetrwMakeDir("")<cr>'
   nnoremap <buffer> <F1>		:he netrw-quickhelp<cr>

  else " remote
"   call Decho("make remote maps")
   call s:RemotePathAnalysis(b:netrw_curdir)
   nnoremap <buffer> <silent> <cr>	:call <SID>NetrwBrowse(0,<SID>NetrwBrowseChgDir(0,<SID>NetrwGetWord()))<cr>
   nnoremap <buffer> <silent> <c-l>	:call <SID>NetrwRefresh(0,<SID>NetrwBrowseChgDir(0,'./'))<cr>
   nnoremap <buffer> <silent> -		:exe "norm! 0"<bar>call <SID>NetrwBrowse(0,<SID>NetrwBrowseChgDir(0,'../'))<cr>
   nnoremap <buffer> <silent> a		:call <SID>NetrwHide(0)<cr>
   nnoremap <buffer> <silent> mb	:<c-u>call <SID>NetrwBookHistHandler(0,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> mc	:<c-u>call <SID>NetrwMarkFileCopy(0)<cr>
   nnoremap <buffer> <silent> md	:<c-u>call <SID>NetrwMarkFileDiff(0)<cr>
   nnoremap <buffer> <silent> me	:<c-u>call <SID>NetrwMarkFileEdit(0)<cr>
   nnoremap <buffer> <silent> mf	:<c-u>call <SID>NetrwMarkFile(0,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> mg	:<c-u>call <SID>NetrwMarkFileGrep(0)<cr>
   nnoremap <buffer> <silent> mh	:<c-u>call <SID>NetrwMarkHideSfx(0)<cr>
   nnoremap <buffer> <silent> mm	:<c-u>call <SID>NetrwMarkFileMove(0)<cr>
   nnoremap <buffer> <silent> mp	:<c-u>call <SID>NetrwMarkFilePrint(0)<cr>
   nnoremap <buffer> <silent> mr	:<c-u>call <SID>NetrwMarkFileRegexp(0)<cr>
   nnoremap <buffer> <silent> ms	:<c-u>call <SID>NetrwMarkFileSource(0)<cr>
   nnoremap <buffer> <silent> mT	:<c-u>call <SID>NetrwMarkFileTag(0)<cr>
   nnoremap <buffer> <silent> mt	:<c-u>call <SID>NetrwMarkFileTgt(0)<cr>
   nnoremap <buffer> <silent> mu	:<c-u>call <SID>NetrwUnMarkFile(0)<cr>
   nnoremap <buffer> <silent> mx	:<c-u>call <SID>NetrwMarkFileExe(0)<cr>
   nnoremap <buffer> <silent> mz	:<c-u>call <SID>NetrwMarkFileCompress(0)<cr>
   nnoremap <buffer> <silent> gb	:<c-u>call <SID>NetrwBookHistHandler(1,b:netrw_cur)<cr>
   nnoremap <buffer> <silent> gd	:<c-u>call <SID>NetrwForceChgDir(0,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> gf	:<c-u>call <SID>NetrwForceFile(0,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> gh	:<c-u>call <SID>NetrwHidden(0)<cr>
   nnoremap <buffer> <silent> gp	:<c-u>call <SID>NetrwChgPerm(0,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> C		:let g:netrw_chgwin= winnr()<cr>
   nnoremap <buffer> <silent> i		:call <SID>NetrwListStyle(0)<cr>
   nnoremap <buffer> <silent> I		:call <SID>NetrwBannerCtrl(1)<cr>
   nnoremap <buffer> <silent> o		:call <SID>NetrwSplit(0)<cr>
   nnoremap <buffer> <silent> O		:call <SID>NetrwObtain(0)<cr>
   nnoremap <buffer> <silent> p		:call <SID>NetrwPreview(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord(),1))<cr>
   nnoremap <buffer> <silent> P		:call <SID>NetrwPrevWinOpen(0)<cr>
   nnoremap <buffer> <silent> qb	:<c-u>call <SID>NetrwBookHistHandler(2,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> mB	:<c-u>call <SID>NetrwBookHistHandler(6,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> qf	:<c-u>call <SID>NetrwFileInfo(0,<SID>NetrwGetWord())<cr>
   nnoremap <buffer> <silent> r		:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetrwBrowse(0,<SID>NetrwBrowseChgDir(0,'./'))<cr>
   nnoremap <buffer> <silent> s		:call <SID>NetrwSortStyle(0)<cr>
   nnoremap <buffer> <silent> S		:call <SID>NetSortSequence(0)<cr>
   nnoremap <buffer> <silent> t		:call <SID>NetrwSplit(1)<cr>
   nnoremap <buffer> <silent> T		:call <SID>NetrwSplit(1)<bar>norm! gT<cr>
   nnoremap <buffer> <silent> u		:<c-u>call <SID>NetrwBookHistHandler(4,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> U		:<c-u>call <SID>NetrwBookHistHandler(5,b:netrw_curdir)<cr>
   nnoremap <buffer> <silent> v		:call <SID>NetrwSplit(2)<cr>
   nnoremap <buffer> <silent> x		:call netrw#NetrwBrowseX(<SID>NetrwBrowseChgDir(0,<SID>NetrwGetWord()),1)<cr>
   nnoremap <buffer> <silent> %		:call <SID>NetrwOpenFile(0)<cr>
   inoremap <buffer> <silent> <cr>	<c-o>:call <SID>NetrwBrowse(0,<SID>NetrwBrowseChgDir(0,<SID>NetrwGetWord()))<cr>
   inoremap <buffer> <silent> <c-l>	<c-o>:call <SID>NetrwRefresh(0,<SID>NetrwBrowseChgDir(0,'./'))<cr>
   inoremap <buffer> <silent> -		<c-o>:exe "norm! 0"<bar>call <SID>NetrwBrowse(0,<SID>NetrwBrowseChgDir(0,'../'))<cr>
   inoremap <buffer> <silent> a		<c-o>:call <SID>NetrwHide(0)<cr>
   inoremap <buffer> <silent> mb	<c-o>:<c-u>call <SID>NetrwBookHistHandler(0,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> mc	<c-o>:<c-u>call <SID>NetrwMarkFileCopy(0)<cr>
   inoremap <buffer> <silent> md	<c-o>:<c-u>call <SID>NetrwMarkFileDiff(0)<cr>
   inoremap <buffer> <silent> me	<c-o>:<c-u>call <SID>NetrwMarkFileEdit(0)<cr>
   inoremap <buffer> <silent> mf	<c-o>:<c-u>call <SID>NetrwMarkFile(0,<SID>NetrwGetWord())<cr>
   inoremap <buffer> <silent> mg	<c-o>:<c-u>call <SID>NetrwMarkFileGrep(0)<cr>
   inoremap <buffer> <silent> mh	<c-o>:<c-u>call <SID>NetrwMarkHideSfx(0)<cr>
   inoremap <buffer> <silent> mm	<c-o>:<c-u>call <SID>NetrwMarkFileMove(0)<cr>
   inoremap <buffer> <silent> mp	<c-o>:<c-u>call <SID>NetrwMarkFilePrint(0)<cr>
   inoremap <buffer> <silent> mr	<c-o>:<c-u>call <SID>NetrwMarkFileRegexp(0)<cr>
   inoremap <buffer> <silent> ms	<c-o>:<c-u>call <SID>NetrwMarkFileSource(0)<cr>
   inoremap <buffer> <silent> mT	<c-o>:<c-u>call <SID>NetrwMarkFileTag(0)<cr>
   inoremap <buffer> <silent> mt	<c-o>:<c-u>call <SID>NetrwMarkFileTgt(0)<cr>
   inoremap <buffer> <silent> mu	<c-o>:<c-u>call <SID>NetrwUnMarkFile(0)<cr>
   inoremap <buffer> <silent> mx	<c-o>:<c-u>call <SID>NetrwMarkFileExe(0)<cr>
   inoremap <buffer> <silent> mz	<c-o>:<c-u>call <SID>NetrwMarkFileCompress(0)<cr>
   inoremap <buffer> <silent> gb	<c-o>:<c-u>call <SID>NetrwBookHistHandler(1,b:netrw_cur)<cr>
   inoremap <buffer> <silent> gh	<c-o>:<c-u>call <SID>NetrwHidden(0)<cr>
   inoremap <buffer> <silent> gp	<c-o>:<c-u>call <SID>NetrwChgPerm(0,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> C		<c-o>:let g:netrw_chgwin= winnr()<cr>
   inoremap <buffer> <silent> i		<c-o>:call <SID>NetrwListStyle(0)<cr>
   inoremap <buffer> <silent> I		<c-o>:call <SID>NetrwBannerCtrl(1)<cr>
   inoremap <buffer> <silent> o		<c-o>:call <SID>NetrwSplit(0)<cr>
   inoremap <buffer> <silent> O		<c-o>:call <SID>NetrwObtain(0)<cr>
   inoremap <buffer> <silent> p		<c-o>:call <SID>NetrwPreview(<SID>NetrwBrowseChgDir(1,<SID>NetrwGetWord(),1))<cr>
   inoremap <buffer> <silent> P		<c-o>:call <SID>NetrwPrevWinOpen(0)<cr>
   inoremap <buffer> <silent> qb	<c-o>:<c-u>call <SID>NetrwBookHistHandler(2,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> mB	<c-o>:<c-u>call <SID>NetrwBookHistHandler(6,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> qf	<c-o>:<c-u>call <SID>NetrwFileInfo(0,<SID>NetrwGetWord())<cr>
   inoremap <buffer> <silent> r		<c-o>:let g:netrw_sort_direction= (g:netrw_sort_direction =~ 'n')? 'r' : 'n'<bar>exe "norm! 0"<bar>call <SID>NetrwBrowse(0,<SID>NetrwBrowseChgDir(0,'./'))<cr>
   inoremap <buffer> <silent> s		<c-o>:call <SID>NetrwSortStyle(0)<cr>
   inoremap <buffer> <silent> S		<c-o>:call <SID>NetSortSequence(0)<cr>
   inoremap <buffer> <silent> t		<c-o>:call <SID>NetrwSplit(1)<cr>
   inoremap <buffer> <silent> T		<c-o>:call <SID>NetrwSplit(1)<bar>norm! gT<cr>
   inoremap <buffer> <silent> u		<c-o>:<c-u>call <SID>NetrwBookHistHandler(4,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> U		<c-o>:<c-u>call <SID>NetrwBookHistHandler(5,b:netrw_curdir)<cr>
   inoremap <buffer> <silent> v		<c-o>:call <SID>NetrwSplit(2)<cr>
   inoremap <buffer> <silent> x		<c-o>:call netrw#NetrwBrowseX(<SID>NetrwBrowseChgDir(0,<SID>NetrwGetWord()),1)<cr>
   inoremap <buffer> <silent> %		<c-o>:call <SID>NetrwOpenFile(0)<cr>
   if !hasmapto('<Plug>NetrwHideEdit')
    nmap <buffer> <c-h> <Plug>NetrwHideEdit
    imap <buffer> <c-h> <Plug>NetrwHideEdit
   endif
   nnoremap <buffer> <silent> <Plug>NetrwHideEdit	:call <SID>NetrwHideEdit(0)<cr>
   if !hasmapto('<Plug>NetrwRefresh')
    nmap <buffer> <c-l> <Plug>NetrwRefresh
    imap <buffer> <c-l> <Plug>NetrwRefresh
   endif

   let mapsafepath     = escape(s:path, s:netrw_map_escape)
   let mapsafeusermach = escape(s:user.s:machine, s:netrw_map_escape)

   nnoremap <buffer> <silent> <Plug>NetrwRefresh		:call <SID>NetrwRefresh(0,<SID>NetrwBrowseChgDir(0,'./'))<cr>
   if g:netrw_mousemaps == 1
    nnoremap <buffer> <silent> <leftmouse>   <leftmouse>:call <SID>NetrwLeftmouse(0)<cr>
    nnoremap <buffer> <silent> <middlemouse> <leftmouse>:call <SID>NetrwPrevWinOpen(0)<cr>
    nnoremap <buffer> <silent> <s-leftmouse> <leftmouse>:call <SID>NetrwMarkFile(0,<SID>NetrwGetWord())<cr>
    nmap     <buffer> <silent> <2-leftmouse> -
    exe 'nnoremap <buffer> <silent> <rightmouse> <leftmouse>:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
    exe 'vnoremap <buffer> <silent> <rightmouse> <leftmouse>:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
    inoremap <buffer> <silent> <leftmouse>   <c-o><leftmouse><c-o>:call <SID>NetrwLeftmouse(0)<cr>
    inoremap <buffer> <silent> <middlemouse> <c-o><leftmouse><c-o>:call <SID>NetrwPrevWinOpen(0)<cr>
    inoremap <buffer> <silent> <s-leftmouse> <c-o><leftmouse><c-o>:call <SID>NetrwMarkFile(0,<SID>NetrwGetWord())<cr>
    exe 'inoremap <buffer> <silent> <rightmouse> <c-o><leftmouse><c-o>:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   endif
   exe 'nnoremap <buffer> <silent> <del>	:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'nnoremap <buffer> <silent> d		:call <SID>NetrwMakeDir("'.mapsafeusermach.'")<cr>'
   exe 'nnoremap <buffer> <silent> D		:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'nnoremap <buffer> <silent> R		:call <SID>NetrwRemoteRename("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'vnoremap <buffer> <silent> <del>	:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'vnoremap <buffer> <silent> D		:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'vnoremap <buffer> <silent> R		:call <SID>NetrwRemoteRename("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'inoremap <buffer> <silent> <del>	<c-o>:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'inoremap <buffer> <silent> d		<c-o>:call <SID>NetrwMakeDir("'.mapsafeusermach.'")<cr>'
   exe 'inoremap <buffer> <silent> D		<c-o>:call <SID>NetrwRemoteRm("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   exe 'inoremap <buffer> <silent> R		<c-o>:call <SID>NetrwRemoteRename("'.mapsafeusermach.'","'.mapsafepath.'")<cr>'
   nnoremap <buffer> <F1>			:he netrw-quickhelp<cr>
   inoremap <buffer> <F1>			<c-o>:he netrw-quickhelp<cr>
  endif

  " set up Rexplore and leftmouse-double-click
  com! Rexplore call s:NetrwRexplore(w:netrw_rexlocal,exists("w:netrw_rexdir")? w:netrw_rexdir : ".")
  if g:netrw_mousemaps && g:netrw_retmap
   if !hasmapto("<Plug>NetrwReturn")
    if maparg("<2-leftmouse>","n") == "" || maparg("<2-leftmouse>","n") =~ '^-$'
"     call Decho("making map for 2-leftmouse")
     nmap <unique> <silent> <2-leftmouse>	<Plug>NetrwReturn
    elseif maparg("<c-leftmouse>","n") == ""
"     call Decho("making map for c-leftmouse")
     nmap <unique> <silent> <c-leftmouse>	<Plug>NetrwReturn
    endif
   endif
   exe 'nnoremap <silent> <Plug>NetrwReturn :Rexplore<cr>'
"   call Decho("made <Plug>NetrwReturn map")
  endif

  keepj call s:SetRexDir(a:islocal,b:netrw_curdir)
"  call Dret("s:NetrwMaps")
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
"  s:NetrwBookHistHandler: {{{2
"    0: (user: <mb>)   bookmark current directory
"    1: (user: <gb>)   change to the bookmarked directory
"    2: (user: <qb>)   list bookmarks
"    3: (browsing)     record current directory history
"    4: (user: <u>)    go up   (previous) bookmark
"    5: (user: <U>)    go down (next)     bookmark
"    6: (user: <mB>)   delete bookmark
fun! s:NetrwBookHistHandler(chg,curdir)
"  call Dfunc("s:NetrwBookHistHandler(chg=".a:chg." curdir<".a:curdir.">) cnt=".v:count." histcnt=".g:netrw_dirhist_cnt." histmax=".g:netrw_dirhistmax)

  if a:chg == 0
   " bookmark the current directory
"   call Decho("(user: <b>) bookmark the current directory")
   if !exists("g:netrw_bookmarklist")
    let g:netrw_bookmarklist= []
   endif
   if index(g:netrw_bookmarklist,a:curdir) == -1
    " curdir not currently in g:netrw_bookmarklist, so include it
    call add(g:netrw_bookmarklist,a:curdir)
    call sort(g:netrw_bookmarklist)
   endif
   echo "bookmarked the current directory"

  elseif a:chg == 1
   " change to the bookmarked directory
"   call Decho("(user: <".v:count."mb>) change to the bookmarked directory")
   if exists("g:netrw_bookmarklist[v:count-1]")
    exe "keepj e ".fnameescape(g:netrw_bookmarklist[v:count-1])
   else
    echomsg "Sorry, bookmark#".v:count." doesn't exist!"
   endif

  elseif a:chg == 2
"   redraw!
   let didwork= 0
   " list user's bookmarks
"   call Decho("(user: <q>) list user's bookmarks")
   if exists("g:netrw_bookmarklist")
"    call Decho('list '.len(g:netrw_bookmarklist).' bookmarks')
    let cnt= 1
    for bmd in g:netrw_bookmarklist
"     call Decho("Netrw Bookmark#".cnt.": ".g:netrw_bookmarklist[cnt-1])
     echo "Netrw Bookmark#".cnt.": ".g:netrw_bookmarklist[cnt-1]
     let didwork = 1
     let cnt     = cnt + 1
    endfor
   endif

   " list directory history
   let cnt     = g:netrw_dirhist_cnt
   let first   = 1
   let histcnt = 0
   if g:netrw_dirhistmax > 0
    while ( first || cnt != g:netrw_dirhist_cnt )
"    call Decho("first=".first." cnt=".cnt." dirhist_cnt=".g:netrw_dirhist_cnt)
     let histcnt= histcnt + 1
     if exists("g:netrw_dirhist_{cnt}")
"     call Decho("Netrw  History#".histcnt.": ".g:netrw_dirhist_{cnt})
      echo "Netrw  History#".histcnt.": ".g:netrw_dirhist_{cnt}
      let didwork= 1
     endif
     let first = 0
     let cnt   = ( cnt - 1 ) % g:netrw_dirhistmax
     if cnt < 0
      let cnt= cnt + g:netrw_dirhistmax
     endif
    endwhile
   else
    let g:netrw_dirhist_cnt= 0
   endif
   if didwork
    call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   endif

  elseif a:chg == 3
   " saves most recently visited directories (when they differ)
"   call Decho("(browsing) record curdir history")
   if !exists("g:netrw_dirhist_cnt") || !exists("g:netrw_dirhist_{g:netrw_dirhist_cnt}") || g:netrw_dirhist_{g:netrw_dirhist_cnt} != a:curdir
    if g:netrw_dirhistmax > 0
     let g:netrw_dirhist_cnt                   = ( g:netrw_dirhist_cnt + 1 ) % g:netrw_dirhistmax
     let g:netrw_dirhist_{g:netrw_dirhist_cnt} = a:curdir
    endif
"    call Decho("save dirhist#".g:netrw_dirhist_cnt."<".g:netrw_dirhist_{g:netrw_dirhist_cnt}.">")
   endif

  elseif a:chg == 4
   " u: change to the previous directory stored on the history list
"   call Decho("(user: <u>) chg to prev dir from history")
   if g:netrw_dirhistmax > 0
    let g:netrw_dirhist_cnt= ( g:netrw_dirhist_cnt - 1 ) % g:netrw_dirhistmax
    if g:netrw_dirhist_cnt < 0
     let g:netrw_dirhist_cnt= g:netrw_dirhist_cnt + g:netrw_dirhistmax
    endif
   else
    let g:netrw_dirhist_cnt= 0
   endif
   if exists("g:netrw_dirhist_{g:netrw_dirhist_cnt}")
"    call Decho("changedir u#".g:netrw_dirhist_cnt."<".g:netrw_dirhist_{g:netrw_dirhist_cnt}.">")
    if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir")
     setlocal ma noro
"     call Decho("setlocal ma noro")
     sil! keepj %d
     setlocal nomod
"     call Decho("setlocal nomod")
    endif
"    "    call Decho("exe e! ".fnameescape(g:netrw_dirhist_{g:netrw_dirhist_cnt}))
    exe "keepj e! ".fnameescape(g:netrw_dirhist_{g:netrw_dirhist_cnt})
   else
    if g:netrw_dirhistmax > 0
     let g:netrw_dirhist_cnt= ( g:netrw_dirhist_cnt + 1 ) % g:netrw_dirhistmax
    else
     let g:netrw_dirhist_cnt= 0
    endif
    echo "Sorry, no predecessor directory exists yet"
   endif

  elseif a:chg == 5
   " U: change to the subsequent directory stored on the history list
"   call Decho("(user: <U>) chg to next dir from history")
   if g:netrw_dirhistmax > 0
    let g:netrw_dirhist_cnt= ( g:netrw_dirhist_cnt + 1 ) % g:netrw_dirhistmax
    if exists("g:netrw_dirhist_{g:netrw_dirhist_cnt}")
"    call Decho("changedir U#".g:netrw_dirhist_cnt."<".g:netrw_dirhist_{g:netrw_dirhist_cnt}.">")
     if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir")
      setlocal ma noro
"     call Decho("setlocal ma noro")
      sil! keepj %d
"     call Decho("removed all lines from buffer (%d)")
      setlocal nomod
"     call Decho("setlocal nomod")
     endif
"    call Decho("exe e! ".fnameescape(g:netrw_dirhist_{g:netrw_dirhist_cnt}))
     exe "keepj e! ".fnameescape(g:netrw_dirhist_{g:netrw_dirhist_cnt})
    else
     let g:netrw_dirhist_cnt= ( g:netrw_dirhist_cnt - 1 ) % g:netrw_dirhistmax
     if g:netrw_dirhist_cnt < 0
      let g:netrw_dirhist_cnt= g:netrw_dirhist_cnt + g:netrw_dirhistmax
     endif
     echo "Sorry, no successor directory exists yet"
    endif
   else
    let g:netrw_dirhist_cnt= 0
    echo "Sorry, no successor directory exists yet (g:netrw_dirhistmax is ".g:netrw_dirhistmax.")"
   endif

  elseif a:chg == 6
   " delete the v:count'th bookmark
"   call Decho("delete bookmark#".v:count."<".g:netrw_bookmarklist[v:count-1].">")
   let savefile= s:NetrwHome()."/.netrwbook"
   if filereadable(savefile)
    keepj call s:NetrwBookHistSave() " done here to merge bookmarks first
    keepj call delete(savefile)
   endif
   keepj call remove(g:netrw_bookmarklist,v:count-1)
  endif
  call s:NetrwBookmarkMenu()
"  call Dret("s:NetrwBookHistHandler")
endfun

" ---------------------------------------------------------------------
" s:NetrwBookHistRead: this function reads bookmarks and history {{{2
"                      Sister function: s:NetrwBookHistSave()
fun! s:NetrwBookHistRead()
"  call Dfunc("s:NetrwBookHistRead()")
  if !exists("s:netrw_initbookhist")
   let home    = s:NetrwHome()
   let savefile= home."/.netrwbook"
   if filereadable(savefile)
"    call Decho("sourcing .netrwbook")
    exe "keepj so ".savefile
   endif
   if g:netrw_dirhistmax > 0
    let savefile= home."/.netrwhist"
    if filereadable(savefile)
"    call Decho("sourcing .netrwhist")
     exe "keepj so ".savefile
    endif
    let s:netrw_initbookhist= 1
    au VimLeave * call s:NetrwBookHistSave()
   endif
  endif
"  call Dret("s:NetrwBookHistRead")
endfun

" ---------------------------------------------------------------------
" s:NetrwBookHistSave: this function saves bookmarks and history {{{2
"                      Sister function: s:NetrwBookHistRead()
"                      I used to do this via viminfo but that appears to
"                      be unreliable for long-term storage
fun! s:NetrwBookHistSave()
"  call Dfunc("s:NetrwBookHistSave() dirhistmax=".g:netrw_dirhistmax)
  if g:netrw_dirhistmax <= 0
"   call Dret("s:NetrwBookHistSave : dirhistmax=".g:netrw_dirhistmax)
   return
  endif

  let savefile= s:NetrwHome()."/.netrwhist"
  1split
  call s:NetrwEnew()
  setlocal cino= com= cpo-=aA fo=nroql2 tw=0 report=10000 noswf
  setlocal nocin noai noci magic nospell nohid wig= noaw
  setlocal ma noro write
  if exists("&acd") | setlocal noacd | endif
  sil! keepj %d

  " save .netrwhist -- no attempt to merge
  sil! file .netrwhist
  call setline(1,"let g:netrw_dirhistmax  =".g:netrw_dirhistmax)
  call setline(2,"let g:netrw_dirhist_cnt =".g:netrw_dirhist_cnt)
  let lastline = line("$")
  let cnt      = 1
  while cnt <= g:netrw_dirhist_cnt
   call setline((cnt+lastline),'let g:netrw_dirhist_'.cnt."='".g:netrw_dirhist_{cnt}."'")
   let cnt= cnt + 1
  endwhile
  exe "sil! w! ".savefile

  sil keepj %d
  if exists("g:netrw_bookmarklist") && g:netrw_bookmarklist != []
   " merge and write .netrwbook
   let savefile= s:NetrwHome()."/.netrwbook"

   if filereadable(savefile)
    let booklist= deepcopy(g:netrw_bookmarklist)
    exe "sil keepj so ".savefile
    for bdm in booklist
     if index(g:netrw_bookmarklist,bdm) == -1
      call add(g:netrw_bookmarklist,bdm)
     endif
    endfor
    call sort(g:netrw_bookmarklist)
    exe "sil! w! ".savefile
   endif

   " construct and save .netrwbook
   call setline(1,"let g:netrw_bookmarklist= ".string(g:netrw_bookmarklist))
   exe "sil! w! ".savefile
  endif
  let bgone= bufnr("%")
  q!
  exe bgone."bwipe!"

"  call Dret("s:NetrwBookHistSave")
endfun

" ---------------------------------------------------------------------
" s:NetrwBrowse: This function uses the command in g:netrw_list_cmd to provide a {{{2
"  list of the contents of a local or remote directory.  It is assumed that the
"  g:netrw_list_cmd has a string, USEPORT HOSTNAME, that needs to be substituted
"  with the requested remote hostname first.
fun! s:NetrwBrowse(islocal,dirname)
  if !exists("w:netrw_liststyle")|let w:netrw_liststyle= g:netrw_liststyle|endif
"  call Dfunc("s:NetrwBrowse(islocal=".a:islocal." dirname<".a:dirname.">) liststyle=".w:netrw_liststyle." ".g:loaded_netrw." buf#".bufnr("%")."<".bufname("%")."> win#".winnr())
"  call Decho("tab#".tabpagenr()." win#".winnr())
"  call Dredir("ls!")
  if !exists("s:netrw_initbookhist")
   keepj call s:NetrwBookHistRead()
  endif

  " simplify the dirname (especially for ".."s in dirnames)
  if a:dirname !~ '^\a\+://'
   let dirname= simplify(a:dirname)
  else
   let dirname= a:dirname
  endif

  if exists("s:netrw_skipbrowse")
   unlet s:netrw_skipbrowse
"   call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"   call Dret("s:NetrwBrowse : s:netrw_skipbrowse=".s:netrw_skipbrowse)
   return
  endif
  if !exists("*shellescape")
   keepj call netrw#ErrorMsg(s:ERROR,"netrw can't run -- your vim is missing shellescape()",69)
"   call Dret("s:NetrwBrowse : missing shellescape()")
   return
  endif
  if !exists("*fnameescape")
   keepj call netrw#ErrorMsg(s:ERROR,"netrw can't run -- your vim is missing fnameescape()",70)
"   call Dret("s:NetrwBrowse : missing fnameescape()")
   return
  endif

  call s:NetrwOptionSave("w:")                                                                                                            

  " re-instate any marked files
  if exists("s:netrwmarkfilelist_{bufnr('%')}")
"   call Decho("clearing marked files")
   exe "2match netrwMarkFile /".s:netrwmarkfilemtch_{bufnr("%")}."/"
  endif

  if a:islocal && exists("w:netrw_acdkeep") && w:netrw_acdkeep
"   call Decho("handle w:netrw_acdkeep:")
"   call Decho("keepjumps lcd ".fnameescape(dirname)." (due to w:netrw_acdkeep=".w:netrw_acdkeep." - acd=".&acd.")")
   exe 'keepj lcd '.fnameescape(dirname)
   call s:NetrwSafeOptions()
"   call Decho("getcwd<".getcwd().">")

  elseif !a:islocal && dirname !~ '[\/]$' && dirname !~ '^"'
   " looks like a regular file, attempt transfer
"   call Decho("attempt transfer as regular file<".dirname.">")

   " remove any filetype indicator from end of dirname, except for the {{{3
   " "this is a directory" indicator (/).
   " There shouldn't be one of those here, anyway.
   let path= substitute(dirname,'[*=@|]\r\=$','','e')
"   call Decho("new path<".path.">")
   call s:RemotePathAnalysis(dirname)

   " remote-read the requested file into current buffer {{{3
   keepj mark '
   call s:NetrwEnew(dirname)
   call s:NetrwSafeOptions()
   setlocal ma noro
"   call Decho("setlocal ma noro")
   let b:netrw_curdir= dirname
"   call Decho("exe sil! keepalt file ".fnameescape(s:method."://".s:user.s:machine."/".s:path)." (bt=".&bt.")")
   exe "sil! keepj keepalt file ".fnameescape(s:method."://".s:user.s:machine."/".s:path)
   exe "sil! keepj keepalt doau BufReadPre ".fnameescape(s:fname)
   sil call netrw#NetRead(2,s:method."://".s:user.s:machine."/".s:path)
   if s:path !~ '.tar.bz2$' && s:path !~ '.tar.gz' && s:path !~ '.tar.xz' && s:path !~ '.txz'
    " netrw.vim and tar.vim have already handled decompression of the tarball; avoiding gzip.vim error
    exe "sil keepj keepalt doau BufReadPost ".fnameescape(s:fname)
   endif

   " save certain window-oriented variables into buffer-oriented variables {{{3
   call s:SetBufWinVars()
   call s:NetrwOptionRestore("w:")
   setlocal ma nomod

"   call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"   call Dret("s:NetrwBrowse : file<".s:fname.">")
   return
  endif

  " use buffer-oriented WinVars if buffer variables exist but associated window variables don't {{{3
  call s:UseBufWinVars()

  " set up some variables {{{3
  let b:netrw_browser_active = 1
  let dirname                = dirname
  let s:last_sort_by         = g:netrw_sort_by

  " set up menu {{{3
  keepj call s:NetrwMenu(1)

  " set up buffer {{{3
  let reusing= s:NetrwGetBuffer(a:islocal,dirname)
  " maintain markfile highlighting
  if exists("s:netrwmarkfilemtch_{bufnr('%')}") && s:netrwmarkfilemtch_{bufnr("%")} != ""
"   call Decho("bufnr(%)=".bufnr('%'))
"   call Decho("exe 2match netrwMarkFile /".s:netrwmarkfilemtch_{bufnr("%")}."/")
   exe "2match netrwMarkFile /".s:netrwmarkfilemtch_{bufnr("%")}."/"
  else
"   call Decho("2match none")
   2match none
  endif
  if reusing
   call s:NetrwOptionRestore("w:")
   setlocal noma nomod nowrap
"   call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"   call Dret("s:NetrwBrowse : re-using buffer")
   return
  endif

  " set b:netrw_curdir to the new directory name {{{3
"  call Decho("set b:netrw_curdir to the new directory name:  (buf#".bufnr("%").")")
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
"    call Decho("handle g:netrw_keepdir=".g:netrw_keepdir.": getcwd<".getcwd()."> acd=".&acd)
"    call Decho("l:acd".(exists("&l:acd")? "=".&l:acd : " doesn't exist"))
    if !exists("&l:acd") || !&l:acd
"     call Decho('exe keepjumps lcd '.fnameescape(b:netrw_curdir))
     try
      exe 'keepj lcd '.fnameescape(b:netrw_curdir)
     catch /^Vim\%((\a\+)\)\=:E472/
      call netrw#ErrorMsg(s:ERROR,"unable to change directory to <".b:netrw_curdir."> (permissions?)",61)
      if exists("w:netrw_prvdir")
       let b:netrw_curdir= w:netrw_prvdir
      else
       call s:NetrwOptionRestore("w:")
       setlocal noma nomod nowrap
       let b:netrw_curdir= dirname
"       call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"       call Dret("s:NetrwBrowse : reusing buffer#".(exists("bufnum")? bufnum : 'N/A')."<".dirname."> getcwd<".getcwd().">")
       return
      endif
     endtry
    endif
   endif

  " --------------------------------
  " remote handling: {{{3
  " --------------------------------
  else
"   call Decho("remote only:")

   " analyze dirname and g:netrw_list_cmd {{{4
"   call Decho("b:netrw_curdir<".(exists("b:netrw_curdir")? b:netrw_curdir : "doesn't exist")."> dirname<".dirname.">")
   if dirname =~ "^NetrwTreeListing\>"
    let dirname= b:netrw_curdir
"    call Decho("(dirname was ".dirname.") dirname<".dirname.">")
   elseif exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir")
    let dirname= substitute(b:netrw_curdir,'\\','/','g')
    if dirname !~ '/$'
     let dirname= dirname.'/'
    endif
    let b:netrw_curdir = dirname
"    call Decho("(liststyle is TREELIST) dirname<".dirname.">")
   else
    let dirname = substitute(dirname,'\\','/','g')
"    call Decho("(normal) dirname<".dirname.">")
   endif

   let dirpat  = '^\(\w\{-}\)://\(\w\+@\)\=\([^/]\+\)/\(.*\)$'
   if dirname !~ dirpat
    if !exists("g:netrw_quiet")
     keepj call netrw#ErrorMsg(s:ERROR,"netrw doesn't understand your dirname<".dirname.">",20)
    endif
    keepj call s:NetrwOptionRestore("w:")
    setlocal noma nomod nowrap
"    call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"    call Dret("s:NetrwBrowse : badly formatted dirname<".dirname.">")
    return
   endif
   let b:netrw_curdir= dirname
"   call Decho("b:netrw_curdir<".b:netrw_curdir."> (remote)")
  endif  " (additional remote handling)

  " -----------------------
  " Directory Listing: {{{3
  " -----------------------
  keepj call s:NetrwMaps(a:islocal)
  keepj call s:PerformListing(a:islocal)
  if v:version >= 700 && has("balloon_eval") && &l:bexpr == ""
   let &l:bexpr= "netrw#NetrwBalloonHelp()"
   set beval
  endif

  " The s:LocalBrowseShellCmdRefresh() function is called by an autocmd
  " installed by s:LocalFastBrowser() when g:netrw_fastbrowse <= 1 (ie. slow, medium speed).
  " However, s:NetrwBrowse() causes the ShellCmdPost event itself to fire once; setting
  " the variable below avoids that second refresh of the screen.  The s:LocalBrowseShellCmdRefresh()
  " function gets called due to that autocmd; it notices that the following variable is set
  " and skips the refresh and sets s:locbrowseshellcmd to zero. Oct 13, 2008
  let s:locbrowseshellcmd= 1

"  call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"  call Dret("s:NetrwBrowse : did PerformListing  ft<".&ft.">")
  return
endfun

" ---------------------------------------------------------------------
" s:NetrwFileInfo: supports qf (query for file information) {{{2
fun! s:NetrwFileInfo(islocal,fname)
"  call Dfunc("s:NetrwFileInfo(islocal=".a:islocal." fname<".a:fname.">)")
  if a:islocal
   if (has("unix") || has("macunix")) && executable("/bin/ls")
    if exists("b:netrw_curdir")
"     call Decho('using ls with b:netrw_curdir<'.b:netrw_curdir.'>')
     if b:netrw_curdir =~ '/$'
      echo system("/bin/ls -lsad ".shellescape(b:netrw_curdir.a:fname))
     else
      echo system("/bin/ls -lsad ".shellescape(b:netrw_curdir."/".a:fname))
     endif
    else
"     call Decho('using ls '.a:fname." using cwd<".getcwd().">")
     echo system("/bin/ls -lsad ".shellescape(a:fname))
    endif
   else
    " use vim functions to return information about file below cursor
"    call Decho("using vim functions to query for file info")
    if !isdirectory(a:fname) && !filereadable(a:fname) && a:fname =~ '[*@/]'
     let fname= substitute(a:fname,".$","","")
    else
     let fname= a:fname
    endif
    let t  = getftime(fname)
    let sz = getfsize(fname)
    echo a:fname.":  ".sz."  ".strftime(g:netrw_timefmt,getftime(fname))
"    call Decho(fname.":  ".sz."  ".strftime(g:netrw_timefmt,getftime(fname)))
   endif
  else
   echo "sorry, \"qf\" not supported yet for remote files"
  endif
"  call Dret("s:NetrwFileInfo")
endfun

" ---------------------------------------------------------------------
" s:NetrwGetBuffer: {{{2
"   returns 0=cleared buffer
"           1=re-used buffer
fun! s:NetrwGetBuffer(islocal,dirname)
"  call Dfunc("s:NetrwGetBuffer(islocal=".a:islocal." dirname<".a:dirname.">) liststyle=".g:netrw_liststyle)
  let dirname= a:dirname

  " re-use buffer if possible {{{3
"  call Decho("--re-use a buffer if possible--")
  if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
   " find NetrwTreeList buffer if there is one
"   call Decho("find NetrwTreeList buffer if there is one")
   if exists("w:netrw_treebufnr") && w:netrw_treebufnr > 0
"    call Decho("  re-use w:netrw_treebufnr=".w:netrw_treebufnr)
    let eikeep= &ei
    set ei=all
    exe "sil! b ".w:netrw_treebufnr
    let &ei= eikeep
"    call Dret("s:NetrwGetBuffer : bufnum#".w:netrw_treebufnr."<NetrwTreeListing>")
    return
   endif
   let bufnum= -1
"   call Decho("  liststyle=TREE but w:netrw_treebufnr doesn't exist")

  else
   " find buffer number of buffer named precisely the same as dirname {{{3
"   call Decho("--find buffer numnber of buffer named precisely the same as dirname--")
"   call Dredir("ls!")

   " get dirname and associated buffer number
   let bufnum  = bufnr(escape(dirname,'\'))
"   call Decho("  find buffer<".dirname.">'s number ")
"   call Decho("  bufnr(dirname<".escape(dirname,'\').">)=".bufnum)

   if bufnum < 0 && dirname !~ '/$'
    " try appending a trailing /
"    call Decho("  try appending a trailing / to dirname<".dirname.">")
    let bufnum= bufnr(escape(dirname.'/','\'))
    if bufnum > 0
     let dirname= dirname.'/'
    endif
   endif

   if bufnum < 0 && dirname =~ '/$'
    " try removing a trailing /
"    call Decho("  try removing a trailing / from dirname<".dirname.">")
    let bufnum= bufnr(escape(substitute(dirname,'/$','',''),'\'))
    if bufnum > 0
     let dirname= substitute(dirname,'/$','','')
    endif
   endif

"   call Decho("  findbuf1: bufnum=bufnr('".dirname."')=".bufnum." bufname(".bufnum.")<".bufname(bufnum)."> (initial)")
   " note: !~ was used just below, but that means using ../ to go back would match (ie. abc/def/  and abc/ matches)
   if bufnum > 0 && bufname(bufnum) != dirname && bufname(bufnum) != '.'
    " handle approximate matches
"    call Decho("  handling approx match: bufnum#".bufnum."<".bufname(bufnum)."> approx-dirname<".dirname.">")
    let ibuf    = 1
    let buflast = bufnr("$")
"    call Decho("  findbuf2: buflast=bufnr($)=".buflast)
    while ibuf <= buflast
     let bname= substitute(bufname(ibuf),'\\','/','g')
     let bname= substitute(bname,'.\zs/$','','')
"     call Decho("  findbuf3: while [ibuf=",ibuf."]<=[buflast=".buflast."]: dirname<".dirname."> bname=bufname(".ibuf.")<".bname.">")
     if bname != '' && dirname =~ '/'.bname.'/\=$' && dirname !~ '^/'
      " bname is not empty
      " dirname ends with bname,
      " dirname doesn't start with /, so its not a absolute path
"      call Decho("  findbuf3a: passes test 1 : dirname<".dirname.'> =~ /'.bname.'/\=$ && dirname !~ ^/')
      break
     endif
     if bname =~ '^'.dirname.'/\=$'
      " bname begins with dirname
"      call Decho('  findbuf3b: passes test 2 : bname<'.bname.'>=~^'.dirname.'/\=$')
      break
     endif
     if dirname =~ '^'.bname.'/$'
"      call Decho('  findbuf3c: passes test 3 : dirname<'.dirname.'>=~^'.bname.'/$')
      break
     endif
     if bname != '' && dirname =~ '/'.bname.'$' && bname == bufname("%") && line("$") == 1
"      call Decho('  findbuf3d: passes test 4 : dirname<'.dirname.'>=~ /'.bname.'$')
      break
     endif
     let ibuf= ibuf + 1
    endwhile
    if ibuf > buflast
     let bufnum= -1
    else
     let bufnum= ibuf
    endif
"    call Decho("  findbuf4: bufnum=".bufnum." (ibuf=".ibuf." buflast=".buflast.")")
   endif
  endif

  " get enew buffer and name it -or- re-use buffer {{{3
  sil! keepj mark '
  if bufnum < 0 || !bufexists(bufnum)
"   call Decho("--get enew buffer and name it (bufexists([bufnum=".bufnum."])=".bufexists(bufnum).")")
   call s:NetrwEnew(dirname)
"   call Decho("  got enew buffer#".bufnr("%")." (altbuf<".expand("#").">)")
   " name the buffer
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
    " Got enew buffer; transform into a NetrwTreeListing
"    call Decho("--transform enew buffer#".bufnr("%")." into a NetrwTreeListing --")
    if !exists("s:netrw_treelistnum")
     let s:netrw_treelistnum= 1
    else
     let s:netrw_treelistnum= s:netrw_treelistnum + 1
    endif
    let w:netrw_treebufnr= bufnr("%")
"    call Decho("  exe sil! keepalt file NetrwTreeListing ".fnameescape(s:netrw_treelistnum))
    exe 'sil! keepalt file NetrwTreeListing\ '.fnameescape(s:netrw_treelistnum)
    set bt=nofile noswf
    nnoremap <silent> <buffer> [	:sil call <SID>TreeListMove('[')<cr>
    nnoremap <silent> <buffer> ]	:sil call <SID>TreeListMove(']')<cr>
    nnoremap <silent> <buffer> [[       :sil call <SID>TreeListMove('[')<cr>
    nnoremap <silent> <buffer> ]]       :sil call <SID>TreeListMove(']')<cr>
"    call Decho("  tree listing#".s:netrw_treelistnum." bufnr=".w:netrw_treebufnr)
   else
"    let v:errmsg= "" " Decho
    let escdirname= fnameescape(dirname)
"    call Decho("  errmsg<".v:errmsg."> bufnr(escdirname<".escdirname.">)=".bufnr(escdirname)." bufname()<".bufname(bufnr(escdirname)).">")
"    call Decho('  exe sil! keepalt file '.escdirname)
"    let v:errmsg= "" " Decho
    exe 'sil! keepalt file '.escdirname
"    call Decho("  errmsg<".v:errmsg."> bufnr(".escdirname.")=".bufnr(escdirname)."<".bufname(bufnr(escdirname)).">")
   endif
"   call Decho("  named enew buffer#".bufnr("%")."<".bufname("%").">")

  else " Re-use the buffer
"   call Decho("--re-use buffer#".bufnum." (bufexists([bufnum=".bufnum."])=".bufexists(bufnum).")")
   let eikeep= &ei
   set ei=all
   if getline(2) =~ '^" Netrw Directory Listing'
"    call Decho("  re-use buffer#".bufnum."<".((bufnum > 0)? bufname(bufnum) : "")."> using:  keepalt b ".bufnum)
    exe "sil! keepalt b ".bufnum
   else
"    call Decho("  reusing buffer#".bufnum."<".((bufnum > 0)? bufname(bufnum) : "")."> using:  b ".bufnum)
    exe "sil! b ".bufnum
   endif
   if bufname("%") == '.'
"    call Decho("exe sil! keepalt file ".fnameescape(getcwd()))
    exe "sil! keepalt file ".fnameescape(getcwd())
   endif
   let &ei= eikeep
   if line("$") <= 1
    keepj call s:NetrwListSettings(a:islocal)
"    call Dret("s:NetrwGetBuffer 0 : re-using buffer#".bufnr("%").", but its empty, so refresh it")
    return 0
   elseif exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
"    call Decho("--re-use tree listing--")
"    call Decho("  clear buffer<".expand("%")."> with :%d")
    sil keepj %d
    keepj call s:NetrwListSettings(a:islocal)
"    call Dret("s:NetrwGetBuffer 0 : re-using buffer#".bufnr("%").", but treelist mode always needs a refresh")
    return 0
   else
"    call Dret("s:NetrwGetBuffer 1 : buf#".bufnr("%"))
    return 1
   endif
  endif

  " do netrw settings: make this buffer not-a-file, modifiable, not line-numbered, etc {{{3
  "     fastbrowse  Local  Remote   Hiding a buffer implies it may be re-used (fast)
  "  slow   0         D      D      Deleting a buffer implies it will not be re-used (slow)
  "  med    1         D      H
  "  fast   2         H      H
"  call Decho("--do netrw settings: make this buffer#".bufnr("%")." not-a-file, modifiable, not line-numbered, etc--")
  let fname= expand("%")
  keepj call s:NetrwListSettings(a:islocal)
"  call Decho("exe sil! keepalt file ".fnameescape(fname))
  exe "sil! keepj keepalt file ".fnameescape(fname)

  " delete all lines from buffer {{{3
"  call Decho("--delete all lines from buffer--")
"  call Decho("  clear buffer<".expand("%")."> with :%d")
  sil! keepalt keepj %d

"  call Dret("s:NetrwGetBuffer 0 : buf#".bufnr("%"))
  return 0
endfun

" ---------------------------------------------------------------------
" s:NetrwGetcwd: get the current directory. {{{2
"   Change backslashes to forward slashes, if any.
"   If doesc is true, escape certain troublesome characters
fun! s:NetrwGetcwd(doesc)
"  call Dfunc("NetrwGetcwd(doesc=".a:doesc.")")
  let curdir= substitute(getcwd(),'\\','/','ge')
  if curdir !~ '[\/]$'
   let curdir= curdir.'/'
  endif
  if a:doesc
   let curdir= fnameescape(curdir)
  endif
"  call Dret("NetrwGetcwd <".curdir.">")
  return curdir
endfun

" ---------------------------------------------------------------------
"  s:NetrwGetWord: it gets the directory/file named under the cursor {{{2
fun! s:NetrwGetWord()
"  call Dfunc("s:NetrwGetWord() line#".line(".")." liststyle=".g:netrw_liststyle." virtcol=".virtcol("."))
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
   keepj norm! 0
   let dirname= "./"
   let curline= getline('.')

   if curline =~ '"\s*Sorted by\s'
    keepj norm s
    let s:netrw_skipbrowse= 1
    echo 'Pressing "s" also works'

   elseif curline =~ '"\s*Sort sequence:'
    let s:netrw_skipbrowse= 1
    echo 'Press "S" to edit sorting sequence'

   elseif curline =~ '"\s*Quick Help:'
    keepj norm ?
    let s:netrw_skipbrowse= 1
    echo 'Pressing "?" also works'

   elseif curline =~ '"\s*\%(Hiding\|Showing\):'
    keepj norm a
    let s:netrw_skipbrowse= 1
    echo 'Pressing "a" also works'

   elseif line("$") > w:netrw_bannercnt
    exe 'sil keepj '.w:netrw_bannercnt
   endif

  elseif w:netrw_liststyle == s:THINLIST
"   call Decho("thin column handling")
   keepj norm! 0
   let dirname= getline('.')

  elseif w:netrw_liststyle == s:LONGLIST
"   call Decho("long column handling")
   keepj norm! 0
   let dirname= substitute(getline('.'),'^\(\%(\S\+ \)*\S\+\).\{-}$','\1','e')

  elseif w:netrw_liststyle == s:TREELIST
"   call Decho("treelist handling")
   let dirname= substitute(getline('.'),'^\(| \)*','','e')

  else
"   call Decho("obtain word from wide listing")
   let dirname= getline('.')

   if !exists("b:netrw_cpf")
    let b:netrw_cpf= 0
    exe 'sil keepj '.w:netrw_bannercnt.',$g/^./if virtcol("$") > b:netrw_cpf|let b:netrw_cpf= virtcol("$")|endif'
    call histdel("/",-1)
"   call Decho("computed cpf=".b:netrw_cpf)
   endif

"   call Decho("buf#".bufnr("%")."<".bufname("%").">")
   let filestart = (virtcol(".")/b:netrw_cpf)*b:netrw_cpf
"   call Decho("filestart= ([virtcol=".virtcol(".")."]/[b:netrw_cpf=".b:netrw_cpf."])*b:netrw_cpf=".filestart."  bannercnt=".w:netrw_bannercnt)
"   call Decho("1: dirname<".dirname.">")
   if filestart == 0
    keepj norm! 0ma
   else
    call cursor(line("."),filestart+1)
    keepj norm! ma
   endif
   let rega= @a
   let eofname= filestart + b:netrw_cpf + 1
   if eofname <= col("$")
    call cursor(line("."),filestart+b:netrw_cpf+1)
    keepj norm! "ay`a
   else
    keepj norm! "ay$
   endif
   let dirname = @a
   let @a      = rega
"   call Decho("2: dirname<".dirname.">")
   let dirname= substitute(dirname,'\s\+$','','e')
"   call Decho("3: dirname<".dirname.">")
  endif

  " symlinks are indicated by a trailing "@".  Remove it before further processing.
  let dirname= substitute(dirname,"@$","","")

  " executables are indicated by a trailing "*".  Remove it before further processing.
  let dirname= substitute(dirname,"\*$","","")

"  call Dret("s:NetrwGetWord <".dirname.">")
  return dirname
endfun

" ---------------------------------------------------------------------
" s:NetrwListSettings: make standard settings for a netrw listing {{{2
fun! s:NetrwListSettings(islocal)
"  call Dfunc("s:NetrwListSettings(islocal=".a:islocal.")")
  let fname= bufname("%")
"  call Decho("setlocal bt=nofile nobl ma nonu nowrap noro")
  setlocal bt=nofile nobl ma nonu nowrap noro
"  call Decho("exe sil! keepalt file ".fnameescape(fname))
  exe "sil! keepalt file ".fnameescape(fname)
  if g:netrw_use_noswf
   setlocal noswf
  endif
"  call Dredir("ls!")
"  call Decho("exe setlocal ts=".g:netrw_maxfilenamelen)
  exe "setlocal ts=".g:netrw_maxfilenamelen
  setlocal isk+=.,~,-
  if g:netrw_fastbrowse > a:islocal
   setlocal bh=hide
  else
   setlocal bh=delete
  endif
"  call Dret("s:NetrwListSettings")
endfun

" ---------------------------------------------------------------------
"  s:NetrwListStyle: {{{2
"  islocal=0: remote browsing
"         =1: local browsing
fun! s:NetrwListStyle(islocal)
"  call Dfunc("NetrwListStyle(islocal=".a:islocal.") w:netrw_liststyle=".w:netrw_liststyle)
  let fname             = s:NetrwGetWord()
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
   keepj call netrw#ErrorMsg(s:WARNING,"bad value for g:netrw_liststyle (=".w:netrw_liststyle.")",46)
   let g:netrw_liststyle = s:THINLIST
   let w:netrw_liststyle = g:netrw_liststyle
   let g:netrw_list_cmd  = substitute(g:netrw_list_cmd,' -l','','ge')
  endif
  setlocal ma noro
"  call Decho("setlocal ma noro")

  " clear buffer - this will cause NetrwBrowse/LocalBrowseCheck to do a refresh
"  call Decho("clear buffer<".expand("%")."> with :%d")
  sil! keepj %d
  " following prevents tree listing buffer from being marked "modified"
  setlocal nomod

  " refresh the listing
"  call Decho("refresh the listing")
  let svpos= netrw#NetrwSavePosn()
  keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)
  keepj call s:NetrwCursor()

  " keep cursor on the filename
  sil! keepj $
  let result= search('\%(^\%(|\+\s\)\=\|\s\{2,}\)\zs'.escape(fname,'.\[]*$^').'\%(\s\{2,}\|$\)','bc')
"  call Decho("search result=".result." w:netrw_bannercnt=".(exists("w:netrw_bannercnt")? w:netrw_bannercnt : 'N/A'))
  if result <= 0 && exists("w:netrw_bannercnt")
   exe "sil! keepj ".w:netrw_bannercnt
  endif

"  call Dret("NetrwListStyle".(exists("w:netrw_liststyle")? ' : w:netrw_liststyle='.w:netrw_liststyle : ""))
endfun

" ---------------------------------------------------------------------
" s:NetrwBannerCtrl: toggles the display of the banner {{{2
fun! s:NetrwBannerCtrl(islocal)
"  call Dfunc("s:NetrwBannerCtrl(islocal=".a:islocal.") g:netrw_banner=".g:netrw_banner)

  " toggle the banner (enable/suppress)
  let g:netrw_banner= !g:netrw_banner

  " refresh the listing
  let svpos= netrw#NetrwSavePosn()
  call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))

  " keep cursor on the filename
  let fname= s:NetrwGetWord()
  sil keepj $
  let result= search('\%(^\%(|\+\s\)\=\|\s\{2,}\)\zs'.escape(fname,'.\[]*$^').'\%(\s\{2,}\|$\)','bc')
"  call Decho("search result=".result." w:netrw_bannercnt=".(exists("w:netrw_bannercnt")? w:netrw_bannercnt : 'N/A'))
  if result <= 0 && exists("w:netrw_bannercnt")
   exe "keepj ".w:netrw_bannercnt
  endif
"  call Dret("s:NetrwBannerCtrl : g:netrw_banner=".g:netrw_banner)
endfun

" ---------------------------------------------------------------------
" s:NetrwBookmarkMenu: Uses menu priorities {{{2
"                      .2.[cnt] for bookmarks, and
"                      .3.[cnt] for history
"                      (see s:NetrwMenu())
fun! s:NetrwBookmarkMenu()
  if !exists("s:netrw_menucnt")
   return
  endif
"  call Dfunc("NetrwBookmarkMenu()  histcnt=".g:netrw_dirhist_cnt." menucnt=".s:netrw_menucnt)

  " the following test assures that gvim is running, has menus available, and has menus enabled.
  if has("gui") && has("menu") && has("gui_running") && &go =~# 'm' && g:netrw_menu
   if exists("g:NetrwTopLvlMenu")
"    call Decho("removing ".g:NetrwTopLvlMenu."Bookmarks menu item(s)")
    exe 'sil! unmenu '.g:NetrwTopLvlMenu.'Bookmarks'
    exe 'sil! unmenu '.g:NetrwTopLvlMenu.'Bookmarks\ and\ History.Bookmark\ Delete'
   endif
   if !exists("s:netrw_initbookhist")
    call s:NetrwBookHistRead()
   endif

   " show bookmarked places
   if exists("g:netrw_bookmarklist") && g:netrw_bookmarklist != []
    let cnt= 1
    for bmd in g:netrw_bookmarklist
"     call Decho('sil! menu '.g:NetrwMenuPriority.".2.".cnt." ".g:NetrwTopLvlMenu.'Bookmark.'.bmd.'	:e '.bmd)
     let bmd= escape(bmd,g:netrw_menu_escape)

     " show bookmarks for goto menu
     exe 'sil! menu '.g:NetrwMenuPriority.".2.".cnt." ".g:NetrwTopLvlMenu.'Bookmarks.'.bmd.'	:e '.bmd."\<cr>"

     " show bookmarks for deletion menu
     exe 'sil! menu '.g:NetrwMenuPriority.".8.2.".cnt." ".g:NetrwTopLvlMenu.'Bookmarks\ and\ History.Bookmark\ Delete.'.bmd.'	'.cnt."mB"
     let cnt= cnt + 1
    endfor

   endif

   " show directory browsing history
   if g:netrw_dirhistmax > 0
    let cnt     = g:netrw_dirhist_cnt
    let first   = 1
    let histcnt = 0
    while ( first || cnt != g:netrw_dirhist_cnt )
     let histcnt  = histcnt + 1
     let priority = g:netrw_dirhist_cnt + histcnt
     if exists("g:netrw_dirhist_{cnt}")
      let histdir= escape(g:netrw_dirhist_{cnt},g:netrw_menu_escape)
"     call Decho('sil! menu '.g:NetrwMenuPriority.".3.".priority." ".g:NetrwTopLvlMenu.'History.'.histdir.'	:e '.histdir)
      exe 'sil! menu '.g:NetrwMenuPriority.".3.".priority." ".g:NetrwTopLvlMenu.'History.'.histdir.'	:e '.histdir."\<cr>"
     endif
     let first = 0
     let cnt   = ( cnt - 1 ) % g:netrw_dirhistmax
     if cnt < 0
      let cnt= cnt + g:netrw_dirhistmax
     endif
    endwhile
   endif

  endif
"  call Dret("NetrwBookmarkMenu")
endfun

" ---------------------------------------------------------------------
"  s:NetrwBrowseChgDir: constructs a new directory based on the current {{{2
"                       directory and a new directory name.  Also, if the
"                       "new directory name" is actually a file,
"                       NetrwBrowseChgDir() edits the file.
fun! s:NetrwBrowseChgDir(islocal,newdir,...)
"  call Dfunc("s:NetrwBrowseChgDir(islocal=".a:islocal."> newdir<".a:newdir.">) a:0=".a:0." curpos<".string(getpos("."))."> b:netrw_curdir<".(exists("b:netrw_curdir")? b:netrw_curdir : "").">")

  if !exists("b:netrw_curdir")
   " Don't try to change-directory: this can happen, for example, when netrw#ErrorMsg has been called
   " and the current window is the NetrwMessage window.
"   call Decho("(NetrwBrowseChgDir) b:netrw_curdir doesn't exist!")
"   call Decho("getcwd<".getcwd().">")
"   call Dredir("ls!")
"   call Dret("s:NetrwBrowseChgDir")
   return
  endif

  keepj call s:NetrwOptionSave("s:")
  keepj call s:NetrwSafeOptions()
  let nbcd_curpos                = netrw#NetrwSavePosn()
  let s:nbcd_curpos_{bufnr('%')} = nbcd_curpos
  if (has("win32") || has("win95") || has("win64") || has("win16"))
   let dirname                   = substitute(b:netrw_curdir,'\\','/','ge')
  else
   let dirname= b:netrw_curdir
  endif
  let newdir    = a:newdir
  let dolockout = 0

  " set up o/s-dependent directory recognition pattern
  if has("amiga")
   let dirpat= '[\/:]$'
  else
   let dirpat= '[\/]$'
  endif
"  call Decho("dirname<".dirname.">  dirpat<".dirpat.">")

  if dirname !~ dirpat
   " apparently vim is "recognizing" that it is in a directory and
   " is removing the trailing "/".  Bad idea, so let's put it back.
   let dirname= dirname.'/'
"   call Decho("adjusting dirname<".dirname.">")
  endif

  if newdir !~ dirpat
   " ------------
   " edit a file:
   " ------------
"   call Decho('case "handling a file": newdir<'.newdir.'> !~ dirpat<'.dirpat.">")
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict") && newdir !~ '^\(/\|\a:\)'
    let dirname= s:NetrwTreeDir()
    if dirname =~ '/$'
     let dirname= dirname.newdir
    else
     let dirname= s:NetrwTreeDir()."/".newdir
    endif
"    call Decho("dirname<".dirname.">")
"    call Decho("tree listing")
   elseif newdir =~ '^\(/\|\a:\)'
    let dirname= newdir
   else
    let dirname= s:ComposePath(dirname,newdir)
   endif
"   call Decho("handling a file: dirname<".dirname."> (a:0=".a:0.")")
   " this lets NetrwBrowseX avoid the edit
   if a:0 < 1
"    call Decho("set up windows for editing<".fnameescape(dirname).">  didsplit=".(exists("s:didsplit")? s:didsplit : "doesn't exist"))
    keepj call s:NetrwOptionRestore("s:")
    if !exists("s:didsplit")
     if     g:netrw_browse_split == 1
      new
      if !&ea
       wincmd _
      endif
     elseif g:netrw_browse_split == 2
      rightb vert new
      if !&ea
       wincmd |
      endif
     elseif g:netrw_browse_split == 3
      tabnew
     elseif g:netrw_browse_split == 4
      if s:NetrwPrevWinOpen(2) == 3
"       call Dret("s:NetrwBrowseChgDir")
       return
      endif
     else
      " handling a file, didn't split, so remove menu
"      call Decho("handling a file+didn't split, so remove menu")
      call s:NetrwMenu(0)
      " optional change to window
      if g:netrw_chgwin >= 1
       exe "keepjumps ".g:netrw_chgwin."wincmd w"
      endif
     endif
    endif

    " the point where netrw actually edits the (local) file
    " if its local only: LocalBrowseCheck() doesn't edit a file, but NetrwBrowse() will
    if a:islocal
"     call Decho("edit local file: exe e! ".fnameescape(dirname))
     exe "e! ".fnameescape(dirname)
    else
"     call Decho("remote file: NetrwBrowse will edit it")
    endif
    let dolockout= 1

    " handle g:Netrw_funcref -- call external-to-netrw functions
    "   This code will handle g:Netrw_funcref as an individual function reference
    "   or as a list of function references.  It will ignore anything that's not
    "   a function reference.  See  :help Funcref  for information about function references.
    if exists("g:Netrw_funcref")
"     call Decho("handle optional Funcrefs")
     if type(g:Netrw_funcref) == 2
"      call Decho("handling a g:Netrw_funcref")
      keepj call g:Netrw_funcref()
     elseif type(g:Netrw_funcref) == 3
"      call Decho("handling a list of g:Netrw_funcrefs")
      for Fncref in g:Netrw_funcref
       if type(FncRef) == 2
        keepj call FncRef()
       endif
      endfor
     endif
    endif
   endif

  elseif newdir =~ '^/'
   " ---------------------------------
   " just go to the new directory spec
   " ---------------------------------
"   call Decho('case "just go to new directory spec": newdir<'.newdir.'>')
   let dirname= newdir
   keepj call s:SetRexDir(a:islocal,dirname)
   keepj call s:NetrwOptionRestore("s:")

  elseif newdir == './'
   " --------------------------
   " refresh the directory list
   " --------------------------
"   call Decho('case "refresh directory listing": newdir == "./"')
   keepj call s:SetRexDir(a:islocal,dirname)

  elseif newdir == '../'
   " -------------------
   " go up one directory
   " -------------------
"   call Decho('case "go up one directory": newdir == "../"')

   if w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict")
    " force a refresh
"    call Decho("clear buffer<".expand("%")."> with :%d")
    setlocal noro ma
"    call Decho("setlocal noro ma")
    keepj %d
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
   keepj call s:SetRexDir(a:islocal,dirname)

  elseif exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict")
"   call Decho('case liststyle is TREELIST and w:netrw_treedict exists')
   " force a refresh (for TREELIST, wait for NetrwTreeDir() to force the refresh)
   setlocal noro ma
"   call Decho("setlocal noro ma")
   if !(exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("b:netrw_curdir"))
"    call Decho("clear buffer<".expand("%")."> with :%d")
    keepj %d
   endif
   let treedir      = s:NetrwTreeDir()
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
"    call Decho("removed     entry<".treedir."> from treedict")
"    call Decho("yielding treedict<".string(w:netrw_treedict).">")
    let dirname= w:netrw_treetop
   else
    " go down one directory
    let dirname= substitute(treedir,'/*$','/','')
"    call Decho("go down one dir: treedir<".treedir.">")
   endif
   keepj call s:SetRexDir(a:islocal,dirname)
   let s:treeforceredraw = 1

  else
   " go down one directory
   let dirname= s:ComposePath(dirname,newdir)
"   call Decho("go down one dir: dirname<".dirname."> newdir<".newdir.">")
   keepj call s:SetRexDir(a:islocal,dirname)
  endif

  keepj call s:NetrwOptionRestore("s:")
  if dolockout
"   call Decho("filewritable(dirname<".dirname.">)=".filewritable(dirname))
   if filewritable(dirname)
"    call Decho("doing modification lockout settings: ma nomod noro")
    setlocal ma nomod noro
   else
"    call Decho("doing modification lockout settings: ma nomod ro")
    setlocal ma nomod ro
   endif
"   call Decho("setlocal ma nomod noro")
  endif

"  call Dret("s:NetrwBrowseChgDir <".dirname."> : curpos<".string(getpos(".")).">")
  return dirname
endfun

" ---------------------------------------------------------------------
" s:NetrwBrowseX:  (implements "x") executes a special "viewer" script or program for the {{{2
"              given filename; typically this means given their extension.
"              0=local, 1=remote
fun! netrw#NetrwBrowseX(fname,remote)
"  call Dfunc("NetrwBrowseX(fname<".a:fname."> remote=".a:remote.")")

  " special core dump handler
  if a:fname =~ '/core\(\.\d\+\)\=$'
   if exists("g:Netrw_corehandler")
    if type(g:Netrw_corehandler) == 2
     " g:Netrw_corehandler is a function reference (see :help Funcref)
"     call Decho("g:Netrw_corehandler is a funcref")
     call g:Netrw_corehandler(a:fname)
    elseif type(g:netrw_corehandler) == 3)
     " g:Netrw_corehandler is a List of function references (see :help Funcref)
"     call Decho("g:Netrw_corehandler is a List")
     for Fncref in g:Netrw_corehandler
      if type(FncRef) == 2
       call FncRef(a:fname)
      endif
     endfor
    endif
"    call Dret("NetrwBrowseX : coredump handler invoked")
    return
   endif
  endif

  " set up the filename
  " (lower case the extension, make a local copy of a remote file)
  let exten= substitute(a:fname,'.*\.\(.\{-}\)','\1','e')
  if has("win32") || has("win95") || has("win64") || has("win16")
   let exten= substitute(exten,'^.*$','\L&\E','')
  endif
"  call Decho("exten<".exten.">")

  " seems kde systems often have gnome-open due to dependencies, even though
  " gnome-open's subsidiary display tools are largely absent.  Kde systems
  " usually have "kdeinit" running, though...  (tnx Mikolaj Machowski)
  if !exists("s:haskdeinit")
   if has("unix")
    let s:haskdeinit= system("ps -e") =~ 'kdeinit' 
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
"   call Decho("a:remote=".a:remote.": create a local copy of <".a:fname.">")
   setlocal bh=delete
   call netrw#NetRead(3,a:fname)
   " attempt to rename tempfile
   let basename= substitute(a:fname,'^\(.*\)/\(.*\)\.\([^.]*\)$','\2','')
   let newname= substitute(s:netrw_tmpfile,'^\(.*\)/\(.*\)\.\([^.]*\)$','\1/'.basename.'.\3','')
"   call Decho("basename<".basename.">")
"   call Decho("newname <".newname.">")
   if rename(s:netrw_tmpfile,newname) == 0
    " renaming succeeded
    let fname= newname
   else
    " renaming failed
    let fname= s:netrw_tmpfile
   endif
  else
   let fname= a:fname
   " special ~ handler for local
   if fname =~ '^\~' && expand("$HOME") != ""
"    call Decho('invoking special ~ handler')
    let fname= substitute(fname,'^\~',expand("$HOME"),'')
   endif
  endif
"  call Decho("fname<".fname.">")
"  call Decho("exten<".exten."> "."netrwFileHandlers#NFH_".exten."():exists=".exists("*netrwFileHandlers#NFH_".exten))

  " set up redirection
  if &srr =~ "%s"
   if (has("win32") || has("win95") || has("win64") || has("win16"))
    let redir= substitute(&srr,"%s","nul","")
   else
    let redir= substitute(&srr,"%s","/dev/null","")
   endif
  elseif (has("win32") || has("win95") || has("win64") || has("win16"))
   let redir= &srr . "nul"
  else
   let redir= &srr . "/dev/null"
  endif
"  call Decho("redir{".redir."} srr{".&srr."}")

  " extract any viewing options.  Assumes that they're set apart by quotes.
  if exists("g:netrw_browsex_viewer")
"   call Decho("g:netrw_browsex_viewer<".g:netrw_browsex_viewer.">")
   if g:netrw_browsex_viewer =~ '\s'
    let viewer  = substitute(g:netrw_browsex_viewer,'\s.*$','','')
    let viewopt = substitute(g:netrw_browsex_viewer,'^\S\+\s*','','')." "
    let oviewer = ''
    let cnt     = 1
    while !executable(viewer) && viewer != oviewer
     let viewer  = substitute(g:netrw_browsex_viewer,'^\(\(^\S\+\s\+\)\{'.cnt.'}\S\+\)\(.*\)$','\1','')
     let viewopt = substitute(g:netrw_browsex_viewer,'^\(\(^\S\+\s\+\)\{'.cnt.'}\S\+\)\(.*\)$','\3','')." "
     let cnt     = cnt + 1
     let oviewer = viewer
"     call Decho("!exe: viewer<".viewer.">  viewopt<".viewopt.">")
    endwhile
   else
    let viewer  = g:netrw_browsex_viewer
    let viewopt = ""
   endif
"   call Decho("viewer<".viewer.">  viewopt<".viewopt.">")
  endif

  " execute the file handler
  if exists("g:netrw_browsex_viewer") && g:netrw_browsex_viewer == '-'
"   call Decho("g:netrw_browsex_viewer<".g:netrw_browsex_viewer.">")
   let ret= netrwFileHandlers#Invoke(exten,fname)

  elseif exists("g:netrw_browsex_viewer") && executable(viewer)
"   call Decho("g:netrw_browsex_viewer<".g:netrw_browsex_viewer.">")
"   call Decho("exe sil !".viewer." ".viewopt.shellescape(fname,1).redir)
   exe "sil !".viewer." ".viewopt.shellescape(fname,1).redir
   let ret= v:shell_error

  elseif has("win32") || has("win64")
   if executable("start")
"    call Decho('exe sil !start rundll32 url.dll,FileProtocolHandler '.shellescape(fname,1))
    exe 'sil !start rundll32 url.dll,FileProtocolHandler '.shellescape(fname,1)
   elseif executable("rundll32")
"    call Decho('exe sil !rundll32 url.dll,FileProtocolHandler '.shellescape(fname,1))
    exe 'sil !rundll32 url.dll,FileProtocolHandler '.shellescape(fname,1)
   else
    call netrw#ErrorMsg(s:WARNING,"rundll32 not on path",74)
   endif
   call inputsave()|call input("Press <cr> to continue")|call inputrestore()
   let ret= v:shell_error

  elseif has("unix") && executable("gnome-open") && !s:haskdeinit
"   call Decho("exe sil !gnome-open ".shellescape(fname,1)." ".redir)
   exe "sil !gnome-open ".shellescape(fname,1).redir
   let ret= v:shell_error

  elseif has("unix") && executable("kfmclient") && s:haskdeinit
"   call Decho("exe sil !kfmclient exec ".shellescape(fname,1)." ".redir)
   exe "sil !kfmclient exec ".shellescape(fname,1)." ".redir
   let ret= v:shell_error

  elseif has("macunix") && executable("open")
"   call Decho("exe sil !open ".shellescape(fname,1)." ".redir)
   exe "sil !open ".shellescape(fname,1)." ".redir
   let ret= v:shell_error

  else
   " netrwFileHandlers#Invoke() always returns 0
   let ret= netrwFileHandlers#Invoke(exten,fname)
  endif

  " if unsuccessful, attempt netrwFileHandlers#Invoke()
  if ret
   let ret= netrwFileHandlers#Invoke(exten,fname)
  endif

  " restoring redraw! after external file handlers
  redraw!

  " cleanup: remove temporary file,
  "          delete current buffer if success with handler,
  "          return to prior buffer (directory listing)
  "          Feb 12, 2008: had to de-activiate removal of
  "          temporary file because it wasn't getting seen.
"  if a:remote == 1 && fname != a:fname
"   call Decho("deleting temporary file<".fname.">")
"   call s:NetrwDelete(fname)
"  endif

  if a:remote == 1
   setlocal bh=delete bt=nofile
   if g:netrw_use_noswf
    setlocal noswf
   endif
   exe "sil! keepj norm! \<c-o>"
"   redraw!
  endif

"  call Dret("NetrwBrowseX")
endfun

" ---------------------------------------------------------------------
" s:NetrwChgPerm: (implements "gp") change file permission {{{2
fun! s:NetrwChgPerm(islocal,curdir)
"  call Dfunc("s:NetrwChgPerm(islocal=".a:islocal." curdir<".a:curdir.">)")
  call inputsave()
  let newperm= input("Enter new permission: ")
  call inputrestore()
  let chgperm= substitute(g:netrw_chgperm,'\<FILENAME\>',shellescape(expand("<cfile>")),'')
  let chgperm= substitute(chgperm,'\<PERM\>',shellescape(newperm),'')
"  call Decho("chgperm<".chgperm.">")
  call system(chgperm)
  if v:shell_error != 0
   keepj call netrw#ErrorMsg(1,"changing permission on file<".expand("<cfile>")."> seems to have failed",75)
  endif
  if a:islocal
   keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  endif
"  call Dret("s:NetrwChgPerm")
endfun

" ---------------------------------------------------------------------
" s:NetrwClearExplore: clear explore variables (if any) {{{2
fun! s:NetrwClearExplore()
"  call Dfunc("s:NetrwClearExplore()")
  2match none
  if exists("s:explore_match")        |unlet s:explore_match        |endif
  if exists("s:explore_indx")         |unlet s:explore_indx         |endif
  if exists("s:netrw_explore_prvdir") |unlet s:netrw_explore_prvdir |endif
  if exists("s:dirstarstar")          |unlet s:dirstarstar          |endif
  if exists("s:explore_prvdir")       |unlet s:explore_prvdir       |endif
  if exists("w:netrw_explore_indx")   |unlet w:netrw_explore_indx   |endif
  if exists("w:netrw_explore_listlen")|unlet w:netrw_explore_listlen|endif
  if exists("w:netrw_explore_list")   |unlet w:netrw_explore_list   |endif
  if exists("w:netrw_explore_bufnr")  |unlet w:netrw_explore_bufnr  |endif
"   redraw!
  echo " "
  echo " "
"  call Dret("s:NetrwClearExplore")
endfun

" ---------------------------------------------------------------------
" netrw#Explore: launch the local browser in the directory of the current file {{{2
"          indx:  == -1: Nexplore
"                 == -2: Pexplore
"                 ==  +: this is overloaded:
"                      * If Nexplore/Pexplore is in use, then this refers to the
"                        indx'th item in the w:netrw_explore_list[] of items which
"                        matched the */pattern **/pattern *//pattern **//pattern
"                      * If Hexplore or Vexplore, then this will override
"                        g:netrw_winsize to specify the qty of rows or columns the
"                        newly split window should have.
"          dosplit==0: the window will be split iff the current file has been modified
"          dosplit==1: the window will be split before running the local browser
"          style == 0: Explore     style == 1: Explore!
"                == 2: Hexplore    style == 3: Hexplore!
"                == 4: Vexplore    style == 5: Vexplore!
"                == 6: Texplore
fun! netrw#Explore(indx,dosplit,style,...)
"  call Dfunc("netrw#Explore(indx=".a:indx." dosplit=".a:dosplit." style=".a:style.",a:1<".a:1.">) &modified=".&modified." a:0=".a:0)
  if !exists("b:netrw_curdir")
   let b:netrw_curdir= getcwd()
"   call Decho("set b:netrw_curdir<".b:netrw_curdir."> (used getcwd)")
  endif
  let curdir     = simplify(b:netrw_curdir)
  let curfiledir = substitute(expand("%:p"),'^\(.*[/\\]\)[^/\\]*$','\1','e')
"  call Decho("curdir<".curdir.">  curfiledir<".curfiledir.">")

  " save registers
  sil! let keepregstar = @*
  sil! let keepregplus = @+
  sil! let keepregslash= @/

  " if dosplit or file has been modified
  if a:dosplit || &modified || a:style == 6
"   call Decho("case dosplit=".a:dosplit." modified=".&modified." a:style=".a:style.": dosplit or file has been modified")
   call s:SaveWinVars()
   let winsize= g:netrw_winsize
   if a:indx > 0
    let winsize= a:indx
   endif

   if a:style == 0      " Explore, Sexplore
"    call Decho("style=0: Explore or Sexplore")
    exe winsize."wincmd s"

   elseif a:style == 1  "Explore!, Sexplore!
"    call Decho("style=1: Explore! or Sexplore!")
    exe winsize."wincmd v"

   elseif a:style == 2  " Hexplore
"    call Decho("style=2: Hexplore")
    exe "bel ".winsize."wincmd s"

   elseif a:style == 3  " Hexplore!
"    call Decho("style=3: Hexplore!")
    exe "abo ".winsize."wincmd s"

   elseif a:style == 4  " Vexplore
"    call Decho("style=4: Vexplore")
    exe "lefta ".winsize."wincmd v"

   elseif a:style == 5  " Vexplore!
"    call Decho("style=5: Vexplore!")
    exe "rightb ".winsize."wincmd v"

   elseif a:style == 6  " Texplore
    call s:SaveBufVars()
"    call Decho("style  = 6: Texplore")
    exe "tabnew ".fnameescape(curdir)
    call s:RestoreBufVars()
   endif
   call s:RestoreWinVars()
"  else " Decho
"   call Decho("case a:dosplit=".a:dosplit." AND modified=".&modified." AND a:style=".a:style." is not 6")
  endif
  keepj norm! 0

  if a:0 > 0
"   call Decho("case [a:0=".a:0."] > 0: a:1<".a:1.">")
   if a:1 =~ '^\~' && (has("unix") || (exists("g:netrw_cygwin") && g:netrw_cygwin))
"    call Decho("case a:1: ~ and unix or cygwin")
    let dirname= simplify(substitute(a:1,'\~',expand("$HOME"),''))
"    call Decho("using dirname<".dirname.">  (case: ~ && unix||cygwin)")
   elseif a:1 == '.'
"    call Decho("case a:1: .")
    let dirname= simplify(exists("b:netrw_curdir")? b:netrw_curdir : getcwd())
    if dirname !~ '/$'
     let dirname= dirname."/"
    endif
"    call Decho("using dirname<".dirname.">  (case: ".(exists("b:netrw_curdir")? "b:netrw_curdir" : "getcwd()").")")
   elseif a:1 =~ '\$'
"    call Decho("case a:1: $")
    let dirname= simplify(expand(a:1))
"    call Decho("using user-specified dirname<".dirname."> with $env-var")
   elseif a:1 !~ '^\*/'
"    call Decho("case a:1: other, not pattern or filepattern")
    let dirname= simplify(a:1)
"    call Decho("using user-specified dirname<".dirname.">")
   else
"    call Decho("case a:1: pattern or filepattern")
    let dirname= a:1
   endif
  else
   " clear explore
"   call Decho("case a:0=".a:0.": clearing Explore list")
   call s:NetrwClearExplore()
"   call Dret("netrw#Explore : cleared list")
   return
  endif

"  call Decho("dirname<".dirname.">")
  if dirname =~ '\.\./\=$'
   let dirname= simplify(fnamemodify(dirname,':p:h'))
  elseif dirname =~ '\.\.' || dirname == '.'
   let dirname= simplify(fnamemodify(dirname,':p'))
  endif
"  call Decho("dirname<".dirname.">  (after simplify)")

  if dirname =~ '/\*\*/'
   " handle .../**/.../filepat
"   call Decho("case Explore .../**/.../filepat")
   let prefixdir= substitute(dirname,'^\(.\{-}\)\*\*.*$','\1','')
   if prefixdir =~ '^/' || (prefixdir =~ '^\a:/' && (has("win32") || has("win95") || has("win64") || has("win16")))
    let b:netrw_curdir = prefixdir
   else
    let b:netrw_curdir= getcwd().'/'.prefixdir
   endif
   let dirname= substitute(dirname,'^.\{-}\(\*\*/.*\)$','\1','')
   let starpat= 4;
"   call Decho("pwd<".getcwd()."> dirname<".dirname.">")
"   call Decho("case Explore ../**/../filepat (starpat=".starpat.")")

  elseif dirname =~ '^\*//'
   " starpat=1: Explore *//pattern   (current directory only search for files containing pattern)
"   call Decho("case Explore *//pattern")
   let pattern= substitute(dirname,'^\*//\(.*\)$','\1','')
   let starpat= 1
"   call Decho("Explore *//pat: (starpat=".starpat.") dirname<".dirname."> -> pattern<".pattern.">")
   if &hls | let keepregslash= s:ExplorePatHls(pattern) | endif

  elseif dirname =~ '^\*\*//'
   " starpat=2: Explore **//pattern  (recursive descent search for files containing pattern)
"   call Decho("case Explore **//pattern")
   let pattern= substitute(dirname,'^\*\*//','','')
   let starpat= 2
"   call Decho("Explore **//pat: (starpat=".starpat.") dirname<".dirname."> -> pattern<".pattern.">")

  elseif dirname =~ '^\*/'
   " starpat=3: Explore */filepat   (search in current directory for filenames matching filepat)
   let starpat= 3
"   call Decho("case Explore */filepat (starpat=".starpat.")")

  elseif dirname=~ '^\*\*/'
   " starpat=4: Explore **/filepat  (recursive descent search for filenames matching filepat)
   let starpat= 4
"   call Decho("case Explore **/filepat (starpat=".starpat.")")

  else
   let starpat= 0
"   call Decho("default case: starpat=".starpat)
  endif

  if starpat == 0 && a:indx >= 0
   " [Explore Hexplore Vexplore Sexplore] [dirname]
"   call Decho("case starpat==0 && a:indx=".a:indx.": dirname<".dirname."> Explore Hexplore Vexplore Sexplore")
   if dirname == ""
    let dirname= curfiledir
"    call Decho("empty dirname, using current file's directory<".dirname.">")
   endif
   if dirname =~ '^scp:' || dirname =~ '^ftp:'
"    call Decho("calling NetrwBrowse(0,dirname<".dirname.">)")
    call s:NetrwBrowse(0,dirname)
   else
    if dirname == ""|let dirname= getcwd()|endif
"    call Decho("calling LocalBrowseCheck(dirname<".dirname.">)")
    call netrw#LocalBrowseCheck(dirname)
   endif

"   call Decho("curdir<".curdir.">")
   if has("win32") || has("win95") || has("win64") || has("win16")
    keepj call search('\<'.substitute(curdir,'^.*[/\\]','','e').'\>','cW')
   else
    keepj call search('\<'.substitute(curdir,'^.*/','','e').'\>','cW')
   endif

  " starpat=1: Explore *//pattern  (current directory only search for files containing pattern)
  " starpat=2: Explore **//pattern (recursive descent search for files containing pattern)
  " starpat=3: Explore */filepat   (search in current directory for filenames matching filepat)
  " starpat=4: Explore **/filepat  (recursive descent search for filenames matching filepat)
  elseif a:indx <= 0
   " Nexplore, Pexplore, Explore: handle starpat
"   call Decho("case a:indx<=0: Nexplore, Pexplore, <s-down>, <s-up> starpat=".starpat." a:indx=".a:indx)
   if !mapcheck("<s-up>","n") && !mapcheck("<s-down>","n") && exists("b:netrw_curdir")
"    call Decho("set up <s-up> and <s-down> maps")
    let s:didstarstar= 1
    nnoremap <buffer> <silent> <s-up>	:Pexplore<cr>
    nnoremap <buffer> <silent> <s-down>	:Nexplore<cr>
   endif

   if has("path_extra")
"    call Decho("starpat=".starpat.": has +path_extra")
    if !exists("w:netrw_explore_indx")
     let w:netrw_explore_indx= 0
    endif

    let indx = a:indx
"    call Decho("starpat=".starpat.": set indx= [a:indx=".indx."]")

    if indx == -1
     " Nexplore
"     call Decho("case Nexplore with starpat=".starpat.": (indx=".indx.")")
     if !exists("w:netrw_explore_list") " sanity check
      keepj call netrw#ErrorMsg(s:WARNING,"using Nexplore or <s-down> improperly; see help for netrw-starstar",40)
      sil! let @* = keepregstar
      sil! let @+ = keepregstar
      sil! let @/ = keepregslash
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
     " Pexplore
"     call Decho("case Pexplore with starpat=".starpat.": (indx=".indx.")")
     if !exists("w:netrw_explore_list") " sanity check
      keepj call netrw#ErrorMsg(s:WARNING,"using Pexplore or <s-up> improperly; see help for netrw-starstar",41)
      sil! let @* = keepregstar
      sil! let @+ = keepregstar
      sil! let @/ = keepregslash
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
"     call Decho("starpat=".starpat.": case Explore: initialize (indx=".indx.")")
     keepj call s:NetrwClearExplore()
     let w:netrw_explore_indx= 0
     if !exists("b:netrw_curdir")
      let b:netrw_curdir= getcwd()
     endif
"     call Decho("starpat=".starpat.": b:netrw_curdir<".b:netrw_curdir.">")

     " switch on starpat to build the w:netrw_explore_list of files
     if starpat == 1
      " starpat=1: Explore *//pattern  (current directory only search for files containing pattern)
"      call Decho("starpat=".starpat.": build *//pattern list")
"      call Decho("pattern<".pattern.">")
      try
       exe "keepj noautocmd vimgrep /".pattern."/gj ".fnameescape(b:netrw_curdir)."/*"
      catch /^Vim\%((\a\+)\)\=:E480/
       call netrw#ErrorMsg(s:WARNING,"no match with pattern<".pattern.">",76)
"       call Dret("netrw#Explore : unable to find pattern<".pattern.">")
       return
      endtry
      let w:netrw_explore_list = s:NetrwExploreListUniq(map(getqflist(),'bufname(v:val.bufnr)'))
      if &hls | let keepregslash= s:ExplorePatHls(pattern) | endif

     elseif starpat == 2
      " starpat=2: Explore **//pattern (recursive descent search for files containing pattern)
"      call Decho("starpat=".starpat.": build **//pattern list")
      try
       exe "sil keepj noautocmd vimgrep /".pattern."/gj "."**/*"
      catch /^Vim\%((\a\+)\)\=:E480/
       call netrw#ErrorMsg(s:WARNING,'no files matched pattern<'.pattern.'>',45)
       if &hls | let keepregslash= s:ExplorePatHls(pattern) | endif
       sil! let @* = keepregstar
       sil! let @+ = keepregstar
       sil! let @/ = keepregslash
"       call Dret("netrw#Explore : no files matched pattern")
       return
      endtry
      let s:netrw_curdir       = b:netrw_curdir
      let w:netrw_explore_list = getqflist()
      let w:netrw_explore_list = s:NetrwExploreListUniq(map(w:netrw_explore_list,'s:netrw_curdir."/".bufname(v:val.bufnr)'))

     elseif starpat == 3
      " starpat=3: Explore */filepat   (search in current directory for filenames matching filepat)
"      call Decho("starpat=".starpat.": build */filepat list")
      let filepat= substitute(dirname,'^\*/','','')
      let filepat= substitute(filepat,'^[%#<]','\\&','')
"      call Decho("b:netrw_curdir<".b:netrw_curdir.">")
"      call Decho("filepat<".filepat.">")
      let w:netrw_explore_list= s:NetrwExploreListUniq(split(expand(b:netrw_curdir."/".filepat),'\n'))
      if &hls | let keepregslash= s:ExplorePatHls(filepat) | endif

     elseif starpat == 4
      " starpat=4: Explore **/filepat  (recursive descent search for filenames matching filepat)
"      call Decho("starpat=".starpat.": build **/filepat list")
      let w:netrw_explore_list= s:NetrwExploreListUniq(split(expand(b:netrw_curdir."/".dirname),'\n'))
      if &hls | let keepregslash= s:ExplorePatHls(dirname) | endif
     endif " switch on starpat to build w:netrw_explore_list

     let w:netrw_explore_listlen = len(w:netrw_explore_list)
"     call Decho("w:netrw_explore_list<".string(w:netrw_explore_list).">")
"     call Decho("w:netrw_explore_listlen=".w:netrw_explore_listlen)

     if w:netrw_explore_listlen == 0 || (w:netrw_explore_listlen == 1 && w:netrw_explore_list[0] =~ '\*\*\/')
      keepj call netrw#ErrorMsg(s:WARNING,"no files matched",42)
      sil! let @* = keepregstar
      sil! let @+ = keepregstar
      sil! let @/ = keepregslash
"      call Dret("netrw#Explore : no files matched")
      return
     endif
    endif  " if indx ... endif

    " NetrwStatusLine support - for exploring support
    let w:netrw_explore_indx= indx
"    call Decho("w:netrw_explore_list<".join(w:netrw_explore_list,',')."> len=".w:netrw_explore_listlen)

    " wrap the indx around, but issue a note
    if indx >= w:netrw_explore_listlen || indx < 0
"     call Decho("wrap indx (indx=".indx." listlen=".w:netrw_explore_listlen.")")
     let indx                = (indx < 0)? ( w:netrw_explore_listlen - 1 ) : 0
     let w:netrw_explore_indx= indx
     keepj call netrw#ErrorMsg(s:NOTE,"no more files match Explore pattern",43)
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
     keepj call search('^'.substitute(dirfile,"^.*/","","").'\>',"W")
    else
     keepj call search('\<'.substitute(dirfile,"^.*/","","").'\>',"w")
    endif
    let w:netrw_explore_mtchcnt = indx + 1
    let w:netrw_explore_bufnr   = bufnr("%")
    let w:netrw_explore_line    = line(".")
    keepj call s:SetupNetrwStatusLine('%f %h%m%r%=%9*%{NetrwStatusLine()}')
"    call Decho("explore: mtchcnt=".w:netrw_explore_mtchcnt." bufnr=".w:netrw_explore_bufnr." line#".w:netrw_explore_line)

   else
"    call Decho("your vim does not have +path_extra")
    if !exists("g:netrw_quiet")
     keepj call netrw#ErrorMsg(s:WARNING,"your vim needs the +path_extra feature for Exploring with **!",44)
    endif
    sil! let @* = keepregstar
    sil! let @+ = keepregstar
    sil! let @/ = keepregslash
"    call Dret("netrw#Explore : missing +path_extra")
    return
   endif

  else
"   call Decho("default case: Explore newdir<".dirname.">")
   if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && dirname =~ '/'
    sil! unlet w:netrw_treedict
    sil! unlet w:netrw_treetop
   endif
   let newdir= dirname
   if !exists("b:netrw_curdir")
    keepj call netrw#LocalBrowseCheck(getcwd())
   else
    keepj call netrw#LocalBrowseCheck(s:NetrwBrowseChgDir(1,newdir))
   endif
  endif

  " visual display of **/ **// */ Exploration files
"  call Decho("w:netrw_explore_indx=".(exists("w:netrw_explore_indx")? w:netrw_explore_indx : "doesn't exist"))
"  call Decho("b:netrw_curdir<".(exists("b:netrw_curdir")? b:netrw_curdir : "n/a").">")
  if exists("w:netrw_explore_indx") && exists("b:netrw_curdir")
"   call Decho("s:explore_prvdir<".(exists("s:explore_prvdir")? s:explore_prvdir : "-doesn't exist-"))
   if !exists("s:explore_prvdir") || s:explore_prvdir != b:netrw_curdir
    " only update match list if current directory isn't the same as before
"    call Decho("only update match list if current directory not the same as before")
    let s:explore_prvdir = b:netrw_curdir
    let s:explore_match  = ""
    let dirlen           = s:Strlen(b:netrw_curdir)
    if b:netrw_curdir !~ '/$'
     let dirlen= dirlen + 1
    endif
    let prvfname= ""
    for fname in w:netrw_explore_list
"     call Decho("fname<".fname.">")
     if fname =~ '^'.b:netrw_curdir
      if s:explore_match == ""
       let s:explore_match= '\<'.escape(strpart(fname,dirlen),g:netrw_markfileesc).'\>'
      else
       let s:explore_match= s:explore_match.'\|\<'.escape(strpart(fname,dirlen),g:netrw_markfileesc).'\>'
      endif
     elseif fname !~ '^/' && fname != prvfname
      if s:explore_match == ""
       let s:explore_match= '\<'.escape(fname,g:netrw_markfileesc).'\>'
      else
       let s:explore_match= s:explore_match.'\|\<'.escape(fname,g:netrw_markfileesc).'\>'
      endif
     endif
     let prvfname= fname
    endfor
"    call Decho("explore_match<".s:explore_match.">")
    exe "2match netrwMarkFile /".s:explore_match."/"
   endif
   echo "<s-up>==Pexplore  <s-down>==Nexplore"
  else
   2match none
   if exists("s:explore_match")  | unlet s:explore_match  | endif
   if exists("s:explore_prvdir") | unlet s:explore_prvdir | endif
   echo " "
"   call Decho("cleared explore match list")
  endif

  sil! let @* = keepregstar
  sil! let @+ = keepregstar
  sil! let @/ = keepregslash
"  call Dret("netrw#Explore : @/<".@/.">")
endfun

" ---------------------------------------------------------------------
" s:NetrwExploreListUniq: {{{2
fun! s:NetrwExploreListUniq(explist)
"  call Dfunc("s:NetrwExploreListUniq(explist)")

  " this assumes that the list is already sorted
  let newexplist= []
  for member in a:explist
   if !exists("uniqmember") || member != uniqmember
    let uniqmember = member
    let newexplist = newexplist + [ member ]
   endif
  endfor

"  call Dret("s:NetrwExploreListUniq")
  return newexplist
endfun

" ---------------------------------------------------------------------
" s:NetrwForceChgDir: (gd support) Force treatment as a directory {{{2
fun! s:NetrwForceChgDir(islocal,newdir)
"  call Dfunc("s:NetrwForceChgDir(islocal=".a:islocal." newdir<".a:newdir.">)")
  if a:newdir !~ '/$'
   " ok, looks like force is needed to get directory-style treatment
   if a:newdir =~ '@$'
    let newdir= substitute(a:newdir,'@$','/','')
   elseif a:newdir =~ '[*=|\\]$'
    let newdir= substitute(a:newdir,'.$','/','')
   else
    let newdir= a:newdir.'/'
   endif
"   call Decho("adjusting newdir<".newdir."> due to gd")
  else
   " should already be getting treatment as a directory
   let newdir= a:newdir
  endif
  call s:NetrwBrowseChgDir(a:islocal,newdir)
  call s:NetrwBrowse(a:islocal,newdir)
"  call Dret("s:NetrwForceChgDir")
endfun

" ---------------------------------------------------------------------
" s:NetrwForceFile: (gf support) Force treatment as a file {{{2
fun! s:NetrwForceFile(islocal,newfile)
"  "  call Dfunc("s:NetrwForceFile(islocal=".a:islocal." newdir<".a:newdir.">)")
  if a:newfile =~ '[/@*=|\\]$'
   let newfile= substitute(a:newfile,'.$','','')
  else
   let newfile= a:newfile
  endif
  call s:NetrwBrowseChgDir(a:islocal,newfile)
"  call Dret("s:NetrwForceFile")
endfun

" ---------------------------------------------------------------------
" s:NetrwHide: this function is invoked by the "a" map for browsing {{{2
"          and switches the hiding mode.  The actual hiding is done by
"          s:NetrwListHide().
"             g:netrw_hide= 0: show all
"                           1: show not-hidden files
"                           2: show hidden files only
fun! s:NetrwHide(islocal)
"  call Dfunc("NetrwHide(islocal=".a:islocal.") g:netrw_hide=".g:netrw_hide)
  let svpos= netrw#NetrwSavePosn()

  if exists("s:netrwmarkfilelist_{bufnr('%')}")
"   call Decho(((g:netrw_hide == 1)? "unhide" : "hide")." files in markfilelist<".string(s:netrwmarkfilelist_{bufnr("%")}).">")
"   call Decho("g:netrw_list_hide<".g:netrw_list_hide.">")

   " hide the files in the markfile list
   for fname in s:netrwmarkfilelist_{bufnr("%")}
"    call Decho("match(g:netrw_list_hide<".g:netrw_list_hide.'> fname<\<'.fname.'\>>)='.match(g:netrw_list_hide,'\<'.fname.'\>')." isk=".&isk)
    if match(g:netrw_list_hide,'\<'.fname.'\>') != -1
     " remove fname from hiding list
     let g:netrw_list_hide= substitute(g:netrw_list_hide,'..\<'.escape(fname,g:netrw_fname_escape).'\>..','','')
     let g:netrw_list_hide= substitute(g:netrw_list_hide,',,',',','g')
     let g:netrw_list_hide= substitute(g:netrw_list_hide,'^,\|,$','','')
"     call Decho("unhide: g:netrw_list_hide<".g:netrw_list_hide.">")
    else
     " append fname to hiding list
     if exists("g:netrw_list_hide") && g:netrw_list_hide != ""
      let g:netrw_list_hide= g:netrw_list_hide.',\<'.escape(fname,g:netrw_fname_escape).'\>'
     else
      let g:netrw_list_hide= '\<'.escape(fname,g:netrw_fname_escape).'\>'
     endif
"     call Decho("hide: g:netrw_list_hide<".g:netrw_list_hide.">")
    endif
   endfor
   keepj call s:NetrwUnmarkList(bufnr("%"),b:netrw_curdir)
   let g:netrw_hide= 1

  else

   " switch between show-all/show-not-hidden/show-hidden
   let g:netrw_hide=(g:netrw_hide+1)%3
   exe "keepj norm! 0"
   if g:netrw_hide && g:netrw_list_hide == ""
    keepj call netrw#ErrorMsg(s:WARNING,"your hiding list is empty!",49)
"    call Dret("NetrwHide")
    return
   endif
  endif

  keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)
"  call Dret("NetrwHide")
endfun

" ---------------------------------------------------------------------
" s:NetrwHidden: invoked by "gh" {{{2
fun! s:NetrwHidden(islocal)
"  call Dfunc("s:NetrwHidden()")
  "  save current position
  let svpos= netrw#NetrwSavePosn()

  if g:netrw_list_hide =~ '\(^\|,\)\\(^\\|\\s\\s\\)\\zs\\.\\S\\+'
   " remove pattern from hiding list
   let g:netrw_list_hide= substitute(g:netrw_list_hide,'\(^\|,\)\\(^\\|\\s\\s\\)\\zs\\.\\S\\+','','')
  elseif s:Strlen(g:netrw_list_hide) >= 1
   let g:netrw_list_hide= g:netrw_list_hide . ',\(^\|\s\s\)\zs\.\S\+'
  else
   let g:netrw_list_hide= '\(^\|\s\s\)\zs\.\S\+'
  endif

  " refresh screen and return to saved position
  keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)
"  call Dret("s:NetrwHidden")
endfun

" ---------------------------------------------------------------------
"  s:NetrwHome: this function determines a "home" for saving bookmarks and history {{{2
fun! s:NetrwHome()
  if exists("g:netrw_home")
   let home= g:netrw_home
  else
   " go to vim plugin home
   for home in split(&rtp,',') + ['']
    if isdirectory(home) && filewritable(home) | break | endif
     let basehome= substitute(home,'[/\\]\.vim$','','')
    if isdirectory(basehome) && filewritable(basehome)
     let home= basehome."/.vim"
     break
    endif
   endfor
   if home == ""
    " just pick the first directory
    let home= substitute(&rtp,',.*$','','')
   endif
   if (has("win32") || has("win95") || has("win64") || has("win16"))
    let home= substitute(home,'/','\\','g')
   endif
  endif
  " insure that the home directory exists
  if !isdirectory(home)
   if exists("g:netrw_mkdir")
    call system(g:netrw_mkdir." ".shellescape(home))
   else
    call mkdir(home)
   endif
  endif
  let g:netrw_home= home
  return home
endfun

" ---------------------------------------------------------------------
" s:NetrwLeftmouse: handles the <leftmouse> when in a netrw browsing window {{{2
fun! s:NetrwLeftmouse(islocal)
"  call Dfunc("s:NetrwLeftmouse(islocal=".a:islocal.")")

  " check if the status bar was clicked on instead of a file/directory name
  while getchar(0) != 0
   "clear the input stream
  endwhile
  call feedkeys("\<LeftMouse>")
  let c          = getchar()
  let mouse_lnum = v:mouse_lnum
  let wlastline  = line('w$')
  let lastline   = line('$')
"  call Decho("v:mouse_lnum=".mouse_lnum." line(w$)=".wlastline." line($)=".lastline." v:mouse_win=".v:mouse_win." winnr#".winnr())
"  call Decho("v:mouse_col =".v:mouse_col."     col=".col(".")."  wincol =".wincol()." winwidth   =".winwidth(0))
  if mouse_lnum >= wlastline + 1 || v:mouse_win != winnr()
   " appears to be a status bar leftmouse click
"   call Dret("s:NetrwLeftmouse : detected a status bar leftmouse click")
   return
  endif
  if v:mouse_col != col('.')
"   call Dret("s:NetrwLeftmouse : detected a vertical separator bar leftmouse click")
   return
  endif

  if a:islocal
   if exists("b:netrw_curdir")
    keepj call netrw#LocalBrowseCheck(s:NetrwBrowseChgDir(1,s:NetrwGetWord()))
   endif
  else
   if exists("b:netrw_curdir")
    keepj call s:NetrwBrowse(0,s:NetrwBrowseChgDir(0,s:NetrwGetWord()))
   endif
  endif
"  call Dret("s:NetrwLeftmouse")
endfun

" ---------------------------------------------------------------------
" s:NetrwListHide: uses [range]g~...~d to delete files that match comma {{{2
" separated patterns given in g:netrw_list_hide
fun! s:NetrwListHide()
"  call Dfunc("NetrwListHide() g:netrw_hide=".g:netrw_hide." g:netrw_list_hide<".g:netrw_list_hide.">")

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
    let listhide = ""
   endif

   " Prune the list by hiding any files which match
   if g:netrw_hide == 1
"    call Decho("hiding<".hide."> listhide<".listhide.">")
    exe 'sil keepj '.w:netrw_bannercnt.',$g'.sep.hide.sep.'d'
   elseif g:netrw_hide == 2
"    call Decho("showing<".hide."> listhide<".listhide.">")
    exe 'sil keepj '.w:netrw_bannercnt.',$g'.sep.hide.sep.'s@^@ /-KEEP-/ @'
   endif
  endwhile
  if g:netrw_hide == 2
   exe 'sil keepj '.w:netrw_bannercnt.',$v@^ /-KEEP-/ @d'
   exe 'sil keepj '.w:netrw_bannercnt.',$s@^\%( /-KEEP-/ \)\+@@e'
  endif

  " remove any blank lines that have somehow remained.
  " This seems to happen under Windows.
  exe 'sil! keepj 1,$g@^\s*$@d'

"  call Dret("NetrwListHide")
endfun

" ---------------------------------------------------------------------
" NetrwHideEdit: allows user to edit the file/directory hiding list
fun! s:NetrwHideEdit(islocal)
"  call Dfunc("NetrwHideEdit(islocal=".a:islocal.")")

  " save current cursor position
  let svpos= netrw#NetrwSavePosn()

  " get new hiding list from user
  call inputsave()
  let newhide= input("Edit Hiding List: ",g:netrw_list_hide)
  call inputrestore()
  let g:netrw_list_hide= newhide
"  call Decho("new g:netrw_list_hide<".g:netrw_list_hide.">")

  " refresh the listing
  sil keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,"./"))

  " restore cursor position
  call netrw#NetrwRestorePosn(svpos)

"  call Dret("NetrwHideEdit")
endfun

" ---------------------------------------------------------------------
" NetSortSequence: allows user to edit the sorting sequence
fun! s:NetSortSequence(islocal)
"  call Dfunc("NetSortSequence(islocal=".a:islocal.")")

  let svpos= netrw#NetrwSavePosn()
  call inputsave()
  let newsortseq= input("Edit Sorting Sequence: ",g:netrw_sort_sequence)
  call inputrestore()

  " refresh the listing
  let g:netrw_sort_sequence= newsortseq
  keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)

"  call Dret("NetSortSequence")
endfun

" ---------------------------------------------------------------------
" s:NetrwMakeDir: this function makes a directory (both local and remote) {{{2
fun! s:NetrwMakeDir(usrhost)
"  call Dfunc("NetrwMakeDir(usrhost<".a:usrhost.">)")

  " get name of new directory from user.  A bare <CR> will skip.
  " if its currently a directory, also request will be skipped, but with
  " a message.
  call inputsave()
  let newdirname= input("Please give directory name: ")
  call inputrestore()
"  call Decho("newdirname<".newdirname.">")

  if newdirname == ""
"   call Dret("NetrwMakeDir : user aborted with bare <cr>")
   return
  endif

  if a:usrhost == ""
"   call Decho("local mkdir")

   " Local mkdir:
   " sanity checks
   let fullnewdir= b:netrw_curdir.'/'.newdirname
"   call Decho("fullnewdir<".fullnewdir.">")
   if isdirectory(fullnewdir)
    if !exists("g:netrw_quiet")
     keepj call netrw#ErrorMsg(s:WARNING,"<".newdirname."> is already a directory!",24)
    endif
"    call Dret("NetrwMakeDir : directory<".newdirname."> exists previously")
    return
   endif
   if s:FileReadable(fullnewdir)
    if !exists("g:netrw_quiet")
     keepj call netrw#ErrorMsg(s:WARNING,"<".newdirname."> is already a file!",25)
    endif
"    call Dret("NetrwMakeDir : file<".newdirname."> exists previously")
    return
   endif

   " requested new local directory is neither a pre-existing file or
   " directory, so make it!
   if exists("*mkdir")
    call mkdir(fullnewdir,"p")
   else
    let netrw_origdir= s:NetrwGetcwd(1)
    exe 'keepj lcd '.fnameescape(b:netrw_curdir)
"    call Decho("netrw_origdir<".netrw_origdir.">: lcd b:netrw_curdir<".fnameescape(b:netrw_curdir).">")
"    call Decho("exe sil! !".g:netrw_local_mkdir.' '.shellescape(newdirname,1))
    exe "sil! !".g:netrw_local_mkdir.' '.shellescape(newdirname,1)
    if !g:netrw_keepdir
     exe 'keepj lcd '.fnameescape(netrw_origdir)
"     call Decho("netrw_keepdir=".g:netrw_keepdir.": keepjumps lcd ".fnameescape(netrw_origdir)." getcwd<".getcwd().">")
    endif
   endif

   if v:shell_error == 0
    " refresh listing
"    call Decho("refresh listing")
    let svpos= netrw#NetrwSavePosn()
    call s:NetrwRefresh(1,s:NetrwBrowseChgDir(1,'./'))
    call netrw#NetrwRestorePosn(svpos)
   elseif !exists("g:netrw_quiet")
    call netrw#ErrorMsg(s:ERROR,"unable to make directory<".newdirname.">",26)
   endif
"   redraw!

  elseif !exists("b:netrw_method") || b:netrw_method == 4
   " Remote mkdir:
"   call Decho("remote mkdir")
   let mkdircmd  = s:MakeSshCmd(g:netrw_mkdir_cmd)
   let newdirname= substitute(b:netrw_curdir,'^\%(.\{-}/\)\{3}\(.*\)$','\1','').newdirname
"   call Decho("exe sil! !".mkdircmd." ".shellescape(newdirname,1))
   exe "sil! !".mkdircmd." ".shellescape(newdirname,1)
   if v:shell_error == 0
    " refresh listing
    let svpos= netrw#NetrwSavePosn()
    keepj call s:NetrwRefresh(0,s:NetrwBrowseChgDir(0,'./'))
    keepj call netrw#NetrwRestorePosn(svpos)
   elseif !exists("g:netrw_quiet")
    keepj call netrw#ErrorMsg(s:ERROR,"unable to make directory<".newdirname.">",27)
   endif
"   redraw!

  elseif b:netrw_method == 2
   " COMBAK -- future work
   keepj call netrw#ErrorMsg(s:ERROR,"making directories via ftp not currently supported",68)
  elseif b:netrw_method == 3
   " COMBAK -- future work
   keepj call netrw#ErrorMsg(s:ERROR,"making directories via ftp not currently supported",68)
  endif

"  call Dret("NetrwMakeDir")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFile: (invoked by mf) This function is used to both {{{2
"                  mark and unmark files.  If a markfile list exists,
"                  then the rename and delete functions will use it instead
"                  of whatever may happen to be under the cursor at that
"                  moment.  When the mouse and gui are available,
"                  shift-leftmouse may also be used to mark files.
"
"  Creates two lists
"    s:netrwmarkfilelist    -- holds complete paths to all marked files
"    s:netrwmarkfilelist_#  -- holds list of marked files in current-buffer's directory (#==bufnr())
"
"  Creates a marked file match string
"    s:netrwmarfilemtch_#   -- used with 2match to display marked files
"
"  Creates a buffer version of islocal
"    b:netrw_islocal
fun! s:NetrwMarkFile(islocal,fname)
"  call Dfunc("s:NetrwMarkFile(islocal=".a:islocal." fname<".a:fname.">)")
  let curbufnr= bufnr("%")
  let curdir  = b:netrw_curdir
  let trailer = '[@=|\/\*]\=\>'

  if exists("s:netrwmarkfilelist_{curbufnr}")
   " markfile list pre-exists
"   call Decho("starting s:netrwmarkfilelist_{curbufnr}<".string(s:netrwmarkfilelist_{curbufnr}).">")
"   call Decho("starting s:netrwmarkfilemtch_{curbufnr}<".s:netrwmarkfilemtch_{curbufnr}.">")
   let b:netrw_islocal= a:islocal

   if index(s:netrwmarkfilelist_{curbufnr},a:fname) == -1
    " append filename to buffer's markfilelist
"    call Decho("append filename<".a:fname."> to local markfilelist_".curbufnr."<".string(s:netrwmarkfilelist_{curbufnr}).">")
    call add(s:netrwmarkfilelist_{curbufnr},a:fname)
    let s:netrwmarkfilemtch_{curbufnr}= s:netrwmarkfilemtch_{curbufnr}.'\|\<'.escape(a:fname,g:netrw_markfileesc."'".g:netrw_markfileesc."'").trailer

   else
    " remove filename from buffer's markfilelist
"    call Decho("remove filename<".a:fname."> from local markfilelist_".curbufnr."<".string(s:netrwmarkfilelist_{curbufnr}).">")
    call filter(s:netrwmarkfilelist_{curbufnr},'v:val != a:fname')
    if s:netrwmarkfilelist_{curbufnr} == []
     " local markfilelist is empty; remove it entirely
"     call Decho("markfile list now empty")
     call s:NetrwUnmarkList(curbufnr,curdir)
    else
     " rebuild match list to display markings correctly
"     call Decho("rebuild s:netrwmarkfilemtch_".curbufnr)
     let s:netrwmarkfilemtch_{curbufnr}= ""
     let first                           = 1
     for fname in s:netrwmarkfilelist_{curbufnr}
      if first
       let s:netrwmarkfilemtch_{curbufnr}= s:netrwmarkfilemtch_{curbufnr}.'\<'.escape(fname,g:netrw_markfileesc."'".g:netrw_markfileesc."'").trailer
      else
       let s:netrwmarkfilemtch_{curbufnr}= s:netrwmarkfilemtch_{curbufnr}.'\|\<'.escape(fname,g:netrw_markfileesc."'".g:netrw_markfileesc."'").trailer
      endif
      let first= 0
     endfor
"     call Decho("ending s:netrwmarkfilelist_"curbufnr."<".string(s:netrwmarkfilelist_{curbufnr}).">")
"     call Decho("ending s:netrwmarkfilemtch_"curbufnr."<".s:netrwmarkfilemtch_{curbufnr}.">")
    endif
   endif

  else
   " initialize new markfilelist

"   call Decho("add fname<".a:fname."> to new markfilelist_".curbufnr)
   let s:netrwmarkfilelist_{curbufnr}= []
   call add(s:netrwmarkfilelist_{curbufnr},a:fname)
"   call Decho("ending s:netrwmarkfilelist_{curbufnr}<".string(s:netrwmarkfilelist_{curbufnr}).">")

   " build initial markfile matching pattern
   if a:fname =~ '/$'
    let s:netrwmarkfilemtch_{curbufnr}= '\<'.escape(a:fname,g:netrw_markfileesc)
   else
    let s:netrwmarkfilemtch_{curbufnr}= '\<'.escape(a:fname,g:netrw_markfileesc).trailer
   endif
"   call Decho("ending s:netrwmarkfilemtch_".curbufnr."<".s:netrwmarkfilemtch_{curbufnr}.">")
  endif

  " handle global markfilelist
  if exists("s:netrwmarkfilelist")
   let dname= s:ComposePath(b:netrw_curdir,a:fname)
   if index(s:netrwmarkfilelist,dname) == -1
    " append new filename to global markfilelist
    call add(s:netrwmarkfilelist,s:ComposePath(b:netrw_curdir,a:fname))
"    call Decho("append filename<".a:fname."> to global markfilelist<".string(s:netrwmarkfilelist).">")
   else
    " remove new filename from global markfilelist
"    call Decho("filter(".string(s:netrwmarkfilelist).",'v:val != '.".dname.")")
    call filter(s:netrwmarkfilelist,'v:val != "'.dname.'"')
"    call Decho("ending s:netrwmarkfilelist  <".string(s:netrwmarkfilelist).">")
    if s:netrwmarkfilelist == []
     unlet s:netrwmarkfilelist
    endif
   endif
  else
   " initialize new global-directory markfilelist
   let s:netrwmarkfilelist= []
   call add(s:netrwmarkfilelist,s:ComposePath(b:netrw_curdir,a:fname))
"   call Decho("init s:netrwmarkfilelist<".string(s:netrwmarkfilelist).">")
  endif

  " set up 2match'ing to netrwmarkfilemtch list
  if exists("s:netrwmarkfilemtch_{curbufnr}") && s:netrwmarkfilemtch_{curbufnr} != ""
"   call Decho("exe 2match netrwMarkFile /".s:netrwmarkfilemtch_{curbufnr}."/")
   if exists("g:did_drchip_netrwlist_syntax")
    exe "2match netrwMarkFile /".s:netrwmarkfilemtch_{curbufnr}."/"
   endif
  else
"   call Decho("2match none")
   2match none
  endif
"  call Dret("s:NetrwMarkFile : s:netrwmarkfilelist_".curbufnr."<".(exists("s:netrwmarkfilelist_{curbufnr}")? string(s:netrwmarkfilelist_{curbufnr}) : " doesn't exist").">")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileCompress: (invoked by mz) This function is used to {{{2
"                          compress/decompress files using the programs
"                          in g:netrw_compress and g:netrw_uncompress,
"                          using g:netrw_compress_suffix to know which to
"                          do.  By default:
"                            g:netrw_compress        = "gzip"
"                            g:netrw_decompress      = { ".gz" : "gunzip" , ".bz2" : "bunzip2" , ".zip" : "unzip" , ".tar" : "tar -xf", ".xz" : "unxz"}
fun! s:NetrwMarkFileCompress(islocal)
"  call Dfunc("s:NetrwMarkFileCompress(islocal=".a:islocal.")")
  let svpos    = netrw#NetrwSavePosn()
  let curdir   = b:netrw_curdir
  let curbufnr = bufnr("%")

  if exists("s:netrwmarkfilelist_{curbufnr}") && exists("g:netrw_compress") && exists("g:netrw_decompress")
   for fname in s:netrwmarkfilelist_{curbufnr}
    " for every filename in the marked list
    for sfx in sort(keys(g:netrw_decompress))
     if fname =~ '\'.sfx.'$'
      " fname has a suffix indicating that its compressed; apply associated decompression routine
      let exe= netrw#WinPath(g:netrw_decompress[sfx])
"      call Decho("fname<".fname."> is compressed so decompress with <".exe.">")
      if a:islocal
       if g:netrw_keepdir
        let fname= shellescape(s:ComposePath(curdir,fname))
       endif
      else
       let fname= shellescape(b:netrw_curdir.fname,1)
      endif
      if executable(exe)
       if a:islocal
	call system(exe." ".fname)
       else
        keepj call s:RemoteSystem(exe." ".fname)
       endif
      else
       keepj call netrw#ErrorMsg(s:WARNING,"unable to apply<".exe."> to file<".fname.">",50)
      endif
      break
     endif
     unlet sfx
    endfor
    if exists("exe")
     unlet exe
    elseif a:islocal
     " fname not a compressed file, so compress it
     call system(netrw#WinPath(g:netrw_compress)." ".shellescape(s:ComposePath(b:netrw_curdir,fname)))
    else
     " fname not a compressed file, so compress it
     keepj call s:RemoteSystem(netrw#WinPath(g:netrw_compress)." ".shellescape(fname))
    endif
   endfor
   call s:NetrwUnmarkList(curbufnr,curdir)
   keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
   keepj call netrw#NetrwRestorePosn(svpos)
  endif
"  call Dret("s:NetrwMarkFileCompress")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileCopy: (invoked by mc) copy marked files to target {{{2
"                      If no marked files, then set up directory as the
"                      target.  Currently does not support copying entire
"                      directories.  Uses the local-buffer marked file list.
"                      Returns 1=success  (used by NetrwMarkFileMove())
"                              0=failure
fun! s:NetrwMarkFileCopy(islocal)
"  call Dfunc("s:NetrwMarkFileCopy(islocal=".a:islocal.") target<".(exists("s:netrwmftgt")? s:netrwmftgt : '---').">")

  " sanity checks
  if !exists("s:netrwmarkfilelist_{bufnr('%')}") || empty(s:netrwmarkfilelist_{bufnr('%')})
   keepj call netrw#ErrorMsg(2,"there are no marked files in this window (:help netrw-mf)",66)
"   call Dret("s:NetrwMarkFileCopy 0")
   return 0
  endif
"  call Decho("sanity chk passed: s:netrwmarkfilelist_".bufnr('%')."<".string(s:netrwmarkfilelist_{bufnr('%')}))
  if !exists("s:netrwmftgt")
   keepj call netrw#ErrorMsg(2,"your marked file target is empty! (:help netrw-mt)",67)
"   call Dret("s:NetrwMarkFileCopy 0")
   return 0
  endif
"  call Decho("sanity chk passed: s:netrwmftgt<".s:netrwmftgt.">")
  let curdir   = b:netrw_curdir
  let curbufnr = bufnr("%")

  if      a:islocal &&  s:netrwmftgt_islocal
   " Copy marked files, local directory to local directory
"   call Decho("copy from local to local")
   let args= join(map(deepcopy(s:netrwmarkfilelist_{bufnr('%')}),"shellescape(b:netrw_curdir.\"/\".v:val)"))
"   call Decho("system(".g:netrw_localcopycmd." ".args." ".shellescape(s:netrwmftgt).")")
   call system(netrw#WinPath(g:netrw_localcopycmd)." ".args." ".shellescape(s:netrwmftgt))

  elseif  a:islocal && !s:netrwmftgt_islocal
   " Copy marked files, local directory to remote directory
"   call Decho("copy from local to remote")
   keepj call s:NetrwUpload(s:netrwmarkfilelist_{bufnr('%')},s:netrwmftgt)

  elseif !a:islocal &&  s:netrwmftgt_islocal
"   call Decho("copy from remote to local")
   keepj call netrw#NetrwObtain(a:islocal,s:netrwmarkfilelist_{bufnr('%')},s:netrwmftgt)

  elseif !a:islocal && !s:netrwmftgt_islocal
"   call Decho("copy from remote to remote")
   let curdir = getcwd()
   let tmpdir = s:GetTempfile("")
   if tmpdir !~ '/'
    let tmpdir= curdir."/".tmpdir
   endif
   if exists("*mkdir")
    call mkdir(tmpdir)
   else
    exe "sil! !".g:netrw_local_mkdir.' '.shellescape(tmpdir,1)
   endif
   if isdirectory(tmpdir)
    exe "keepj lcd ".fnameescape(tmpdir)
    keepj call netrw#NetrwObtain(a:islocal,s:netrwmarkfilelist_{bufnr('%')},tmpdir)
    let localfiles= map(deepcopy(s:netrwmarkfilelist_{bufnr('%')}),'substitute(v:val,"^.*/","","")')
    keepj call s:NetrwUpload(localfiles,s:netrwmftgt)
    if getcwd() == tmpdir
     for fname in s:netrwmarkfilelist_{bufnr('%')}
      keepj call s:NetrwDelete(fname)
     endfor
     exe "keepj lcd ".fnameescape(curdir)
     exe "sil !".g:netrw_local_rmdir." ".shellescape(tmpdir,1)
    else
     exe "keepj lcd ".fnameescape(curdir)
    endif
   endif
  endif

  " -------
  " cleanup
  " -------
"  call Decho("cleanup")

  " remove markings from local buffer
  call s:NetrwUnmarkList(curbufnr,curdir)

  " refresh buffers
  if !s:netrwmftgt_islocal
   call s:NetrwRefreshDir(s:netrwmftgt_islocal,s:netrwmftgt)
  endif
  if a:islocal
   keepj call s:NetrwRefreshDir(a:islocal,curdir)
  endif
  if g:netrw_fastbrowse <= 1
   keepj call s:LocalBrowseShellCmdRefresh()
  endif
  
"  call Dret("s:NetrwMarkFileCopy 1")
  return 1
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileDiff: (invoked by md) This function is used to {{{2
"                      invoke vim's diff mode on the marked files.
"                      Either two or three files can be so handled.
"                      Uses the global marked file list.
fun! s:NetrwMarkFileDiff(islocal)
"  call Dfunc("s:NetrwMarkFileDiff(islocal=".a:islocal.") b:netrw_curdir<".b:netrw_curdir.">")
  let curbufnr= bufnr("%")

  if exists("s:netrwmarkfilelist_{curbufnr}")
   let cnt    = 0
   let curdir = b:netrw_curdir
   for fname in s:netrwmarkfilelist
    let cnt= cnt + 1
    if cnt == 1
"     call Decho("diffthis: fname<".fname.">")
     exe "e ".fnameescape(fname)
     diffthis
    elseif cnt == 2 || cnt == 3
     vsplit
     wincmd l
"     call Decho("diffthis: ".fname)
     exe "e ".fnameescape(fname)
     diffthis
    else
     break
    endif
   endfor
   call s:NetrwUnmarkList(curbufnr,curdir)
  endif

"  call Dret("s:NetrwMarkFileDiff")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileEdit: (invoked by me) put marked files on arg list and start editing them {{{2
"                       Uses global markfilelist
fun! s:NetrwMarkFileEdit(islocal)
"  call Dfunc("s:NetrwMarkFileEdit(islocal=".a:islocal.")")

  let curdir   = b:netrw_curdir
  let curbufnr = bufnr("%")
  if exists("s:netrwmarkfilelist_{curbufnr}")
   call s:SetRexDir(a:islocal,curdir)
   let flist= join(map(deepcopy(s:netrwmarkfilelist), "fnameescape(v:val)"))
   " unmark markedfile list
"   call s:NetrwUnmarkList(curbufnr,curdir)
   call s:NetrwUnmarkAll()
"   call Decho("exe sil args ".flist)
   exe "sil args ".flist
  endif
  
"  call Dret("s:NetrwMarkFileEdit")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileExe: (invoked by mx) execute arbitrary command on marked files, one at a time {{{2
"                     Uses the local marked-file list.
fun! s:NetrwMarkFileExe(islocal)
"  call Dfunc("s:NetrwMarkFileExe(islocal=".a:islocal.")")
  let svpos    = netrw#NetrwSavePosn()
  let curdir   = b:netrw_curdir
  let curbufnr = bufnr("%")

  if exists("s:netrwmarkfilelist_{curbufnr}")
   " get the command
   call inputsave()
   let cmd= input("Enter command: ","","file")
   call inputrestore()
"   call Decho("cmd<".cmd.">")

   " apply command to marked files.  Substitute: filename -> %
   " If no %, then append a space and the filename to the command
   for fname in s:netrwmarkfilelist_{curbufnr}
    if a:islocal
     if g:netrw_keepdir
      let fname= shellescape(netrw#WinPath(s:ComposePath(curdir,fname)))
     endif
    else
     let fname= shellescape(netrw#WinPath(b:netrw_curdir.fname))
    endif
    if cmd =~ '%'
     let xcmd= substitute(cmd,'%',fname,'g')
    else
     let xcmd= cmd.' '.fname
    endif
    if a:islocal
"     call Decho("local: xcmd<".xcmd.">")
     let ret= system(xcmd)
    else
"     call Decho("remote: xcmd<".xcmd.">")
     let ret= s:RemoteSystem(xcmd)
    endif
    if v:shell_error < 0
     keepj call netrw#ErrorMsg(s:ERROR,"command<".xcmd."> failed, aborting",54)
     break
    else
     echo ret
    endif
   endfor

   " unmark marked file list
   call s:NetrwUnmarkList(curbufnr,curdir)

   " refresh the listing
   keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
   keepj call netrw#NetrwRestorePosn(svpos)
  else
   keepj call netrw#ErrorMsg(s:ERROR,"no files marked!",59)
  endif
  
"  call Dret("s:NetrwMarkFileExe")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkHideSfx: (invoked by mh) (un)hide files having same suffix
"                  as the marked file(s) (toggles suffix presence)
"                  Uses the local marked file list.
fun! s:NetrwMarkHideSfx(islocal)
"  call Dfunc("s:NetrwMarkHideSfx(islocal=".a:islocal.")")
  let svpos    = netrw#NetrwSavePosn()
  let curbufnr = bufnr("%")

  " s:netrwmarkfilelist_{curbufnr}: the List of marked files
  if exists("s:netrwmarkfilelist_{curbufnr}")

   for fname in s:netrwmarkfilelist_{curbufnr}
"     call Decho("s:NetrwMarkFileCopy: fname<".fname.">")
     " construct suffix pattern
     if fname =~ '\.'
      let sfxpat= "^.*".substitute(fname,'^.*\(\.[^. ]\+\)$','\1','')
     else
      let sfxpat= '^\%(\%(\.\)\@!.\)*$'
     endif
     " determine if its in the hiding list or not
     let inhidelist= 0
     if g:netrw_list_hide != ""
      let itemnum = 0
      let hidelist= split(g:netrw_list_hide,',')
      for hidepat in hidelist
       if sfxpat == hidepat
        let inhidelist= 1
        break
       endif
       let itemnum= itemnum + 1
      endfor
     endif
"     call Decho("fname<".fname."> inhidelist=".inhidelist." sfxpat<".sfxpat.">")
     if inhidelist
      " remove sfxpat from list
      call remove(hidelist,itemnum)
      let g:netrw_list_hide= join(hidelist,",")
     elseif g:netrw_list_hide != ""
      " append sfxpat to non-empty list
      let g:netrw_list_hide= g:netrw_list_hide.",".sfxpat
     else
      " set hiding list to sfxpat
      let g:netrw_list_hide= sfxpat
     endif
    endfor

   " refresh the listing
   keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
   keepj call netrw#NetrwRestorePosn(svpos)
  else
   keepj call netrw#ErrorMsg(s:ERROR,"no files marked!",59)
  endif

"  call Dret("s:NetrwMarkHideSfx")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileGrep: (invoked by mg) This function applies vimgrep to marked files {{{2
"                     Uses the global markfilelist
fun! s:NetrwMarkFileGrep(islocal)
"  call Dfunc("s:NetrwMarkFileGrep(islocal=".a:islocal.")")
  let svpos    = netrw#NetrwSavePosn()
  let curbufnr = bufnr("%")

  if exists("s:netrwmarkfilelist")
"  call Decho("s:netrwmarkfilelist".string(s:netrwmarkfilelist).">")
   let netrwmarkfilelist= join(map(deepcopy(s:netrwmarkfilelist), "fnameescape(v:val)"))
   call s:NetrwUnmarkAll()

   " ask user for pattern
   call inputsave()
   let pat= input("Enter pattern: ","")
   call inputrestore()
   if pat !~ '^\s'
    if pat !~ '^/'
     let pat= '/'.pat.'/'
    endif
    let pat= " ".pat
   endif

   " use vimgrep for both local and remote
"   call Decho("exe vimgrep".pat." ".netrwmarkfilelist)
   try
    exe "keepj noautocmd vimgrep".pat." ".netrwmarkfilelist
    catch /^Vim\%((\a\+)\)\=:E480/
     keepj call netrw#ErrorMsg(s:WARNING,"no match with pattern<".pattern.">",76)
"     call Dret("s:NetrwMarkFileGrep : unable to find pattern<".pattern.">")
     return
   endtry

   2match none
   keepj call netrw#NetrwRestorePosn(svpos)
  endif

"  call Dret("s:NetrwMarkFileGrep")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileMove: (invoked by mm) execute arbitrary command on marked files, one at a time {{{2
"                      uses the global marked file list
"                      s:netrwmfloc= 0: target directory is remote
"                                  = 1: target directory is local
fun! s:NetrwMarkFileMove(islocal)
"  call Dfunc("s:NetrwMarkFileMove(islocal=".a:islocal.")")
  let curdir   = b:netrw_curdir
  let curbufnr = bufnr("%")

  " sanity check
  if !exists("s:netrwmarkfilelist_{bufnr('%')}") || empty(s:netrwmarkfilelist_{bufnr('%')})
   keepj call netrw#ErrorMsg(2,"there are no marked files in this window (:help netrw-mf)",66)
"   call Dret("s:NetrwMarkFileMove")
   return
  endif
"  call Decho("sanity chk passed: s:netrwmarkfilelist_".bufnr('%')."<".string(s:netrwmarkfilelist_{bufnr('%')}))
  if !exists("s:netrwmftgt")
   keepj call netrw#ErrorMsg(2,"your marked file target is empty! (:help netrw-mt)",67)
"   call Dret("s:NetrwMarkFileCopy 0")
   return 0
  endif
"  call Decho("sanity chk passed: s:netrwmftgt<".s:netrwmftgt.">")

  if      a:islocal &&  s:netrwmftgt_islocal
   " move: local -> local
"   call Decho("move from local to local")
"   call Decho("(s:NetrwMarkFileMove) local to local move")
   if executable(g:netrw_localmovecmd)
    for fname in s:netrwmarkfilelist_{bufnr("%")}
"     call Decho("system(".g:netrw_localmovecmd." ".shellescape(fname)." ".shellescape(s:netrwmftgt).")")
     let ret= system(g:netrw_localmovecmd." ".shellescape(fname)." ".shellescape(s:netrwmftgt))
     if v:shell_error < 0
      call netrw#ErrorMsg(s:ERROR,"command<".g:netrw_localmovecmd."> failed, aborting",54)
      break
     endif
    endfor
   else
    keepj call netrw#ErrorMsg(s:ERROR,"command<".g:netrw_localmovecmd."> is not executable!",57)
   endif

  elseif  a:islocal && !s:netrwmftgt_islocal
   " move: local -> remote
"   call Decho("move from local to remote")
"   call Decho("copy")
   let mflist= s:netrwmarkfilelist_{bufnr("%")}
   keepj call s:NetrwMarkFileCopy(a:islocal)
"   call Decho("remove")
   for fname in mflist
    let barefname = substitute(fname,'^\(.*/\)\(.\{-}\)$','\2','')
    let ok        = s:NetrwLocalRmFile(b:netrw_curdir,barefname,1)
   endfor
   unlet mflist

  elseif !a:islocal &&  s:netrwmftgt_islocal
   " move: remote -> local
"   call Decho("move from remote to local")
"   call Decho("copy")
   let mflist= s:netrwmarkfilelist_{bufnr("%")}
   keepj call s:NetrwMarkFileCopy(a:islocal)
"   call Decho("remove")
   for fname in mflist
    let barefname = substitute(fname,'^\(.*/\)\(.\{-}\)$','\2','')
    let ok        = s:NetrwRemoteRmFile(b:netrw_curdir,barefname,1)
   endfor
   unlet mflist

  elseif !a:islocal && !s:netrwmftgt_islocal
   " move: remote -> remote
"   call Decho("move from remote to remote")
"   call Decho("copy")
   let mflist= s:netrwmarkfilelist_{bufnr("%")}
   keepj call s:NetrwMarkFileCopy(a:islocal)
"   call Decho("remove")
   for fname in mflist
    let barefname = substitute(fname,'^\(.*/\)\(.\{-}\)$','\2','')
    let ok        = s:NetrwRemoteRmFile(b:netrw_curdir,barefname,1)
   endfor
   unlet mflist
  endif

  " -------
  " cleanup
  " -------
"  call Decho("cleanup")

  " remove markings from local buffer
  call s:NetrwUnmarkList(curbufnr,curdir)                   " remove markings from local buffer

  " refresh buffers
  if !s:netrwmftgt_islocal
"   call Decho("refresh netrwmftgt<".s:netrwmftgt.">")
   keepj call s:NetrwRefreshDir(s:netrwmftgt_islocal,s:netrwmftgt)
  endif
  if a:islocal
"   call Decho("refresh b:netrw_curdir<".b:netrw_curdir.">")
   keepj call s:NetrwRefreshDir(a:islocal,b:netrw_curdir)
  endif
  if g:netrw_fastbrowse <= 1
"   call Decho("since g:netrw_fastbrowse=".g:netrw_fastbrowse.", perform shell cmd refresh")
   keepj call s:LocalBrowseShellCmdRefresh()
  endif
  
"  call Dret("s:NetrwMarkFileMove")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFilePrint: (invoked by mp) This function prints marked files {{{2
"                       using the hardcopy command.  Local marked-file list only.
fun! s:NetrwMarkFilePrint(islocal)
"  call Dfunc("s:NetrwMarkFilePrint(islocal=".a:islocal.")")
  let curbufnr= bufnr("%")
  if exists("s:netrwmarkfilelist_{curbufnr}")
   let netrwmarkfilelist = s:netrwmarkfilelist_{curbufnr}
   let curdir            = b:netrw_curdir
   call s:NetrwUnmarkList(curbufnr,curdir)
   for fname in netrwmarkfilelist
    if a:islocal
     if g:netrw_keepdir
      let fname= s:ComposePath(curdir,fname)
     endif
    else
     let fname= curdir.fname
    endif
    1split
    " the autocmds will handle both local and remote files
"    call Decho("exe sil e ".escape(fname,' '))
    exe "sil e ".fnameescape(fname)
"    call Decho("hardcopy")
    hardcopy
    q
   endfor
   2match none
  endif
"  call Dret("s:NetrwMarkFilePrint")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileRegexp: (invoked by mr) This function is used to mark {{{2
"                        files when given a regexp (for which a prompt is
"                        issued).
fun! s:NetrwMarkFileRegexp(islocal)
"  call Dfunc("s:NetrwMarkFileRegexp(islocal=".a:islocal.")")

  " get the regular expression
  call inputsave()
  let regexp= input("Enter regexp: ","","file")
  call inputrestore()

  if a:islocal
   " get the matching list of files using local glob()
"   call Decho("handle local regexp")
   let dirname  = escape(b:netrw_curdir,g:netrw_glob_escape)
   let files = glob(s:ComposePath(dirname,regexp))
"   call Decho("files<".files.">")
   let filelist= split(files,"\n")

  " mark the list of files
  for fname in filelist
"   call Decho("fname<".fname.">")
   keepj call s:NetrwMarkFile(a:islocal,substitute(fname,'^.*/','',''))
  endfor

  else
"   call Decho("handle remote regexp")

   " convert displayed listing into a filelist
   let eikeep = &ei
   let areg   = @a
   sil keepj %y a
   set ei=all ma
"   call Decho("set ei=all ma")
   1split
   keepj call s:NetrwEnew()
   keepj call s:NetrwSafeOptions()
   sil keepj norm! "ap
   keepj 2
   let bannercnt= search('^" =====','W')
   exe "sil keepj 1,".bannercnt."d"
   set bt=nofile
   if     g:netrw_liststyle == s:LONGLIST
    sil keepj %s/\s\{2,}\S.*$//e
    call histdel("/",-1)
   elseif g:netrw_liststyle == s:WIDELIST
    sil keepj %s/\s\{2,}/\r/ge
    call histdel("/",-1)
   elseif g:netrw_liststyle == s:TREELIST
    sil keepj %s/^| //e
    sil! keepj g/^ .*$/d
    call histdel("/",-1)
    call histdel("/",-1)
   endif
   " convert regexp into the more usual glob-style format
   let regexp= substitute(regexp,'\*','.*','g')
"   call Decho("regexp<".regexp.">")
   exe "sil! keepj v/".escape(regexp,'/')."/d"
   call histdel("/",-1)
   let filelist= getline(1,line("$"))
   q!
   for filename in filelist
    keepj call s:NetrwMarkFile(a:islocal,substitute(filename,'^.*/','',''))
   endfor
   unlet filelist
   let @a  = areg
   let &ei = eikeep
  endif

"  call Dret("s:NetrwMarkFileRegexp")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileSource: (invoked by ms) This function sources marked files {{{2
"                        Uses the local marked file list.
fun! s:NetrwMarkFileSource(islocal)
"  call Dfunc("s:NetrwMarkFileSource(islocal=".a:islocal.")")
  let curbufnr= bufnr("%")
  if exists("s:netrwmarkfilelist_{curbufnr}")
   let netrwmarkfilelist = s:netrwmarkfilelist_{bufnr("%")}
   let curdir            = b:netrw_curdir
   call s:NetrwUnmarkList(curbufnr,curdir)
   for fname in netrwmarkfilelist
    if a:islocal
     if g:netrw_keepdir
      let fname= s:ComposePath(curdir,fname)
     endif
    else
     let fname= curdir.fname
    endif
    " the autocmds will handle sourcing both local and remote files
"    call Decho("exe so ".fnameescape(fname))
    exe "so ".fnameescape(fname)
   endfor
   2match none
  endif
"  call Dret("s:NetrwMarkFileSource")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileTag: (invoked by mT) This function applies g:netrw_ctags to marked files {{{2
"                     Uses the global markfilelist
fun! s:NetrwMarkFileTag(islocal)
"  call Dfunc("s:NetrwMarkFileTag(islocal=".a:islocal.")")
  let svpos    = netrw#NetrwSavePosn()
  let curdir   = b:netrw_curdir
  let curbufnr = bufnr("%")

  if exists("s:netrwmarkfilelist")
"   call Decho("s:netrwmarkfilelist".string(s:netrwmarkfilelist).">")
   let netrwmarkfilelist= join(map(deepcopy(s:netrwmarkfilelist), "shellescape(v:val,".!a:islocal.")"))
   call s:NetrwUnmarkAll()

   if a:islocal
    if executable(g:netrw_ctags)
"     call Decho("call system(".g:netrw_ctags." ".netrwmarkfilelist.")")
     call system(g:netrw_ctags." ".netrwmarkfilelist)
    else
     call netrw#ErrorMsg(s:ERROR,"g:netrw_ctags<".g:netrw_ctags."> is not executable!",51)
    endif
   else
    let cmd   = s:RemoteSystem(g:netrw_ctags." ".netrwmarkfilelist)
    call netrw#NetrwObtain(a:islocal,"tags")
    let curdir= b:netrw_curdir
    1split
    e tags
    let path= substitute(curdir,'^\(.*\)/[^/]*$','\1/','')
"    call Decho("curdir<".curdir."> path<".path.">")
    exe 'keepj %s/\t\(\S\+\)\t/\t'.escape(path,"/\n\r\\").'\1\t/e'
    call histdel("/",-1)
    wq!
   endif
   2match none
   call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
   call netrw#NetrwRestorePosn(svpos)
  endif

"  call Dret("s:NetrwMarkFileTag")
endfun

" ---------------------------------------------------------------------
" s:NetrwMarkFileTgt:  (invoked by mt) This function sets up a marked file target {{{2
"   Sets up two variables, 
"     s:netrwmftgt : holds the target directory
"     s:netrwmftgt_islocal : 0=target directory is remote
"                    1=target directory is local
fun! s:NetrwMarkFileTgt(islocal)
"  call Dfunc("s:NetrwMarkFileTgt(islocal=".a:islocal.")")
  let svpos  = netrw#NetrwSavePosn()
  let curdir = b:netrw_curdir
  let hadtgt = exists("s:netrwmftgt")
  if !exists("w:netrw_bannercnt")
   let w:netrw_bannercnt= b:netrw_bannercnt
  endif

  " set up target
  if line(".") < w:netrw_bannercnt
   " if cursor in banner region, use b:netrw_curdir for the target
   let s:netrwmftgt= b:netrw_curdir
"   call Decho("inbanner: s:netrwmftgt<".s:netrwmftgt.">")

  else
   " get word under cursor.
   "  * If directory, use it for the target.
   "  * If file, use b:netrw_curdir for the target
   let curword= s:NetrwGetWord()
   let tgtdir = s:ComposePath(curdir,curword)
   if a:islocal && isdirectory(tgtdir)
    let s:netrwmftgt = tgtdir
"    call Decho("local isdir: s:netrwmftgt<".s:netrwmftgt.">")
   elseif !a:islocal && tgtdir =~ '/$'
    let s:netrwmftgt = tgtdir
"    call Decho("remote isdir: s:netrwmftgt<".s:netrwmftgt.">")
   else
    let s:netrwmftgt = curdir
"    call Decho("isfile: s:netrwmftgt<".s:netrwmftgt.">")
   endif
  endif
  if a:islocal
   " simplify the target (eg. /abc/def/../ghi -> /abc/ghi)
   let s:netrwmftgt= simplify(s:netrwmftgt)
"   call Decho("simplify: s:netrwmftgt<".s:netrwmftgt.">")
  endif
  if g:netrw_cygwin
   let s:netrwmftgt= substitute(system("cygpath ".shellescape(s:netrwmftgt)),'\n$','','')
   let s:netrwmftgt= substitute(s:netrwmftgt,'\n$','','')
  endif
  let s:netrwmftgt_islocal= a:islocal

  if g:netrw_fastbrowse <= 1
   call s:LocalBrowseShellCmdRefresh()
  endif
  call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  call netrw#NetrwRestorePosn(svpos)
  if !hadtgt
   sil! keepj norm! j
  endif

"  call Dret("s:NetrwMarkFileTgt : netrwmftgt<".(exists("s:netrwmftgt")? s:netrwmftgt : "").">")
endfun

" ---------------------------------------------------------------------
" s:NetrwOpenFile: query user for a filename and open it {{{2
fun! s:NetrwOpenFile(islocal)
"  call Dfunc("s:NetrwOpenFile(islocal=".a:islocal.")")
  call inputsave()
  let fname= input("Enter filename: ")
  call inputrestore()
  if fname !~ '[/\\]'
   if exists("b:netrw_curdir")
    if exists("g:netrw_quiet")
     let netrw_quiet_keep = g:netrw_quiet
    endif
    let g:netrw_quiet    = 1
    if b:netrw_curdir =~ '/$'
     exe "e ".fnameescape(b:netrw_curdir.fname)
    else
     exe "e ".fnameescape(b:netrw_curdir."/".fname)
    endif
    if exists("netrw_quiet_keep")
     let g:netrw_quiet= netrw_quiet_keep
    else
     unlet g:netrw_quiet
    endif
   endif
  else
   exe "e ".fnameescape(fname)
  endif
"  call Dret("s:NetrwOpenFile")
endfun

" ---------------------------------------------------------------------
" s:NetrwUnmarkList: delete local marked file lists and remove their contents from the global marked-file list {{{2
fun! s:NetrwUnmarkList(curbufnr,curdir)
"  call Dfunc("s:NetrwUnmarkList(curbufnr=".a:curbufnr." curdir<".a:curdir.">)")

  "  remove all files in local marked-file list from global list
  if exists("s:netrwmarkfilelist_{a:curbufnr}")
   for mfile in s:netrwmarkfilelist_{a:curbufnr}
    let dfile = s:ComposePath(a:curdir,mfile)       " prepend directory to mfile
    let idx   = index(s:netrwmarkfilelist,dfile)    " get index in list of dfile
    call remove(s:netrwmarkfilelist,idx)            " remove from global list
   endfor
   if s:netrwmarkfilelist == []
    unlet s:netrwmarkfilelist
   endif
 
   " getting rid of the local marked-file lists is easy
   unlet s:netrwmarkfilelist_{a:curbufnr}
  endif
  if exists("s:netrwmarkfilemtch_{a:curbufnr}")
   unlet s:netrwmarkfilemtch_{a:curbufnr}
  endif
  2match none
"  call Dret("s:NetrwUnmarkList")
endfun

" ---------------------------------------------------------------------
" s:NetrwUnmarkAll: remove the global marked file list and all local ones {{{2
fun! s:NetrwUnmarkAll()
"  call Dfunc("s:NetrwUnmarkAll()")
  if exists("s:netrwmarkfilelist")
   unlet s:netrwmarkfilelist
  endif
  sil call s:NetrwUnmarkAll2()
  2match none
"  call Dret("s:NetrwUnmarkAll")
endfun

" ---------------------------------------------------------------------
" s:NetrwUnmarkAll2: {{{2
fun! s:NetrwUnmarkAll2()
"  call Dfunc("s:NetrwUnmarkAll2()")
  redir => netrwmarkfilelist_let
  let
  redir END
  let netrwmarkfilelist_list= split(netrwmarkfilelist_let,'\n')          " convert let string into a let list
  call filter(netrwmarkfilelist_list,"v:val =~ '^s:netrwmarkfilelist_'") " retain only those vars that start as s:netrwmarkfilelist_ 
  call map(netrwmarkfilelist_list,"substitute(v:val,'\\s.*$','','')")    " remove what the entries are equal to
  for flist in netrwmarkfilelist_list
   let curbufnr= substitute(flist,'s:netrwmarkfilelist_','','')
   unlet s:netrwmarkfilelist_{curbufnr}
   unlet s:netrwmarkfilemtch_{curbufnr}
  endfor
"  call Dret("s:NetrwUnmarkAll2")
endfun

" ---------------------------------------------------------------------
" s:NetrwUnMarkFile: {{{2
fun! s:NetrwUnMarkFile(islocal)
"  call Dfunc("s:NetrwUnMarkFile(islocal=".a:islocal.")")
  let svpos    = netrw#NetrwSavePosn()
  let curbufnr = bufnr("%")

  " unmark marked file list (although I expect s:NetrwUpload()
  " to do it, I'm just making sure)
  if exists("s:netrwmarkfilelist_{bufnr('%')}")
"   call Decho("unlet'ing: s:netrwmarkfile[list|mtch]_".bufnr("%"))
   unlet s:netrwmarkfilelist
   unlet s:netrwmarkfilelist_{curbufnr}
   unlet s:netrwmarkfilemtch_{curbufnr}
   2match none
  endif

"  call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  call netrw#NetrwRestorePosn(svpos)
"  call Dret("s:NetrwUnMarkFile")
endfun

" ---------------------------------------------------------------------
" s:NetrwMenu: generates the menu for gvim and netrw {{{2
fun! s:NetrwMenu(domenu)

  if !exists("g:NetrwMenuPriority")
   let g:NetrwMenuPriority= 80
  endif

  if has("menu") && has("gui_running") && &go =~# 'm' && g:netrw_menu
"   call Dfunc("NetrwMenu(domenu=".a:domenu.")")

   if !exists("s:netrw_menu_enabled") && a:domenu
"    call Decho("initialize menu")
    let s:netrw_menu_enabled= 1
    exe 'sil! menu '.g:NetrwMenuPriority.'.1     '.g:NetrwTopLvlMenu.'Help<tab><F1>	<F1>'
    exe 'sil! menu '.g:NetrwMenuPriority.'.5     '.g:NetrwTopLvlMenu.'-Sep1-	:'
    exe 'sil! menu '.g:NetrwMenuPriority.'.6     '.g:NetrwTopLvlMenu.'Go\ Up\ Directory<tab>-	-'
    exe 'sil! menu '.g:NetrwMenuPriority.'.7     '.g:NetrwTopLvlMenu.'Apply\ Special\ Viewer<tab>x	x'
    exe 'sil! menu '.g:NetrwMenuPriority.'.8.1   '.g:NetrwTopLvlMenu.'Bookmarks\ and\ History.Bookmark\ Current\ Directory<tab>mb	mb'
    exe 'sil! menu '.g:NetrwMenuPriority.'.8.4   '.g:NetrwTopLvlMenu.'Bookmarks\ and\ History.Goto\ Prev\ Dir\ (History)<tab>u	u'
    exe 'sil! menu '.g:NetrwMenuPriority.'.8.5   '.g:NetrwTopLvlMenu.'Bookmarks\ and\ History.Goto\ Next\ Dir\ (History)<tab>U	U'
    exe 'sil! menu '.g:NetrwMenuPriority.'.8.6   '.g:NetrwTopLvlMenu.'Bookmarks\ and\ History.List<tab>qb	qb'
    exe 'sil! menu '.g:NetrwMenuPriority.'.9.1   '.g:NetrwTopLvlMenu.'Browsing\ Control.Edit\ File\ Hiding\ List<tab><ctrl-h>'."	\<c-h>'"
    exe 'sil! menu '.g:NetrwMenuPriority.'.9.2   '.g:NetrwTopLvlMenu.'Browsing\ Control.Edit\ Sorting\ Sequence<tab>S	S'
    exe 'sil! menu '.g:NetrwMenuPriority.'.9.3   '.g:NetrwTopLvlMenu.'Browsing\ Control.Quick\ Hide/Unhide\ Dot\ Files<tab>'."gh	gh"
    exe 'sil! menu '.g:NetrwMenuPriority.'.9.4   '.g:NetrwTopLvlMenu.'Browsing\ Control.Refresh\ Listing<tab>'."<ctrl-l>	\<c-l>"
    exe 'sil! menu '.g:NetrwMenuPriority.'.9.5   '.g:NetrwTopLvlMenu.'Browsing\ Control.Settings/Options<tab>:NetrwSettings	'.":NetrwSettings\<cr>"
    exe 'sil! menu '.g:NetrwMenuPriority.'.10    '.g:NetrwTopLvlMenu.'Delete\ File/Directory<tab>D	D'
    exe 'sil! menu '.g:NetrwMenuPriority.'.11.1  '.g:NetrwTopLvlMenu.'Edit\ File/Dir.Create\ New\ File<tab>%	%'
    exe 'sil! menu '.g:NetrwMenuPriority.'.11.1  '.g:NetrwTopLvlMenu.'Edit\ File/Dir.In\ Current\ Window<tab><cr>	'."\<cr>"
    exe 'sil! menu '.g:NetrwMenuPriority.'.11.2  '.g:NetrwTopLvlMenu.'Edit\ File/Dir.Preview\ File/Directory<tab>p	p'
    exe 'sil! menu '.g:NetrwMenuPriority.'.11.3  '.g:NetrwTopLvlMenu.'Edit\ File/Dir.In\ Previous\ Window<tab>P	P'
    exe 'sil! menu '.g:NetrwMenuPriority.'.11.4  '.g:NetrwTopLvlMenu.'Edit\ File/Dir.In\ New\ Window<tab>o	o'
    exe 'sil! menu '.g:NetrwMenuPriority.'.11.5  '.g:NetrwTopLvlMenu.'Edit\ File/Dir.In\ New\ Vertical\ Window<tab>v	v'
    exe 'sil! menu '.g:NetrwMenuPriority.'.12.1  '.g:NetrwTopLvlMenu.'Explore.Directory\ Name	:Explore '
    exe 'sil! menu '.g:NetrwMenuPriority.'.12.2  '.g:NetrwTopLvlMenu.'Explore.Filenames\ Matching\ Pattern\ (curdir\ only)<tab>:Explore\ */	:Explore */'
    exe 'sil! menu '.g:NetrwMenuPriority.'.12.2  '.g:NetrwTopLvlMenu.'Explore.Filenames\ Matching\ Pattern\ (+subdirs)<tab>:Explore\ **/	:Explore **/'
    exe 'sil! menu '.g:NetrwMenuPriority.'.12.3  '.g:NetrwTopLvlMenu.'Explore.Files\ Containing\ String\ Pattern\ (curdir\ only)<tab>:Explore\ *//	:Explore *//'
    exe 'sil! menu '.g:NetrwMenuPriority.'.12.4  '.g:NetrwTopLvlMenu.'Explore.Files\ Containing\ String\ Pattern\ (+subdirs)<tab>:Explore\ **//	:Explore **//'
    exe 'sil! menu '.g:NetrwMenuPriority.'.12.4  '.g:NetrwTopLvlMenu.'Explore.Next\ Match<tab>:Nexplore	:Nexplore<cr>'
    exe 'sil! menu '.g:NetrwMenuPriority.'.12.4  '.g:NetrwTopLvlMenu.'Explore.Prev\ Match<tab>:Pexplore	:Pexplore<cr>'
    exe 'sil! menu '.g:NetrwMenuPriority.'.13    '.g:NetrwTopLvlMenu.'Make\ Subdirectory<tab>d	d'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.1  '.g:NetrwTopLvlMenu.'Marked\ Files.Mark\ File<tab>mf	mf'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.2  '.g:NetrwTopLvlMenu.'Marked\ Files.Mark\ Files\ by\ Regexp<tab>mr	mr'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.3  '.g:NetrwTopLvlMenu.'Marked\ Files.Hide-Show-List\ Control<tab>a	a'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.4  '.g:NetrwTopLvlMenu.'Marked\ Files.Copy\ To\ Target<tab>mc	mc'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.5  '.g:NetrwTopLvlMenu.'Marked\ Files.Delete<tab>D	D'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.6  '.g:NetrwTopLvlMenu.'Marked\ Files.Diff<tab>md	md'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.7  '.g:NetrwTopLvlMenu.'Marked\ Files.Edit<tab>me	me'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.8  '.g:NetrwTopLvlMenu.'Marked\ Files.Exe\ Cmd<tab>mx	mx'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.9  '.g:NetrwTopLvlMenu.'Marked\ Files.Move\ To\ Target<tab>mm	mm'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.10 '.g:NetrwTopLvlMenu.'Marked\ Files.Obtain<tab>O	O'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.11 '.g:NetrwTopLvlMenu.'Marked\ Files.Print<tab>mp	mp'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.12 '.g:NetrwTopLvlMenu.'Marked\ Files.Replace<tab>R	R'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.13 '.g:NetrwTopLvlMenu.'Marked\ Files.Set\ Target<tab>mt	mt'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.14 '.g:NetrwTopLvlMenu.'Marked\ Files.Tag<tab>mT	mT'
    exe 'sil! menu '.g:NetrwMenuPriority.'.14.15 '.g:NetrwTopLvlMenu.'Marked\ Files.Zip/Unzip/Compress/Uncompress<tab>mz	mz'
    exe 'sil! menu '.g:NetrwMenuPriority.'.15    '.g:NetrwTopLvlMenu.'Obtain\ File<tab>O	O'
    exe 'sil! menu '.g:NetrwMenuPriority.'.16.1  '.g:NetrwTopLvlMenu.'Style.Listing\ Style\ (thin-long-wide-tree)<tab>i	i'
    exe 'sil! menu '.g:NetrwMenuPriority.'.16.2  '.g:NetrwTopLvlMenu.'Style.Normal-Hide-Show<tab>a	a'
    exe 'sil! menu '.g:NetrwMenuPriority.'.16.3  '.g:NetrwTopLvlMenu.'Style.Reverse\ Sorting\ Order<tab>'."r	r"
    exe 'sil! menu '.g:NetrwMenuPriority.'.16.4  '.g:NetrwTopLvlMenu.'Style.Sorting\ Method\ (name-time-size)<tab>s	s'
    exe 'sil! menu '.g:NetrwMenuPriority.'.17    '.g:NetrwTopLvlMenu.'Rename\ File/Directory<tab>R	R'
    exe 'sil! menu '.g:NetrwMenuPriority.'.18    '.g:NetrwTopLvlMenu.'Set\ Current\ Directory<tab>c	c'
    call s:NetrwBookmarkMenu() " provide some history!  uses priorities 2,3, reserves 4, 8.2.x
    let s:netrw_menucnt= 28

   elseif !a:domenu
    let s:netrwcnt = 0
    let curwin     = winnr()
    windo if getline(2) =~ "Netrw" | let s:netrwcnt= s:netrwcnt + 1 | endif
    exe curwin."wincmd w"

    if s:netrwcnt <= 1
"     call Decho("clear menus")
     exe 'sil! unmenu '.g:NetrwTopLvlMenu
"     call Decho('exe sil! unmenu '.g:NetrwTopLvlMenu.'*')
     sil! unlet s:netrw_menu_enabled
    endif
   endif
"   call Dret("NetrwMenu")
  endif

endfun

" ---------------------------------------------------------------------
" s:NetrwObtain: obtain file under cursor or from markfile list {{{2
"                Used by the O maps (as <SID>NetrwObtain())
fun! s:NetrwObtain(islocal)
"  call Dfunc("NetrwObtain(islocal=".a:islocal.")")

  if exists("s:netrwmarkfilelist_{bufnr('%')}")
   let islocal= s:netrwmarkfilelist_{bufnr('%')}[1] !~ '^\a\+://'
   call netrw#NetrwObtain(islocal,s:netrwmarkfilelist_{bufnr('%')})
   call s:NetrwUnmarkList(bufnr('%'),b:netrw_curdir)
  else
   call netrw#NetrwObtain(a:islocal,expand("<cWORD>"))
  endif

"  call Dret("NetrwObtain")
endfun

" ---------------------------------------------------------------------
" netrw#NetrwObtain: {{{2
"   netrw#NetrwObtain(islocal,fname[,tgtdirectory])
"     islocal=0  obtain from remote source
"            =1  obtain from local source
"     fname  :   a filename or a list of filenames
"     tgtdir :   optional place where files are to go  (not present, uses getcwd())
fun! netrw#NetrwObtain(islocal,fname,...)
"  call Dfunc("netrw#NetrwObtain(islocal=".a:islocal." fname<".((type(a:fname) == 1)? a:fname : string(a:fname)).">) a:0=".a:0)
  " NetrwStatusLine support - for obtaining support

  if type(a:fname) == 1
   let fnamelist= [ a:fname ]
  elseif type(a:fname) == 3
   let fnamelist= a:fname
  else
   call netrw#ErrorMsg(s:ERROR,"attempting to use NetrwObtain on something not a filename or a list",62)
"   call Dret("netrw#NetrwObtain")
   return
  endif
"  call Decho("fnamelist<".string(fnamelist).">")
  if a:0 > 0
   let tgtdir= a:1
  else
   let tgtdir= getcwd()
  endif
"  call Decho("tgtdir<".tgtdir.">")

  if exists("b:netrw_islocal") && b:netrw_islocal
   " obtain a file from local b:netrw_curdir to (local) tgtdir
"   call Decho("obtain a file from local ".b:netrw_curdir." to ".tgtdir)
   if exists("b:netrw_curdir") && getcwd() != b:netrw_curdir
    let topath= s:ComposePath(tgtdir,"")
    if (has("win32") || has("win95") || has("win64") || has("win16"))
     " transfer files one at time
"     call Decho("transfer files one at a time")
     for fname in fnamelist
"      call Decho("system(".g:netrw_localcopycmd." ".shellescape(fname)." ".shellescape(topath).")")
      call system(g:netrw_localcopycmd." ".shellescape(fname)." ".shellescape(topath))
     endfor
    else
     " transfer files with one command
"     call Decho("transfer files with one command")
     let filelist= join(map(deepcopy(fnamelist),"shellescape(v:val)"))
"     call Decho("system(".g:netrw_localcopycmd." ".filelist." ".shellescape(topath).")")
     call system(g:netrw_localcopycmd." ".filelist." ".shellescape(topath))
    endif
   elseif !exists("b:netrw_curdir")
    call netrw#ErrorMsg(s:ERROR,"local browsing directory doesn't exist!",36)
   else
    call netrw#ErrorMsg(s:WARNING,"local browsing directory and current directory are identical",37)
   endif

  else
   " obtain files from remote b:netrw_curdir to local tgtdir
"   call Decho("obtain a file from remote ".b:netrw_curdir." to ".tgtdir)
   if type(a:fname) == 1
    call s:SetupNetrwStatusLine('%f %h%m%r%=%9*Obtaining '.a:fname)
   endif
   call s:NetrwMethod(b:netrw_curdir)

   if b:netrw_method == 4
    " obtain file using scp
"    call Decho("obtain via scp (method#4)")
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " ".g:netrw_scpport." ".g:netrw_port
    else
     let useport= ""
    endif
    if b:netrw_fname =~ '/'
     let path= substitute(b:netrw_fname,'^\(.*/\).\{-}$','\1','')
    else
     let path= ""
    endif
    let filelist= join(map(deepcopy(fnamelist),'shellescape(g:netrw_machine.":".path.v:val,1)'))
"    call Decho("exe ".s:netrw_silentxfer."!".g:netrw_scp_cmd.shellescape(useport,1)." ".filelist." ".shellescape(tgtdir,1))
    exe s:netrw_silentxfer."!".g:netrw_scp_cmd.shellescape(useport,1)." ".filelist." ".shellescape(tgtdir,1)

   elseif b:netrw_method == 2
    " obtain file using ftp + .netrc
"     call Decho("obtain via ftp+.netrc (method #2)")
     call s:SaveBufVars()|sil keepjumps new|call s:RestoreBufVars()
     let tmpbufnr= bufnr("%")
     setlocal ff=unix
     if exists("g:netrw_ftpmode") && g:netrw_ftpmode != ""
      keepj put =g:netrw_ftpmode
"      call Decho("filter input: ".getline('$'))
     endif

     if exists("b:netrw_fname") && b:netrw_fname != ""
      call setline(line("$")+1,'cd "'.b:netrw_fname.'"')
"      call Decho("filter input: ".getline('$'))
     endif

     if exists("g:netrw_ftpextracmd")
      keepj put =g:netrw_ftpextracmd
"      call Decho("filter input: ".getline('$'))
     endif
     for fname in fnamelist
      call setline(line("$")+1,'get "'.fname.'"')
"      call Decho("filter input: ".getline('$'))
     endfor
     if exists("g:netrw_port") && g:netrw_port != ""
"      call Decho("executing: %!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1))
      exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1)
     else
"      call Decho("executing: %!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1))
      exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)
     endif
     " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
     if getline(1) !~ "^$" && !exists("g:netrw_quiet") && getline(1) !~ '^Trying '
      let debugkeep= &debug
      setlocal debug=msg
      call netrw#ErrorMsg(s:ERROR,getline(1),4)
      let &debug= debugkeep
     endif

   elseif b:netrw_method == 3
    " obtain with ftp + machine, id, passwd, and fname (ie. no .netrc)
"    call Decho("obtain via ftp+mipf (method #3)")
    call s:SaveBufVars()|sil keepjumps new|call s:RestoreBufVars()
    let tmpbufnr= bufnr("%")
    setlocal ff=unix

    if exists("g:netrw_port") && g:netrw_port != ""
     keepj put ='open '.g:netrw_machine.' '.g:netrw_port
"     call Decho("filter input: ".getline('$'))
    else
     keepj put ='open '.g:netrw_machine
"     call Decho("filter input: ".getline('$'))
    endif

    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     keepj put =g:netrw_uid
"     call Decho("filter input: ".getline('$'))
     keepj put ='\"'.s:netrw_passwd.'\"'
"     call Decho("filter input: ".getline('$'))
    else
     keepj put ='user \"'.g:netrw_uid.'\" \"'.s:netrw_passwd.'\"'
"     call Decho("filter input: ".getline('$'))
    endif

    if exists("g:netrw_ftpmode") && g:netrw_ftpmode != ""
     keepj put =g:netrw_ftpmode
"     call Decho("filter input: ".getline('$'))
    endif

    if exists("b:netrw_fname") && b:netrw_fname != ""
     keepj call setline(line("$")+1,'cd "'.b:netrw_fname.'"')
"     call Decho("filter input: ".getline('$'))
    endif

    if exists("g:netrw_ftpextracmd")
     keepj put =g:netrw_ftpextracmd
"     call Decho("filter input: ".getline('$'))
    endif

    if exists("g:netrw_ftpextracmd")
     keepj put =g:netrw_ftpextracmd
"     call Decho("filter input: ".getline('$'))
    endif
    for fname in fnamelist
     keepj call setline(line("$")+1,'get "'.fname.'"')
    endfor
"    call Decho("filter input: ".getline('$'))

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
    keepj norm! 1Gdd
"    call Decho("executing: %!".s:netrw_ftp_cmd." -i -n")
    exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i -n"
    " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
    if getline(1) !~ "^$"
"     call Decho("error<".getline(1).">")
     if !exists("g:netrw_quiet")
      keepj call netrw#ErrorMsg(s:ERROR,getline(1),5)
     endif
    endif
   elseif !exists("b:netrw_method") || b:netrw_method < 0
"    call Dfunc("netrw#NetrwObtain : unsupported method")
    return
   endif

   " restore status line
   if type(a:fname) == 1 && exists("s:netrw_users_stl")
    keepj call s:SetupNetrwStatusLine(s:netrw_users_stl)
   endif

  endif

  " cleanup
  if exists("tmpbufnr")
   if bufnr("%") != tmpbufnr
    exe tmpbufnr."bw!"
   else
    q!
   endif
  endif

"  call Dret("netrw#NetrwObtain")
endfun

" ---------------------------------------------------------------------
" s:NetrwPrevWinOpen: open file/directory in previous window.  {{{2
"   If there's only one window, then the window will first be split.
"   Returns:
"     choice = 0 : didn't have to choose
"     choice = 1 : saved modified file in window first
"     choice = 2 : didn't save modified file, opened window
"     choice = 3 : cancel open
fun! s:NetrwPrevWinOpen(islocal)
"  call Dfunc("NetrwPrevWinOpen(islocal=".a:islocal.")")

  " grab a copy of the b:netrw_curdir to pass it along to newly split windows
  let curdir    = b:netrw_curdir

  " get last window number and the word currently under the cursor
  let lastwinnr = winnr("$")
  let curword   = s:NetrwGetWord()
  let choice    = 0
"  call Decho("lastwinnr=".lastwinnr." curword<".curword.">")

  let didsplit  = 0
  if lastwinnr == 1
   " if only one window, open a new one first
"   call Decho("only one window, so open a new one (g:netrw_alto=".g:netrw_alto.")")
   if g:netrw_preview
"    call Decho("exe ".(g:netrw_alto? "top " : "bot ")."vert ".g:netrw_winsize."wincmd s")
    exe (g:netrw_alto? "top " : "bot ")."vert ".g:netrw_winsize."wincmd s"
   else
"    call Decho("exe ".(g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s")
    exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   endif
   let didsplit  = 1

  else
   keepj call s:SaveBufVars()
"   call Decho("wincmd p")
   wincmd p
   keepj call s:RestoreBufVars()
   " if the previous window's buffer has been changed (is modified),
   " and it doesn't appear in any other extant window, then ask the
   " user if s/he wants to abandon modifications therein.
   let bnr    = winbufnr(0)
   let bnrcnt = 0
   if &mod
"    call Decho("detected: prev window's buffer has been modified: bnr=".bnr." winnr#".winnr())
    let eikeep= &ei
    set ei=all
    windo if winbufnr(0) == bnr | let bnrcnt=bnrcnt+1 | endif
    exe bnr."wincmd p"
    let &ei= eikeep
"    call Decho("bnr=".bnr." bnrcnt=".bnrcnt." buftype=".&bt." winnr#".winnr())
    if bnrcnt == 1
     let bufname = bufname(winbufnr(winnr()))
     let choice  = confirm("Save modified file<".bufname.">?","&Yes\n&No\n&Cancel")
"     call Decho("bufname<".bufname."> choice=".choice." winnr#".winnr())

     if choice == 1
      " Yes -- write file & then browse
      let v:errmsg= ""
      sil w
      if v:errmsg != ""
       call netrw#ErrorMsg(s:ERROR,"unable to write <".bufname.">!",30)
       if didsplit
       	q
       else
       	wincmd p
       endif
"       call Dret("NetrwPrevWinOpen ".choice." : unable to write <".bufname.">")
       return choice
      endif

     elseif choice == 2
      " No -- don't worry about changed file, just browse anyway
      setlocal nomod
      keepj call netrw#ErrorMsg(s:WARNING,bufname." changes to ".bufname." abandoned",31)
      wincmd p

     else
      " Cancel -- don't do this
      if didsplit
       q
      else
       wincmd p
      endif
"      call Dret("NetrwPrevWinOpen ".choice." : cancelled")
      return choice
     endif
    endif
   endif
  endif

  " restore b:netrw_curdir (window split/enew may have lost it)
  let b:netrw_curdir= curdir
  if a:islocal < 2
   if a:islocal
    call netrw#LocalBrowseCheck(s:NetrwBrowseChgDir(a:islocal,curword))
   else
    call s:NetrwBrowse(a:islocal,s:NetrwBrowseChgDir(a:islocal,curword))
   endif
  endif
"  call Dret("NetrwPrevWinOpen ".choice)
  return choice
endfun

" ---------------------------------------------------------------------
" s:NetrwUpload: load fname to tgt (used by NetrwMarkFileCopy()) {{{2
"                Always assumed to be local -> remote
"                call s:NetrwUpload(filename, target)
"                call s:NetrwUpload(filename, target, fromdirectory)
fun! s:NetrwUpload(fname,tgt,...)
"  call Dfunc("s:NetrwUpload(fname<".((type(a:fname) == 1)? a:fname : string(a:fname))."> tgt<".a:tgt.">) a:0=".a:0)

  if a:tgt =~ '^\a\+://'
   let tgtdir= substitute(a:tgt,'^\a\+://[^/]\+/\(.\{-}\)$','\1','')
  else
   let tgtdir= substitute(a:tgt,'^\(.*\)/[^/]*$','\1','')
  endif
"  call Decho("tgtdir<".tgtdir.">")

  if a:0 > 0
   let fromdir= a:1
  else
   let fromdir= getcwd()
  endif
"  call Decho("fromdir<".fromdir.">")

  if type(a:fname) == 1
   " handle uploading a single file using NetWrite
"   call Decho("handle uploading a single file via NetWrite")
   1split
"   call Decho("exe e ".fnameescape(a:fname))
   exe "e ".fnameescape(a:fname)
"   call Decho("now locally editing<".expand("%").">, has ".line("$")." lines")
   if a:tgt =~ '/$'
    let wfname= substitute(a:fname,'^.*/','','')
"    call Decho("exe w! ".fnameescape(wfname))
    exe "w! ".fnameescape(a:tgt.wfname)
   else
"    call Decho("writing local->remote: exe w ".fnameescape(a:tgt))
    exe "w ".fnameescape(a:tgt)
"    call Decho("done writing local->remote")
   endif
   q!

  elseif type(a:fname) == 3
   " handle uploading a list of files via scp
"   call Decho("handle uploading a list of files via scp")
   let curdir= getcwd()
   if a:tgt =~ '^scp:'
    exe "keepjumps sil lcd ".fnameescape(fromdir)
    let filelist= deepcopy(s:netrwmarkfilelist_{bufnr('%')})
    let args    = join(map(filelist,"shellescape(v:val, 1)"))
    if exists("g:netrw_port") && g:netrw_port != ""
     let useport= " ".g:netrw_scpport." ".g:netrw_port
    else
     let useport= ""
    endif
    let machine = substitute(a:tgt,'^scp://\([^/:]\+\).*$','\1','')
    let tgt     = substitute(a:tgt,'^scp://[^/]\+/\(.*\)$','\1','')
"    call Decho("exe ".s:netrw_silentxfer."!".g:netrw_scp_cmd.shellescape(useport,1)." ".args." ".shellescape(machine.":".tgt,1))
    exe s:netrw_silentxfer."!".g:netrw_scp_cmd.shellescape(useport,1)." ".args." ".shellescape(machine.":".tgt,1)
    exe "keepjumps sil lcd ".fnameescape(curdir)

   elseif a:tgt =~ '^ftp:'
    call s:NetrwMethod(a:tgt)

    if b:netrw_method == 2
     " handle uploading a list of files via ftp+.netrc
     let netrw_fname = b:netrw_fname
     sil keepj new
"     call Decho("filter input window#".winnr())

     keepj put =g:netrw_ftpmode
"     call Decho("filter input: ".getline('$'))

     if exists("g:netrw_ftpextracmd")
      keepj put =g:netrw_ftpextracmd
"      call Decho("filter input: ".getline('$'))
     endif

     keepj call setline(line("$")+1,'lcd "'.fromdir.'"')
"     call Decho("filter input: ".getline('$'))

     if tgtdir == ""
      let tgtdir= '/'
     endif
     keepj call setline(line("$")+1,'cd "'.tgtdir.'"')
"     call Decho("filter input: ".getline('$'))

     for fname in a:fname
      keepj call setline(line("$")+1,'put "'.fname.'"')
"      call Decho("filter input: ".getline('$'))
     endfor

     if exists("g:netrw_port") && g:netrw_port != ""
"      call Decho("executing: ".s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1))
      exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1)
     else
"      call Decho("filter input window#".winnr())
"      call Decho("executing: ".s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1))
      exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)
     endif
     " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
     sil keepj g/Local directory now/d
     call histdel("/",-1)
     if getline(1) !~ "^$" && !exists("g:netrw_quiet") && getline(1) !~ '^Trying '
      call netrw#ErrorMsg(s:ERROR,getline(1),14)
     else
      bw!|q
     endif

    elseif b:netrw_method == 3
     " upload with ftp + machine, id, passwd, and fname (ie. no .netrc)
     let netrw_fname= b:netrw_fname
     keepj call s:SaveBufVars()|sil keepj new|keepj call s:RestoreBufVars()
     let tmpbufnr= bufnr("%")
     setlocal ff=unix

     if exists("g:netrw_port") && g:netrw_port != ""
      keepj put ='open '.g:netrw_machine.' '.g:netrw_port
"      call Decho("filter input: ".getline('$'))
     else
      keepj put ='open '.g:netrw_machine
"      call Decho("filter input: ".getline('$'))
     endif

     if exists("g:netrw_ftp") && g:netrw_ftp == 1
      keepj put =g:netrw_uid
"      call Decho("filter input: ".getline('$'))
      keepj call setline(line("$")+1,'"'.s:netrw_passwd.'"')
"      call Decho("filter input: ".getline('$'))
     else
      keepj put ='user \"'.g:netrw_uid.'\" \"'.s:netrw_passwd.'\"'
"      call Decho("filter input: ".getline('$'))
     endif

     keepj call setline(line("$")+1,'lcd "'.fromdir.'"')
"     call Decho("filter input: ".getline('$'))

     if exists("b:netrw_fname") && b:netrw_fname != ""
      keepj call setline(line("$")+1,'cd "'.b:netrw_fname.'"')
"      call Decho("filter input: ".getline('$'))
     endif

     if exists("g:netrw_ftpextracmd")
      keepj put =g:netrw_ftpextracmd
"      call Decho("filter input: ".getline('$'))
     endif

     for fname in a:fname
      keepj call setline(line("$")+1,'put "'.fname.'"')
"      call Decho("filter input: ".getline('$'))
     endfor

     " perform ftp:
     " -i       : turns off interactive prompting from ftp
     " -n  unix : DON'T use <.netrc>, even though it exists
     " -n  win32: quit being obnoxious about password
     keepj norm! 1Gdd
"     call Decho("executing: ".s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i -n")
     exe s:netrw_silentxfer."%!".s:netrw_ftp_cmd." -i -n"
     " If the result of the ftp operation isn't blank, show an error message (tnx to Doug Claar)
     sil keepj g/Local directory now/d
     call histdel("/",-1)
     if getline(1) !~ "^$" && !exists("g:netrw_quiet") && getline(1) !~ '^Trying '
      let debugkeep= &debug
      setlocal debug=msg
      call netrw#ErrorMsg(s:ERROR,getline(1),15)
      let &debug = debugkeep
      let mod    = 1
     else
      bw!|q
     endif
    elseif !exists("b:netrw_method") || b:netrw_method < 0
"     call Dfunc("netrw#NetrwUpload : unsupported method")
     return
    endif
   else
    call netrw#ErrorMsg(s:ERROR,"can't obtain files with protocol from<".a:tgt.">",63)
   endif
  endif

"  call Dret("s:NetrwUpload")
endfun

" ---------------------------------------------------------------------
" s:NetrwPreview: {{{2
fun! s:NetrwPreview(path) range
"  call Dfunc("NetrwPreview(path<".a:path.">)")
  keepj call s:NetrwOptionSave("s:")
  keepj call s:NetrwSafeOptions()
  if has("quickfix")
   if !isdirectory(a:path)
    if g:netrw_preview && !g:netrw_alto
     let pvhkeep= &pvh
     let &pvh   = winwidth(0) - g:netrw_winsize
    endif
    exe (g:netrw_alto? "top " : "bot ").(g:netrw_preview? "vert " : "")."pedit ".fnameescape(a:path)
    if exists("pvhkeep")
     let &pvh= pvhkeep
    endif
   elseif !exists("g:netrw_quiet")
    keepj call netrw#ErrorMsg(s:WARNING,"sorry, cannot preview a directory such as <".a:path.">",38)
   endif
  elseif !exists("g:netrw_quiet")
   keepj call netrw#ErrorMsg(s:WARNING,"sorry, to preview your vim needs the quickfix feature compiled in",39)
  endif
  keepj call s:NetrwOptionRestore("s:")
"  call Dret("NetrwPreview")
endfun

" ---------------------------------------------------------------------
" s:NetrwRefresh: {{{2
fun! s:NetrwRefresh(islocal,dirname)
"  call Dfunc("NetrwRefresh(islocal<".a:islocal.">,dirname=".a:dirname.") hide=".g:netrw_hide." sortdir=".g:netrw_sort_direction)
  " at the current time (Mar 19, 2007) all calls to NetrwRefresh() call NetrwBrowseChgDir() first.
  " NetrwBrowseChgDir() may clear the display; hence a NetrwSavePosn() may not work if its placed here.
  " Also, NetrwBrowseChgDir() now does a NetrwSavePosn() itself.
  setlocal ma noro
"  call Decho("setlocal ma noro")
"  call Decho("clear buffer<".expand("%")."> with :%d")
  sil! keepj %d
  if a:islocal
   keepj call netrw#LocalBrowseCheck(a:dirname)
  else
   keepj call s:NetrwBrowse(a:islocal,a:dirname)
  endif
  keepj call netrw#NetrwRestorePosn()

  " restore file marks
  if exists("s:netrwmarkfilemtch_{bufnr('%')}") && s:netrwmarkfilemtch_{bufnr("%")} != ""
"   call Decho("exe 2match netrwMarkFile /".s:netrwmarkfilemtch_{bufnr("%")}."/")
   exe "2match netrwMarkFile /".s:netrwmarkfilemtch_{bufnr("%")}."/"
  else
"   call Decho("2match none")
   2match none
  endif

"  redraw!
"  call Dret("NetrwRefresh")
endfun

" ---------------------------------------------------------------------
" s:NetrwRefreshDir: refreshes a directory by name {{{2
"                    Called by NetrwMarkFileCopy()
"                    Interfaces to s:NetrwRefresh() and s:LocalBrowseShellCmdRefresh()
fun! s:NetrwRefreshDir(islocal,dirname)
"  call Dfunc("s:NetrwRefreshDir(islocal=".a:islocal." dirname<".a:dirname.">) fastbrowse=".g:netrw_fastbrowse)
  if g:netrw_fastbrowse == 0
   " slowest mode (keep buffers refreshed, local or remote)
"   call Decho("slowest mode: keep buffers refreshed, local or remote")
   let tgtwin= bufwinnr(a:dirname)
"   call Decho("tgtwin= bufwinnr(".a:dirname.")=".tgtwin)

   if tgtwin > 0
    " tgtwin is being displayed, so refresh it
    let curwin= winnr()
"    call Decho("refresh tgtwin#".tgtwin." (curwin#".curwin.")")
    exe tgtwin."wincmd w"
    keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./')) 
    exe curwin."wincmd w"

   elseif bufnr(a:dirname) > 0
    let bn= bufnr(a:dirname)
"    call Decho("bd bufnr(".a:dirname.")=".bn)
    exe "sil bd ".bn
   endif

  elseif g:netrw_fastbrowse <= 1
"   call Decho("medium-speed mode: refresh local buffers only")
   keepj call s:LocalBrowseShellCmdRefresh()
  endif
"  call Dret("s:NetrwRefreshDir")
endfun

" ---------------------------------------------------------------------
" s:NetrwSetSort: sets up the sort based on the g:netrw_sort_sequence {{{2
"          What this function does is to compute a priority for the patterns
"          in the g:netrw_sort_sequence.  It applies a substitute to any
"          "files" that satisfy each pattern, putting the priority / in
"          front.  An "*" pattern handles the default priority.
fun! s:NetrwSetSort()
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
   if priority < 10
    let spriority= "00".priority.g:netrw_sepchr
   elseif priority < 100
    let spriority= "0".priority.g:netrw_sepchr
   else
    let spriority= priority.g:netrw_sepchr
   endif
"   call Decho("priority=".priority." spriority<".spriority."> seq<".seq."> seqlist<".seqlist.">")

   " sanity check
   if w:netrw_bannercnt > line("$")
    " apparently no files were left after a Hiding pattern was used
"    call Dret("SetSort : no files left after hiding")
    return
   endif
   if seq == '*'
    let starpriority= spriority
   else
    exe 'sil keepj '.w:netrw_bannercnt.',$g/'.seq.'/s/^/'.spriority.'/'
    call histdel("/",-1)
    " sometimes multiple sorting patterns will match the same file or directory.
    " The following substitute is intended to remove the excess matches.
    exe 'sil keepj '.w:netrw_bannercnt.',$g/^\d\{3}'.g:netrw_sepchr.'\d\{3}\//s/^\d\{3}'.g:netrw_sepchr.'\(\d\{3}\/\).\@=/\1/e'
    keepj call histdel("/",-1)
   endif
   let priority = priority + 1
  endwhile
  if exists("starpriority")
   exe 'sil keepj '.w:netrw_bannercnt.',$v/^\d\{3}'.g:netrw_sepchr.'/s/^/'.starpriority.'/'
   keepj call histdel("/",-1)
  endif

  " Following line associated with priority -- items that satisfy a priority
  " pattern get prefixed by ###/ which permits easy sorting by priority.
  " Sometimes files can satisfy multiple priority patterns -- only the latest
  " priority pattern needs to be retained.  So, at this point, these excess
  " priority prefixes need to be removed, but not directories that happen to
  " be just digits themselves.
  exe 'sil keepj '.w:netrw_bannercnt.',$s/^\(\d\{3}'.g:netrw_sepchr.'\)\%(\d\{3}'.g:netrw_sepchr.'\)\+\ze./\1/e'
  keepj call histdel("/",-1)

"  call Dret("SetSort")
endfun

" =====================================================================
" s:NetrwSortStyle: change sorting style (name - time - size) and refresh display {{{2
fun! s:NetrwSortStyle(islocal)
"  call Dfunc("s:NetrwSortStyle(islocal=".a:islocal.") netrw_sort_by<".g:netrw_sort_by.">")
  keepj call s:NetrwSaveWordPosn()
  let svpos= netrw#NetrwSavePosn()

  let g:netrw_sort_by= (g:netrw_sort_by =~ 'n')? 'time' : (g:netrw_sort_by =~ 't')? 'size' : 'name'
  keepj norm! 0
  keepj call s:NetrwRefresh(a:islocal,s:NetrwBrowseChgDir(a:islocal,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)

"  call Dret("s:NetrwSortStyle : netrw_sort_by<".g:netrw_sort_by.">")
endfun

" ---------------------------------------------------------------------
" s:NetrwSplit: mode {{{2
"           =0 : net   and o
"           =1 : net   and t
"           =2 : net   and v
"           =3 : local and o
"           =4 : local and t
"           =5 : local and v
fun! s:NetrwSplit(mode)
"  call Dfunc("s:NetrwSplit(mode=".a:mode.") alto=".g:netrw_alto." altv=".g:netrw_altv)

  call s:SaveWinVars()

  if a:mode == 0
   " remote and o
"   call Decho("exe ".(g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s")
   exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   let s:didsplit= 1
   keepj call s:RestoreWinVars()
   keepj call s:NetrwBrowse(0,s:NetrwBrowseChgDir(0,s:NetrwGetWord()))
   unlet s:didsplit

  elseif a:mode == 1
   " remote and t
   let newdir  = s:NetrwBrowseChgDir(0,s:NetrwGetWord())
"   call Decho("tabnew")
   tabnew
   let s:didsplit= 1
   keepj call s:RestoreWinVars()
   keepj call s:NetrwBrowse(0,newdir)
   unlet s:didsplit

  elseif a:mode == 2
   " remote and v
"   call Decho("exe ".(g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v")
   exe (g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v"
   let s:didsplit= 1
   keepj call s:RestoreWinVars()
   keepj call s:NetrwBrowse(0,s:NetrwBrowseChgDir(0,s:NetrwGetWord()))
   unlet s:didsplit

  elseif a:mode == 3
   " local and o
"   call Decho("exe ".(g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s")
   exe (g:netrw_alto? "bel " : "abo ").g:netrw_winsize."wincmd s"
   let s:didsplit= 1
   keepj call s:RestoreWinVars()
   keepj call netrw#LocalBrowseCheck(s:NetrwBrowseChgDir(1,s:NetrwGetWord()))
   unlet s:didsplit

  elseif a:mode == 4
   " local and t
   let cursorword  = s:NetrwGetWord()
   let netrw_curdir= s:NetrwTreeDir()
"   call Decho("tabnew")
   tabnew
   let b:netrw_curdir= netrw_curdir
   let s:didsplit= 1
   keepj call s:RestoreWinVars()
   keepj call netrw#LocalBrowseCheck(s:NetrwBrowseChgDir(1,cursorword))
   unlet s:didsplit

  elseif a:mode == 5
   " local and v
"   call Decho("exe ".(g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v")
   exe (g:netrw_altv? "rightb " : "lefta ").g:netrw_winsize."wincmd v"
   let s:didsplit= 1
   keepj call s:RestoreWinVars()
   keepj call netrw#LocalBrowseCheck(s:NetrwBrowseChgDir(1,s:NetrwGetWord()))
   unlet s:didsplit

  else
   keepj call netrw#ErrorMsg(s:ERROR,"(NetrwSplit) unsupported mode=".a:mode,45)
  endif

"  call Dret("s:NetrwSplit")
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
   if exists("w:netrw_explore_line") |unlet w:netrw_explore_line |endif
   return ""
  else
   return "Match ".w:netrw_explore_mtchcnt." of ".w:netrw_explore_listlen
  endif
endfun

" ---------------------------------------------------------------------
" s:NetrwTreeDir: determine tree directory given current cursor position {{{2
" (full path directory with trailing slash returned)
fun! s:NetrwTreeDir()
"  call Dfunc("NetrwTreeDir() curline#".line(".")."<".getline('.')."> b:netrw_curdir<".b:netrw_curdir."> tab#".tabpagenr()." win#".winnr()." buf#".bufnr("%")."<".bufname("%").">")

  let treedir= b:netrw_curdir
"  call Decho("set initial treedir<".treedir.">")
  let s:treecurpos= netrw#NetrwSavePosn()

  if w:netrw_liststyle == s:TREELIST
"   call Decho("w:netrrw_liststyle is TREELIST:")
"   call Decho("line#".line(".")." getline(.)<".getline('.')."> treecurpos<".string(s:treecurpos).">")

   " extract tree directory if on a line specifying a subdirectory (ie. ends with "/")
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
    sil! keepj %d
"    call Dret("NetrwTreeDir <".treedir."> : (side effect) s:treecurpos<".string(s:treecurpos).">")
    return b:netrw_curdir
   endif

   " elide all non-depth information
   let depth = substitute(getline('.'),'^\(\%(| \)*\)[^|].\{-}$','\1','e')
"   call Decho("depth<".depth."> 1st subst (non-depth info removed)")

   " elide first depth
   let depth = substitute(depth,'^| ','','')
"   call Decho("depth<".depth."> 2nd subst (first depth removed)")

   " construct treedir by searching backwards at correct depth
"   call Decho("constructing treedir<".treedir."> depth<".depth.">")
   while depth != "" && search('^'.depth.'[^|].\{-}/$','bW')
    let dirname= substitute(getline('.'),'^\(| \)*','','e')
    let treedir= dirname.treedir
    let depth  = substitute(depth,'^| ','','')
"    call Decho("constructing treedir<".treedir.">: dirname<".dirname."> while depth<".depth.">")
   endwhile
   if w:netrw_treetop =~ '/$'
    let treedir= w:netrw_treetop.treedir
   else
    let treedir= w:netrw_treetop.'/'.treedir
   endif
"   call Decho("bufnr(.)=".bufnr("%")." line($)=".line("$")." line(.)=".line("."))
  endif
  let treedir= substitute(treedir,'//$','/','')

"  call Dret("NetrwTreeDir <".treedir."> : (side effect) s:treecurpos<".string(s:treecurpos).">")
  return treedir
endfun

" ---------------------------------------------------------------------
" s:NetrwTreeDisplay: recursive tree display {{{2
fun! s:NetrwTreeDisplay(dir,depth)
"  call Dfunc("NetrwTreeDisplay(dir<".a:dir."> depth<".a:depth.">)")

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
    keepj call s:NetrwTreeDisplay(direntry,depth)
   elseif entry =~ '/$' && has_key(w:netrw_treedict,direntry.'/')
"    call Decho("<".direntry."/> is a key in treedict - display subtree for it")
    keepj call s:NetrwTreeDisplay(direntry.'/',depth)
   else
"    call Decho("<".entry."> is not a key in treedict (no subtree)")
    sil! keepj call setline(line("$")+1,depth.entry)
   endif
  endfor
"  call Dret("NetrwTreeDisplay")
endfun

" ---------------------------------------------------------------------
" s:NetrwTreeListing: displays tree listing from treetop on down, using NetrwTreeDisplay() {{{2
fun! s:NetrwTreeListing(dirname)
  if w:netrw_liststyle == s:TREELIST
"   call Dfunc("NetrwTreeListing() bufname<".expand("%").">")
"   call Decho("curdir<".a:dirname.">")
"   call Decho("win#".winnr().": w:netrw_treetop ".(exists("w:netrw_treetop")? "exists" : "doesn't exit")." w:netrw_treedict ".(exists("w:netrw_treedict")? "exists" : "doesn't exit"))

   " update the treetop
"   call Decho("update the treetop")
   if !exists("w:netrw_treetop")
    let w:netrw_treetop= a:dirname
"    call Decho("w:netrw_treetop<".w:netrw_treetop."> (reusing)")
   elseif (w:netrw_treetop =~ ('^'.a:dirname) && s:Strlen(a:dirname) < s:Strlen(w:netrw_treetop)) || a:dirname !~ ('^'.w:netrw_treetop)
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
   exe "sil! keepj ".w:netrw_bannercnt.',$g@^\.\.\=/$@d'
   let w:netrw_treedict[a:dirname]= getline(w:netrw_bannercnt,line("$"))
"   call Decho("w:treedict[".a:dirname."]= ".string(w:netrw_treedict[a:dirname]))
   exe "sil! keepj ".w:netrw_bannercnt.",$d"

   " if past banner, record word
   if exists("w:netrw_bannercnt") && line(".") > w:netrw_bannercnt
    let fname= expand("<cword>")
   else
    let fname= ""
   endif
"   call Decho("fname<".fname.">")

   " display from treetop on down
   keepj call s:NetrwTreeDisplay(w:netrw_treetop,"")

"   call Dret("NetrwTreeListing : bufname<".expand("%").">")
  endif
endfun

" ---------------------------------------------------------------------
" s:NetrwWideListing: {{{2
fun! s:NetrwWideListing()

  if w:netrw_liststyle == s:WIDELIST
"   call Dfunc("NetrwWideListing() w:netrw_liststyle=".w:netrw_liststyle.' fo='.&fo.' l:fo='.&l:fo)
   " look for longest filename (cpf=characters per filename)
   " cpf: characters per filename
   " fpl: filenames per line
   " fpc: filenames per column
   setlocal ma noro
"   call Decho("setlocal ma noro")
   let b:netrw_cpf= 0
   if line("$") >= w:netrw_bannercnt
    exe 'sil keepj '.w:netrw_bannercnt.',$g/^./if virtcol("$") > b:netrw_cpf|let b:netrw_cpf= virtcol("$")|endif'
    keepj call histdel("/",-1)
   else
"    call Dret("NetrwWideListing")
    return
   endif
   let b:netrw_cpf= b:netrw_cpf + 2
"   call Decho("b:netrw_cpf=max_filename_length+2=".b:netrw_cpf)

   " determine qty files per line (fpl)
   let w:netrw_fpl= winwidth(0)/b:netrw_cpf
   if w:netrw_fpl <= 0
    let w:netrw_fpl= 1
   endif
"   call Decho("fpl= [winwidth=".winwidth(0)."]/[b:netrw_cpf=".b:netrw_cpf.']='.w:netrw_fpl)

   " make wide display
   exe 'sil keepj '.w:netrw_bannercnt.',$s/^.*$/\=escape(printf("%-'.b:netrw_cpf.'s",submatch(0)),"\\")/'
   keepj call histdel("/",-1)
   let fpc         = (line("$") - w:netrw_bannercnt + w:netrw_fpl)/w:netrw_fpl
   let newcolstart = w:netrw_bannercnt + fpc
   let newcolend   = newcolstart + fpc - 1
"   call Decho("bannercnt=".w:netrw_bannercnt." fpl=".w:netrw_fpl." fpc=".fpc." newcol[".newcolstart.",".newcolend."]")
   sil! let keepregstar = @*
   while line("$") >= newcolstart
    if newcolend > line("$") | let newcolend= line("$") | endif
    let newcolqty= newcolend - newcolstart
    exe newcolstart
    if newcolqty == 0
     exe "sil! keepj norm! 0\<c-v>$hx".w:netrw_bannercnt."G$p"
    else
     exe "sil! keepj norm! 0\<c-v>".newcolqty.'j$hx'.w:netrw_bannercnt.'G$p'
    endif
    exe "sil! keepj ".newcolstart.','.newcolend.'d'
    exe 'sil! keepj '.w:netrw_bannercnt
   endwhile
   sil! let @*= keepregstar
   exe "sil! keepj ".w:netrw_bannercnt.',$s/\s\+$//e'
   keepj call histdel("/",-1)
   setlocal noma nomod ro
"   call Dret("NetrwWideListing")
  endif

endfun

" ---------------------------------------------------------------------
" s:PerformListing: {{{2
fun! s:PerformListing(islocal)
"  call Dfunc("s:PerformListing(islocal=".a:islocal.") bufnr(%)=".bufnr("%")."<".bufname("%").">")

  keepj call s:NetrwSafeOptions()
  setlocal noro ma
"  call Decho("setlocal noro ma")

"  if exists("g:netrw_silent") && g:netrw_silent == 0 && &ch >= 1	" Decho
"   call Decho("(netrw) Processing your browsing request...")
"  endif								" Decho

"  call Decho('w:netrw_liststyle='.(exists("w:netrw_liststyle")? w:netrw_liststyle : 'n/a'))
  if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST && exists("w:netrw_treedict")
   " force a refresh for tree listings
"   call Decho("force refresh for treelisting: clear buffer<".expand("%")."> with :%d")
   sil! keepj %d
  endif

  " save current directory on directory history list
  keepj call s:NetrwBookHistHandler(3,b:netrw_curdir)

  " Set up the banner {{{3
  if g:netrw_banner
"   call Decho("set up banner")
   keepj call setline(1,'" ============================================================================')
   keepj call setline(2,'" Netrw Directory Listing                                        (netrw '.g:loaded_netrw.')')
   keepj call setline(3,'"   '.b:netrw_curdir)
   let w:netrw_bannercnt= 3
   keepj exe "sil! keepj ".w:netrw_bannercnt
  else
   keepj 1
   let w:netrw_bannercnt= 1
  endif

  let sortby= g:netrw_sort_by
  if g:netrw_sort_direction =~ "^r"
   let sortby= sortby." reversed"
  endif

  " Sorted by... {{{3
  if g:netrw_banner
"   call Decho("handle specified sorting: g:netrw_sort_by<".g:netrw_sort_by.">")
   if g:netrw_sort_by =~ "^n"
"   call Decho("directories will be sorted by name")
    " sorted by name
    keepj put ='\"   Sorted by      '.sortby
    keepj put ='\"   Sort sequence: '.g:netrw_sort_sequence
    let w:netrw_bannercnt= w:netrw_bannercnt + 2
   else
"   call Decho("directories will be sorted by size or time")
    " sorted by size or date
    keepj put ='\"   Sorted by '.sortby
    let w:netrw_bannercnt= w:netrw_bannercnt + 1
   endif
   exe "sil! keepj ".w:netrw_bannercnt
  endif

  " show copy/move target, if any
  if g:netrw_banner
   if exists("s:netrwmftgt") && exists("s:netrwmftgt_islocal")
"    call Decho("show copy/move target<".s:netrwmftgt.">")
    keepj put =''
    if s:netrwmftgt_islocal
     sil! keepj call setline(line("."),'"   Copy/Move Tgt: '.s:netrwmftgt.' (local)')
    else
     sil! keepj call setline(line("."),'"   Copy/Move Tgt: '.s:netrwmftgt.' (remote)')
    endif
    let w:netrw_bannercnt= w:netrw_bannercnt + 1
   else
"    call Decho("s:netrwmftgt does not exist, don't make Copy/Move Tgt")
   endif
   exe "sil! keepj ".w:netrw_bannercnt
  endif

  " Hiding...  -or-  Showing... {{{3
  if g:netrw_banner
"   call Decho("handle hiding/showing (g:netrw_hide=".g:netrw_list_hide." g:netrw_list_hide<".g:netrw_list_hide.">)")
   if g:netrw_list_hide != "" && g:netrw_hide
    if g:netrw_hide == 1
     keepj put ='\"   Hiding:        '.g:netrw_list_hide
    else
     keepj put ='\"   Showing:       '.g:netrw_list_hide
    endif
    let w:netrw_bannercnt= w:netrw_bannercnt + 1
   endif
   exe "keepjumps ".w:netrw_bannercnt
   keepj put ='\"   Quick Help: <F1>:help  -:go up dir  D:delete  R:rename  s:sort-by  x:exec'
   keepj put ='\" ============================================================================'
   let w:netrw_bannercnt= w:netrw_bannercnt + 2
  endif

  " bannercnt should index the line just after the banner
  if g:netrw_banner
   let w:netrw_bannercnt= w:netrw_bannercnt + 1
   exe "sil! keepj ".w:netrw_bannercnt
"   call Decho("bannercnt=".w:netrw_bannercnt." (should index line just after banner) line($)=".line("$"))
  endif

  " set up syntax highlighting {{{3
"  call Decho("set up syntax highlighting")
  if has("syntax")
   if !exists("g:syntax_on") || !g:syntax_on
"    call Decho("but g:syntax_on".(exists("g:syntax_on")? "=".g:syntax_on : "<doesn't exist>"))
    setlocal ft=
   elseif &ft != "netrw"
    setlocal ft=netrw
   endif
  endif

  " get list of files
"  call Decho("Get list of files - islocal=".a:islocal)
  if a:islocal
   keepj call s:LocalListing()
  else " remote
   keepj call s:NetrwRemoteListing()
  endif
"  call Decho("g:netrw_banner=".g:netrw_banner." w:netrw_bannercnt=".w:netrw_bannercnt." (banner complete)")

  " manipulate the directory listing (hide, sort) {{{3
  if !g:netrw_banner || line("$") >= w:netrw_bannercnt
"   call Decho("manipulate directory listing (hide)")
"   call Decho("g:netrw_hide=".g:netrw_hide." g:netrw_list_hide<".g:netrw_list_hide.">")
   if g:netrw_hide && g:netrw_list_hide != ""
    keepj call s:NetrwListHide()
   endif
   if !g:netrw_banner || line("$") >= w:netrw_bannercnt
"    call Decho("manipulate directory listing (sort) : g:netrw_sort_by<".g:netrw_sort_by.">")

    if g:netrw_sort_by =~ "^n"
     " sort by name
     keepj call s:NetrwSetSort()

     if !g:netrw_banner || w:netrw_bannercnt < line("$")
"      call Decho("g:netrw_sort_direction=".g:netrw_sort_direction." (bannercnt=".w:netrw_bannercnt.")")
      if g:netrw_sort_direction =~ 'n'
       " normal direction sorting
       exe 'sil keepj '.w:netrw_bannercnt.',$sort'.' '.g:netrw_sort_options
      else
       " reverse direction sorting
       exe 'sil keepj '.w:netrw_bannercnt.',$sort!'.' '.g:netrw_sort_options
      endif
     endif
     " remove priority pattern prefix
"     call Decho("remove priority pattern prefix")
     exe 'sil! keepj '.w:netrw_bannercnt.',$s/^\d\{3}'.g:netrw_sepchr.'//e'
     keepj call histdel("/",-1)

    elseif a:islocal
     if !g:netrw_banner || w:netrw_bannercnt < line("$")
"      call Decho("g:netrw_sort_direction=".g:netrw_sort_direction)
      if g:netrw_sort_direction =~ 'n'
"       call Decho('exe sil keepjumps '.w:netrw_bannercnt.',$sort')
       exe 'sil! keepj '.w:netrw_bannercnt.',$sort'.' '.g:netrw_sort_options
      else
"       call Decho('exe sil keepjumps '.w:netrw_bannercnt.',$sort!')
       exe 'sil! keepj '.w:netrw_bannercnt.',$sort!'.' '.g:netrw_sort_options
      endif
     exe 'sil! keepj '.w:netrw_bannercnt.',$s/^\d\{-}\///e'
     keepj call histdel("/",-1)
     endif
    endif

   elseif g:netrw_sort_direction =~ 'r'
"    call Decho('reverse the sorted listing')
    if !g:netrw_banner || w:netrw_bannercnt < line('$')
     exe 'sil! keepj '.w:netrw_bannercnt.',$g/^/m '.w:netrw_bannercnt
     call histdel("/",-1)
    endif
   endif
  endif

  " convert to wide/tree listing {{{3
"  call Decho("modify display if wide/tree listing style")
  keepj call s:NetrwWideListing()
  keepj call s:NetrwTreeListing(b:netrw_curdir)

  if exists("w:netrw_bannercnt") && (line("$") > w:netrw_bannercnt || !g:netrw_banner)
   " place cursor on the top-left corner of the file listing
"   call Decho("place cursor on top-left corner of file listing")
   exe 'sil! keepj '.w:netrw_bannercnt
   sil! keepj norm! 0
  endif

  " record previous current directory
  let w:netrw_prvdir= b:netrw_curdir
"  call Decho("record netrw_prvdir<".w:netrw_prvdir.">")

  " save certain window-oriented variables into buffer-oriented variables {{{3
  keepj call s:SetBufWinVars()
  keepj call s:NetrwOptionRestore("w:")

  " set display to netrw display settings
"  call Decho("set display to netrw display settings (noma nomod etc)")
  exe "setl ".g:netrw_bufsettings
  if exists("s:treecurpos")

   keepj call netrw#NetrwRestorePosn(s:treecurpos)
   unlet s:treecurpos
  endif

"  call Dret("s:PerformListing : curpos<".string(getpos(".")).">")
endfun

" ---------------------------------------------------------------------
" s:SetupNetrwStatusLine: {{{2
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
  redraw

"  call Dret("SetupNetrwStatusLine : stl=".&stl)
endfun

" ---------------------------------------------------------------------
"  Remote Directory Browsing Support:    {{{1
" ===========================================

" ---------------------------------------------------------------------
" s:NetrwRemoteListing: {{{2
fun! s:NetrwRemoteListing()
"  call Dfunc("s:NetrwRemoteListing() b:netrw_curdir<".b:netrw_curdir.">)")

  call s:RemotePathAnalysis(b:netrw_curdir)

  " sanity check:
  if exists("b:netrw_method") && b:netrw_method =~ '[235]'
"   call Decho("b:netrw_method=".b:netrw_method)
   if !executable("ftp")
    if !exists("g:netrw_quiet")
     call netrw#ErrorMsg(s:ERROR,"this system doesn't support remote directory listing via ftp",18)
    endif
    call s:NetrwOptionRestore("w:")
"    call Dret("s:NetrwRemoteListing")
    return
   endif

  elseif !exists("g:netrw_list_cmd") || g:netrw_list_cmd == ''
   if !exists("g:netrw_quiet")
    if g:netrw_list_cmd == ""
     keepj call netrw#ErrorMsg(s:ERROR,g:netrw_ssh_cmd." is not executable on your system",47)
    else
     keepj call netrw#ErrorMsg(s:ERROR,"this system doesn't support remote directory listing via ".g:netrw_list_cmd,19)
    endif
   endif

   keepj call s:NetrwOptionRestore("w:")
"   call Dret("s:NetrwRemoteListing")
   return
  endif  " (remote handling sanity check)

  if exists("b:netrw_method")
"   call Decho("setting w:netrw_method<".b:netrw_method.">")
   let w:netrw_method= b:netrw_method
  endif

  if s:method == "ftp"
   " use ftp to get remote file listing {{{3
"   call Decho("use ftp to get remote file listing")
   let s:method  = "ftp"
   let listcmd = g:netrw_ftp_list_cmd
   if g:netrw_sort_by =~ '^t'
    let listcmd= g:netrw_ftp_timelist_cmd
   elseif g:netrw_sort_by =~ '^s'
    let listcmd= g:netrw_ftp_sizelist_cmd
   endif
"   call Decho("listcmd<".listcmd."> (using g:netrw_ftp_list_cmd)")
   call s:NetrwRemoteFtpCmd(s:path,listcmd)
"   exe "sil! keepalt keepj ".w:netrw_bannercnt.',$g/^./call Decho("raw listing: ".getline("."))'

   if w:netrw_liststyle == s:THINLIST || w:netrw_liststyle == s:WIDELIST || w:netrw_liststyle == s:TREELIST
    " shorten the listing
"    call Decho("generate short listing")
    exe "sil! keepalt keepj ".w:netrw_bannercnt

    " cleanup
    if g:netrw_ftp_browse_reject != ""
     exe "sil! keepalt keepj g/".g:netrw_ftp_browse_reject."/keepj d"
     keepj call histdel("/",-1)
    endif
    sil! keepj %s/\r$//e
    keepj call histdel("/",-1)

    " if there's no ../ listed, then put ./ and ../ in
    let line1= line(".")
    exe "sil! keepj ".w:netrw_bannercnt
    let line2= search('^\.\.\/\%(\s\|$\)','cnW')
    if line2 == 0
"     call Decho("netrw is putting ./ and ../ into listing")
     sil! keepj put='../'
     sil! keepj put='./'
    endif
    exe "sil! keepj ".line1
    sil! keepj norm! 0

"    call Decho("line1=".line1." line2=".line2." line(.)=".line("."))
    if search('^\d\{2}-\d\{2}-\d\{2}\s','n') " M$ ftp site cleanup
"     call Decho("M$ ftp cleanup")
     exe 'sil! keepj '.w:netrw_bannercnt.',$s/^\d\{2}-\d\{2}-\d\{2}\s\+\d\+:\d\+[AaPp][Mm]\s\+\%(<DIR>\|\d\+\)\s\+//'
     keepj call histdel("/",-1)
    else " normal ftp cleanup
"     call Decho("normal ftp cleanup")
     exe 'sil! keepj '.w:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2/e'
     exe "sil! keepj ".w:netrw_bannercnt.',$g/ -> /s# -> .*/$#/#e'
     exe "sil! keepj ".w:netrw_bannercnt.',$g/ -> /s# -> .*$#/#e'
     keepj call histdel("/",-1)
     keepj call histdel("/",-1)
     keepj call histdel("/",-1)
    endif
   endif

  else
   " use ssh to get remote file listing {{{3
"   call Decho("use ssh to get remote file listing: s:path<".s:path.">")
   let listcmd= s:MakeSshCmd(g:netrw_list_cmd)
"   call Decho("listcmd<".listcmd."> (using g:netrw_list_cmd)")
   if g:netrw_scp_cmd =~ '^pscp'
"    call Decho("1: exe sil r! ".shellescape(listcmd.s:path, 1))
    exe "sil! keepj r! ".listcmd.shellescape(s:path, 1)
    " remove rubbish and adjust listing format of 'pscp' to 'ssh ls -FLa' like
    sil! keepj g/^Listing directory/keepj d
    sil! keepj g/^d[-rwx][-rwx][-rwx]/keepj s+$+/+e
    sil! keepj g/^l[-rwx][-rwx][-rwx]/keepj s+$+@+e
    keepj call histdel("/",-1)
    keepj call histdel("/",-1)
    keepj call histdel("/",-1)
    if g:netrw_liststyle != s:LONGLIST
     sil! keepj g/^[dlsp-][-rwx][-rwx][-rwx]/keepj s/^.*\s\(\S\+\)$/\1/e
     keepj call histdel("/",-1)
    endif
   else
    if s:path == ""
"     call Decho("2: exe sil r! ".listcmd)
     exe "sil! keepalt r! ".listcmd
    else
"     call Decho("3: exe sil r! ".listcmd.' '.shellescape(s:path,1))
     exe "sil! keepalt r! ".listcmd.' '.shellescape(s:path,1)
"     call Decho("listcmd<".listcmd."> path<".s:path.">")
    endif
   endif

   " cleanup
   if g:netrw_ftp_browse_reject != ""
"    call Decho("(cleanup) exe sil! g/".g:netrw_ssh_browse_reject."/keepjumps d")
    exe "sil! g/".g:netrw_ssh_browse_reject."/keepj d"
    keepj call histdel("/",-1)
   endif
  endif

  if w:netrw_liststyle == s:LONGLIST
   " do a long listing; these substitutions need to be done prior to sorting {{{3
"   call Decho("fix long listing:")

   if s:method == "ftp"
    " cleanup
    exe "sil! keepj ".w:netrw_bannercnt
    while getline('.') =~ g:netrw_ftp_browse_reject
     sil! keepj d
    endwhile
    " if there's no ../ listed, then put ./ and ../ in
    let line1= line(".")
    sil! keepj 1
    sil! keepj call search('^\.\.\/\%(\s\|$\)','W')
    let line2= line(".")
    if line2 == 0
     exe 'sil! keepj '.w:netrw_bannercnt."put='./'"
     if b:netrw_curdir != '/'
      exe 'sil! keepj '.w:netrw_bannercnt."put='../'"
     endif
    endif
    exe "sil! keepj ".line1
    sil! keepj norm! 0
   endif

   if search('^\d\{2}-\d\{2}-\d\{2}\s','n') " M$ ftp site cleanup
"    call Decho("M$ ftp site listing cleanup")
    exe 'sil! keepj '.w:netrw_bannercnt.',$s/^\(\d\{2}-\d\{2}-\d\{2}\s\+\d\+:\d\+[AaPp][Mm]\s\+\%(<DIR>\|\d\+\)\s\+\)\(\w.*\)$/\2\t\1/'
   elseif exists("w:netrw_bannercnt") && w:netrw_bannercnt <= line("$")
"    call Decho("normal ftp site listing cleanup: bannercnt=".w:netrw_bannercnt." line($)=".line("$"))
    exe 'sil keepj '.w:netrw_bannercnt.',$s/ -> .*$//e'
    exe 'sil keepj '.w:netrw_bannercnt.',$s/^\(\%(\S\+\s\+\)\{7}\S\+\)\s\+\(\S.*\)$/\2\t\1/e'
    exe 'sil keepj '.w:netrw_bannercnt
    keepj call histdel("/",-1)
    keepj call histdel("/",-1)
    keepj call histdel("/",-1)
   endif
  endif

"  if exists("w:netrw_bannercnt") && w:netrw_bannercnt <= line("$") " Decho
"   exe "keepj ".w:netrw_bannercnt.',$g/^./call Decho("listing: ".getline("."))'
"  endif " Decho
"  call Dret("s:NetrwRemoteListing")
endfun

" ---------------------------------------------------------------------
" s:NetrwRemoteRm: remove/delete a remote file or directory {{{2
fun! s:NetrwRemoteRm(usrhost,path) range
"  call Dfunc("s:NetrwRemoteRm(usrhost<".a:usrhost."> path<".a:path.">) virtcol=".virtcol("."))
"  call Decho("firstline=".a:firstline." lastline=".a:lastline)
  let svpos= netrw#NetrwSavePosn()

  let all= 0
  if exists("s:netrwmarkfilelist_{bufnr('%')}")
   " remove all marked files
"   call Decho("remove all marked files with bufnr#".bufnr("%"))
   for fname in s:netrwmarkfilelist_{bufnr("%")}
    let ok= s:NetrwRemoteRmFile(a:path,fname,all)
    if ok =~ 'q\%[uit]'
     break
    elseif ok =~ 'a\%[ll]'
     let all= 1
    endif
   endfor
   call s:NetrwUnmarkList(bufnr("%"),b:netrw_curdir)

  else
   " remove files specified by range
"   call Decho("remove files specified by range")

   " preparation for removing multiple files/directories
   let ctr= a:firstline

   " remove multiple files and directories
   while ctr <= a:lastline
    exe ctr
    let ok= s:NetrwRemoteRmFile(a:path,s:NetrwGetWord(),all)
    if ok =~ 'q\%[uit]'
     break
    elseif ok =~ 'a\%[ll]'
     let all= 1
    endif
    let ctr= ctr + 1
   endwhile
  endif

  " refresh the (remote) directory listing
"  call Decho("refresh remote directory listing")
  keepj call s:NetrwRefresh(0,s:NetrwBrowseChgDir(0,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)

"  call Dret("s:NetrwRemoteRm")
endfun

" ---------------------------------------------------------------------
" s:NetrwRemoteRmFile: {{{2
fun! s:NetrwRemoteRmFile(path,rmfile,all)
"  call Dfunc("s:NetrwRemoteRmFile(path<".a:path."> rmfile<".a:rmfile.">) all=".a:all)

  let all= a:all
  let ok = ""

  if a:rmfile !~ '^"' && (a:rmfile =~ '@$' || a:rmfile !~ '[\/]$')
   " attempt to remove file
"    call Decho("attempt to remove file (all=".all.")")
   if !all
    echohl Statement
"    call Decho("case all=0:")
    call inputsave()
    let ok= input("Confirm deletion of file<".a:rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
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
"    call Decho("case all=".all." or ok<".ok.">".(exists("w:netrw_method")? ': netrw_method='.w:netrw_method : ""))
    if exists("w:netrw_method") && (w:netrw_method == 2 || w:netrw_method == 3)
"     call Decho("case ftp:")
     let path= a:path
     if path =~ '^\a\+://'
      let path= substitute(path,'^\a\+://[^/]\+/','','')
     endif
     sil! keepj .,$d
     call s:NetrwRemoteFtpCmd(path,"delete ".'"'.a:rmfile.'"')
    else
"     call Decho("case ssh: g:netrw_rm_cmd<".g:netrw_rm_cmd.">")
     let netrw_rm_cmd= s:MakeSshCmd(g:netrw_rm_cmd)
"     call Decho("netrw_rm_cmd<".netrw_rm_cmd.">")
     if !exists("b:netrw_curdir")
      keepj call netrw#ErrorMsg(s:ERROR,"for some reason b:netrw_curdir doesn't exist!",53)
      let ok="q"
     else
      let remotedir= substitute(b:netrw_curdir,'^.*//[^/]\+/\(.*\)$','\1','')
"      call Decho("netrw_rm_cmd<".netrw_rm_cmd.">")
"      call Decho("remotedir<".remotedir.">")
"      call Decho("rmfile<".a:rmfile.">")
      if remotedir != ""
       let netrw_rm_cmd= netrw_rm_cmd." ".shellescape(fnameescape(remotedir.a:rmfile))
      else
       let netrw_rm_cmd= netrw_rm_cmd." ".shellescape(fnameescape(a:rmfile))
      endif
"      call Decho("call system(".netrw_rm_cmd.")")
      let ret= system(netrw_rm_cmd)
      if ret != 0
       keepj call netrw#ErrorMsg(s:WARNING,"cmd<".netrw_rm_cmd."> failed",60)
      endif
"      call Decho("returned=".ret." errcode=".v:shell_error)
     endif
    endif
   elseif ok =~ 'q\%[uit]'
"    call Decho("ok==".ok)
    break
   endif

  else
   " attempt to remove directory
"    call Decho("attempt to remove directory")
   if !all
    call inputsave()
    let ok= input("Confirm deletion of directory<".a:rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
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
     keepj call s:NetrwRemoteFtpCmd(a:path,"rmdir ".a:rmfile)
    else
     let rmfile          = substitute(a:path.a:rmfile,'/$','','')
     let netrw_rmdir_cmd = s:MakeSshCmd(netrw#WinPath(g:netrw_rmdir_cmd)).' '.shellescape(netrw#WinPath(rmfile))
"      call Decho("attempt to remove dir: system(".netrw_rmdir_cmd.")")
     let ret= system(netrw_rmdir_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)

     if v:shell_error != 0
"      call Decho("v:shell_error not 0")
      let netrw_rmf_cmd= s:MakeSshCmd(netrw#WinPath(g:netrw_rmf_cmd)).' '.shellescape(netrw#WinPath(substitute(rmfile,'[\/]$','','e')))
"      call Decho("2nd attempt to remove dir: system(".netrw_rmf_cmd.")")
      let ret= system(netrw_rmf_cmd)
"      call Decho("returned=".ret." errcode=".v:shell_error)

      if v:shell_error != 0 && !exists("g:netrw_quiet")
      	keepj call netrw#ErrorMsg(s:ERROR,"unable to remove directory<".rmfile."> -- is it empty?",22)
      endif
     endif
    endif

   elseif ok =~ 'q\%[uit]'
    break
   endif
  endif

"  call Dret("s:NetrwRemoteRmFile ".ok)
  return ok
endfun

" ---------------------------------------------------------------------
" s:NetrwRemoteFtpCmd: unfortunately, not all ftp servers honor options for ls {{{2
"  This function assumes that a long listing will be received.  Size, time,
"  and reverse sorts will be requested of the server but not otherwise
"  enforced here.
fun! s:NetrwRemoteFtpCmd(path,listcmd)
"  call Dfunc("NetrwRemoteFtpCmd(path<".a:path."> listcmd<".a:listcmd.">) netrw_method=".w:netrw_method)
"  call Decho("line($)=".line("$")." bannercnt=".w:netrw_bannercnt)

  " because WinXX ftp uses unix style input
  let ffkeep= &ff
  setlocal ma ff=unix noro
"  call Decho("setlocal ma ff=unix noro")

  " clear off any older non-banner lines
  " note that w:netrw_bannercnt indexes the line after the banner
"  call Decho('exe sil! keepjumps '.w:netrw_bannercnt.",$d  (clear off old non-banner lines)")
  exe "sil! keepjumps ".w:netrw_bannercnt.",$d"

  ".........................................
  if w:netrw_method == 2 || w:netrw_method == 5
   " ftp + <.netrc>:  Method #2
   if a:path != ""
    keepj put ='cd \"'.a:path.'\"'
   endif
   if exists("g:netrw_ftpextracmd")
    keepj put =g:netrw_ftpextracmd
"    call Decho("filter input: ".getline('.'))
   endif
   keepj call setline(line("$")+1,a:listcmd)
"   exe "keepjumps ".w:netrw_bannercnt.',$g/^./call Decho("ftp#".line(".").": ".getline("."))'
   if exists("g:netrw_port") && g:netrw_port != ""
"    call Decho("exe ".s:netrw_silentxfer.w:netrw_bannercnt.",$!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1))
    exe s:netrw_silentxfer." keepjumps ".w:netrw_bannercnt.",$!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)." ".shellescape(g:netrw_port,1)
   else
"    call Decho("exe ".s:netrw_silentxfer.w:netrw_bannercnt.",$!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1))
    exe s:netrw_silentxfer." keepjumps ".w:netrw_bannercnt.",$!".s:netrw_ftp_cmd." -i ".shellescape(g:netrw_machine,1)
   endif

   ".........................................
  elseif w:netrw_method == 3
   " ftp + machine,id,passwd,filename:  Method #3
    setlocal ff=unix
    if exists("g:netrw_port") && g:netrw_port != ""
     keepj put ='open '.g:netrw_machine.' '.g:netrw_port
    else
     keepj put ='open '.g:netrw_machine
    endif

    if exists("g:netrw_ftp") && g:netrw_ftp == 1
     keepj put =g:netrw_uid
     keepj put ='\"'.s:netrw_passwd.'\"'
    else
     keepj put ='user \"'.g:netrw_uid.'\" \"'.s:netrw_passwd.'\"'
    endif

   if a:path != ""
    keepj put ='cd \"'.a:path.'\"'
   endif
   if exists("g:netrw_ftpextracmd")
    keepj put =g:netrw_ftpextracmd
"    call Decho("filter input: ".getline('.'))
   endif
   keepj call setline(line("$")+1,a:listcmd)

    " perform ftp:
    " -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
    " -n  win32: quit being obnoxious about password
"    exe w:netrw_bannercnt.',$g/^./call Decho("ftp#".line(".").": ".getline("."))'
"    call Decho("exe ".s:netrw_silentxfer.w:netrw_bannercnt.",$!".s:netrw_ftp_cmd." -i -n")
    exe s:netrw_silentxfer.w:netrw_bannercnt.",$!".s:netrw_ftp_cmd." -i -n"

   ".........................................
  else
   keepj call netrw#ErrorMsg(s:WARNING,"unable to comply with your request<" . choice . ">",23)
  endif

  " cleanup for Windows
  if has("win32") || has("win95") || has("win64") || has("win16")
   sil! keepj %s/\r$//e
   keepj call histdel("/",-1)
  endif
  if a:listcmd == "dir"
   " infer directory/link based on the file permission string
   sil! keepj g/d\%([-r][-w][-x]\)\{3}/keepj s@$@/@
   sil! keepj g/l\%([-r][-w][-x]\)\{3}/keepj s/$/@/
   keepj call histdel("/",-1)
   keepj call histdel("/",-1)
   if w:netrw_liststyle == s:THINLIST || w:netrw_liststyle == s:WIDELIST || w:netrw_liststyle == s:TREELIST
    exe "sil! keepj ".w:netrw_bannercnt.',$s/^\%(\S\+\s\+\)\{8}//e'
    keepj call histdel("/",-1)
   endif
  endif

  " ftp's listing doesn't seem to include ./ or ../
  if !search('^\.\/$\|\s\.\/$','wn')
   exe 'keepj '.w:netrw_bannercnt
   keepj put ='./'
  endif
  if !search('^\.\.\/$\|\s\.\.\/$','wn')
   exe 'keepj '.w:netrw_bannercnt
   keepj put ='../'
  endif

  " restore settings
  let &ff= ffkeep
"  call Dret("NetrwRemoteFtpCmd")
endfun

" ---------------------------------------------------------------------
" s:NetrwRemoteRename: rename a remote file or directory {{{2
fun! s:NetrwRemoteRename(usrhost,path) range
"  call Dfunc("NetrwRemoteRename(usrhost<".a:usrhost."> path<".a:path.">)")

  " preparation for removing multiple files/directories
  let svpos      = netrw#NetrwSavePosn()
  let ctr        = a:firstline
  let rename_cmd = s:MakeSshCmd(g:netrw_rename_cmd)

  " rename files given by the markfilelist
  if exists("s:netrwmarkfilelist_{bufnr('%')}")
   for oldname in s:netrwmarkfilelist_{bufnr("%")}
"    call Decho("oldname<".oldname.">")
    if exists("subfrom")
     let newname= substitute(oldname,subfrom,subto,'')
"     call Decho("subfrom<".subfrom."> subto<".subto."> newname<".newname.">")
    else
     call inputsave()
     let newname= input("Moving ".oldname." to : ",oldname)
     call inputrestore()
     if newname =~ '^s/'
      let subfrom = substitute(newname,'^s/\([^/]*\)/.*/$','\1','')
      let subto   = substitute(newname,'^s/[^/]*/\(.*\)/$','\1','')
      let newname = substitute(oldname,subfrom,subto,'')
"      call Decho("subfrom<".subfrom."> subto<".subto."> newname<".newname.">")
     endif
    endif
   
    if exists("w:netrw_method") && (w:netrw_method == 2 || w:netrw_method == 3)
     keepj call s:NetrwRemoteFtpCmd(a:path,"rename ".oldname." ".newname)
    else
     let oldname= shellescape(a:path.oldname)
     let newname= shellescape(a:path.newname)
"     call Decho("system(netrw#WinPath(".rename_cmd.") ".oldname.' '.newname.")")
     let ret    = system(netrw#WinPath(rename_cmd).' '.oldname.' '.newname)
    endif

   endfor
   call s:NetrwUnMarkFile(1)

  else

  " attempt to rename files/directories
   while ctr <= a:lastline
    exe "keepj ".ctr

    let oldname= s:NetrwGetWord()
"   call Decho("oldname<".oldname.">")

    call inputsave()
    let newname= input("Moving ".oldname." to : ",oldname)
    call inputrestore()

    if exists("w:netrw_method") && (w:netrw_method == 2 || w:netrw_method == 3)
     call s:NetrwRemoteFtpCmd(a:path,"rename ".oldname." ".newname)
    else
     let oldname= shellescape(a:path.oldname)
     let newname= shellescape(a:path.newname)
"     call Decho("system(netrw#WinPath(".rename_cmd.") ".oldname.' '.newname.")")
     let ret    = system(netrw#WinPath(rename_cmd).' '.oldname.' '.newname)
    endif

    let ctr= ctr + 1
   endwhile
  endif

  " refresh the directory
  keepj call s:NetrwRefresh(0,s:NetrwBrowseChgDir(0,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)

"  call Dret("NetrwRemoteRename")
endfun

" ---------------------------------------------------------------------
"  Local Directory Browsing Support:    {{{1
" ==========================================

" ---------------------------------------------------------------------
" netrw#LocalBrowseCheck: {{{2
fun! netrw#LocalBrowseCheck(dirname)
  " unfortunate interaction -- split window debugging can't be
  " used here, must use D-echoRemOn or D-echoTabOn -- the BufEnter
  " event triggers another call to LocalBrowseCheck() when attempts
  " to write to the DBG buffer are made.
  " The &ft == "netrw" test was installed because the BufEnter event
  " would hit when re-entering netrw windows, creating unexpected
  " refreshes (and would do so in the middle of NetrwSaveOptions(), too)
"  call Decho("netrw#LocalBrowseCheck: isdir<".a:dirname.">=".isdirectory(a:dirname).((exists("s:treeforceredraw")? " treeforceredraw" : "")))
"  call Dredir("LocalBrowseCheck","ls!")|redraw!|sleep 3
  if isdirectory(a:dirname)
"   call Decho(" ft<".&ft."> b:netrw_curdir<".(exists("b:netrw_curdir")? b:netrw_curdir : " doesn't exist")."> dirname<".a:dirname.">"." line($)=".line("$"))
   if &ft != "netrw" || (exists("b:netrw_curdir") && b:netrw_curdir != a:dirname)
    sil! keepj call s:NetrwBrowse(1,a:dirname)
   elseif &ft == "netrw" && line("$") == 1
    sil! keepj call s:NetrwBrowse(1,a:dirname)
   elseif exists("s:treeforceredraw")
    unlet s:treeforceredraw
    sil! keepj call s:NetrwBrowse(1,a:dirname)
   endif
  endif
  " not a directory, ignore it
endfun

" ---------------------------------------------------------------------
"  s:LocalListing: does the job of "ls" for local directories {{{2
fun! s:LocalListing()
"  call Dfunc("s:LocalListing()")
"  call Decho("&ma=".&ma)
"  call Decho("&mod=".&mod)
"  call Decho("&ro=".&ro)
"  call Decho("bufname(%)<".bufname("%").">")

"  if exists("b:netrw_curdir") |call Decho('b:netrw_curdir<'.b:netrw_curdir.">")  |else|call Decho("b:netrw_curdir doesn't exist") |endif
"  if exists("g:netrw_sort_by")|call Decho('g:netrw_sort_by<'.g:netrw_sort_by.">")|else|call Decho("g:netrw_sort_by doesn't exist")|endif

  " get the list of files contained in the current directory
  let dirname    = b:netrw_curdir
  let dirnamelen = s:Strlen(b:netrw_curdir)
  let filelist   = glob(s:ComposePath(dirname,"*"))
"  call Decho("glob(dirname<".dirname."/*>)=".filelist)
  if filelist != ""
   let filelist= filelist."\n"
  endif
  let filelist= filelist.glob(s:ComposePath(dirname,".*"))
"  call Decho("glob(dirname<".dirname."/.*>)=".filelist)

  " Coding choice: either   elide   ./ if present
  "                or       include ./ if not present
  if filelist =~ '[\\/]\.[\\/]\=\(\n\|$\)'
   " elide /path/. from glob() entries if present
"   call Decho("elide /path/. from glob entries if present")
   let filelist = substitute(filelist,'\n','\t','g')
   let filelist = substitute(filelist,'^[^\t]\+[/\\]\.\t','','')
   let filelist = substitute(filelist,'[^\t]\+[/\\]\.$','','')
   let filelist = substitute(filelist,'\t\zs[^\t]\+[/\\]\.\t','','')
   let filelist = substitute(filelist,'\t','\n','g')
  endif
"  call Decho("filelist<".filelist.">")
  if filelist !~ '[\\/]\.\.[\\/]\=\(\n\|$\)'
    " include ../ in the glob() entry if its missing
"   call Decho("forcibly tacking on ..")
   let filelist= filelist."\n".s:ComposePath(b:netrw_curdir,"../")
"   call Decho("filelist<".filelist.">")
  endif
  if b:netrw_curdir == '/'
   " remove .. from filelist when current directory is root directory
"   call Decho("remove .. from filelist")
   let filelist= substitute(filelist,'/\.\.\n','','')
  endif
  " remove multiple contiguous newlines
  let filelist= substitute(filelist,'\n\{2,}','\n','ge')
  if !g:netrw_cygwin && (has("win32") || has("win95") || has("win64") || has("win16"))
   " change all \s to /s
"   call Decho('change all \s to /s')
   let filelist= substitute(filelist,'\','/','g')
  else
   " escape all \s to \\
"   call Decho('escape all \s to \\')
   let filelist= substitute(filelist,'\','\\','g')
  endif

"  call Decho("(before while) dirname<".dirname.">")
"  call Decho("(before while) dirnamelen<".dirnamelen.">")
"  call Decho("(before while) filelist<".filelist.">")

  while filelist != ""
   if filelist =~ '\n'
    let filename = substitute(filelist,'\n.*$','','e')
    let filelist = substitute(filelist,'^.\{-}\n\(.*\)$','\1','e')
   else
    let filename = filelist
    let filelist = ""
   endif
"   call Decho(" ")
"   call Decho("(while) filelist<".filelist.">")
"   call Decho("(while) filename<".filename.">")

   if getftype(filename) == "link"
    " indicate a symbolic link
"    call Decho("indicate <".filename."> is a symbolic link with trailing @")
    let pfile= filename."@"

   elseif getftype(filename) == "socket"
    " indicate a socket
"    call Decho("indicate <".filename."> is a socket with trailing =")
    let pfile= filename."="

   elseif getftype(filename) == "fifo"
    " indicate a fifo
"    call Decho("indicate <".filename."> is a fifo with trailing |")
    let pfile= filename."|"

   elseif isdirectory(filename)
    " indicate a directory
"    call Decho("indicate <".filename."> is a directory with trailing /")
    let pfile= filename."/"

   elseif exists("b:netrw_curdir") && b:netrw_curdir !~ '^.*://' && !isdirectory(filename)
    if (has("win32") || has("win95") || has("win64") || has("win16"))
     if filename =~ '\.[eE][xX][eE]$' || filename =~ '\.[cC][oO][mM]$' || filename =~ '\.[bB][aA][tT]$'
      " indicate an executable
"      call Decho("indicate <".filename."> is executable with trailing *")
      let pfile= filename."*"
     else
      " normal file
      let pfile= filename
     endif
    elseif executable(filename)
     " indicate an executable
"     call Decho("indicate <".filename."> is executable with trailing *")
     let pfile= filename."*"
    else
     " normal file
     let pfile= filename
    endif

   else
    " normal file
    let pfile= filename
   endif
"   call Decho("pfile<".pfile."> (after *@/ appending)")

   if pfile =~ '//$'
    let pfile= substitute(pfile,'//$','/','e')
"    call Decho("change // to /: pfile<".pfile.">")
   endif
   let pfile= strpart(pfile,dirnamelen)
   let pfile= substitute(pfile,'^[/\\]','','e')
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
    sil! keepj put=ftpfile

   elseif g:netrw_sort_by =~ "^s"
    " sort by size (handles file sizes up to 1 quintillion bytes, US)
"    call Decho("getfsize(".filename.")=".getfsize(filename))
    let sz   = getfsize(filename)
    let fsz  = strpart("000000000000000000",1,18-strlen(sz)).sz
"    call Decho("exe keepjumps put ='".fsz.'/'.filename."'")
    let fszpfile= fsz.'/'.pfile
    sil! keepj put =fszpfile

   else
    " sort by name
"    call Decho("exe keepjumps put ='".pfile."'")
    sil! keepj put=pfile
   endif
  endwhile

  " cleanup any windows mess at end-of-line
  sil! keepj g/^$/d
  sil! keepj %s/\r$//e
  call histdel("/",-1)
  exe "setlocal ts=".g:netrw_maxfilenamelen
"  call Decho("setlocal ts=".g:netrw_maxfilenamelen)

"  call Dret("s:LocalListing")
endfun

" ---------------------------------------------------------------------
" s:LocalBrowseShellCmdRefresh: this function is called after a user has {{{2
" performed any shell command.  The idea is to cause all local-browsing
" buffers to be refreshed after a user has executed some shell command,
" on the chance that s/he removed/created a file/directory with it.
fun! s:LocalBrowseShellCmdRefresh()
"  call Dfunc("LocalBrowseShellCmdRefresh() browselist=".(exists("s:netrw_browselist")? string(s:netrw_browselist) : "empty")." ".tabpagenr("$")." tabs")
  " determine which buffers currently reside in a tab
  if !exists("s:netrw_browselist")
"   call Dret("LocalBrowseShellCmdRefresh : browselist is empty")
   return
  endif
  if !exists("w:netrw_bannercnt")
"   call Dret("LocalBrowseShellCmdRefresh : don't refresh when focus not on netrw window")
   return
  endif
  if exists("s:locbrowseshellcmd")
   if s:locbrowseshellcmd
    let s:locbrowseshellcmd= 0
"    call Dret("LocalBrowseShellCmdRefresh : NetrwBrowse itself caused the refresh")
    return
   endif
   let s:locbrowseshellcmd= 0
  endif
  let itab       = 1
  let buftablist = []
  while itab <= tabpagenr("$")
   let buftablist = buftablist + tabpagebuflist()
   let itab       = itab + 1
   tabn
  endwhile
"  call Decho("buftablist".string(buftablist))
"  call Decho("s:netrw_browselist<".(exists("s:netrw_browselist")? string(s:netrw_browselist) : "").">")
  "  GO through all buffers on netrw_browselist (ie. just local-netrw buffers):
  "   | refresh any netrw window
  "   | wipe out any non-displaying netrw buffer
  let curwin = winnr()
  let ibl    = 0
  for ibuf in s:netrw_browselist
"   call Decho("bufwinnr(".ibuf.") index(buftablist,".ibuf.")=".index(buftablist,ibuf))
   if bufwinnr(ibuf) == -1 && index(buftablist,ibuf) == -1
    " wipe out any non-displaying netrw buffer
"    call Decho("wiping  buf#".ibuf,"<".bufname(ibuf).">")
    exe "sil! bd ".fnameescape(ibuf)
    call remove(s:netrw_browselist,ibl)
"    call Decho("browselist=".string(s:netrw_browselist))
    continue
   elseif index(tabpagebuflist(),ibuf) != -1
    " refresh any netrw buffer
"    call Decho("refresh buf#".ibuf.'-> win#'.bufwinnr(ibuf))
    exe bufwinnr(ibuf)."wincmd w"
    keepj call s:NetrwRefresh(1,s:NetrwBrowseChgDir(1,'./'))
   endif
   let ibl= ibl + 1
  endfor
  exe curwin."wincmd w"

"  call Dret("LocalBrowseShellCmdRefresh")
endfun

" ---------------------------------------------------------------------
" s:NetrwLocalRm: {{{2
fun! s:NetrwLocalRm(path) range
"  call Dfunc("s:NetrwLocalRm(path<".a:path.">)")
"  call Decho("firstline=".a:firstline." lastline=".a:lastline)

  " preparation for removing multiple files/directories
  let ret   = 0
  let all   = 0
  let svpos = netrw#NetrwSavePosn()

  if exists("s:netrwmarkfilelist_{bufnr('%')}")
   " remove all marked files
"   call Decho("remove all marked files")
   for fname in s:netrwmarkfilelist_{bufnr("%")}
    let ok= s:NetrwLocalRmFile(a:path,fname,all)
    if ok =~ 'q\%[uit]' || ok == "no"
     break
    elseif ok =~ 'a\%[ll]'
     let all= 1
    endif
   endfor
   call s:NetrwUnMarkFile(1)

  else
  " remove (multiple) files and directories
"   call Decho("remove files in range [".a:firstline.",".a:lastline."]")

   let ctr = a:firstline
   while ctr <= a:lastline
    exe "keepj ".ctr

    " sanity checks
    if line(".") < w:netrw_bannercnt
     let ctr= ctr + 1
     continue
    endif
    let curword= s:NetrwGetWord()
    if curword == "./" || curword == "../"
     let ctr= ctr + 1
     continue
    endif
    let ok= s:NetrwLocalRmFile(a:path,curword,all)
    if ok =~ 'q\%[uit]' || ok == "no"
     break
    elseif ok =~ 'a\%[ll]'
     let all= 1
    endif
    let ctr= ctr + 1
   endwhile
  endif

  " refresh the directory
"  call Decho("bufname<".bufname("%").">")
  if bufname("%") != "NetrwMessage"
   keepj call s:NetrwRefresh(1,s:NetrwBrowseChgDir(1,'./'))
   keepj call netrw#NetrwRestorePosn(svpos)
  endif

"  call Dret("s:NetrwLocalRm")
endfun

" ---------------------------------------------------------------------
" s:NetrwLocalRmFile: remove file fname given the path {{{2
"                     Give confirmation prompt unless all==1
fun! s:NetrwLocalRmFile(path,fname,all)
"  call Dfunc("s:NetrwLocalRmFile(path<".a:path."> fname<".a:fname."> all=".a:all)
  
  let all= a:all
  let ok = ""
  keepj norm! 0
  let rmfile= s:ComposePath(a:path,a:fname)
"  call Decho("rmfile<".rmfile.">")

  if rmfile !~ '^"' && (rmfile =~ '@$' || rmfile !~ '[\/]$')
   " attempt to remove file
"   call Decho("attempt to remove file<".rmfile.">")
   if !all
    echohl Statement
    call inputsave()
    let ok= input("Confirm deletion of file<".rmfile."> ","[{y(es)},n(o),a(ll),q(uit)] ")
    call inputrestore()
    echohl NONE
    if ok == ""
     let ok="no"
    endif
"    call Decho("response: ok<".ok.">")
    let ok= substitute(ok,'\[{y(es)},n(o),a(ll),q(uit)]\s*','','e')
"    call Decho("response: ok<".ok."> (after sub)")
    if ok =~ 'a\%[ll]'
     let all= 1
    endif
   endif

   if all || ok =~ 'y\%[es]' || ok == ""
    let ret= s:NetrwDelete(rmfile)
"    call Decho("errcode=".v:shell_error." ret=".ret)
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
"    call Decho("1st attempt: system(netrw#WinPath(".g:netrw_local_rmdir.') '.shellescape(rmfile).')')
    call system(netrw#WinPath(g:netrw_local_rmdir).' '.shellescape(rmfile))
"    call Decho("v:shell_error=".v:shell_error)

    if v:shell_error != 0
"     call Decho("2nd attempt to remove directory<".rmfile.">")
     let errcode= s:NetrwDelete(rmfile)
"     call Decho("errcode=".errcode)

     if errcode != 0
      if has("unix")
"       call Decho("3rd attempt to remove directory<".rmfile.">")
       call system("rm ".shellescape(rmfile))
       if v:shell_error != 0 && !exists("g:netrw_quiet")
        call netrw#ErrorMsg(s:ERROR,"unable to remove directory<".rmfile."> -- is it empty?",34)
	let ok="no"
       endif
      elseif !exists("g:netrw_quiet")
       call netrw#ErrorMsg(s:ERROR,"unable to remove directory<".rmfile."> -- is it empty?",35)
       let ok="no"
      endif
     endif
    endif
   endif
  endif

"  call Dret("s:NetrwLocalRmFile ".ok)
  return ok
endfun

" ---------------------------------------------------------------------
" s:NetrwLocalRename: rename a remote file or directory {{{2
fun! s:NetrwLocalRename(path) range
"  call Dfunc("NetrwLocalRename(path<".a:path.">)")

  " preparation for removing multiple files/directories
  let ctr  = a:firstline
  let svpos= netrw#NetrwSavePosn()

  " rename files given by the markfilelist
  if exists("s:netrwmarkfilelist_{bufnr('%')}")
   for oldname in s:netrwmarkfilelist_{bufnr("%")}
"    call Decho("oldname<".oldname.">")
    if exists("subfrom")
     let newname= substitute(oldname,subfrom,subto,'')
"     call Decho("subfrom<".subfrom."> subto<".subto."> newname<".newname.">")
    else
     call inputsave()
     let newname= input("Moving ".oldname." to : ",oldname)
     call inputrestore()
     if newname =~ '^s/'
      let subfrom = substitute(newname,'^s/\([^/]*\)/.*/$','\1','')
      let subto   = substitute(newname,'^s/[^/]*/\(.*\)/$','\1','')
"      call Decho("subfrom<".subfrom."> subto<".subto."> newname<".newname.">")
      let newname = substitute(oldname,subfrom,subto,'')
     endif
    endif
    call rename(oldname,newname)
   endfor
   call s:NetrwUnmarkList(bufnr("%"),b:netrw_curdir)
  
  else

   " attempt to rename files/directories
   while ctr <= a:lastline
    exe "keepj ".ctr

    " sanity checks
    if line(".") < w:netrw_bannercnt
     let ctr= ctr + 1
     continue
    endif
    let curword= s:NetrwGetWord()
    if curword == "./" || curword == "../"
     let ctr= ctr + 1
     continue
    endif

    keepj norm! 0
    let oldname= s:ComposePath(a:path,curword)
"   call Decho("oldname<".oldname.">")

    call inputsave()
    let newname= input("Moving ".oldname." to : ",substitute(oldname,'/*$','','e'))
    call inputrestore()

    call rename(oldname,newname)
"   call Decho("renaming <".oldname."> to <".newname.">")

    let ctr= ctr + 1
   endwhile
  endif

  " refresh the directory
"  call Decho("refresh the directory listing")
  keepj call s:NetrwRefresh(1,s:NetrwBrowseChgDir(1,'./'))
  keepj call netrw#NetrwRestorePosn(svpos)

"  call Dret("NetrwLocalRename")
endfun

" ---------------------------------------------------------------------
" s:LocalFastBrowser: handles setting up/taking down fast browsing for the local browser {{{2
"
"     g:netrw_    Directory Is
"     fastbrowse  Local  Remote   
"  slow   0         D      D      D=Deleting a buffer implies it will not be re-used (slow)
"  med    1         D      H      H=Hiding a buffer implies it may be re-used        (fast)
"  fast   2         H      H      
"
"  Deleting a buffer means that it will be re-loaded when examined, hence "slow".
"  Hiding   a buffer means that it will be re-used   when examined, hence "fast".
"           (re-using a buffer may not be as accurate)
fun! s:LocalFastBrowser()
"  call Dfunc("LocalFastBrowser() g:netrw_fastbrowse=".g:netrw_fastbrowse)

  " initialize browselist, a list of buffer numbers that the local browser has used
  if !exists("s:netrw_browselist")
"   call Decho("initialize s:netrw_browselist")
   let s:netrw_browselist= []
  endif

  " append current buffer to fastbrowse list
  if empty(s:netrw_browselist) || bufnr("%") > s:netrw_browselist[-1]
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
"     call Decho("autocmd: ShellCmdPost * call s:LocalBrowseShellCmdRefresh()")
     au ShellCmdPost			*	call s:LocalBrowseShellCmdRefresh()
    else
     au ShellCmdPost,FocusGained	*	call s:LocalBrowseShellCmdRefresh()
"     call Decho("autocmd: ShellCmdPost,FocusGained * call s:LocalBrowseShellCmdRefresh()")
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

"  call Dret("LocalFastBrowser : browselist<".string(s:netrw_browselist).">")
endfun

" ---------------------------------------------------------------------
" netrw#FileUrlRead: handles reading file:///* files {{{2
fun! netrw#FileUrlRead(fname)
"  call Dfunc("netrw#FileUrlRead()")
  let fname2396 = netrw#RFC2396(a:fname)
  let fname2396e= fnameescape(fname2396)
  let plainfname= substitute(fname2396,'file://\(.*\)','\1',"")
"  call Decho("fname2396<".fname2396.">")
"  call Decho("plainfname<".plainfname.">")
  exe "sil doau BufReadPre ".fname2396e
  exe 'r '.plainfname
  exe 'file! '.plainfname
  1d
  setlocal nomod
"  call Dret("netrw#FileUrlRead")
  exe "sil doau BufReadPost ".fname2396e
endfun

" ---------------------------------------------------------------------
" Support Functions: {{{1

" ---------------------------------------------------------------------
" netrw#ErrorMsg: {{{2
"   0=note     = s:NOTE
"   1=warning  = s:WARNING
"   2=error    = s:ERROR
"  Dec 03, 2009 : max errnum currently is 77
fun! netrw#ErrorMsg(level,msg,errnum)
"  call Dfunc("netrw#ErrorMsg(level=".a:level." msg<".a:msg."> errnum=".a:errnum.") g:netrw_use_errorwindow=".g:netrw_use_errorwindow)

  if a:level == 1
   let level= "**warning** (netrw) "
  elseif a:level == 2
   let level= "**error** (netrw) "
  else
   let level= "**note** (netrw) "
  endif
"  call Decho("level=".level)

  if g:netrw_use_errorwindow
   " (default) netrw creates a one-line window to show error/warning
   " messages (reliably displayed)

   " record current window number for NetrwRestorePosn()'s benefit
   let s:winBeforeErr= winnr()
"   call Decho("s:winBeforeErr=".s:winBeforeErr)

   " getting messages out reliably is just plain difficult!
   " This attempt splits the current window, creating a one line window.
   if bufexists("NetrwMessage") && bufwinnr("NetrwMessage") > 0
"    call Decho("write to NetrwMessage buffer")
    exe bufwinnr("NetrwMessage")."wincmd w"
"    call Decho("setlocal ma noro")
    setlocal ma noro
    keepj call setline(line("$")+1,level.a:msg)
    keepj $
   else
"    call Decho("create a NetrwMessage buffer window")
    bo 1split
    call s:NetrwEnew()
    keepj call s:NetrwSafeOptions()
    setlocal bt=nofile
    keepj file NetrwMessage
"    call Decho("setlocal ma noro")
    setlocal ma noro
    call setline(line("$"),level.a:msg)
   endif
"   call Decho("wrote msg<".level.a:msg."> to NetrwMessage win#".winnr())
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
"   redraw!
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
" netrw#NetrwRestorePosn: restores the cursor and file position as saved by NetrwSavePosn() {{{2
fun! netrw#NetrwRestorePosn(...)
"  call Dfunc("netrw#NetrwRestorePosn() a:0=".a:0." winnr=".(exists("w:netrw_winnr")? w:netrw_winnr : -1)." line=".(exists("w:netrw_line")? w:netrw_line : -1)." col=".(exists("w:netrw_col")? w:netrw_col : -1)." hline=".(exists("w:netrw_hline")? w:netrw_hline : -1))
  let eikeep= &ei
  set ei=all
  if expand("%") == "NetrwMessage"
   exe s:winBeforeErr."wincmd w"
  endif

  if a:0 > 0
   exe a:1
  endif

  " restore window
  if exists("w:netrw_winnr")
"   call Decho("restore window: exe sil! ".w:netrw_winnr."wincmd w")
   exe "sil! ".w:netrw_winnr."wincmd w"
  endif
  if v:shell_error == 0
   " as suggested by Bram M: redraw on no error
   " allows protocol error messages to remain visible
"   redraw!
  endif

  " restore top-of-screen line
  if exists("w:netrw_hline")
"   call Decho("restore topofscreen: exe norm! ".w:netrw_hline."G0z")
   exe "keepj norm! ".w:netrw_hline."G0z\<CR>"
  endif

  " restore position
  if exists("w:netrw_line") && exists("w:netrw_col")
"   call Decho("restore posn: exe norm! ".w:netrw_line."G0".w:netrw_col."|")
   exe "keepj norm! ".w:netrw_line."G0".w:netrw_col."\<bar>"
  endif

  let &ei= eikeep
"  call Dret("netrw#NetrwRestorePosn")
endfun

" ---------------------------------------------------------------------
" netrw#NetrwSavePosn: saves position of cursor on screen {{{2
fun! netrw#NetrwSavePosn()
"  call Dfunc("netrw#NetrwSavePosn()")
  " Save current line and column
  let w:netrw_winnr= winnr()
  let w:netrw_line = line(".")
  let w:netrw_col  = virtcol(".")
"  call Decho("currently, win#".w:netrw_winnr." line#".w:netrw_line." col#".w:netrw_col)

  " Save top-of-screen line
  keepj norm! H0
  let w:netrw_hline= line(".")

  " set up string holding position parameters
  let ret          = "let w:netrw_winnr=".w:netrw_winnr."|let w:netrw_line=".w:netrw_line."|let w:netrw_col=".w:netrw_col."|let w:netrw_hline=".w:netrw_hline

  keepj call netrw#NetrwRestorePosn()
"  call Dret("netrw#NetrwSavePosn : winnr=".w:netrw_winnr." line=".w:netrw_line." col=".w:netrw_col." hline=".w:netrw_hline)
  return ret
endfun

" ------------------------------------------------------------------------
"  netrw#RFC2396: converts %xx into characters {{{2
fun! netrw#RFC2396(fname)
"  call Dfunc("netrw#RFC2396(fname<".a:fname.">)")
  let fname = escape(substitute(a:fname,'%\(\x\x\)','\=nr2char("0x".submatch(1))','ge')," \t")
"  call Dret("netrw#RFC2396 ".fname)
  return fname
endfun

" ---------------------------------------------------------------------
"  s:ComposePath: Appends a new part to a path taking different systems into consideration {{{2
fun! s:ComposePath(base,subdir)
"  call Dfunc("s:ComposePath(base<".a:base."> subdir<".a:subdir.">)")

  if(has("amiga"))
"   call Decho("amiga")
   let ec = a:base[s:Strlen(a:base)-1]
   if ec != '/' && ec != ':'
    let ret = a:base . "/" . a:subdir
   else
    let ret = a:base . a:subdir
   endif

  elseif a:subdir =~ '^\a:[/\\][^/\\]' && (has("win32") || has("win95") || has("win64") || has("win16"))
"   call Decho("windows")
   let ret= a:subdir

  elseif a:base =~ '^\a:[/\\][^/\\]' && (has("win32") || has("win95") || has("win64") || has("win16"))
"   call Decho("windows")
   if a:base =~ '[/\\]$'
    let ret= a:base.a:subdir
   else
    let ret= a:base."/".a:subdir
   endif

  elseif a:base =~ '^\a\+://'
"   call Decho("remote linux/macos")
   let urlbase = substitute(a:base,'^\(\a\+://.\{-}/\)\(.*\)$','\1','')
   let curpath = substitute(a:base,'^\(\a\+://.\{-}/\)\(.*\)$','\2','')
   if a:subdir == '../'
    if curpath =~ '[^/]/[^/]\+/$'
     let curpath= substitute(curpath,'[^/]\+/$','','')
    else
     let curpath=""
    endif
    let ret= urlbase.curpath
   else
    let ret= urlbase.curpath.a:subdir
   endif
"   call Decho("urlbase<".urlbase.">")
"   call Decho("curpath<".curpath.">")
"   call Decho("ret<".ret.">")

  else
"   call Decho("local linux/macos")
   let ret = substitute(a:base."/".a:subdir,"//","/","g")
   if a:base =~ '^//'
    " keeping initial '//' for the benefit of network share listing support
    let ret= '/'.ret
   endif
   let ret= simplify(ret)
  endif

"  call Dret("s:ComposePath ".ret)
  return ret
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

   let tmpfile= substitute(tmpfile,'\','/','ge')
"   call Decho("tmpfile<".tmpfile."> : chgd any \\ -> /")

   " sanity check -- does the temporary file's directory exist?
   if !isdirectory(substitute(tmpfile,'[^/]\+$','','e'))
"    call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
    keepj call netrw#ErrorMsg(s:ERROR,"your <".substitute(tmpfile,'[^/]\+$','','e')."> directory is missing!",2)
"    call Dret("s:GetTempfile getcwd<".getcwd().">")
    return ""
   endif

   " let netrw#NetSource() know about the tmpfile
   let s:netrw_tmpfile= tmpfile " used by netrw#NetSource() and netrw#NetrwBrowseX()
"   call Decho("tmpfile<".tmpfile."> s:netrw_tmpfile<".s:netrw_tmpfile.">")

   " o/s dependencies
   if g:netrw_cygwin != 0
    let tmpfile = substitute(tmpfile,'^\(\a\):','/cygdrive/\1','e')
   elseif has("win32") || has("win95") || has("win64") || has("win16")
    if !exists("+shellslash") || !&ssl
     let tmpfile = substitute(tmpfile,'/','\','g')
    endif
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
    if a:fname =~ '\.tar\.gz$' || a:fname =~ '\.tar\.bz2$' || a:fname =~ '\.tar\.xz$'
     let suffix = ".tar".substitute(a:fname,'^.*\(\.[^./]\+\)$','\1','e')
    elseif a:fname =~ '.txz$'
     let suffix = ".txz".substitute(a:fname,'^.*\(\.[^./]\+\)$','\1','e')
    else
     let suffix = substitute(a:fname,'^.*\(\.[^./]\+\)$','\1','e')
    endif
"    call Decho("suffix<".suffix.">")
    let tmpfile= substitute(tmpfile,'\.tmp$','','e')
"    call Decho("chgd tmpfile<".tmpfile."> (removed any .tmp suffix)")
    let tmpfile .= suffix
"    call Decho("chgd tmpfile<".tmpfile."> (added ".suffix." suffix) netrw_fname<".b:netrw_fname.">")
    let s:netrw_tmpfile= tmpfile " supports netrw#NetSource()
   endif
  endif

"  call Decho("ro=".&l:ro." ma=".&l:ma." mod=".&l:mod." wrap=".&l:wrap)
"  call Dret("s:GetTempfile <".tmpfile.">")
  return tmpfile
endfun

" ---------------------------------------------------------------------
" s:MakeSshCmd: transforms input command using USEPORT HOSTNAME into {{{2
"               a correct command for use with a system() call
fun! s:MakeSshCmd(sshcmd)
"  call Dfunc("s:MakeSshCmd(sshcmd<".a:sshcmd.">) user<".s:user."> machine<".s:machine.">")
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
" s:NetrwBMShow: {{{2
fun! s:NetrwBMShow()
"  call Dfunc("s:NetrwBMShow()")
  redir => bmshowraw
   menu
  redir END
  let bmshowlist = split(bmshowraw,'\n')
  if bmshowlist != []
   let bmshowfuncs= filter(bmshowlist,'v:val =~ "<SNR>\\d\\+_BMShow()"')
   if bmshowfuncs != []
    let bmshowfunc = substitute(bmshowfuncs[0],'^.*:\(call.*BMShow()\).*$','\1','')
    if bmshowfunc =~ '^call.*BMShow()'
     exe "sil! keepj ".bmshowfunc
    endif
   endif
  endif
"  call Dret("s:NetrwBMShow : bmshowfunc<".(exists("bmshowfunc")? bmshowfunc : 'n/a').">")
endfun

" ---------------------------------------------------------------------
" s:NetrwCursor: responsible for setting cursorline/cursorcolumn based upon g:netrw_cursor {{{2
fun! s:NetrwCursor()
  if !exists("w:netrw_liststyle")
   let w:netrw_liststyle= g:netrw_liststyle
  endif
"  call Dfunc("s:NetrwCursor() liststyle=".w:netrw_liststyle." g:netrw_cursor=".g:netrw_cursor." s:netrw_usercuc=".s:netrw_usercuc." s:netrw_usercul=".s:netrw_usercul)

  if &ft != "netrw"
   " if the current window isn't a netrw directory listing window, then use user cursorline/column
   " settings.  Affects when netrw is used to read/write a file using scp/ftp/etc.
   let &l:cursorline   = s:netrw_usercul
   let &l:cursorcolumn = s:netrw_usercuc

  elseif g:netrw_cursor == 4
   " all styles: cursorline, cursorcolumn
   setlocal cursorline
   setlocal cursorcolumn

  elseif g:netrw_cursor == 3
   " thin-long-tree: cursorline, user's cursorcolumn
   " wide          : cursorline, cursorcolumn
   if w:netrw_liststyle == s:WIDELIST
    setlocal cursorline
    setlocal cursorcolumn
   else
    setlocal cursorline
    let &l:cursorcolumn   = s:netrw_usercuc
   endif

  elseif g:netrw_cursor == 2
   " thin-long-tree: cursorline, user's cursorcolumn
   " wide          : cursorline, user's cursorcolumn
   let &l:cursorcolumn = s:netrw_usercuc
   setlocal cursorline

  elseif g:netrw_cursor == 1
   " thin-long-tree: user's cursorline, user's cursorcolumn
   " wide          : cursorline,        user's cursorcolumn
   let &l:cursorcolumn = s:netrw_usercuc
   if w:netrw_liststyle == s:WIDELIST
    setlocal cursorline
   else
    let &l:cursorline   = s:netrw_usercul
   endif

  else
   " all styles: user's cursorline, user's cursorcolumn
   let &l:cursorline   = s:netrw_usercul
   let &l:cursorcolumn = s:netrw_usercuc
  endif

"  call Dret("s:NetrwCursor : l:cursorline=".&l:cursorline." l:cursorcolumn=".&l:cursorcolumn)
endfun

" ---------------------------------------------------------------------
" s:RestoreCursorline: restores cursorline/cursorcolumn to original user settings {{{2
fun! s:RestoreCursorline()
"  call Dfunc("s:RestoreCursorline() currently, cul=".&l:cursorline." cuc=".&l:cursorcolumn." win#".winnr()." buf#".bufnr("%"))
  if exists("s:netrw_usercul")
   let &l:cursorline   = s:netrw_usercul
  endif
  if exists("s:netrw_usercuc")
   let &l:cursorcolumn = s:netrw_usercuc
  endif
"  call Dret("s:RestoreCursorline : restored cul=".&l:cursorline." cuc=".&l:cursorcolumn)
endfun

" ---------------------------------------------------------------------
" s:NetrwDelete: Deletes a file. {{{2
"           Uses Steve Hall's idea to insure that Windows paths stay
"           acceptable.  No effect on Unix paths.
"  Examples of use:  let result= s:NetrwDelete(path)
fun! s:NetrwDelete(path)
"  call Dfunc("s:NetrwDelete(path<".a:path.">)")

  let path = netrw#WinPath(a:path)
  if !g:netrw_cygwin && (has("win32") || has("win95") || has("win64") || has("win16"))
   if exists("+shellslash")
    let sskeep= &shellslash
    setlocal noshellslash
    let result      = delete(path)
    let &shellslash = sskeep
   else
"    call Decho("exe let result= ".a:cmd."('".path."')")
    let result= delete(path)
   endif
  else
"   call Decho("let result= delete(".path.")")
   let result= delete(path)
  endif
  if result < 0
   keepj call netrw#ErrorMsg(s:WARNING,"delete(".path.") failed!",71)
  endif

"  call Dret("s:NetrwDelete ".result)
  return result
endfun

" ---------------------------------------------------------------------
" s:NetrwEnew: opens a new buffer, passes netrw buffer variables through {{{2
fun! s:NetrwEnew(...)
"  call Dfunc("s:NetrwEnew() a:0=".a:0." bufnr($)=".bufnr("$"))
"  call Decho("curdir<".((a:0>0)? a:1 : "")."> buf#".bufnr("%")."<".bufname("%").">")

  " grab a function-local-variable copy of buffer variables
"  call Decho("make function-local copy of netrw variables")
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

  keepj call s:NetrwOptionRestore("w:")
"  call Decho("generate a buffer with keepjumps keepalt enew!")
  let netrw_keepdiff= &l:diff
  " COMBAK: Benzinger: using tree mode, vim -o Foo/ file shows Foo/ Foo/ instead.  Place return here, problem goes away (beeps result, but who knows)
"  call Dredir("Benzinger 1:","ls!")
  keepj keepalt enew!
"  call Dredir("Benzinger 2:","ls!")
  " COMBAK: Benzinger: using tree mode, vim -o Foo/ file shows Foo/ Foo/ instead.  Place return here, problem remains.
  let &l:diff= netrw_keepdiff
"  call Decho("bufnr($)=".bufnr("$"))
  keepj call s:NetrwOptionSave("w:")

  " copy function-local-variables to buffer variable equivalents
"  call Decho("copy function-local variables back to buffer netrw variables")
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

  if a:0 > 0
   let b:netrw_curdir= a:1
   if b:netrw_curdir =~ '/$'
    if exists("w:netrw_liststyle") && w:netrw_liststyle == s:TREELIST
     file NetrwTreeListing
     set bt=nowrite noswf
     nno <silent> <buffer> [	:sil call <SID>TreeListMove('[')<cr>
     nno <silent> <buffer> ]	:sil call <SID>TreeListMove(']')<cr>
    else
     exe "sil! keepalt file ".fnameescape(b:netrw_curdir)
    endif
   endif
  endif

"  call Dret("s:NetrwEnew : buf#".bufnr("%")."<".bufname("%")."> expand(%)<".expand("%")."> expand(#)<".expand("#").">")
endfun

" ------------------------------------------------------------------------
" s:NetrwSaveWordPosn: used to keep cursor on same word after refresh, {{{2
" changed sorting, etc.  Also see s:NetrwRestoreWordPosn().
fun! s:NetrwSaveWordPosn()
"  call Dfunc("NetrwSaveWordPosn()")
  let s:netrw_saveword= '^'.fnameescape(getline('.')).'$'
"  call Dret("NetrwSaveWordPosn : saveword<".s:netrw_saveword.">")
endfun

" ---------------------------------------------------------------------
" s:NetrwRestoreWordPosn: used to keep cursor on same word after refresh, {{{2
"  changed sorting, etc.  Also see s:NetrwSaveWordPosn().
fun! s:NetrwRestoreWordPosn()
"  call Dfunc("NetrwRestoreWordPosn()")
  sil! call search(s:netrw_saveword,'w')
"  call Dret("NetrwRestoreWordPosn")
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
" s:RemoteSystem: runs a command on a remote host using ssh {{{2
"                 Returns status
" Runs system() on
"    [cd REMOTEDIRPATH;] a:cmd
" Note that it doesn't do shellescape(a:cmd)!
fun! s:RemoteSystem(cmd)
"  call Dfunc("s:RemoteSystem(cmd<".a:cmd.">)")
  if !executable(g:netrw_ssh_cmd)
   keepj call netrw#ErrorMsg(s:ERROR,"g:netrw_ssh_cmd<".g:netrw_ssh_cmd."> is not executable!",52)
  elseif !exists("b:netrw_curdir")
   keepj call netrw#ErrorMsg(s:ERROR,"for some reason b:netrw_curdir doesn't exist!",53)
  else
   let cmd      = s:MakeSshCmd(g:netrw_ssh_cmd." USEPORT HOSTNAME")
   let remotedir= substitute(b:netrw_curdir,'^.*//[^/]\+/\(.*\)$','\1','')
   if remotedir != ""
    let cmd= cmd.' cd '.shellescape(remotedir).";"
   else
    let cmd= cmd.' '
   endif
   let cmd= cmd.a:cmd
"   call Decho("call system(".cmd.")")
   let ret= system(cmd)
  endif
"  call Dret("s:RemoteSystem ".ret)
  return ret
endfun

" ---------------------------------------------------------------------
" s:RestoreWinVars: (used by Explore() and NetrwSplit()) {{{2
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
" s:Rexplore: implements returning from a buffer to a netrw directory {{{2
"
"             s:SetRexDir() sets up <2-leftmouse> maps (if g:netrw_retmap
"             is true) and a command, :Rexplore, which call this function.
"
"             s:nbcd_curpos_{bufnr('%')} is set up by s:NetrwBrowseChgDir()
fun! s:NetrwRexplore(islocal,dirname)
"  call Dfunc("s:NetrwRexplore() w:netrw_rexlocal=".w:netrw_rexlocal." w:netrw_rexdir<".w:netrw_rexdir.">")
  if w:netrw_rexlocal
   keepj call netrw#LocalBrowseCheck(w:netrw_rexdir)
  else
   keepj call s:NetrwBrowse(0,w:netrw_rexdir)
  endif
  if exists("s:nbcd_curpos_{bufnr('%')}")
   keepj call netrw#NetrwRestorePosn(s:nbcd_curpos_{bufnr('%')})
   unlet s:nbcd_curpos_{bufnr('%')}
  endif
  if exists("s:explore_match")
   exe "2match netrwMarkFile /".s:explore_match."/"
  endif
"  call Dret("s:NetrwRexplore")
endfun

" ---------------------------------------------------------------------
" s:SaveBufVars: {{{2
fun! s:SaveBufVars()
"  call Dfunc("s:SaveBufVars() buf#".bufnr("%"))

  if exists("b:netrw_curdir")        |let s:netrw_curdir         = b:netrw_curdir        |endif
  if exists("b:netrw_lastfile")      |let s:netrw_lastfile       = b:netrw_lastfile      |endif
  if exists("b:netrw_method")        |let s:netrw_method         = b:netrw_method        |endif
  if exists("b:netrw_fname")         |let s:netrw_fname          = b:netrw_fname         |endif
  if exists("b:netrw_machine")       |let s:netrw_machine        = b:netrw_machine       |endif
  if exists("b:netrw_browser_active")|let s:netrw_browser_active = b:netrw_browser_active|endif

"  call Dret("s:SaveBufVars")
endfun

" ---------------------------------------------------------------------
" s:SaveWinVars: (used by Explore() and NetrwSplit()) {{{2
fun! s:SaveWinVars()
"  call Dfunc("s:SaveWinVars() win#".winnr())
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
" s:SetBufWinVars: (used by NetrwBrowse() and LocalBrowseCheck()) {{{2
"   To allow separate windows to have their own activities, such as
"   Explore **/pattern, several variables have been made window-oriented.
"   However, when the user splits a browser window (ex: ctrl-w s), these
"   variables are not inherited by the new window.  SetBufWinVars() and
"   UseBufWinVars() get around that.
fun! s:SetBufWinVars()
"  call Dfunc("s:SetBufWinVars() win#".winnr())
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
" s:SetRexDir: set directory for :Rexplore {{{2
fun! s:SetRexDir(islocal,dirname)
"  call Dfunc("s:SetRexDir(islocal=".a:islocal." dirname<".a:dirname.">)")
  let w:netrw_rexdir   = a:dirname
  let w:netrw_rexlocal = a:islocal
"  call Dret("s:SetRexDir")
endfun

" ---------------------------------------------------------------------
" s:Strlen: this function returns the length of a string, even if its {{{2
"           using two-byte etc characters.
"           Solution from Nicolai Weibull, vim docs (:help strlen()), Tony Mechelynck,
"           and a bit from me.
"           if g:netrw_xstrlen is zero (default), then the builtin strlen() function is used.
fun! s:Strlen(x)
"  call Dfunc("s:Strlen(x<".a:x.">")
  if g:netrw_xstrlen == 1
   " number of codepoints (Latin a + combining circumflex is two codepoints)
   " (comment from TM, solution from NW)
   let ret= strlen(substitute(a:x,'.','c','g'))

  elseif g:netrw_xstrlen == 2
   " number of spacing codepoints (Latin a + combining circumflex is one spacing 
   " codepoint; a hard tab is one; wide and narrow CJK are one each; etc.)
   " (comment from TM, solution from TM)
   let ret=strlen(substitute(a:x, '.\Z', 'x', 'g')) 

  elseif g:netrw_xstrlen == 3
   " virtual length (counting, for instance, tabs as anything between 1 and 
   " 'tabstop', wide CJK as 2 rather than 1, Arabic alif as zero when immediately 
   " preceded by lam, one otherwise, etc.)
   " (comment from TM, solution from me)
   let modkeep= &mod
   exe "keepj norm! o\<esc>"
   call setline(line("."),a:x)
   let ret= virtcol("$") - 1
   keepj d
   let &mod= modkeep

  else
   " at least give a decent default
   let ret= strlen(a:x)
  endif
"  call Dret("s:Strlen ".ret)
  return ret
endfun

" ---------------------------------------------------------------------
" s:TreeListMove: {{{2
fun! s:TreeListMove(dir)
"  call Dfunc("s:TreeListMove(dir<".a:dir.">)")
  let curline  = getline('.')
  let prvline  = (line(".") > 1)?         getline(line(".")-1) : ''
  let nxtline  = (line(".") < line("$"))? getline(line(".")+1) : ''
  let curindent= substitute(curline,'^\([| ]*\).\{-}$','\1','')
  let indentm1 = substitute(curindent,'^| ','','')
"  call Decho("prvline  <".prvline."> #".line(".")-1)
"  call Decho("curline  <".curline."> #".line("."))
"  call Decho("nxtline  <".nxtline."> #".line(".")+1)
"  call Decho("curindent<".curindent.">")
"  call Decho("indentm1 <".indentm1.">")

  if curline !~ '/$'
"   call Decho('regfile')
   if     a:dir == '[' && prvline != ''
    keepj norm! 0
    let nl = search('^'.indentm1.'[^|]','bWe')    " search backwards from regular file
"    call Decho("regfile srch back: ".nl)
   elseif a:dir == ']' && nxtline != ''
    keepj norm! $
    let nl = search('^'.indentm1.'[^|]','We')     " search forwards from regular file
"    call Decho("regfile srch fwd: ".nl)
   endif

  elseif a:dir == '[' && prvline != ''
   keepj norm! 0
   let curline= line(".")
   let nl     = search('^'.curindent.'[^|]','bWe') " search backwards From directory, same indentation
"   call Decho("dir srch back ind: ".nl)
   if nl != 0
    if line(".") == curline-1
     let nl= search('^'.indentm1.'[^|]','bWe')     " search backwards from directory, indentation - 1
"     call Decho("dir srch back ind-1: ".nl)
    endif
   endif

  elseif a:dir == ']' && nxtline != ''
   keepj norm! $
   let curline = line(".")
   let nl      = search('^'.curindent.'[^|]','We') " search forwards from directory, same indentation
"   call Decho("dir srch fwd ind: ".nl)
   if nl != 0
    if line(".") == curline+1
     let nl= search('^'.indentm1.'[^|]','We')         " search forwards from directory, indentation - 1
"     call Decho("dir srch fwd ind-1: ".nl)
    endif
   endif

  endif

"  call Dret("s:TreeListMove")
endfun

" ---------------------------------------------------------------------
" s:UpdateBuffersMenu: does emenu Buffers.Refresh (but due to locale, the menu item may not be called that) {{{2
"                      The Buffers.Refresh menu calls s:BMShow(); unfortunately, that means that that function
"                      can't be called except via emenu.  But due to locale, that menu line may not be called
"                      Buffers.Refresh; hence, s:NetrwBMShow() utilizes a "cheat" to call that function anyway.
fun! s:UpdateBuffersMenu()
"  call Dfunc("s:UpdateBuffersMenu()")
  if has("gui") && has("menu") && has("gui_running") && &go =~# 'm' && g:netrw_menu
   try
    sil emenu Buffers.Refresh\ menu
   catch /^Vim\%((\a\+)\)\=:E/
    let v:errmsg= ""
    sil keepj call s:NetrwBMShow()
   endtry
  endif
"  call Dret("s:UpdateBuffersMenu")
endfun

" ---------------------------------------------------------------------
" s:UseBufWinVars: (used by NetrwBrowse() and LocalBrowseCheck() {{{2
"              Matching function to s:SetBufWinVars()
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
" netrw#WinPath: tries to insure that the path is windows-acceptable, whether cygwin is used or not {{{2
fun! netrw#WinPath(path)
"  call Dfunc("netrw#WinPath(path<".a:path.">)")
  if (!g:netrw_cygwin || &shell !~ '\%(\<bash\>\|\<zsh\>\)\%(\.exe\)\=$') && (has("win32") || has("win95") || has("win64") || has("win16"))
   " remove cygdrive prefix, if present
   let path = substitute(a:path,'/cygdrive/\(.\)','\1:','')
   " remove trailing slash (Win95)
   let path = substitute(path, '\(\\\|/\)$', '', 'g')
   " remove escaped spaces
   let path = substitute(path, '\ ', ' ', 'g')
   " convert slashes to backslashes
   let path = substitute(path, '/', '\', 'g')
  else
   let path= a:path
  endif
"  call Dret("netrw#WinPath <".path.">")
  return path
endfun

" ---------------------------------------------------------------------
" Settings Restoration: {{{2
let &cpo= s:keepcpo
unlet s:keepcpo

" ------------------------------------------------------------------------
" Modelines: {{{1
" vim:ts=8 fdm=marker
