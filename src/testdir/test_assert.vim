" Test that the methods used for testing work.

func Test_assert_false()
  call assert_equal(0, assert_false(0))
  call assert_equal(0, assert_false(v:false))

  call assert_equal(1, assert_false(123))
  call assert_match("Expected False but got 123", v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_assert_true()
  call assert_equal(0, assert_true(1))
  call assert_equal(0, assert_true(123))
  call assert_equal(0, assert_true(v:true))

  call assert_equal(1, assert_true(0))
  call assert_match("Expected True but got 0", v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_assert_equal()
  let s = 'foo'
  call assert_equal(0, assert_equal('foo', s))
  let n = 4
  call assert_equal(0, assert_equal(4, n))
  let l = [1, 2, 3]
  call assert_equal(0, assert_equal([1, 2, 3], l))

  let s = 'foo'
  call assert_equal(1, assert_equal('bar', s))
  call assert_match("Expected 'bar' but got 'foo'", v:errors[0])
  call remove(v:errors, 0)

  call assert_equal('XxxxxxxxxxxxxxxxxxxxxxX', 'XyyyyyyyyyyyyyyyyyyyyyyyyyX')
  call assert_match("Expected 'X\\\\\\[x occurs 21 times]X' but got 'X\\\\\\[y occurs 25 times]X'", v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_assert_equalfile()
  call assert_equal(1, assert_equalfile('abcabc', 'xyzxyz'))
  call assert_match("E485: Can't read file abcabc", v:errors[0])
  call remove(v:errors, 0)

  let goodtext = ["one", "two", "three"]
  call writefile(goodtext, 'Xone')
  call assert_equal(1, assert_equalfile('Xone', 'xyzxyz'))
  call assert_match("E485: Can't read file xyzxyz", v:errors[0])
  call remove(v:errors, 0)

  call writefile(goodtext, 'Xtwo')
  call assert_equal(0, assert_equalfile('Xone', 'Xtwo'))

  call writefile([goodtext[0]], 'Xone')
  call assert_equal(1, assert_equalfile('Xone', 'Xtwo'))
  call assert_match("first file is shorter", v:errors[0])
  call remove(v:errors, 0)

  call writefile(goodtext, 'Xone')
  call writefile([goodtext[0]], 'Xtwo')
  call assert_equal(1, assert_equalfile('Xone', 'Xtwo'))
  call assert_match("second file is shorter", v:errors[0])
  call remove(v:errors, 0)

  call writefile(['1234X89'], 'Xone')
  call writefile(['1234Y89'], 'Xtwo')
  call assert_equal(1, assert_equalfile('Xone', 'Xtwo'))
  call assert_match("difference at byte 4", v:errors[0])
  call remove(v:errors, 0)

  call delete('Xone')
  call delete('Xtwo')
endfunc

func Test_assert_notequal()
  let n = 4
  call assert_equal(0, assert_notequal('foo', n))
  let s = 'foo'
  call assert_equal(0, assert_notequal([1, 2, 3], s))

  call assert_equal(1, assert_notequal('foo', s))
  call assert_match("Expected not equal to 'foo'", v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_assert_report()
  call assert_equal(1, assert_report('something is wrong'))
  call assert_match('something is wrong', v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_assert_exception()
  try
    nocommand
  catch
    call assert_equal(0, assert_exception('E492:'))
  endtry

  try
    nocommand
  catch
    try
      " illegal argument, get NULL for error
      call assert_equal(1, assert_exception([]))
    catch
      call assert_equal(0, assert_exception('E730:'))
    endtry
  endtry
endfunc

func Test_wrong_error_type()
  let save_verrors = v:errors
  let v:['errors'] = {'foo': 3}
  call assert_equal('yes', 'no')
  let verrors = v:errors
  let v:errors = save_verrors
  call assert_equal(type([]), type(verrors))
endfunc

func Test_compare_fail()
  let s:v = {}          
  let s:x = {"a": s:v} 
  let s:v["b"] = s:x   
  let s:w = {"c": s:x, "d": ''}
  try
    call assert_equal(s:w, '')
  catch
    call assert_equal(0, assert_exception('E724:'))
    call assert_match("Expected NULL but got ''", v:errors[0])
    call remove(v:errors, 0)
  endtry
endfunc

func Test_match()
  call assert_equal(0, assert_match('^f.*b.*r$', 'foobar'))

  call assert_equal(1, assert_match('bar.*foo', 'foobar'))
  call assert_match("Pattern 'bar.*foo' does not match 'foobar'", v:errors[0])
  call remove(v:errors, 0)

  call assert_equal(1, assert_match('bar.*foo', 'foobar', 'wrong'))
  call assert_match('wrong', v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_notmatch()
  call assert_equal(0, assert_notmatch('foo', 'bar'))
  call assert_equal(0, assert_notmatch('^foobar$', 'foobars'))

  call assert_equal(1, assert_notmatch('foo', 'foobar'))
  call assert_match("Pattern 'foo' does match 'foobar'", v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_assert_fail_fails()
  call assert_equal(1, assert_fails('xxx', {}))
  call assert_match("Expected {} but got 'E731:", v:errors[0])
  call remove(v:errors, 0)

  call assert_equal(1, assert_fails('xxx', {}, 'stupid'))
  call assert_match("stupid: Expected {} but got 'E731:", v:errors[0])
  call remove(v:errors, 0)

  call assert_equal(1, assert_fails('echo', '', 'echo command'))
  call assert_match("command did not fail: echo command", v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_assert_beeps()
  new
  call assert_equal(0, assert_beeps('normal h'))

  call assert_equal(1, assert_beeps('normal 0'))
  call assert_match("command did not beep: normal 0", v:errors[0])
  call remove(v:errors, 0)
  bwipe
endfunc

func Test_assert_inrange()
  call assert_equal(0, assert_inrange(7, 7, 7))
  call assert_equal(0, assert_inrange(5, 7, 5))
  call assert_equal(0, assert_inrange(5, 7, 6))
  call assert_equal(0, assert_inrange(5, 7, 7))

  call assert_equal(1, assert_inrange(5, 7, 4))
  call assert_match("Expected range 5 - 7, but got 4", v:errors[0])
  call remove(v:errors, 0)
  call assert_equal(1, assert_inrange(5, 7, 8))
  call assert_match("Expected range 5 - 7, but got 8", v:errors[0])
  call remove(v:errors, 0)

  call assert_fails('call assert_inrange(1, 1)', 'E119:')

  if has('float')
    call assert_equal(0, assert_inrange(7.0, 7, 7))
    call assert_equal(0, assert_inrange(7, 7.0, 7))
    call assert_equal(0, assert_inrange(7, 7, 7.0))
    call assert_equal(0, assert_inrange(5, 7, 5.0))
    call assert_equal(0, assert_inrange(5, 7, 6.0))
    call assert_equal(0, assert_inrange(5, 7, 7.0))

    call assert_equal(1, assert_inrange(5, 7, 4.0))
    call assert_match("Expected range 5.0 - 7.0, but got 4.0", v:errors[0])
    call remove(v:errors, 0)
    call assert_equal(1, assert_inrange(5, 7, 8.0))
    call assert_match("Expected range 5.0 - 7.0, but got 8.0", v:errors[0])
    call remove(v:errors, 0)
  endif
endfunc

func Test_assert_with_msg()
  call assert_equal('foo', 'bar', 'testing')
  call assert_match("testing: Expected 'foo' but got 'bar'", v:errors[0])
  call remove(v:errors, 0)
endfunc

func Test_override()
  call test_override('char_avail', 1)
  call test_override('redraw', 1)
  call test_override('ALL', 0)
  call assert_fails("call test_override('xxx', 1)", 'E475')
  call assert_fails("call test_override('redraw', 'yes')", 'E474')
endfunc

func Test_user_is_happy()
  smile
  sleep 300m
endfunc

" Must be last.
func Test_zz_quit_detected()
  " Verify that if a test function ends Vim the test script detects this.
  quit
endfunc
