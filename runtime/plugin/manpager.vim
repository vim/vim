" Vim plugin for using Vim as manpager.
" Maintainer: Enno Nagel <ennonagel+vim@gmail.com>
" Last Change: 2017 November 07

" $MAN_PN is supposed to be set by MANPAGER, see ":help manpager.vim".
if empty($MAN_PN)
  finish
endif

command! -nargs=0 MANPAGER call s:MANPAGER() | delcommand MANPAGER

function! s:MANPAGER()
  let page_pattern = '\v\w[-_.:0-9A-Za-z]*'
  let sec_pattern = '\v\w+%(\+\w+)*'
  let pagesec_pattern = '\v(' . page_pattern . ')\((' . sec_pattern . ')\)'

  if $MAN_PN is '1'
    let manpage = tolower(matchstr( getline(nextnonblank(1)), '^' . pagesec_pattern ))
  else
    let manpage = expand($MAN_PN)
  endif

  let page_sec = matchlist(manpage, '^' . pagesec_pattern  . '$')

  bwipe!

  setlocal filetype=man
  exe 'Man' page_sec[2] page_sec[1]
endfunction
