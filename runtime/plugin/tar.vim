" vim:set ts=4 sw=4 ai nobackup:

" tar.vim -- a vim plugin for browsing tarfiles
" Copyright (c) 2002, Michael C. Toren <mct@toren.net>
"
" Updates are available from <http://michael.toren.net/code/>.  If you
" find this script useful, or have suggestions for improvements, please
" let me know.
"
" Usage:
" Once this script is installed, attempting to edit a tarfile will present
" the user with a list of files contained in the tar archive.  By moving the
" cursor over a filename and pressing ENTER, the contents of a file can be
" viewed in read-only mode, in a new window.  Unfortunately, write support
" for tarfile components is not currently possible.
" 
" Requirements:
" GNU tar, or a tar implementation that supports the "P" (don't strip
" out leading /'s from filenames), and "O" (extract files to standard
" output) options.  Additionally, gzip is required for handling *.tar.Z,
" *.tar.gz, and *.tgz compressed tarfiles, and bzip2 is required for
" handling *.tar.bz2 compressed tarfiles.  A unix-like operating system
" is probably also required.
" 
" Installation:
" Place this file, tar.vim, in your $HOME/.vim/plugin directory, and
" either restart vim, or execute ":source $HOME/.vim/plugin/tar.vim"
"
" Todo:
" - Handle zipfiles?
" - Implement write support, somehow.
"
" License:
" This program is free software; you can redistribute it and/or modify it
" under the terms of the GNU General Public License, version 2, as published
" by the Free Software Foundation.
"
" This program is distributed in the hope that it will be useful, but
" WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
" or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
" for more details.
"
" A copy of the GNU GPL is available as /usr/doc/copyright/GPL on Debian
" systems, or on the World Wide Web at http://www.gnu.org/copyleft/gpl.html
" You can also obtain it by writing to the Free Software Foundation, Inc.,
" 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
"
" Changelog:
" Tue Dec 31 13:38:08 EST 2002	First release to beta testers
" Sat Jan  4 14:06:19 EST 2003	Version 1.00 released

let s:version = "1.00"

if has("autocmd")
augroup tar
	au!
	au BufReadCmd	tarfile:*	call s:TarRead(expand("<afile>"), 1)
	au BufReadCmd	tarfile:*/*	call s:TarRead(expand("<afile>"), 1)
	au FileReadCmd	tarfile:*	call s:TarRead(expand("<afile>"), 0)
	au FileReadCmd	tarfile:*/*	call s:TarRead(expand("<afile>"), 0)

	au BufWriteCmd	tarfile:*	call s:TarWrite(expand("<afile>"))
	au BufWriteCmd	tarfile:*/*	call s:TarWrite(expand("<afile>"))
	au FileWriteCmd	tarfile:*	call s:TarWrite(expand("<afile>"))
	au FileWriteCmd	tarfile:*/*	call s:TarWrite(expand("<afile>"))

	au BufReadCmd	*.tar		call s:TarBrowse(expand("<afile>"))
	au BufReadCmd	*.tar.gz	call s:TarBrowse(expand("<afile>"))
	au BufReadCmd	*.tar.bz2	call s:TarBrowse(expand("<afile>"))
	au BufReadCmd	*.tar.Z		call s:TarBrowse(expand("<afile>"))
	au BufReadCmd	*.tgz		call s:TarBrowse(expand("<afile>"))
augroup END
endif

function! s:TarWrite(argument)
	echo "ERROR: Sorry, no write support for tarfiles yet"
endfunction

function! s:TarRead(argument, cleanup)
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

function! s:TarBrowse(tarfile)
	setlocal noswapfile
	setlocal buftype=nofile
	setlocal bufhidden=hide
	setlocal filetype=
	setlocal nobuflisted
	setlocal buftype=nofile
	setlocal wrap

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
	call s:Say("\" Hit ENTER to view contents in new window")
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
