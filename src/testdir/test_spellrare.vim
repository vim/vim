" Test spell checking
" Note: this file uses latin1 encoding, but is used with utf-8 encoding.

source check.vim
CheckFeature spell

source screendump.vim

func TearDown()
  set nospell
  call delete('Xtest.aff')
  call delete('Xtest.dic')
  call delete('Xtest.latin1.add')
  call delete('Xtest.latin1.add.spl')
  call delete('Xtest.latin1.spl')
  call delete('Xtest.latin1.sug')
  " set 'encoding' to clear the word list
  set encoding=utf-8
endfunc

" Test spellbadword() with argument
func Test_spellrareword()
  set spell

  " Create a small word list to test that spellbadword('...')
  " can return ['...', 'rare'].
  e Xwords
  insert
foo
foobar/?
foobara/?
.
   w!
   mkspell! Xwords.spl Xwords
   set spelllang=Xwords.spl
   call assert_equal(['foobar', 'rare'], spellbadword('foo foobar'))

  new
  call setline(1, ['foo', '', 'foo bar foo bar foobara foo foo foo foobar', '', 'End'])
  set spell wrapscan
  normal ]s
  call assert_equal('foo', expand('<cword>'))
  normal ]s
  call assert_equal('bar', expand('<cword>'))

  normal ]r
  call assert_equal('foobara', expand('<cword>'))
  normal ]r
  call assert_equal('foobar', expand('<cword>'))
  normal ]r
  call assert_equal('foobara', expand('<cword>'))
  normal 2]r
  call assert_equal('foobara', expand('<cword>'))
 
  normal [r
  call assert_equal('foobar', expand('<cword>'))
  normal [r
  call assert_equal('foobara', expand('<cword>'))
  normal [r
  call assert_equal('foobar', expand('<cword>'))
  normal 2[r
  call assert_equal('foobar', expand('<cword>'))

  bwipe!
  set nospell

  call delete('Xwords.spl')
  call delete('Xwords')
  set spelllang&
  set spell&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
