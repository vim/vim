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

