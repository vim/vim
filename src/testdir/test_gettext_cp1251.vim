source check.vim
" TODO: Why does this fail on MacOS 14 and Windows MSVC (Github CI)?
CheckNotMac
CheckNotMSWindows

" Test for gettext()
func Test_gettext()
  if has("gettext")
    set encoding=cp1251
    call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))

    try
      call assert_true(bindtextdomain("__PACKAGE__", getcwd()))

      try
        language ru_RU
        call assert_equal('Œÿ»¡ ¿: ', gettext("ERROR: ", "__PACKAGE__"))
      catch /^Vim\%((\a\+)\)\=:E197:/
        throw "Skipped: not possible to set locale to ru (missing?)"
      endtry
      try
        language en_GB.UTF-8
        call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))
      catch /^Vim\%((\a\+)\)\=:E197:/
        throw "Skipped: not possible to set locale to en (missing?)"
      endtry
    catch /^Vim\%((\a\+)\)\=:E342:/
      throw "Skipped: out of memory executing bindtextdomain()"
    endtry
    set encoding&
  endif
endfunc

" vim: shiftwidth=2 sts=2 expandtab
