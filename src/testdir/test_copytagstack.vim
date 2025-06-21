" test 'copytagstack' option

source check.vim
source view_util.vim

func Test_copytagstack()
  call writefile(["int Foo;"], 'file.c', 'D')
  call writefile(["Foo\tfile.c\t1"], 'Xtags', 'D')
  set tags=Xtags

  tag Foo

  let nr0 = winnr()
  call assert_equal(1, gettagstack(nr0)['length'])

  split Xtext

  let nr1 = winnr()
  call assert_equal(1, gettagstack(nr1)['length'])

  set tags&
  bwipe
endfunc

func Test_nocopytagstack()
  call writefile(["int Foo;"], 'file.c', 'D')
  call writefile(["Foo\tfile.c\t1"], 'Xtags', 'D')
  set tags=Xtags
  set nocopytagstack

  tag Foo

  let nr0 = winnr()
  call assert_equal(1, gettagstack(nr0)['length'])

  split Xtext

  let nr1 = winnr()
  call assert_equal(0, gettagstack(nr1)['length'])

  set tags&
  set copytagstack&
  bwipe
endfunc

" vim: shiftwidth=2 sts=2 expandtab
