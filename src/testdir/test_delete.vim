" Test for delete().

func Test_file_delete()
  call writefile(['a', 'b'], 'Xfile')

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
  call mkdir('Xdir1/subdir', 'p')
  call mkdir('Xdir1/empty')
  call writefile(['a', 'b'], 'Xdir1/Xfile')
  call writefile(['a', 'b'], 'Xdir1/subdir/Xfile')

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

  call writefile(['a', 'b'], 'Xfile')
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

  call mkdir('Xdir3/subdir', 'p')
  call mkdir('Xdir4')
  call writefile(['a', 'b'], 'Xdir3/Xfile')
  call writefile(['a', 'b'], 'Xdir3/subdir/Xfile')
  call writefile(['a', 'b'], 'Xdir4/Xfile')
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
  call mkdir('Xcomplicated/[complicated-1 ]', 'p')
  call mkdir('Xcomplicated/{complicated,2 }', 'p')
  call writefile(['a', 'b'], 'Xcomplicated/Xfile')
  call writefile(['a', 'b'], 'Xcomplicated/[complicated-1 ]/Xfile')
  call writefile(['a', 'b'], 'Xcomplicated/{complicated,2 }/Xfile')

  call assert_true(isdirectory('Xcomplicated'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/Xfile'))
  call assert_true(isdirectory('Xcomplicated/[complicated-1 ]'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/[complicated-1 ]/Xfile'))
  call assert_true(isdirectory('Xcomplicated/{complicated,2 }'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/{complicated,2 }/Xfile'))
  call assert_equal(0, delete('Xcomplicated', 'rf'))
  call assert_false(isdirectory('Xcomplicated'))
  call assert_equal(-1, delete('Xcomplicated', 'd'))
endfunc

func Test_complicated_name_recursive_delete_unix()
  if !has('unix')
    return
  endif

  call mkdir('Xcomplicated/[complicated-1 ?', 'p')
  call writefile(['a', 'b'], 'Xcomplicated/Xfile')
  call writefile(['a', 'b'], 'Xcomplicated/[complicated-1 ?/Xfile')
  call writefile(['a', 'b'], 'Xcomplicated/[complicated-1 |/Xfile')

  call assert_true(isdirectory('Xcomplicated'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/Xfile'))
  call assert_true(isdirectory('Xcomplicated/[complicated-1 ?'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/[complicated-1 ?/Xfile'))
  call assert_true(isdirectory('Xcomplicated/(complicated-2 |'))
  call assert_equal(['a', 'b'], readfile('Xcomplicated/(complicated-2 |/Xfile'))
  call assert_equal(0, delete('Xcomplicated', 'rf'))
  call assert_false(isdirectory('Xcomplicated'))
  call assert_equal(-1, delete('Xcomplicated', 'd'))
endfunc
