" Test :recover

func Test_recover_root_dir()
  " This used to access invalid memory.
  set dir=/
  call assert_fails('recover', 'E305:')
endfunc

" TODO: move recover tests from tests78.in to here.
