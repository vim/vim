" Tests for setbufline() and getbufline()
func Test_setbufline_getbufline()
  new
  let b = bufnr('%')
  hide
  call setbufline(b, 1, ['foo', 'bar'])
  call assert_equal(['foo'], getbufline(b, 1))
  call assert_equal(['bar'], getbufline(b, 2))
  call assert_equal(['foo', 'bar'], getbufline(b, 1, 2))
  exe "bd!" b
  call assert_equal([], getbufline(b, 1, 2))
endfunc
