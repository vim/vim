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
  call mkdir('Xdir1/empty')
  split Xdir1/Xfile
  call setline(1, ['a', 'b'])
  w
  w Xdir1/subdir/Xfile
  close
  call assert_true(isdirectory('Xdir1'))
  call assert_equal(['a', 'b'], readfile('Xdir1/Xfile'))
  call assert_true(isdirectory('Xdir1/subdir'))
  call assert_equal(['a', 'b'], readfile('Xdir1/subdir/Xfile'))
  call assert_true(isdirectory('Xdir1/empty'))
  call assert_equal(0, delete('Xdir1', 'rf'))
  call assert_false(isdirectory('Xdir1'))
  call assert_equal(-1, delete('Xdir1', 'd'))
endfunc

func Test_symlink_delete()
  if !has('unix')
    return
  endif
  split Xfile
  call setline(1, ['a', 'b'])
  wq
  silent !ln -s Xfile Xlink
  " Delete the link, not the file
  call assert_equal(0, delete('Xlink'))
  call assert_equal(-1, delete('Xlink'))
  call assert_equal(0, delete('Xfile'))
endfunc

func Test_symlink_dir_delete()
  if !has('unix')
    return
  endif
  call mkdir('Xdir1')
  silent !ln -s Xdir1 Xlink
  call assert_true(isdirectory('Xdir1'))
  call assert_true(isdirectory('Xlink'))
  " Delete the link, not the directory
  call assert_equal(0, delete('Xlink'))
  call assert_equal(-1, delete('Xlink'))
  call assert_equal(0, delete('Xdir1', 'd'))
endfunc

func Test_symlink_recursive_delete()
  if !has('unix')
    return
  endif
  call mkdir('Xdir3')
  call mkdir('Xdir3/subdir')
  call mkdir('Xdir4')
  split Xdir3/Xfile
  call setline(1, ['a', 'b'])
  w
  w Xdir3/subdir/Xfile
  w Xdir4/Xfile
  close
  silent !ln -s ../Xdir4 Xdir3/Xlink

  call assert_true(isdirectory('Xdir3'))
  call assert_equal(['a', 'b'], readfile('Xdir3/Xfile'))
  call assert_true(isdirectory('Xdir3/subdir'))
  call assert_equal(['a', 'b'], readfile('Xdir3/subdir/Xfile'))
  call assert_true(isdirectory('Xdir4'))
  call assert_true(isdirectory('Xdir3/Xlink'))
  call assert_equal(['a', 'b'], readfile('Xdir4/Xfile'))

  call assert_equal(0, delete('Xdir3', 'rf'))
  call assert_false(isdirectory('Xdir3'))
  call assert_equal(-1, delete('Xdir3', 'd'))
  " symlink is deleted, not the directory it points to
  call assert_true(isdirectory('Xdir4'))
  call assert_equal(['a', 'b'], readfile('Xdir4/Xfile'))
  call assert_equal(0, delete('Xdir4/Xfile'))
  call assert_equal(0, delete('Xdir4', 'd'))
endfunc

func Test_complicated_name_recursive_delete()
  call mkdir('Xcomplicated')
  call mkdir('Xcomplicated/[complicated-1 ]')
  call mkdir('Xcomplicated/{complicated,2 }')
  call mkdir('Xcomplicated/(complicated-3 |')
  split Xcomplicated/Xfile
  call setline(1, ['a', 'b'])
  w
  w Xcomplicated/\[complicated-1\ \]/Xfile
  w Xcomplicated/\{complicated,2\ \}/Xfile
  w Xcomplicated/\(complicated-3\ \|/Xfile
  close
  call assert_true(isdirectory('Xcomplicated'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/Xfile'))
  call assert_true(isdirectory('Xcomplicated/[complicated-1 ]'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/[complicated-1 ]/Xfile'))
  call assert_true(isdirectory('Xcomplicated/{complicated,2 }'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/{complicated,2 }/Xfile'))
  call assert_true(isdirectory('Xcomplicated/(complicated-3 |'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/(complicated-3 |/Xfile'))
  call assert_equal(0, delete('Xcomplicated', 'rf'))
  call assert_false(isdirectory('Xcomplicated'))
  call assert_equal(-1, delete('Xcomplicated', 'd'))
endfunc

func Test_complicated_name_recursive_delete_unix()
  if !has('unix')
    return
  endif
  call mkdir('Xcomplicated')
  call mkdir('Xcomplicated/[complicated ?')
  split Xcomplicated/Xfile
  call setline(1, ['a', 'b'])
  w
  w Xcomplicated/\[complicated\ \?/Xfile
  close
  call assert_true(isdirectory('Xcomplicated'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/Xfile'))
  call assert_true(isdirectory('Xcomplicated/[complicated ?'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/[complicated ?/Xfile'))
  call assert_equal(0, delete('Xcomplicated', 'rf'))
  call assert_false(isdirectory('Xcomplicated'))
  call assert_equal(-1, delete('Xcomplicated', 'd'))
endfunc
