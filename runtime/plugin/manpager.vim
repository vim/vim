" Vim plugin for using Vim as manpager.
" Maintainer: Enno Nagel <ennonagel+vim@gmail.com>
" Last Change: 2022 Jun 05

command! -nargs=0 MANPAGER call s:ManPager() | delcommand MANPAGER

function s:ManPager()
  " global options, keep these to a minimum to avoid side effects
  if &compatible
    set nocompatible
  endif
  if exists('+viminfofile')
    set viminfofile=NONE
  endif
  set noswapfile 

  setlocal ft=man
  runtime ftplugin/man.vim
  setlocal buftype=nofile bufhidden=hide iskeyword+=: modifiable

  " Emulate 'col -b'
  silent! keepj keepp %s/\v(.)\b\ze\1?//ge

  " Remove ansi sequences
  silent! keepj keepp %s/\v\e\[%(%(\d;)?\d{1,2})?[mK]//ge

  " Remove empty lines above the header
  call cursor(1, 1)
  let n = search(".*(.*)", "c")
  if n > 1
    exe "1," . n-1 . "d"
  endif
  setlocal nomodifiable nomodified readonly nowrite

  syntax on
endfunction
