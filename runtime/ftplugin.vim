" Vim support file to switch on loading plugins for file types
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last change:	2003 May 10

if exists("did_load_ftplugin")
  finish
endif
let did_load_ftplugin = 1

augroup filetypeplugin
  au FileType * call s:LoadFTPlugin()
  func! s:LoadFTPlugin()
    if expand("<amatch>") != ""
      if exists("b:undo_ftplugin")
	exe b:undo_ftplugin
	unlet b:undo_ftplugin b:did_ftplugin
      endif
      if &cpo =~# "S" && exists("b:did_ftplugin")
	" In compatible mode options are reset to the global values, need to
	" set the local values also when a plugin was already used.
	unlet b:did_ftplugin
      endif
      runtime! ftplugin/<amatch>.vim ftplugin/<amatch>_*.vim ftplugin/<amatch>/*.vim
    endif
  endfunc
augroup END
