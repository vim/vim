" Vim support file to switch off loading plugins for file types
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2002 Apr 04

if exists("did_load_ftplugin")
  unlet did_load_ftplugin
endif

" Remove all autocommands in the filetypeplugin group
silent! au! filetypeplugin *
