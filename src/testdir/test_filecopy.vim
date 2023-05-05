" Test filecopy()

source shared.vim

func Test_copy_file_to_file()
  call writefile(['foo'], 'Xcopy1')

  call assert_equal(0, copy('Xcopy1', 'Xcopy2'))

  call assert_equal('', glob('Xcopy1'))
  call assert_equal(['foo'], readfile('Xcopy2'))

  " When the destination file already exists, it should be overwritten.
  call writefile(['foo'], 'Xcopy1')
  call writefile(['bar'], 'Xcopy2', 'D')

  call assert_equal(0, copy('Xcopy1', 'Xcopy2'))
  call assert_equal('', glob('Xcopy1'))
  call assert_equal(['foo'], readfile('Xcopy2'))

  call delete('Xcopy2')
endfunc

func Test_copy_dir_to_dir()
  call mkdir('Xcopydir1')
  call writefile(['foo'], 'Xcopydir1/Xcopyfile')

  call assert_equal(0, copy('Xcopydir1', 'Xcopydir2'))

  call assert_equal('', glob('Xcopydir1'))
  call assert_equal(['foo'], readfile('Xcopydir2/Xcopyfile'))

  call delete('Xcopydir2/Xcopyfile')
  call delete('Xcopydir2', 'd')
endfunc

func Test_copy_fails()
  call writefile(['foo'], 'Xcopyfile', 'D')

  " Can't copy into a non-existing directory.
  call assert_notequal(0, copy('Xcopyfile', 'Xdoesnotexist/Xcopyfile'))

  " Can't copy a non-existing file.
  call assert_notequal(0, copy('Xdoesnotexist', 'Xcopyfile2'))
  call assert_equal('', glob('Xcopyfile2'))

  " Can't copy to en empty file name.
  call assert_notequal(0, copy('Xcopyfile', ''))

  call assert_fails('call copy("Xcopyfile", [])', 'E730:')
  call assert_fails('call copy(0z, "Xcopyfile")', 'E976:')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
