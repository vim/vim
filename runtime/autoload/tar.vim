"
" tar.vim -- a Vim plugin for browsing tarfiles
" Copyright (c) 2002, Michael C. Toren <mct@toren.net>
" Distributed under the GNU General Public License.
"
" Version:	2
" Date:		Sep 14, 2005
" Modified By:	Charles E. Campbell, Jr.
"
" Updates are available from <http://michael.toren.net/code/>.  If you
" find this script useful, or have suggestions for improvements, please
" let me know.
" Also look there for further comments and documentation.
"
" This part defines the functions.  The autocommands are in plugin/tar.vim.
if exists("g:loaded_tar") || &cp
 finish
endif
let g:loaded_tar= "v2"

" ---------------------------------------------------------------------
"  tar#Read: {{{1
fun! tar#Read(argument, cleanup)
"    call Dfunc("tar#Read(argument<".a:argument."> cleanup=".a:cleanup.")")
    let l:argument = a:argument
    let l:argument = substitute(l:argument, '^tarfile:', '', '')
    let l:argument = substitute(l:argument, '^\~', $HOME, '')

    let l:tarfile = l:argument
    while 1
	if (l:tarfile == "" || l:tarfile == "/")
	    echo "***error*** (tar#Read) Could not find a readable tarfile in path:" l:argument
"            call Dret("tar#Read")
	    return
	endif

	if filereadable(l:tarfile) " found it!
	    break
	endif

	let l:tarfile = fnamemodify(l:tarfile, ":h")
    endwhile

    let l:toextract = strpart(l:argument, strlen(l:tarfile) + 1)

    if (l:toextract == "")
"        call Dret("tar#Read")
	return
    endif

    let l:cat = s:TarCatCommand(l:tarfile)
    execute "r !" . l:cat . " < '" . l:tarfile . "'"
	\ " | tar OPxf - '" . l:toextract . "'"

    if (a:cleanup)
	0d "blank line
	execute "doautocmd BufReadPost " . expand("%")
	setlocal nomod
	silent preserve
    endif
"    call Dret("tar#Read")
endfun

" ---------------------------------------------------------------------
" tar#Write: {{{1
fun! tar#Write(argument)
"  call Dfunc("tar#Write(argument<".a:argument.">)")
"
  " sanity checks
  if !executable("tar")
   echo "***error*** (TarWrite) sorry, your system doesn't appear to have the tar pgm"
"   call Dret("tar#Write")
   return
  endif
  if !exists("*mkdir")
   echo "***error*** (TarWrite) sorry, mkdir() doesn't work on your system"
"   call Dret("tar#Write")
   return
  endif

  let curdir= getcwd()
  let tmpdir= tempname()
"  call Decho("orig tempname<".tmpdir.">")
  if tmpdir =~ '\.'
   let tmpdir= substitute(tmpdir,'\.[^.]*$','','e')
  endif
