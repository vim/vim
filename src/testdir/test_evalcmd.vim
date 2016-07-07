" test evalcmd()

func Test_evalcmd()
  call assert_equal("\nnocompatible", evalcmd('set compatible?'))
  call assert_equal("\nsomething\nnice", evalcmd('echo "something\nnice"'))
  call assert_fails('call evalcmd("doesnotexist")', 'E492:')
endfunc

