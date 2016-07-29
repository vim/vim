" Check that loading startup.vim works.

func Test_startup_script()
  set compatible
  source $VIMRUNTIME/defaults.vim

  call assert_equal(0, &compatible)
endfunc
