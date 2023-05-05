" Test filecopy()

source shared.vim

func Test_copy_file_to_file()
  call writefile(['foo'], 'Xcopy1')

  call assert_equal(0, filecopy('Xcopy1', 'Xcopy2'))

  call assert_equal(['foo'], readfile('Xcopy2'))

  " When the destination file already exists, it should be overwritten.
  call writefile(['foo'], 'Xcopy1')
  call writefile(['bar'], 'Xcopy2', 'D')

  call assert_equal(0, filecopy('Xcopy1', 'Xcopy2'))
  call assert_equal(['foo'], readfile('Xcopy2'))

  call delete('Xcopy2')
  call delete('Xcopy1')
endfunc

func Test_copy_dir_to_dir()
  call mkdir('Xcopydir1')
  call writefile(['foo'], 'Xcopydir1/Xfilecopy')
  call mkdir('Xcopydir2')

  call assert_equal(0, filecopy('Xcopydir1', 'Xcopydir2'))

  call assert_equal(['foo'], readfile('Xcopydir2/Xfilecopy'))

  call delete('Xcopydir2', 'rf')
  call delete('Xcopydir1', 'rf')
endfunc

func Test_copy_fails()
  call writefile(['foo'], 'Xfilecopy', 'D')

  " Can't copy into a non-existing directory.
  call assert_notequal(0, filecopy('Xfilecopy', 'Xdoesnotexist/Xfilecopy'))

  " Can't copy a non-existing file.
  call assert_notequal(0, filecopy('Xdoesnotexist', 'Xfilecopy2'))
  call assert_equal('', glob('Xfilecopy2'))

  " Can't copy to en empty file name.
  call assert_notequal(0, filecopy('Xfilecopy', ''))

  call assert_fails('call filecopy("Xfilecopy", [])', 'E730:')
  call assert_fails('call filecopy(0z, "Xfilecopy")', 'E976:')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
