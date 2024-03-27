source check.vim

" Test for gettext()
func Test_gettext()
  if has('bind_codeset')
    call bindtextdomain("__PACKAGE__", getcwd())
    language ru_RU
    call assert_equal('Œÿ»¡ ¿: ', gettext("ERROR: ", "__PACKAGE__", "cp1251"))
  endif
endfunc

" vim: shiftwidth=2 sts=2 expandtab
