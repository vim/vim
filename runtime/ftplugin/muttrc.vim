" Vim filetype plugin file
" Language:             mutt RC File
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2006-04-19

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal comments=:# commentstring=#\ %s
setlocal formatoptions-=t formatoptions+=croql

let &l:include = '^\s*source\>'

let b:undo_ftplugin = "setl com< cms< inc< fo<"

if !has('unix')
  finish
endif

if !has('gui_running')
  command! -buffer -nargs=1 Sman
        \ silent exe '!' . 'LESS= MANPAGER="less --pattern=''^\s+' . <q-args> . '\b'' --hilite-search" man ' . 'muttrc' |
        \ redraw!
elseif has('terminal')
  command! -buffer -nargs=1 Sman
        \ silent exe 'term ' . 'env LESS= MANPAGER="less --pattern=''' . escape('^\s+' . <q-args> . '\b', '\') . ''' --hilite-search" man ' . 'muttrc'
else
  finish
endif
setlocal iskeyword+=-
setlocal keywordprg=:Sman

let b:undo_ftplugin .= '| setlocal keywordprg< iskeyword<'
