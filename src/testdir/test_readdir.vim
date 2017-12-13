" Tests for the readdir() function.

func Test_readdir()
  call mkdir('Xdir')
  call writefile([], 'Xdir/foo.txt')
  call mkdir('Xdir/dir')

  let files = sort(readir('Xdir'))
  call assert_equal(["foo.txt", "dir"], files)

  call delete('dir1', 'rf')
endfunc
