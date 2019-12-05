" Tests for the preview window
source check.vim
CheckFeature quickfix

func Test_Psearch()
  " this used to cause ml_get errors
  help
  let wincount = winnr('$')
  0f
  ps.
  call assert_equal(wincount + 1, winnr('$'))
  pclose
  call assert_equal(wincount, winnr('$'))
  bwipe
endfunc
