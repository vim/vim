source check.vim
" This fail on CI MacOS 14 because bindtextdomain() is not available there
" (missing library?)
CheckNotMac
CheckFeature gettext

" Test for gettext()
func Test_gettext()
  set encoding=cp1251
  call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))

  try
    call assert_true(bindtextdomain("__PACKAGE__", getcwd()))

    try
      language messages ru_RU
      call assert_equal('Œÿ»¡ ¿: ', gettext("ERROR: ", "__PACKAGE__"))
    catch /^Vim\%((\a\+)\)\=:E197:/
      throw "Skipped: not possible to set locale to ru (missing?)"
    endtry

    try
      language messages en_GB.UTF-8
      call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))
    catch /^Vim\%((\a\+)\)\=:E197:/
      throw "Skipped: not possible to set locale to en (missing?)"
    endtry

  catch /^Vim\%((\a\+)\)\=:E342:/
    throw "Skipped: out of memory executing bindtextdomain()"
  endtry
  set encoding&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
