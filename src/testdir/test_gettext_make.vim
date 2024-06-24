source check.vim
"CheckNotMSWindows
CheckNotMac

" Test for package translation Makefile
func Test_gettext_makefile()
  cd ../po
  if has('win32')
    call system('nmake.exe -f Make_mvc.mak "VIMPROG=' .. getenv('VIMPROG') ..
          \ '" "GETTEXT_PATH=' .. getenv('GETTEXT_PATH') ..
          \ '" PLUGPACKAGE=test_gettext
          \ "PO_PLUG_INPUTLIST=..\testdir\test_gettext_makefile_in1.vim
          \ ..\testdir\test_gettext_makefile_in2.vim
          \ ..\testdir\test_gettext_makefile_in3.vim
          \ ..\testdir\test_gettext_makefile_in4.vim" test_gettext.pot')
  else
" Will it work on macOS?
    call system("make -f Makefile PLUGPACKAGE=test_gettext
          \ PO_PLUG_INPUTLIST=\"../testdir/test_gettext_makefile_in1.vim
          \ ../testdir/test_gettext_makefile_in2.vim
          \ ../testdir/test_gettext_makefile_in3.vim
          \ ../testdir/test_gettext_makefile_in4.vim\" test_gettext.pot")
  endif
  let expected = [
          \  '# SOME DESCRIPTIVE TITLE.',
          \  '# Copyright (C) YEAR THE PACKAGE''S COPYRIGHT HOLDER',
          \  '# This file is distributed under the same license as the test_gettext package.',
          \  '# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.',
          \  '#',
          \  '#, fuzzy',
          \  'msgid ""',
          \  'msgstr ""',
          \  '"Project-Id-Version: test_gettext\n"',
          \  '"Report-Msgid-Bugs-To: \n"',
          \  '"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"',
          \  '"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"',
          \  '"Language-Team: LANGUAGE <LL@li.org>\n"',
          \  '"Language: \n"',
          \  '"MIME-Version: 1.0\n"',
          \  '"Content-Type: text/plain; charset=CHARSET\n"',
          \  '"Content-Transfer-Encoding: 8bit\n"',
          \  '',
          \  '#: ../testdir/test_gettext_makefile_in1.vim:4 ../testdir/test_gettext_makefile_in1.vim:6',
          \  '#: ../testdir/test_gettext_makefile_in2.vim:5 ../testdir/test_gettext_makefile_in4.vim:4',
          \  'msgid "This is a test"',
          \  'msgstr ""',
          \  '',
          \  '#: ../testdir/test_gettext_makefile_in1.vim:5',
          \  'msgid "This is another test"',
          \  'msgstr ""',
          \  '',
          \  '#: ../testdir/test_gettext_makefile_in2.vim:4',
          \  'msgid "This is a test from the second file"',
          \  'msgstr ""',
          \  '',
          \  '#: ../testdir/test_gettext_makefile_in4.vim:5',
          \  'msgid "This is a fourth test"',
          \  'msgstr ""']
  let potfile = filter(readfile("test_gettext.pot"), 'v:val !~ "POT-Creation-Date"')
  call assert_equal(expected, potfile)
  call delete('test_gettext.pot')
  cd -
endfunc

" vim: shiftwidth=2 sts=2 expandtab
