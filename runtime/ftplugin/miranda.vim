" Vim filetype plugin file
" Language:	Miranda
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Sep 20

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal comments=:\|\|
setlocal commentstring=\|\|\ %s
setlocal formatoptions+=croql formatoptions-=t

setlocal iskeyword=a-z,A-Z,48-57,_,'

let &l:include = '%\%(insert\|include\)'
setlocal suffixes+=.x
setlocal suffixesadd=.m

let b:undo_ftplugin = "setl cms< com< fo< inc< isk< su< sua<"

if (has("gui_win32") || has("gui_gtk")) && !exists("b:browsefilter")
  let b:browsefilter =
	\ "Miranda Script Files (*.m)\t*.m\n" ..
	\ "Miranda Literate Script Files (*.lit.m)\t*.lit.m\n"
  if has("win32")
    let b:browsefilter ..= "All Files (*.*)\t*\n"
  else
    let b:browsefilter ..= "All Files (*)\t*\n"
  endif
  let b:undo_ftplugin ..= " | unlet! b:browsefilter"
endif

" vim: nowrap sw=2 sts=2 ts=8 noet:
