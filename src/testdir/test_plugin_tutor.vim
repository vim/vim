func SetUp()
  set nocompatible
  runtime plugin/tutor.vim
endfunc

func Test_auto_enable_interactive()
  Tutor
  call assert_equal('nofile', &buftype)
  call assert_match('tutor#EnableInteractive', b:undo_ftplugin)

  edit Xtutor/Xtest.tutor
  call assert_equal('', &buftype)
  call assert_match('tutor#EnableInteractive', b:undo_ftplugin)
endfunc
