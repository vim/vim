source check.vim
" TODO: Why does this fail on MacOS 14 (Github CI)?
CheckNotMac
"CheckNotMSWindows

" Test for gettext()
func Test_gettext()
  set encoding=cp1251
  call bindtextdomain("__PACKAGE__", getcwd())
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
  set encoding&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
