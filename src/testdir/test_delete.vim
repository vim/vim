" Test for delete().

func Test_file_delete()
  split Xfile
  call setline(1, ['a', 'b'])
  wq
  call assert_equal(['a', 'b'], readfile('Xfile'))
  call assert_equal(0, delete('Xfile'))
  call assert_fails('call readfile("Xfile")', 'E484:')
  call assert_equal(-1, delete('Xfile'))
endfunc

func Test_dir_delete()
  call mkdir('Xdir1')
  call assert_true(isdirectory('Xdir1'))
  call assert_equal(0, delete('Xdir1', 'd'))
  call assert_false(isdirectory('Xdir1'))
  call assert_equal(-1, delete('Xdir1', 'd'))
endfunc

func Test_recursive_delete()
  call mkdir('Xdir1')
  call mkdir('Xdir1/subdir')
  split Xdir1/Xfile
  call setline(1, ['a', 'b'])
  w
  w Xdir1/subdir/Xfile
  close
  call assert_true(isdirectory('Xdir1'))
  call assert_equal(['a', 'b'], readfile('Xdir1/Xfile'))
  call assert_true(isdirectory('Xdir1/subdir'))
  call assert_equal(['a', 'b'], readfile('Xdir1/subdir/Xfile'))
  call assert_equal(0, delete('Xdir1', 'rf'))
  call assert_false(isdirectory('Xdir1'))
  call assert_equal(-1, delete('Xdir1', 'd'))
endfunc
