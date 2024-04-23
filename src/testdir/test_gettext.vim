source check.vim

" Test for gettext()
func Test_gettext()
  call assert_fails('call gettext(1)', 'E1174:')
  call assert_fails('call bindtextdomain("vim", "test")', 'E475:')
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx"))
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "vim"))
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "__PACKAGE__"))
  call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))
  if !has('bind_codeset')
    call assert_fails('call gettext("vim", "test")', 'E118:')
  endif
endfunc

" vim: shiftwidth=2 sts=2 expandtab
