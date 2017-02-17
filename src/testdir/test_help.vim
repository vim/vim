" Tests for :help

func Test_help_restore_snapshot()
  help
  set buftype=
  help
  edit x
  help
  helpclose
endfunc
