source check.vim

" Test for gettext()
func Test_gettext()
  if has('bind_codeset')
    set encoding=cp1251
    call bindtextdomain("__PACKAGE__", getcwd())
    language ru_RU
    call assert_equal('Œÿ»¡ ¿: ', gettext("ERROR: ", "__PACKAGE__"))
  endif
endfunc

" vim: shiftwidth=2 sts=2 expandtab
