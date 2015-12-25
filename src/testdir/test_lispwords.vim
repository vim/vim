" Tests for 'lispwords' settings being global-local

set nocompatible viminfo+=nviminfo

func Test_global_local_lispwords()
  setglobal lispwords=foo,bar,baz
  setlocal lispwords-=foo | setlocal lispwords+=quux
  call assert_equal('foo,bar,baz', &g:lispwords)
  call assert_equal('bar,baz,quux', &l:lispwords)
  call assert_equal('bar,baz,quux', &lispwords)

  setlocal lispwords<
  call assert_equal('foo,bar,baz', &g:lispwords)
  call assert_equal('foo,bar,baz', &l:lispwords)
  call assert_equal('foo,bar,baz', &lispwords)
endfunc
