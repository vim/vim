" Test 'autochdir' behavior

if !exists("+autochdir")
  finish
endif

func Test_set_filename()
  let cwd = getcwd()
  call test_autochdir()
  set acd

  let s:li = []
  autocmd DirChanged auto call add(s:li, "autocd")
  autocmd DirChanged auto call add(s:li, expand("<afile>"))

  new
  w samples/Xtest
  call assert_equal("Xtest", expand('%'))
  call assert_equal("samples", substitute(getcwd(), '.*/\(\k*\)', '\1', ''))
  call assert_equal(["autocd", getcwd()], s:li)

  bwipe!
  au! DirChanged
  set noacd
  exe 'cd ' . cwd
  call delete('samples/Xtest')
endfunc
