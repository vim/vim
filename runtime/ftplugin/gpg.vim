" Vim filetype plugin file
" Language:             gpg(1) configuration file
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2008-07-09

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal comments=:# commentstring=#\ %s formatoptions-=t formatoptions+=croql
let b:undo_ftplugin = "setl com< cms< fo<"

if !has('unix')
  finish
endif

if !has('gui_running')
  command! -buffer -nargs=1 Sman
        \ silent exe '!' . 'LESS= MANPAGER="less --pattern=''^\s+--' . <q-args> . '\b'' --hilite-search" man ' . 'gpg' |
        \ redraw!
elseif has('terminal')
  command! -buffer -nargs=1 Sman
        \ silent exe ':term ' . 'env LESS= MANPAGER="less --pattern=''' . escape('^\s+--' . <q-args> . '\b', '\') . ''' --hilite-search" man ' . 'gpg'
else
  finish
endif
setlocal iskeyword+=-
setlocal keywordprg=:Sman
let b:undo_ftplugin .= '| setlocal keywordprg< iskeyword<'
