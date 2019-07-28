" Test get() function.

" get({dict}, {key} [, {default}])
func Test_get_dict()
  let d = {'foo': 42}
  call assert_equal(42, get(d, 'foo', 99))
  call assert_equal(999, get(d, 'bar', 999))
endfunc

" get({list}, {idx} [, {default}])
func Test_get_list()
  let l = [1,2,3]
  call assert_equal(1, get(l, 0, 999))
  call assert_equal(3, get(l, -1, 999))
  call assert_equal(999, get(l, 3, 999))
endfunc

" get({blob}, {idx} [, {default}])
func Test_get_blob()
  let b = 0zDEADBEEF
  call assert_equal(0xDE, get(b, 0, 999))
  call assert_equal(0xEF, get(b, -1, 999))
  call assert_equal(999, get(b, 4, 999))
endfunc

" get({lambda}, {what} [, {default}])
func Test_get_lambda()
  let l:L = {-> 42}
  call assert_match('^<lambda>', get(l:L, 'name'))
  call assert_equal(l:L, get(l:L, 'func'))
  call assert_equal({'lambda has': 'no dict'}, get(l:L, 'dict', {'lambda has': 'no dict'}))
  call assert_equal(0, get(l:L, 'dict'))
  call assert_equal([], get(l:L, 'args'))
endfunc

" get({func}, {what} [, {default}])
func Test_get_func()
  let l:F = function('tr')
  call assert_equal('tr', get(l:F, 'name'))
  call assert_equal(l:F, get(l:F, 'func'))
  call assert_equal({'func has': 'no dict'}, get(l:F, 'dict', {'func has': 'no dict'}))
  call assert_equal(0, get(l:F, 'dict'))
  call assert_equal([], get(l:F, 'args'))
endfunc

" get({partial}, {what} [, {default}])
func Test_get_partial()
  let l:P = function('substitute', ['hello there', 'there'])
  call assert_equal('substitute', get(l:P, 'name'))
  call assert_equal(function('substitute'), get(l:P, 'func'))")
  call assert_equal({'partial has': 'no dict'}, get(l:P, 'dict', {'partial has': 'no dict'}))
  call assert_equal(0, get(l:P, 'dict'))
  call assert_equal(['hello there', 'there'], get(l:P, 'args'))
endfunc
