" Test for triming strings.
func Test_flatten()
  call assert_fails('call flatten(1)', 'E686:')
  call assert_fails('call flatten({})', 'E686:')
  call assert_fails('call flatten("string")', 'E686:')
  call assert_equal([], flatten([], 0))
  call assert_equal([], flatten([[[[[[[[[[[[]]]]]]]]]]]]))

  call assert_equal([1, 2, 3], flatten([1, 2, 3]))
  call assert_equal([1, 2, 3], flatten([[1], 2, 3]))
  call assert_equal([1, 2, 3], flatten([1, [2], 3]))
  call assert_equal([1, 2, 3], flatten([1, 2, [3]]))
  call assert_equal([1, 2, 3], flatten([[1], [2], 3]))
  call assert_equal([1, 2, 3], flatten([1, [2], [3]]))
  call assert_equal([1, 2, 3], flatten([[1], 2, [3]]))
  call assert_equal([1, 2, 3], flatten([[1], [2], [3]]))

  call assert_equal([1, 2, 3], flatten([[[[1]]], [2], [3]]))
  call assert_equal([1, 2, 3], flatten([[1, 2, 3], []]))
  call assert_equal([1, 2, 3], flatten([[], [1, 2, 3]]))
  call assert_equal([1, 2, 3], flatten([[1, 2], [], [3]]))
  call assert_equal([1, 2, 3], flatten([[], [1, 2, 3], []]))

  call assert_equal([1, 2, 3], flatten([[1, 2, 3]], 1))
  call assert_equal([1, 2, 3], flatten([[1, 2, 3]], 2))
  call assert_equal([[1], [2], [3]], flatten([[[1], [2], [3]]], 1))

  let l:list = [[1], [2], [3]]
  call assert_equal([1, 2, 3], flatten(l:list))
  call assert_equal([1, 2, 3], l:list)

  " Tests for checking reference counter works well.
  let l:x = {'foo': 'bar'}
  call assert_equal([1, 2, l:x, 3], flatten([1, [2, l:x], 3]))
  call garbagecollect()
  call assert_equal('bar', l:x.foo)

  let l:list = [[1], [2], [3]]
  call assert_equal([1, 2, 3], flatten(l:list))
  call garbagecollect()
  call assert_equal([1, 2, 3], l:list)
endfunc
