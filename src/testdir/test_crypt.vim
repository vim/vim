" Tests for encryption.

if !has('cryptv')
  finish
endif

func Common_head_only(text)
  " This was crashing Vim
  split Xtest.txt
  call setline(1, a:text)
  wq
  call feedkeys(":split Xtest.txt\<CR>foobar\<CR>", "tx")
  call delete('Xtest.txt')
  call assert_match('VimCrypt', getline(1))
  bwipe!
endfunc

func Test_head_only_2()
  call Common_head_only('VimCrypt~02!abc')
endfunc

func Test_head_only_3()
  call Common_head_only('VimCrypt~03!abc')
endfunc
