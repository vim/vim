" Tests for various functions.

source shared.vim
source check.vim
source term_util.vim
source screendump.vim
import './vim9.vim' as v9

" Test for gettext()
func Test_gettext()
  call bindtextdomain("__PACKAGE__", getcwd())
  call assert_fails('call gettext(1)', 'E1174:')
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx"))
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "vim"))
  call assert_equal('xxxTESTxxx', gettext("xxxTESTxxx", "__PACKAGE__"))
  call assert_equal('ERROR: ', gettext("ERROR: ", "__PACKAGE__"))
  language ru_RU
  call assert_equal('ОШИБКА: ', gettext("ERROR: ", "__PACKAGE__", "utf8"))
endfunc

" vim: shiftwidth=2 sts=2 expandtab
