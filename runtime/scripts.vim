vim9script noclear

# Vim support file to detect file types in scripts
#
# Maintainer:	The Vim Project <https://github.com/vim/vim>
# Last Change:	2023 Aug 27
# Former Maintainer:	Bram Moolenaar <Bram@vim.org>

# This file is called by an autocommand for every file that has just been
# loaded into a buffer.  It checks if the type of file can be recognized by
# the file contents.  The autocommand is in $VIMRUNTIME/filetype.vim.

import "./autoload/dist/script.vim"

# Bail out when a FileType autocommand has already set the filetype.
if did_filetype()
  finish
endif

# Load the user defined scripts file first
# Only do this when the FileType autocommand has not been triggered yet
if exists("g:myscriptsfile") && filereadable(expand(g:myscriptsfile))
  execute "source " .. g:myscriptsfile
  if did_filetype()
    finish
  endif
endif

# The main code is in a compiled function for speed.
script.DetectFiletype()
