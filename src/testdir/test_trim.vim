" Test for triming strings.

func Test_trim()
  call assert_equal(trim("  \t\r\r\x0BTesting  \t\n\r\n\t\x0B\x0B"), "Testing")
  call assert_equal(trim("  \t  \r\r\n\n\x0BTesting  \t\n\r\n\t\x0B\x0B"), "Testing")
  call assert_equal(trim("xyz \twwRESERVEzyww \t\t", " wxyz\t"), "RESERVE")
  call assert_equal(trim("wRE    \tSERVEzyww"), "wRE    \tSERVEzyww")
  call assert_equal(trim(" \tabcd\t     xxxx   tail"), "abcd\t     xxxx   tail")
  call assert_equal(trim(" \tabcd\t     xxxx   tail", " "), "\tabcd\t     xxxx   tail")
  call assert_equal(trim(" \tabcd\t     xxxx   tail", "abx"), " \tabcd\t     xxxx   tail")
  call assert_equal(trim("你RESERVE好", "你好"), "RESERVE")
  call assert_equal(trim("你好您R E SER V E早好你你", "你好"), "您R E SER V E早")
  call assert_equal(trim(" \n\r\r   你好您R E SER V E早好你你    \t  \x0B", ), "你好您R E SER V E早好你你")
  call assert_equal(trim("    你好您R E SER V E早好你你    \t  \x0B", " 你好"), "您R E SER V E早好你你    \t  \x0B")
  call assert_equal(trim("    tteesstttt你好您R E SER V E早好你你    \t  \x0B ttestt", " 你好tes"), "您R E SER V E早好你你    \t  \x0B")
  call assert_equal(trim("    tteesstttt你好您R E SER V E早好你你    \t  \x0B ttestt", "   你你你好好好tttsses"), "您R E SER V E早好你你    \t  \x0B")
  call assert_equal(trim("这些些不要这些留下这些", "这些不要"), "留下")
endfunc
