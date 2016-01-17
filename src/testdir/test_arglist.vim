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

func Test_argadd()
  %argdelete
  argadd a b c
  call assert_equal(0, argidx())

  %argdelete
  argadd a
  call assert_equal(0, argidx())
  argadd b c d
  call assert_equal(0, argidx())

  call Init_abc()
  argadd x
  call Assert_argc(['a', 'b', 'x', 'c'])
  call assert_equal(1, argidx())

  call Init_abc()
  0argadd x
  call Assert_argc(['x', 'a', 'b', 'c'])
  call assert_equal(2, argidx())

  call Init_abc()
  1argadd x
  call Assert_argc(['a', 'x', 'b', 'c'])
  call assert_equal(2, argidx())

  call Init_abc()
  $argadd x
  call Assert_argc(['a', 'b', 'c', 'x'])
  call assert_equal(1, argidx())

  call Init_abc()
  $argadd x
  +2argadd y
  call Assert_argc(['a', 'b', 'c', 'x', 'y'])
  call assert_equal(1, argidx())
endfunc

func Init_abc()
  args a b c
  next
endfunc

func Assert_argc(l)
  call assert_equal(len(a:l), argc())
  let i = 0
  while i < len(a:l) && i < argc()
    call assert_equal(a:l[i], argv(i))
    let i += 1
  endwhile
endfunc
