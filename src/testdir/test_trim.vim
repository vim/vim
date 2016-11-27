" Test for triming strings.

func Test_trim()
  call assert_equal(trim("  \t\r\r\x0BTesting  \t\n\r\n\t\x0B\x0B"), "Testing")
  call assert_equal(trim("  \t  \r\r\n\n\x0BTesting  \t\n\r\n\t\x0B\x0B"), "Testing")
  call assert_equal(trim("xyz \twwRESERVEzyww \t\t", " wxyz\t"), "RESERVE")
  call assert_equal(trim("wRE    \tSERVEzyww"), "wRE    \tSERVEzyww")
  call assert_equal(trim(" \tabcd\t     xxxx   tail"), "abcd\t     xxxx   tail")
endfunc
