" Vim plugin for editing compressed files.
" Maintainer: Bram Moolenaar <Bram@vim.org>
" Last Change: 2005 May 18

" Exit quickly when:
" - this plugin was already loaded
" - when 'compatible' is set
" - some autocommands are already taking care of compressed files
if exists("loaded_gzip") || &cp || exists("#BufReadPre#*.gz")
  finish
endif
let loaded_gzip = 1

augroup gzip
  " Remove all gzip autocommands
  au!

  " Enable editing of gzipped files
  " set binary mode before reading the file
  " use "gzip -d", gunzip isn't always available
  autocmd BufReadPre,FileReadPre	*.gz,*.bz2,*.Z setlocal bin
  autocmd BufReadPost,FileReadPost	*.gz  call s:read("gzip -dn")
  autocmd BufReadPost,FileReadPost	*.bz2 call s:read("bzip2 -d")
  autocmd BufReadPost,FileReadPost	*.Z   call s:read("uncompress")
  autocmd BufWritePost,FileWritePost	*.gz  call s:write("gzip")
  autocmd BufWritePost,FileWritePost	*.bz2 call s:write("bzip2")
  autocmd BufWritePost,FileWritePost	*.Z   call s:write("compress -f")
  autocmd FileAppendPre			*.gz  call s:appre("gzip -dn")
  autocmd FileAppendPre			*.bz2 call s:appre("bzip2 -d")
  autocmd FileAppendPre			*.Z   call s:appre("uncompress")
  autocmd FileAppendPost		*.gz  call s:write("gzip")
  autocmd FileAppendPost		*.bz2 call s:write("bzip2")
  autocmd FileAppendPost		*.Z   call s:write("compress -f")
augroup END

" Function to check that executing "cmd [-f]" works.
" The result is cached in s:have_"cmd" for speed.
fun s:check(cmd)
  let name = substitute(a:cmd, '\(\S*\).*', '\1', '')
  if !exists("s:have_" . name)
    let e = executable(name)
    if e < 0
      let r = system(name . " --version")
      let e = (r !~ "not found" && r != "")
    endif
    exe "let s:have_" . name . "=" . e
  endif
  exe "return s:have_" . name
endfun

" Set b:gzip_comp_arg to the gzip argument to be used for compression, based on
" the flags in the compressed file.
" The only compression methods that can be detected are max speed (-1) and max
" compression (-9).
fun s:set_compression(line)
  " get the Compression Method
  let l:cm = char2nr(a:line[2])
  " if it's 8 (DEFLATE), we can check for the compression level
  if l:cm == 8
    " get the eXtra FLags
    let l:xfl = char2nr(a:line[8])
    " max compression
    if l:xfl == 2
      let b:gzip_comp_arg = "-9"
    " min compression
    elseif l:xfl == 4
      let b:gzip_comp_arg = "-1"
    endif
  endif
endfun


" After reading compressed file: Uncompress text in buffer with "cmd"
fun s:read(cmd)
  " don't do anything if the cmd is not supported
  if !s:check(a:cmd)
    return
  endif

  " for gzip check current compression level and set b:gzip_comp_arg.
  silent! unlet b:gzip_comp_arg
  if a:cmd[0] == 'g'
    call s:set_compression(getline(1))
  endif

  " make 'patchmode' empty, we don't want a copy of the written file
  let pm_save = &pm
  set pm=
  " remove 'a' and 'A' from 'cpo' to avoid the alternate file changes
  let cpo_save = &cpo
  set cpo-=a cpo-=A
  " set 'modifiable'
  let ma_save = &ma
  setlocal ma
  " when filtering the whole buffer, it will become empty
  let empty = line("'[") == 1 && line("']") == line("$")
  let tmp = tempname()
  let tmpe = tmp . "." . expand("<afile>:e")
  " write the just read lines to a temp file "'[,']w tmp.gz"
  execute "silent '[,']w " . tmpe
  " uncompress the temp file: call system("gzip -dn tmp.gz")
  call system(a:cmd . " " . tmpe)
  if !filereadable(tmp)
    " uncompress didn't work!  Keep the compressed file then.
    echoerr "Error: Could not read uncompressed file"
    return
  endif
  " delete the compressed lines; remember the line number
  let l = line("'[") - 1
  if exists(":lockmarks")
    lockmarks '[,']d _
  else
    '[,']d _
  endif
  " read in the uncompressed lines "'[-1r tmp"
  setlocal nobin
  if exists(":lockmarks")
    execute "silent lockmarks " . l . "r " . tmp
  else
    execute "silent " . l . "r " . tmp
  endif

  " if buffer became empty, delete trailing blank line
  if empty
    silent $delete _
    1
  endif
  " delete the temp file and the used buffers
  call delete(tmp)
  silent! exe "bwipe " . tmp
  silent! exe "bwipe " . tmpe
  let &pm = pm_save
  let &cpo = cpo_save
  let &l:ma = ma_save
  " When uncompressed the whole buffer, do autocommands
  if empty
    if &verbose >= 8
      execute "doau BufReadPost " . expand("%:r")
    else
      execute "silent! doau BufReadPost " . expand("%:r")
    endif
  endif
endfun

" After writing compressed file: Compress written file with "cmd"
fun s:write(cmd)
  " don't do anything if the cmd is not supported
  if s:check(a:cmd)
    " Rename the file before compressing it.
    let nm = resolve(expand("<afile>"))
    let nmt = s:tempname(nm)
    if rename(nm, nmt) == 0
      if exists("b:gzip_comp_arg")
	call system(a:cmd . " " . b:gzip_comp_arg . " " . nmt)
      else
	call system(a:cmd . " " . nmt)
      endif
      call rename(nmt . "." . expand("<afile>:e"), nm)
    endif
  endif
endfun

" Before appending to compressed file: Uncompress file with "cmd"
fun s:appre(cmd)
  " don't do anything if the cmd is not supported
  if s:check(a:cmd)
    let nm = expand("<afile>")

    " for gzip check current compression level and set b:gzip_comp_arg.
    silent! unlet b:gzip_comp_arg
    if a:cmd[0] == 'g'
      call s:set_compression(readfile(nm, "b", 1)[0])
    endif

    " Rename to a weird name to avoid the risk of overwriting another file
    let nmt = expand("<afile>:p:h") . "/X~=@l9q5"
    let nmte = nmt . "." . expand("<afile>:e")
    if rename(nm, nmte) == 0
      if &patchmode != "" && getfsize(nm . &patchmode) == -1
	" Create patchmode file by creating the decompressed file new
	call system(a:cmd . " -c " . nmte . " > " . nmt)
	call rename(nmte, nm . &patchmode)
      else
	call system(a:cmd . " " . nmte)
      endif
      call rename(nmt, nm)
    endif
  endif
endfun

" find a file name for the file to be compressed.  Use "name" without an
" extension if possible.  Otherwise use a weird name to avoid overwriting an
" existing file.
fun s:tempname(name)
  let fn = fnamemodify(a:name, ":r")
  if !filereadable(fn) && !isdirectory(fn)
    return fn
  endif
  return fnamemodify(a:name, ":p:h") . "/X~=@l9q5"
endfun

" vim: set sw=2 :
