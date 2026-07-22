" Test for the HelpCurwin package

func Test_helpcurwin_1()
  packadd helpcurwin
  call assert_equal(2, exists(':HelpCurwin'))
  new Xfoobar.txt
  only
  HelpCurwin tips.txt
  call assert_match('.*tips.txt', bufname('%'))
  call assert_equal(1, winnr('$'))
  call assert_true(bufexists('Xfoobar.txt'))
  %bw
endfunc
