" Test for :cd

func Test_cd_large_path()
  " This used crash with heap write overflow.
  call assert_fails('cd ' . repeat('x', 5000), 'E472:')
endfunc
