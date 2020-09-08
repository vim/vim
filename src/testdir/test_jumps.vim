" Test for '' mark in an empty buffer

func Test_empty_buffer()
  new
  insert
a
b
c
d
.
  call assert_equal(1, line("''"))
  bwipe!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
