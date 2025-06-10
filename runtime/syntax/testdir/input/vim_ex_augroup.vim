" Vim :augroup command
" VIM_TEST_SETUP let g:vimsyn_folding = "a"
" VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax
" VIM_TEST_SETUP highlight link vimAugroupName Todo


augroup foo
  autocmd!
  autocmd BufRead * echo "Foo"
augroup END

augroup  foo | autocmd! | augroup END
augroup! foo

augroup !@#$%^&*()_+
  autocmd BufRead * echomsg "Foo"
augroup END

augroup  !@#$%^&*()_+ | autocmd! | augroup END
augroup! !@#$%^&*()_+

augroup  !@#$%^&*()_+ | autocmd! | augroup END
augroup! !@#$%^&*()_+

augroup  no\|echo | autocmd! | augroup END
augroup! no\|echo

augroup  no\"echo | autocmd! | augroup END
augroup! no\"echo

augroup  \|echo\| | autocmd! | augroup END
augroup! \|echo\|

augroup  \"echo\" | autocmd! | augroup END
augroup! \"echo\"

augroup  \|\" | autocmd! | augroup END
augroup! \|\"

augroup  \"\| | autocmd! | augroup END
augroup! \"\|


augroup  foo"comment
  au!
  au BufRead * echo "Foo"
augroup END"comment

augroup  foo|echo "Foo"
  au!
  au BufRead * echo "Foo"
augroup END|echo "Foo"

augroup! foo"comment
augroup! foo|echo "Foo"


" list groups
augroup
augroup | echo "Foo"
augroup " comment

