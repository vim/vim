source check.vim

" Test for gettext()
func Test_gettext()
  call bindtextdomain("__PACKAGE__", getcwd())
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx"))
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "vim"))
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "__PACKAGE__"))
  call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))
  language ru_RU
  call assert_equal('Œÿ»¡ ¿: ', gettext("ERROR: ", "__PACKAGE__", "cp1251"))
endfunc

" vim: shiftwidth=2 sts=2 expandtab
