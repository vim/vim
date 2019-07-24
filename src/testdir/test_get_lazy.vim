" Test get_lazy() function.

" get_lazy({dict}, {key} [, {defaultfunc}])
func Test_get_lazy_dict()
  let d = {'foo': 42}
  let result = get_lazy(d, 'foo', {-> 999})
  call assert_equal(42, result, 'result == 42')
  let result = get_lazy(d, 'bar', {-> 999})
  call assert_equal(999, result, 'result == 999')
endfunc

" get_lazy({list}, {idx} [, {defaultfunc}])
func Test_get_lazy_list()
  let l = [1,2,3]
  let result = get_lazy(l, 0, {-> 999})
  call assert_equal(1, result, 'result == 1')
  let result = get_lazy(l, -1, {-> 999})
  call assert_equal(3, result, 'result == 3')
  let result = get_lazy(l, 3, {-> 999})
  call assert_equal(999, result, 'result == 999')
endfunc

" get_lazy({blob}, {idx} [, {defaultfunc}])
func Test_get_lazy_blob()
  let b = 0zDEADBEEF
  let result = get_lazy(b, 0, {-> 999})
  call assert_equal(0xDE, result, 'result == 0xDE')
  let result = get_lazy(b, -1, {-> 999})
  call assert_equal(0xEF, result, 'result == 0xEF')
  let result = get_lazy(b, 4, {-> 999})
  call assert_equal(999, result, 'result == 999')
endfunc

" get_lazy({lambda}, {what})
func Test_get_lazy_lambda()
  let l:L = {-> 42}
  let l:Result = get_lazy(l:L, 'name')
  call assert_match('^<lambda>', l:Result, "l:Result =~ '^<lambda>'")
  let l:Result = get_lazy(l:L, 'func')
  call assert_equal(l:L, l:Result, "l:Result == l:L")
  " FIXME: weird dict value was returned...
  " let l:Result = get_lazy(l:L, 'dict', {-> {'lambda has': 'no dict'}})
  " call assert_equal({}, l:Result, "l:Result == {'lambda has': 'no dict'}")
  let l:Result = get_lazy(l:L, 'args')
  call assert_equal([], l:Result, "l:Result == []")
endfunc

" get_lazy({func}, {what})
func Test_get_lazy_func()
  let l:F = function('tr')
  let l:Result = get_lazy(l:F, 'name')
  call assert_equal('tr', l:Result, "l:Result == 'tr'")
  let l:Result = get_lazy(l:F, 'func')
  call assert_equal(l:F, l:Result, "l:Result == l:F")
  " FIXME: weird dict value was returned...
  " let l:Result = get_lazy(l:F, 'dict', {-> {'func has': 'no dict'}})
  " call assert_equal({}, l:Result, "l:Result == {'func has': 'no dict'}")
  let l:Result = get_lazy(l:F, 'args')
  call assert_equal([], l:Result, "l:Result == []")
endfunc

" get_lazy({partial}, {what})
func Test_get_lazy_partial()
  let l:P = function('substitute', ['hello there', 'there'])
  let l:Result = get_lazy(l:P, 'name')
  call assert_equal('substitute', l:Result, "l:Result == 'substitute'")
  let l:Result = get_lazy(l:P, 'func')
  call assert_equal(function('substitute'), l:Result, "l:Result == function('substitute')")
  " FIXME: weird dict value was returned...
  " let l:Result = get_lazy(l:P, 'dict', {-> {'partial has': 'no dict'}})
  " call assert_equal({}, l:Result, "l:Result == {'partial has': 'no dict'}")
  let l:Result = get_lazy(l:P, 'args')
  call assert_equal(['hello there', 'there'], l:Result, "l:Result == ['hello there', 'there']")
endfunc

func Test_get_lazy_heavy_computation()
  let called_init = 0
  let d = {}
  let key = 'missing'
  func s:init_missing(dict, key) closure
    let called_init = 1
    " assume doing some heavy computations here...
    let val = len(a:key) * 2
    let a:dict[a:key] = val
    return val
  endfunc
  let result = get_lazy(d, key, {-> s:init_missing(d, key)})
  call assert_true(called_init, 's:init_missing() was called')
  call assert_equal({'missing': len('missing') * 2}, d, "d == {'missing': len('missing') * 2}")
endfunc
