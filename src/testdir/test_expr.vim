" Tests for expressions.

func Test_equal()
  let base = {}
  func base.method()
    return 1
  endfunc
  func base.other() dict
    return 1
  endfunc
  let instance = copy(base)
  call assert_true(base.method == instance.method)
  call assert_true([base.method] == [instance.method])
  call assert_true(base.other == instance.other)
  call assert_true([base.other] == [instance.other])

  call assert_false(base.method == base.other)
  call assert_false([base.method] == [base.other])
  call assert_false(base.method == instance.other)
  call assert_false([base.method] == [instance.other])

  call assert_fails('echo base.method > instance.method')
endfunc

func Test_version()
  call assert_true(has('patch-7.4.001'))
  call assert_true(has('patch-7.4.01'))
  call assert_true(has('patch-7.4.1'))
  call assert_true(has('patch-6.9.999'))
  call assert_true(has('patch-7.1.999'))
  call assert_true(has('patch-7.4.123'))

  call assert_false(has('patch-7'))
  call assert_false(has('patch-7.4'))
  call assert_false(has('patch-7.4.'))
  call assert_false(has('patch-9.1.0'))
  call assert_false(has('patch-9.9.1'))
endfunc

func Test_dict()
  let d = {'': 'empty', 'a': 'a', 0: 'zero'}
  call assert_equal('empty', d[''])
  call assert_equal('a', d['a'])
  call assert_equal('zero', d[0])
  call assert_true(has_key(d, ''))
  call assert_true(has_key(d, 'a'))

  let d[''] = 'none'
  let d['a'] = 'aaa'
  call assert_equal('none', d[''])
  call assert_equal('aaa', d['a'])
endfunc
