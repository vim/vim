" Vim support file to detect file types in scripts
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last change:	2022 Feb 13

" This file is called by an autocommand for every file that has just been
" loaded into a buffer.  It checks if the type of file can be recognized by
" the file contents.  The autocommand is in $VIMRUNTIME/filetype.vim.
"
" Note that the pattern matches are done with =~# to avoid the value of the
" 'ignorecase' option making a difference.  Where case is to be ignored use
" =~? instead.  Do not use =~ anywhere.


" Bail out when a FileType autocommand has already set the filetype.
if did_filetype()
  finish
endif

" Load the user defined scripts file first
" Only do this when the FileType autocommand has not been triggered yet
if exists("myscriptsfile") && filereadable(expand(myscriptsfile))
  execute "source " . myscriptsfile
  if did_filetype()
    finish
  endif
endif

" The main code is in a compiled function for speed.
call dist#script#DetectFiletype()
