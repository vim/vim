" Tests for the readdir() function.

func Test_readdir()
  call mkdir('Xdir')
  call writefile([], 'Xdir/foo.txt')
  call writefile([], 'Xdir/bar.txt')
  call mkdir('Xdir/dir')

  let files = readdir('Xdir')
  call assert_equal(['bar.txt', 'dir', 'foo.txt'], files)

  let files = readdir('Xdir', {x->stridx(x,'f')!=-1})
  call assert_equal(['foo.txt'], files)

  let l = []
  let files = readdir('Xdir', {x->len(add(l, x)) == 2 ? -1 : 1})
  call assert_equal(1, len(files))

  call delete('Xdir', 'rf')
endfunc
