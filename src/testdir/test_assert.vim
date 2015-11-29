" Test that the methods used for testing work.

func Test_assertFalse()
  call assertFalse(0)
endfunc

func Test_assertTrue()
  call assertTrue(1)
  call assertTrue(123)
endfunc

func Test_assertEqual()
  let s = 'foo'
  call assertEqual('foo', s)
  let n = 4
  call assertEqual(4, n)
  let l = [1, 2, 3]
  call assertEqual([1, 2, 3], l)
endfunc
