source check.vim

CheckFeature gettext

" Test for gettext()
func Test_gettext()
  call assert_fails('call bindtextdomain("test")', 'E119:')
  call assert_fails('call bindtextdomain("vim", "test")', 'E475:')

  call assert_fails('call gettext(1)', 'E1174:')
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx"))

  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "vim"))
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "__PACKAGE__"))
  call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))
endfunc

" vim: shiftwidth=2 sts=2 expandtab
