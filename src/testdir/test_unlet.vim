" Tests for :unlet

func Test_read_only()
  " these caused a crash
  call assert_fails('unlet count', 'E795:')
  call assert_fails('unlet errmsg', 'E795:')
endfunc

func Test_existing()
  let does_exist = 1
  call assert_true(exists('does_exist'))
  unlet does_exist
  call assert_false(exists('does_exist'))
endfunc

func Test_not_existing()
  unlet! does_not_exist
  call assert_fails('unlet does_not_exist', 'E108:')
endfunc

func Test_unlet_fails()
  call assert_fails('unlet v:["count"]', 'E46:')
endfunc
