" Vim filetype plugin file
" Language:	TypeScript
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Jan 14
" 		2024 May 23 by Riley Bruins <ribru17@gmail.com> ('commentstring')

runtime! ftplugin/javascript.vim

" Change the :browse e filter to primarily show TypeScript-related files.
if (has("gui_win32") || has("gui_gtk")) && !exists("b:browsefilter")
    let  b:browsefilter="TypeScript Files (*.ts)\t*.ts\n" .
		\	"TypeScript Declaration Files (*.d.ts)\t*.d.ts\n" .
		\	"TSX Files (*.tsx)\t*.tsx\n" .
		\	"JavaScript Files (*.js)\t*.js\n" .
		\	"JavaScript Modules (*.es, *.cjs, *.mjs)\t*.es;*.cjs;*.mjs\n" .
		\	"JSON Files (*.json)\t*.json\n"
    if has("win32")
	let b:browsefilter .= "All Files (*.*)\t*\n"
    else
	let b:browsefilter .= "All Files (*)\t*\n"
    endif
    let b:undo_ftplugin .= " | unlet! b:browsefilter"
endif
