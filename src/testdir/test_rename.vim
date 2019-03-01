" Test rename()

func Test_rename_file_to_file()
  call writefile(['foo'], 'Xrename1')

  call assert_equal(0, rename('Xrename1', 'Xrename2'))

  call assert_equal('', glob('Xrename1'))
  call assert_equal(['foo'], readfile('Xrename2'))

  " When the destination file already exists, it should be overwritten.
  call writefile(['foo'], 'Xrename1')
  call writefile(['bar'], 'Xrename2')

  call assert_equal(0, rename('Xrename1', 'Xrename2'))
  call assert_equal('', glob('Xrename1'))
  call assert_equal(['foo'], readfile('Xrename2'))

  call delete('Xrename2')
endfunc

func Test_rename_same_file()
  call writefile(['foo'], 'Xrename')

  " When the source and destination are the same file, nothing
  " should be done. The source file should not be deleted.
  call assert_equal(0, rename('Xrename', 'Xrename'))
  call assert_equal(['foo'], readfile('Xrename'))

  call assert_equal(0, rename('./Xrename', 'Xrename'))
  call assert_equal(['foo'], readfile('Xrename'))

  call delete('Xrename')
endfunc

func Test_rename_dir_to_dir()
  call mkdir('Xdir1')
  call writefile(['foo'], 'Xdir1/Xfile')

  call assert_equal(0, rename('Xdir1', 'Xdir2'))

  call assert_equal('', glob('Xdir1'))
  call assert_equal(['foo'], readfile('Xdir2/Xfile'))

  call delete('Xdir2/Xfile')
  call delete('Xdir2', 'd')
endfunc

func Test_rename_same_dir()
  call mkdir('Xdir')
  call writefile(['foo'], 'Xdir/Xfile')

  call assert_equal(0, rename('Xdir', 'Xdir'))

  call assert_equal(['foo'], readfile('Xdir/Xfile'))

  call delete('Xdir/Xfile')
  call delete('Xdir', 'd')
endfunc

func Test_rename_copy()
  " Check that when original file can't be deleted, rename()
  " still succeeds but copies the file, and preserve its permissions.
  call mkdir('Xdir')
  call writefile(['foo'], 'Xdir/Xfile')
  call setfperm('Xdir', 'r-xr-xr-x')

  call assert_equal(0, rename('Xdir/Xfile', 'Xfile'))

  call assert_equal(['foo'], readfile('Xdir/Xfile'))
  call assert_equal(['foo'], readfile('Xfile'))

  call setfperm('Xdir', 'rwxrwxrwx')
  call delete('Xdir/Xfile')
  call delete('Xdir', 'd')
  call delete('Xfile')
endfunc

func Test_rename_fails()
  call writefile(['foo'], 'Xfile')

  " Can't rename into a non-existing directory.
  call assert_notequal(0, rename('Xfile', 'Xdoesnotexist/Xfile'))

  " Can't rename a non-existing file.
  call assert_notequal(0, rename('Xdoesnotexist', 'Xfile2'))
  call assert_equal('', glob('Xfile2'))

  " When rename files, the destination file should not be deleted.
  call assert_notequal(0, rename('Xdoesnotexist', 'Xfile'))
  call assert_equal(['foo'], readfile('Xfile'))

  " Can't rename to en empty file name.
  call assert_notequal(0, rename('Xfile', ''))

  call assert_fails('call rename("Xfile", [])', 'E730')
  call assert_fails('call rename(0z, "Xfile")', 'E976')

  call delete('Xfile')
endfunc
