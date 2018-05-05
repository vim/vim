function Test_GetEnv()
  unlet! $TESTENV
  call assert_equal(v:null, getenv('TESTENV'))
  let $TESTENV = "foo"
  call assert_equal('foo', getenv('TESTENV'))

  unlet! $TESTENV
  call assert_equal(0, has_key(getenv(), 'TESTENV'))
  let $TESTENV = "foo"
  call assert_equal(1, has_key(getenv(), 'TESTENV'))
endfunc
