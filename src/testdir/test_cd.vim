" Test for :cd

func Test_cd_large_path()
  " This used to crash with a heap write overflow.
  call assert_fails('cd ' . repeat('x', 5000), 'E472:')
endfunc

func Test_cd_up_and_down()
  let path = getcwd()
  cd ..
  call assert_notequal(path, getcwd())
  exe 'cd ' . path
  call assert_equal(path, getcwd())
endfunc

func Test_cd_no_arg()
  " Test that cd without argument goes to $HOME directory on Unix systems.
  if has('unix')
    let path = getcwd()
    cd
    call assert_equal($HOME, getcwd())
    call assert_notequal(path, getcwd())
    exe 'cd ' . path
    call assert_equal(path, getcwd())
  endif
endfunc

func Test_cd_minus()
  " Test the  :cd -  goes back to the previous directory.
  let path = getcwd()
  cd ..
  let path_dotdot = getcwd()
  call assert_notequal(path, path_dotdot)
  cd -
  call assert_equal(path, getcwd())
  cd -
  call assert_equal(path_dotdot, getcwd())
  cd -
  call assert_equal(path, getcwd())
endfunc

func Test_cd_with_cpo_chdir()
  e Xfoo
  call setline(1, 'foo')
  let path = getcwd()
  set cpo+=.

  " :cd should fail when buffer is modified and 'cpo' contains dot.
  call assert_fails('cd ..', 'E747:')
  call assert_equal(path, getcwd())

  " :cd with exclamation mark should succeed.
  cd! ..
  call assert_notequal(path, getcwd())

  " :cd should succeed when buffer has been written.
  w!
  exe 'cd ' . path
  call assert_equal(path, getcwd())

  call delete('Xfoo')
  set cpo&
  bw!
endfunc
