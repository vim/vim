" Tests for Unicode manipulations

source check.vim

" Pixel size of a cell is terminal-dependent, so in the test, only the list and size 2 are checked.
func Test_getcellpixels()
  " Not yet Windows-compatible
  CheckNotMSWindows
  let cellpixels = getcellpixels()
  call assert_equal(2, len(cellpixels))
endfunc

" vim: shiftwidth=2 sts=2 expandtab
