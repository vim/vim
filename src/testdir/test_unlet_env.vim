" Test for unlet $FOOBAR.

func Test_UnletEnv()
  let envcmd = has('win32') ? 'set' : 'env'

  let $FOOBAR='test'
  let found = 0
  for kv in split(system(envcmd), "\r*\n")
    if kv == 'FOOBAR=test'
      let found = 1
    endif
  endfor
  call assert_equal(1, found)

  unlet $FOOBAR
  let found = 0
  for kv in split(system(envcmd), "\r*\n")
    if kv == 'FOOBAR=test'
      let found = 1
    endif
  endfor
  call assert_equal(0, found)

  unlet $MUST_NOT_BE_AN_ERROR
endfunc
