source check.vim
" This fail on CI MacOS 14 because bindtextdomain() is not available there
" (missing library?)
CheckNotMac
CheckFeature gettext

" Test for gettext()
func Test_gettext()
  set encoding=utf-8
  call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))

  try
    call assert_true(bindtextdomain("__PACKAGE__", getcwd()))

    try
      language messages ru_RU
      call assert_equal('ОШИБКА: for __PACKAGE__', gettext("ERROR: ", "__PACKAGE__"))

      call assert_equal('ОШИБКА: for __PACKAGE__', ngettext("ERROR: ", "ERRORS: ", 1, "__PACKAGE__"))
      call assert_equal('ОШИБКА: for __PACKAGE__', ngettext("ERROR: ", "ERRORS: ", 2, "__PACKAGE__"))

      call assert_equal('%d буфер удалён из памяти for __PACKAGE__', ngettext("%d buffer unloaded", "%d buffers unloaded", 1, "__PACKAGE__"))
      call assert_equal('%d буфера удалено из памяти for __PACKAGE__', ngettext("%d buffer unloaded", "%d buffers unloaded", 2, "__PACKAGE__"))
      call assert_equal('%d буферов удалено из памяти for __PACKAGE__', ngettext("%d buffer unloaded", "%d buffers unloaded", 5, "__PACKAGE__"))
    catch /^Vim\%((\a\+)\)\=:E197:/
      throw "Skipped: not possible to set locale to ru (missing?)"
    endtry

    try
      language messages en_GB.UTF-8
      call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))

      call assert_equal('ERROR: ', ngettext("ERROR: ", "ERRORS: ", 1, "__PACKAGE__"))
      call assert_equal('ERRORS: ', ngettext("ERROR: ", "ERRORS: ", 2, "__PACKAGE__"))

      call assert_equal('%d buffer unloaded', ngettext("%d buffer unloaded", "%d buffers unloaded", 1, "__PACKAGE__"))
      call assert_equal('%d buffers unloaded', ngettext("%d buffer unloaded", "%d buffers unloaded", 2, "__PACKAGE__"))
      call assert_equal('%d buffers unloaded', ngettext("%d buffer unloaded", "%d buffers unloaded", 5, "__PACKAGE__"))
    catch /^Vim\%((\a\+)\)\=:E197:/
      throw "Skipped: not possible to set locale to en (missing?)"
    endtry

  catch /^Vim\%((\a\+)\)\=:E342:/
    throw "Skipped: out of memory executing bindtextdomain()"
  endtry
  set encoding&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
