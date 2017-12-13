" Tests for the readdir() function.

func Test_readdir()
  call mkdir('Xdir')
  call writefile([], 'Xdir/foo.txt')
  call mkdir('Xdir/dir')

  let files = sort(readdir('Xdir'))
  call assert_equal(['dir', 'foo.txt'], files)

  let files = sort(readdir('Xdir', {x->stridx(x,'f')==-1}))
  call assert_equal(['foo.txt'], files)

  call delete('dir1', 'rf')
endfunc
