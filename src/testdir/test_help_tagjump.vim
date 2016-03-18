" Tests for :help! {subject}

func Test_help_tagjump()
  help
  call assert_equal("help", &filetype)
  call assert_true(getline('.') =~ '\*help.txt\*')
  helpclose

  exec "help! ('textwidth'"
  call assert_equal("help", &filetype)
  call assert_true(getline('.') =~ "\\*'textwidth'\\*")
  helpclose

  exec "help! ('buflisted'),"
  call assert_equal("help", &filetype)
  call assert_true(getline('.') =~ "\\*'buflisted'\\*")
  helpclose
endfunc
