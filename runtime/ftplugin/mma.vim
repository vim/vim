" Vim filetype plugin file
" Language:	Mathematica
" Maintainer:	Ian Ford <ianf@wolfram.com>
" Last Change:	22 January 2019
" 		23 May 2024 by Riley Bruins <ribru17@gmail.com> ('commentstring')

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
	finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

let b:undo_ftplugin = "setlocal commentstring<"

setlocal commentstring=\(*\ %s\ *\)
