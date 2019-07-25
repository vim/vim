" Test get() function.

" get({dict}, {key} [, {default}])
func Test_get_dict()
  let d = {'foo': 42}
  let result = get(d, 'foo', 99)
  call assert_equal(42, result, 'result == 42')
  let result = get(d, 'bar', 999)
  call assert_equal(999, result, 'result == 999')
endfunc

" get({list}, {idx} [, {default}])
func Test_get_list()
  let l = [1,2,3]
  let result = get(l, 0, 999)
  call assert_equal(1, result, 'result == 1')
  let result = get(l, -1, 999)
  call assert_equal(3, result, 'result == 3')
  let result = get(l, 3, 999)
  call assert_equal(999, result, 'result == 999')
endfunc

" get({blob}, {idx} [, {default}])
func Test_get_blob()
  let b = 0zDEADBEEF
  let result = get(b, 0, 999)
  call assert_equal(0xDE, result, 'result == 0xDE')
  let result = get(b, -1, 999)
  call assert_equal(0xEF, result, 'result == 0xEF')
  let result = get(b, 4, 999)
  call assert_equal(999, result, 'result == 999')
endfunc

" get({lambda}, {what} [, {default}])
func Test_get_lambda()
  let l:L = {-> 42}
  let l:Result = get(l:L, 'name')
  call assert_match('^<lambda>', l:Result, "l:Result =~ '^<lambda>'")
  let l:Result = get(l:L, 'func')
  call assert_equal(l:L, l:Result, "l:Result == l:L")
  let l:Result = get(l:L, 'dict', {'lambda has': 'no dict'})
  call assert_equal({'lambda has': 'no dict'}, l:Result,
  \                 "l:Result == {'lambda has': 'no dict'}")
  let l:Result = get(l:L, 'dict')
  call assert_equal(0, l:Result, 'l:Result == 0')
  let l:Result = get(l:L, 'args')
  call assert_equal([], l:Result, "l:Result == []")
endfunc

" get({func}, {what} [, {default}])
func Test_get_func()
  let l:F = function('tr')
  let l:Result = get(l:F, 'name')
  call assert_equal('tr', l:Result, "l:Result == 'tr'")
  let l:Result = get(l:F, 'func')
  call assert_equal(l:F, l:Result, "l:Result == l:F")
  let l:Result = get(l:F, 'dict', {'func has': 'no dict'})
  call assert_equal({'func has': 'no dict'}, l:Result,
  \                 "l:Result == {'func has': 'no dict'}")
  let l:Result = get(l:F, 'dict')
  call assert_equal(0, l:Result, 'l:Result == 0')
  let l:Result = get(l:F, 'args')
  call assert_equal([], l:Result, "l:Result == []")
endfunc

" get({partial}, {what} [, {default}])
func Test_get_partial()
  let l:P = function('substitute', ['hello there', 'there'])
  let l:Result = get(l:P, 'name')
  call assert_equal('substitute', l:Result, "l:Result == 'substitute'")
  let l:Result = get(l:P, 'func')
  call assert_equal(function('substitute'), l:Result, "l:Result == function('substitute')")
  let l:Result = get(l:P, 'dict', {'partial has': 'no dict'})
  call assert_equal({'partial has': 'no dict'}, l:Result,
  \                 "l:Result == {'partial has': 'no dict'}")
  let l:Result = get(l:P, 'dict')
  call assert_equal(0, l:Result, 'l:Result == 0')
  let l:Result = get(l:P, 'args')
  call assert_equal(['hello there', 'there'], l:Result, "l:Result == ['hello there', 'there']")
endfunc