"  call Decho("tmpdir<".tmpdir.">")
  call mkdir(tmpdir,"p")

  " attempt to change to the indicated directory
  try
   exe "cd ".escape(tmpdir,' \')
  catch /^Vim\%((\a\+)\)\=:E344/
   echo "***error*** (TarWrite) cannot cd to temporary directory"
"   call Dret("tar#Write")
   return
  endtry
"  call Decho("current directory now: ".getcwd())

  " place temporary files under .../_TARVIM_/
  if isdirectory("_TARVIM_")
   call s:Rmdir("_TARVIM_")
  endif
  call mkdir("_TARVIM_")
  cd _TARVIM_
"  call Decho("current directory now: ".getcwd())

  let tarfile = curdir."/".substitute(a:argument,'tarfile:\([^/]\{-}\)/.*$','\1','')
  let path    = substitute(a:argument,'^.\{-}/','','')
  let dirpath = substitute(path,'/\=[^/]\+$','','')
"  call Decho("path   <".path.">")
"  call Decho("dirpath<".dirpath.">")
  call mkdir(dirpath,"p")
  exe "w! ".path
  if executable("cygpath")
   let path    = substitute(system("cygpath ".path),'\n','','e')
   let tarfile = substitute(system("cygpath ".tarfile),'\n','','e')
  endif

"  call Decho("tar --delete -f ".tarfile." ".path)
  call system("tar --delete -f ".tarfile." ".path)
  if v:shell_error != 0
   echo "***error*** (TarWrite) sorry, your tar pgm doesn't support deletion of ".path
  else
"   call Decho("tar -rf ".tarfile." ".path)
   call system("tar -rf ".tarfile." ".path)
  endif
  
  " cleanup and restore current directory
  cd ..
  call s:Rmdir("_TARVIM_")
  exe "cd ".escape(curdir,' \')
  setlocal nomod

"  call Dret("tar#Write")
endfun

" ---------------------------------------------------------------------
"  tar#Browse: {{{1
fun! tar#Browse(tarfile)
"    call Dfunc("tar#Browse(tarfile<".a:tarfile.">)")
    setlocal noswapfile
    setlocal buftype=nofile
    setlocal bufhidden=hide
    setlocal filetype=
    setlocal nobuflisted
    setlocal buftype=nofile
    setlocal wrap
    setlocal syntax=tar

    let l:tarfile = a:tarfile
    let b:tarfile = l:tarfile
    let l:cat = s:TarCatCommand(l:tarfile)

    if ! filereadable(l:tarfile)
	let l:tarfile = substitute(l:tarfile, '^tarfile:', '', '')
    endif

    if ! filereadable(l:tarfile)
	echo "***error*** (tar#Browse) File not readable:" l:tarfile
"        call Dret("tar#Browse")
	return
    endif

    call s:Say("\" tar.vim version " . g:loaded_tar)
    call s:Say("\" Browsing tarfile " . l:tarfile)
    call s:Say("\" Hit ENTER to view a file in a new window")
    call s:Say("")

    silent execute "r!" . l:cat . "<'" . l:tarfile . "'| tar Ptf - "
    0d "blank line
    /^$/1

    setlocal noma nomod ro

    noremap <silent> <buffer> <cr> :call <SID>TarBrowseSelect()<cr>
"    call Dret("tar#Browse")
endfun

" ---------------------------------------------------------------------
"  TarBrowseSelect: {{{1
fun! s:TarBrowseSelect()
    let l:line = getline(".")

    if (l:line =~ '^" ')
	return
    endif

    if (l:line =~ '/$')
	echo "Please specify a file, not a directory"
	return
    endif

    let l:selection = "tarfile:" .  b:tarfile . "/" . l:line
    new
    wincmd _
    execute "e " . l:selection
endfun

" ---------------------------------------------------------------------
" TarCatCommand: kludge to deal with compressed archives {{{1
fun! s:TarCatCommand(tarfile)
"    call Dfunc("s:TarCatCommand(tarfile<".a:tarfile.">)")
    if a:tarfile =~# '\.\(gz\|tgz\|Z\)$'
	let l:cat = "gzip -d -c"
    elseif a:tarfile =~# '\.bz2$'
	let l:cat = "bzip2 -d -c"
    else
	let l:cat = "cat"
    endif
"    call Dret("s:TarCatCommand ".l:cat)
    return l:cat
endfun

" ---------------------------------------------------------------------
"  Say: {{{1
fun! s:Say(string)
    let @" = a:string
    $ put
endfun

" ---------------------------------------------------------------------
" Rmdir: {{{1
fun! s:Rmdir(fname)
"  call Dfunc("Rmdir(fname<".a:fname.">)")
  if has("unix")
   call system("/bin/rm -rf ".a:fname)
  elseif has("win32") || has("win95") || has("win64") || has("win16")
   if &shell =~? "sh$"
    call system("/bin/rm -rf ".a:fname)
   else
    call system("del /S ".a:fname)
   endif
  endif
"  call Dret("Rmdir")
endfun

" ---------------------------------------------------------------------
"  Modelines: {{{1
" vim:set ts=8 sts=4 sw=4 fdm=marker:
