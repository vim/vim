" vim:set ts=8 sts=4 sw=4:
"
" tar.vim -- a Vim plugin for browsing tarfiles
" Copyright (c) 2002, Michael C. Toren <mct@toren.net>
" Distributed under the GNU General Public License.
"
" Version: 1.01
" Last Change: 2005 Jul 26
"
" Updates are available from <http://michael.toren.net/code/>.  If you
" find this script useful, or have suggestions for improvements, please
" let me know.
" Also look there for further comments and documentation.
"
" This part defines the functions.  The autocommands are in plugin/tar.vim.

let s:version = "1.01"

function! tar#Write(argument)
    echo "ERROR: Sorry, no write support for tarfiles yet"
endfunction

function! tar#Read(argument, cleanup)
    let l:argument = a:argument
    let l:argument = substitute(l:argument, '^tarfile:', '', '')
    let l:argument = substitute(l:argument, '^\~', $HOME, '')

    let l:tarfile = l:argument
    while 1
	if (l:tarfile == "" || l:tarfile == "/")
	    echo "ERROR: Could not find a readable tarfile in path:" l:argument
	    return
	endif

	if filereadable(l:tarfile) " found it!
	    break
	endif

	let l:tarfile = fnamemodify(l:tarfile, ":h")
    endwhile

    let l:toextract = strpart(l:argument, strlen(l:tarfile) + 1)

    if (l:toextract == "")
	return
    endif

    let l:cat = s:TarCatCommand(l:tarfile)
    execute "r !" . l:cat . " < '" . l:tarfile . "'"
	\ " | tar OPxf - '" . l:toextract . "'"

    if (a:cleanup)
	0d "blank line
	execute "doautocmd BufReadPost " . expand("%")
	setlocal readonly
	silent preserve
    endif
endfunction

function! tar#Browse(tarfile)
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
	echo "ERROR: File not readable:" l:tarfile
	return
    endif

    call s:Say("\" tar.vim version " . s:version)
    call s:Say("\" Browsing tarfile " . l:tarfile)
    call s:Say("\" Hit ENTER to view a file in a new window")
    call s:Say("")

    silent execute "r!" . l:cat . "<'" . l:tarfile . "'| tar Ptf - "
    0d "blank line
    /^$/1

    setlocal readonly
    setlocal nomodifiable
    noremap <silent> <buffer> <cr> :call <SID>TarBrowseSelect()<cr>
endfunction

function! s:TarBrowseSelect()
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
endfunction

" kludge to deal with compressed archives
function! s:TarCatCommand(tarfile)
    if a:tarfile =~# '\.\(gz\|tgz\|Z\)$'
	let l:cat = "gzip -d -c"
    elseif a:tarfile =~# '\.bz2$'
	let l:cat = "bzip2 -d -c"
    else
	let l:cat = "cat"
    endif
    return l:cat
endfunction

function! s:Say(string)
    let @" = a:string
    $ put
endfunction
