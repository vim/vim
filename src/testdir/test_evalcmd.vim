" test evalcmd()

func NestedEval()
  let nested = evalcmd('echo "nested\nlines"')
  echo 'got: "' . nested . '"'
endfunc

func NestedRedir()
  redir => var
  echo 'broken'
  redir END
endfunc

func Test_evalcmd()
  call assert_equal("\nnocompatible", evalcmd('set compatible?'))
  call assert_equal("\nsomething\nnice", evalcmd('echo "something\nnice"'))
  call assert_equal("noendofline", evalcmd('echon "noendofline"'))
  call assert_equal("", evalcmd(123))

  call assert_equal("\ngot: \"\nnested\nlines\"", evalcmd('call NestedEval()'))
  redir => redired
  echo 'this'
  let evaled = evalcmd('echo "that"')
  echo 'theend'
  redir END
  call assert_equal("\nthis\ntheend", redired)
  call assert_equal("\nthat", evaled)

  call assert_fails('call evalcmd("doesnotexist")', 'E492:')
  call assert_fails('call evalcmd(3.4)', 'E806:')
  call assert_fails('call evalcmd("call NestedRedir()")', 'E930:')
endfunc

