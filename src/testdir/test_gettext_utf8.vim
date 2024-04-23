source check.vim

" Test for gettext()
func Test_gettext()
  if has('bind_codeset')
    set encoding=utf-8
    call bindtextdomain("__PACKAGE__", getcwd())
    language ru_RU
    call assert_equal('ОШИБКА: ', gettext("ERROR: ", "__PACKAGE__"))
  endif
endfunc

" vim: shiftwidth=2 sts=2 expandtab
