" Test argument list commands

func Test_argidx()
  args a b c
  last
  call assert_equal(2, argidx())
  %argdelete
  call assert_equal(0, argidx())

  args a b c
  call assert_equal(0, argidx())
  next
  call assert_equal(1, argidx())
  next
  call assert_equal(2, argidx())
  1argdelete
  call assert_equal(1, argidx())
  1argdelete
  call assert_equal(0, argidx())
  1argdelete
  call assert_equal(0, argidx())
endfunc
