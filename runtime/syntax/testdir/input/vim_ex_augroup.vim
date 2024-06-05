" Vim :augroup command

augroup foo
  autocmd BufRead * echomsg "Foo"
augroup END

augroup foo | autocmd! | augroup END
augroup! foo

augroup !@#$%^&*()_+
  autocmd BufRead * echomsg "Foo"
augroup END

augroup !@#$%^&*()_+ | autocmd! | augroup END
augroup! !@#$%^&*()_+

" list groups
augroup

" bang is an error for doautocmd and doautoall
augroup! foobar
autocmd! foobar

doautocmd! FileType,BufEnter
doautoall! BufWinEnter,WinEnter
