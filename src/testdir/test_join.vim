" Test for joining lines.

func Test_join_with_count()
  new
  call setline(1, ['one', 'two', 'three', 'four'])
  normal J
  call assert_equal('one two', getline(1))
  %del
  call setline(1, ['one', 'two', 'three', 'four'])
  normal 10J
  call assert_equal('one two three four', getline(1))
  quit!
endfunc
