" Vim filetype plugin
" Language:	git config file
" Maintainer:	Tim Pope <vimNOSPAM@tpope.info>
" Last Change:	2007 Dec 16

" Only do this when not done yet for this buffer
if (exists("b:did_ftplugin"))
  finish
endif
let b:did_ftplugin = 1

setlocal formatoptions-=t formatoptions+=croql
setlocal comments=:#,:; commentstring=;\ %s

let b:undo_ftplugin = "setl fo< com< cms<"
