" Vim plugin for using Vim as manpager.
" Maintainer: Enno Nagel <ennonagel+vim@gmail.com>
" Last Change: 2022 Jun 17

" Set up the current buffer (likely read from stdin) as a manpage
command MANPAGER call s:ManPager()

function s:ManPager()
  " global options, keep these to a minimum to avoid side effects
  if &compatible
    set nocompatible
  endif
  if exists('+viminfofile')
    set viminfofile=NONE
  endif
  syntax on

  " Make this an unlisted, readonly scratch buffer
  setlocal buftype=nofile noswapfile bufhidden=hide nobuflisted readonly

  " Is this useful?  Should allow for using K on word with a colon.
  setlocal iskeyword+=:

  " Ensure text width matches window width
  setlocal foldcolumn& nofoldenable nonumber norelativenumber

  " In case Vim was invoked with -M
  setlocal modifiable

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

  " Finished preprocessing the buffer, prevent any further modifications
  setlocal nomodified nomodifiable

  " Set filetype to man even if ftplugin is disabled
  setlocal iskeyword+=: filetype=man
  runtime ftplugin/man.vim
endfunction
