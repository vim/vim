source check.vim

" Test for gettext()
func Test_gettext()
  call assert_fails('call gettext(1)', 'E1174:')
  call assert_fails('call bindtextdomain("vim", "test")', 'E475:')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
