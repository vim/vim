" Tests for Vim9 script expressions

source check.vim

" Check that "line" inside ":def" results in an "error" message.
func CheckDefFailure(line, error)
  call writefile(['def! Func()', a:line, 'enddef'], 'Xdef')
  call assert_fails('so Xdef', a:error, a:line)
  call delete('Xdef')
endfunc

func CheckDefFailureMult(lines, error)
  call writefile(['def! Func()'] + a:lines + ['enddef'], 'Xdef')
  call assert_fails('so Xdef', a:error, join(a:lines, ' | '))
  call delete('Xdef')
endfunc

" Check that "line" inside ":def" results in an "error" message when executed.
func CheckDefExecFailure(line, error)
  call writefile(['def! Func()', a:line, 'enddef'], 'Xdef')
  so Xdef
  call assert_fails('call Func()', a:error, a:line)
  call delete('Xdef')
endfunc

func CheckDefFailureList(lines, error)
  call writefile(['def! Func()'] + a:lines + ['enddef'], 'Xdef')
  call assert_fails('so Xdef', a:error, string(a:lines))
  call delete('Xdef')
endfunc

" test cond ? expr : expr
def Test_expr1()
  assert_equal('one', true ? 'one' : 'two')
  assert_equal('one', 1 ? 'one' : 'two')
  if has('float')
    assert_equal('one', 0.1 ? 'one' : 'two')
  endif
  assert_equal('one', 'x' ? 'one' : 'two')
  assert_equal('one', 0z1234 ? 'one' : 'two')
  assert_equal('one', [0] ? 'one' : 'two')
  assert_equal('one', #{x: 0} ? 'one' : 'two')
  let var = 1
  assert_equal('one', var ? 'one' : 'two')

  assert_equal('two', false ? 'one' : 'two')
  assert_equal('two', 0 ? 'one' : 'two')
  if has('float')
    assert_equal('two', 0.0 ? 'one' : 'two')
  endif
  assert_equal('two', '' ? 'one' : 'two')
  assert_equal('two', 0z ? 'one' : 'two')
  assert_equal('two', [] ? 'one' : 'two')
  assert_equal('two', {} ? 'one' : 'two')
  var = 0
  assert_equal('two', var ? 'one' : 'two')
enddef

func Test_expr1_fails()
  call CheckDefFailure("let x = 1 ? 'one'", "Missing ':' after '?'")
  call CheckDefFailure("let x = 1 ? 'one' : xxx", "E1001:")

  let msg = "white space required before and after '?'"
  call CheckDefFailure("let x = 1? 'one' : 'two'", msg)
  call CheckDefFailure("let x = 1 ?'one' : 'two'", msg)
  call CheckDefFailure("let x = 1?'one' : 'two'", msg)

  let msg = "white space required before and after ':'"
  call CheckDefFailure("let x = 1 ? 'one': 'two'", msg)
  call CheckDefFailure("let x = 1 ? 'one' :'two'", msg)
  call CheckDefFailure("let x = 1 ? 'one':'two'", msg)
endfunc

" TODO: define inside test function
def Record(val: any): any
  g:vals->add(val)
  return val
enddef

" test ||
def Test_expr2()
  assert_equal(2, 2 || 0)
  assert_equal(7, 0 || 0 || 7)
  assert_equal(0, 0 || 0)
  assert_equal('', 0 || '')

  g:vals = []
  assert_equal(3, Record(3) || Record(1))
  assert_equal([3], g:vals)

  g:vals = []
  assert_equal(5, Record(0) || Record(5))
  assert_equal([0, 5], g:vals)

  g:vals = []
  assert_equal(4, Record(0) || Record(4) || Record(0))
  assert_equal([0, 4], g:vals)

  g:vals = []
  assert_equal(0, Record([]) || Record('') || Record(0))
  assert_equal([[], '', 0], g:vals)
enddef

func Test_expr2_fails()
  let msg = "white space required before and after '||'"
  call CheckDefFailure("let x = 1||2", msg)
  call CheckDefFailure("let x = 1 ||2", msg)
  call CheckDefFailure("let x = 1|| 2", msg)
endfunc

" test &&
def Test_expr3()
  assert_equal(0, 2 && 0)
  assert_equal(0, 0 && 0 && 7)
  assert_equal(7, 2 && 3 && 7)
  assert_equal(0, 0 && 0)
  assert_equal(0, 0 && '')
  assert_equal('', 8 && '')

  g:vals = []
  assert_equal(1, Record(3) && Record(1))
  assert_equal([3, 1], g:vals)

  g:vals = []
  assert_equal(0, Record(0) && Record(5))
  assert_equal([0], g:vals)

  g:vals = []
  assert_equal(0, Record(0) && Record(4) && Record(0))
  assert_equal([0], g:vals)

  g:vals = []
  assert_equal(0, Record(8) && Record(4) && Record(0))
  assert_equal([8, 4, 0], g:vals)

  g:vals = []
  assert_equal(0, Record([1]) && Record('z') && Record(0))
  assert_equal([[1], 'z', 0], g:vals)
enddef

func Test_expr3_fails()
  let msg = "white space required before and after '&&'"
  call CheckDefFailure("let x = 1&&2", msg)
  call CheckDefFailure("let x = 1 &&2", msg)
  call CheckDefFailure("let x = 1&& 2", msg)
endfunc

let atrue = v:true
let afalse = v:false
let anone = v:none
let anull = v:null
let anint = 10
let alsoint = 4
if has('float')
  let afloat = 0.1
endif
let astring = 'asdf'
let ablob = 0z01ab
let alist = [2, 3, 4]
let adict = #{aaa: 2, bbb: 8}

" test == comperator
def Test_expr4_equal()
  assert_equal(true, true == true)
  assert_equal(false, true == false)
  assert_equal(true, true == g:atrue)
  assert_equal(false, g:atrue == false)

  assert_equal(true, v:none == v:none)
  assert_equal(false, v:none == v:null)
  assert_equal(true, g:anone == v:none)
  assert_equal(false, v:none == g:anull)

  assert_equal(false, 2 == 0)
  assert_equal(true, 61 == 61)
  assert_equal(true, g:anint == 10)
  assert_equal(false, 61 == g:anint)

  if has('float')
    assert_equal(true, 0.3 == 0.3)
    assert_equal(false, 0.4 == 0.3)
    assert_equal(true, 0.1 == g:afloat)
    assert_equal(false, g:afloat == 0.3)

    assert_equal(true, 3.0 == 3)
    assert_equal(true, 3 == 3.0)
    assert_equal(false, 3.1 == 3)
    assert_equal(false, 3 == 3.1)
  endif

  assert_equal(true, 'abc' == 'abc')
  assert_equal(false, 'xyz' == 'abc')
  assert_equal(true, g:astring == 'asdf')
  assert_equal(false, 'xyz' == g:astring)

  assert_equal(false, 'abc' == 'aBc')
  assert_equal(false, 'abc' ==# 'aBc')
  assert_equal(true, 'abc' ==? 'aBc')

  assert_equal(false, 'abc' == 'ABC')
  set ignorecase
  assert_equal(false, 'abc' == 'ABC')
  assert_equal(false, 'abc' ==# 'ABC')
  set noignorecase

  call CheckDefFailure("let x = 'a' == xxx", 'E1001:')

  assert_equal(true, 0z3f == 0z3f)
  assert_equal(false, 0z3f == 0z4f)
  assert_equal(true, g:ablob == 0z01ab)
  assert_equal(false, 0z3f == g:ablob)

  assert_equal(true, [1, 2, 3] == [1, 2, 3])
  assert_equal(false, [1, 2, 3] == [2, 3, 1])
  assert_equal(true, [2, 3, 4] == g:alist)
  assert_equal(false, g:alist == [2, 3, 1])
  assert_equal(false, [1, 2, 3] == [])
  assert_equal(false, [1, 2, 3] == ['1', '2', '3'])

  assert_equal(true, #{one: 1, two: 2} == #{one: 1, two: 2})
  assert_equal(false, #{one: 1, two: 2} == #{one: 2, two: 2})
  assert_equal(false, #{one: 1, two: 2} == #{two: 2})
  assert_equal(false, #{one: 1, two: 2} == #{})
  assert_equal(true, g:adict == #{bbb: 8, aaa: 2})
  assert_equal(false, #{ccc: 9, aaa: 2} == g:adict)

  assert_equal(true, function('Test_expr4_equal') == function('Test_expr4_equal'))
  assert_equal(false, function('Test_expr4_equal') == function('Test_expr4_is'))

  assert_equal(true, function('Test_expr4_equal', [123]) == function('Test_expr4_equal', [123]))
  assert_equal(false, function('Test_expr4_equal', [123]) == function('Test_expr4_is', [123]))
  assert_equal(false, function('Test_expr4_equal', [123]) == function('Test_expr4_equal', [999]))
enddef

" test != comperator
def Test_expr4_notequal()
  assert_equal(false, true != true)
  assert_equal(true, true != false)
  assert_equal(false, true != g:atrue)
  assert_equal(true, g:atrue != false)

  assert_equal(false, v:none != v:none)
  assert_equal(true, v:none != v:null)
  assert_equal(false, g:anone != v:none)
  assert_equal(true, v:none != g:anull)

  assert_equal(true, 2 != 0)
  assert_equal(false, 55 != 55)
  assert_equal(false, g:anint != 10)
  assert_equal(true, 61 != g:anint)

  if has('float')
    assert_equal(false, 0.3 != 0.3)
    assert_equal(true, 0.4 != 0.3)
    assert_equal(false, 0.1 != g:afloat)
    assert_equal(true, g:afloat != 0.3)

    assert_equal(false, 3.0 != 3)
    assert_equal(false, 3 != 3.0)
    assert_equal(true, 3.1 != 3)
    assert_equal(true, 3 != 3.1)
  endif

  assert_equal(false, 'abc' != 'abc')
  assert_equal(true, 'xyz' != 'abc')
  assert_equal(false, g:astring != 'asdf')
  assert_equal(true, 'xyz' != g:astring)

  assert_equal(true, 'abc' != 'ABC')
  set ignorecase
  assert_equal(true, 'abc' != 'ABC')
  set noignorecase

  assert_equal(false, 0z3f != 0z3f)
  assert_equal(true, 0z3f != 0z4f)
  assert_equal(false, g:ablob != 0z01ab)
  assert_equal(true, 0z3f != g:ablob)

  assert_equal(false, [1, 2, 3] != [1, 2, 3])
  assert_equal(true, [1, 2, 3] != [2, 3, 1])
  assert_equal(false, [2, 3, 4] != g:alist)
  assert_equal(true, g:alist != [2, 3, 1])
  assert_equal(true, [1, 2, 3] != [])
  assert_equal(true, [1, 2, 3] != ['1', '2', '3'])

  assert_equal(false, #{one: 1, two: 2} != #{one: 1, two: 2})
  assert_equal(true, #{one: 1, two: 2} != #{one: 2, two: 2})
  assert_equal(true, #{one: 1, two: 2} != #{two: 2})
  assert_equal(true, #{one: 1, two: 2} != #{})
  assert_equal(false, g:adict != #{bbb: 8, aaa: 2})
  assert_equal(true, #{ccc: 9, aaa: 2} != g:adict)

  assert_equal(false, function('Test_expr4_equal') != function('Test_expr4_equal'))
  assert_equal(true, function('Test_expr4_equal') != function('Test_expr4_is'))

  assert_equal(false, function('Test_expr4_equal', [123]) != function('Test_expr4_equal', [123]))
  assert_equal(true, function('Test_expr4_equal', [123]) != function('Test_expr4_is', [123]))
  assert_equal(true, function('Test_expr4_equal', [123]) != function('Test_expr4_equal', [999]))
enddef

" test > comperator
def Test_expr4_greater()
  assert_true(2 > 0)
  assert_true(2 > 1)
  assert_false(2 > 2)
  assert_false(2 > 3)
  if has('float')
    assert_true(2.0 > 0.0)
    assert_true(2.0 > 1.0)
    assert_false(2.0 > 2.0)
    assert_false(2.0 > 3.0)
  endif
enddef

" test >= comperator
def Test_expr4_greaterequal()
  assert_true(2 >= 0)
  assert_true(2 >= 2)
  assert_false(2 >= 3)
  if has('float')
    assert_true(2.0 >= 0.0)
    assert_true(2.0 >= 2.0)
    assert_false(2.0 >= 3.0)
  endif
enddef

" test < comperator
def Test_expr4_smaller()
  assert_false(2 < 0)
  assert_false(2 < 2)
  assert_true(2 < 3)
  if has('float')
    assert_false(2.0 < 0.0)
    assert_false(2.0 < 2.0)
    assert_true(2.0 < 3.0)
  endif
enddef

" test <= comperator
def Test_expr4_smallerequal()
  assert_false(2 <= 0)
  assert_false(2 <= 1)
  assert_true(2 <= 2)
  assert_true(2 <= 3)
  if has('float')
    assert_false(2.0 <= 0.0)
    assert_false(2.0 <= 1.0)
    assert_true(2.0 <= 2.0)
    assert_true(2.0 <= 3.0)
  endif
enddef

" test =~ comperator
def Test_expr4_match()
  assert_equal(false, '2' =~ '0')
  assert_equal(true, '2' =~ '[0-9]')
enddef

" test !~ comperator
def Test_expr4_nomatch()
  assert_equal(true, '2' !~ '0')
  assert_equal(false, '2' !~ '[0-9]')
enddef

" test is comperator
def Test_expr4_is()
  let mylist = [2]
  assert_false(mylist is [2])
  let other = mylist
  assert_true(mylist is other)

  let myblob = 0z1234
  assert_false(myblob is 0z1234)
  let otherblob = myblob
  assert_true(myblob is otherblob)
enddef

" test isnot comperator
def Test_expr4_isnot()
  let mylist = [2]
  assert_true('2' isnot '0')
  assert_true(mylist isnot [2])
  let other = mylist
  assert_false(mylist isnot other)

  let myblob = 0z1234
  assert_true(myblob isnot 0z1234)
  let otherblob = myblob
  assert_false(myblob isnot otherblob)
enddef

def RetVoid()
  let x = 1
enddef

func Test_expr4_fails()
  let msg = "white space required before and after '>'"
  call CheckDefFailure("let x = 1>2", msg)
  call CheckDefFailure("let x = 1 >2", msg)
  call CheckDefFailure("let x = 1> 2", msg)

  let msg = "white space required before and after '=='"
  call CheckDefFailure("let x = 1==2", msg)
  call CheckDefFailure("let x = 1 ==2", msg)
  call CheckDefFailure("let x = 1== 2", msg)

  let msg = "white space required before and after 'is'"
  call CheckDefFailure("let x = '1'is'2'", msg)
  call CheckDefFailure("let x = '1' is'2'", msg)
  call CheckDefFailure("let x = '1'is '2'", msg)

  let msg = "white space required before and after 'isnot'"
  call CheckDefFailure("let x = '1'isnot'2'", msg)
  call CheckDefFailure("let x = '1' isnot'2'", msg)
  call CheckDefFailure("let x = '1'isnot '2'", msg)

  call CheckDefFailure("let x = 1 is# 2", 'E15:')
  call CheckDefFailure("let x = 1 is? 2", 'E15:')
  call CheckDefFailure("let x = 1 isnot# 2", 'E15:')
  call CheckDefFailure("let x = 1 isnot? 2", 'E15:')

  call CheckDefFailure("let x = 1 == '2'", 'Cannot compare number with string')
  call CheckDefFailure("let x = '1' == 2", 'Cannot compare string with number')
  call CheckDefFailure("let x = 1 == RetVoid()", 'Cannot use void value')
  call CheckDefFailure("let x = RetVoid() == 1", 'Cannot compare void with number')

  call CheckDefFailure("let x = true > false", 'Cannot compare bool with bool')
  call CheckDefFailure("let x = true >= false", 'Cannot compare bool with bool')
  call CheckDefFailure("let x = true < false", 'Cannot compare bool with bool')
  call CheckDefFailure("let x = true <= false", 'Cannot compare bool with bool')
  call CheckDefFailure("let x = true =~ false", 'Cannot compare bool with bool')
  call CheckDefFailure("let x = true !~ false", 'Cannot compare bool with bool')
  call CheckDefFailure("let x = true is false", 'Cannot use "is" with bool')
  call CheckDefFailure("let x = true isnot false", 'Cannot use "isnot" with bool')

  call CheckDefFailure("let x = v:none is v:null", 'Cannot use "is" with special')
  call CheckDefFailure("let x = v:none isnot v:null", 'Cannot use "isnot" with special')
  call CheckDefFailure("let x = 123 is 123", 'Cannot use "is" with number')
  call CheckDefFailure("let x = 123 isnot 123", 'Cannot use "isnot" with number')
  if has('float')
    call CheckDefFailure("let x = 1.3 is 1.3", 'Cannot use "is" with float')
    call CheckDefFailure("let x = 1.3 isnot 1.3", 'Cannot use "isnot" with float')
  endif

  call CheckDefFailure("let x = 0za1 > 0z34", 'Cannot compare blob with blob')
  call CheckDefFailure("let x = 0za1 >= 0z34", 'Cannot compare blob with blob')
  call CheckDefFailure("let x = 0za1 < 0z34", 'Cannot compare blob with blob')
  call CheckDefFailure("let x = 0za1 <= 0z34", 'Cannot compare blob with blob')
  call CheckDefFailure("let x = 0za1 =~ 0z34", 'Cannot compare blob with blob')
  call CheckDefFailure("let x = 0za1 !~ 0z34", 'Cannot compare blob with blob')

  call CheckDefFailure("let x = [13] > [88]", 'Cannot compare list with list')
  call CheckDefFailure("let x = [13] >= [88]", 'Cannot compare list with list')
  call CheckDefFailure("let x = [13] < [88]", 'Cannot compare list with list')
  call CheckDefFailure("let x = [13] <= [88]", 'Cannot compare list with list')
  call CheckDefFailure("let x = [13] =~ [88]", 'Cannot compare list with list')
  call CheckDefFailure("let x = [13] !~ [88]", 'Cannot compare list with list')

  call CheckDefFailureMult(['let j: job', 'let chan: channel', 'let r = j == chan'], 'Cannot compare job with channel')
  call CheckDefFailureMult(['let j: job', 'let x: list<any>', 'let r = j == x'], 'Cannot compare job with list')
  call CheckDefFailureMult(['let j: job', 'let x: func', 'let r = j == x'], 'Cannot compare job with func')
  call CheckDefFailureMult(['let j: job', 'let x: partial', 'let r = j == x'], 'Cannot compare job with partial')
endfunc

" test addition, subtraction, concatenation
def Test_expr5()
  assert_equal(66, 60 + 6)
  assert_equal(70, 60 + g:anint)
  assert_equal(9, g:alsoint + 5)
  assert_equal(14, g:alsoint + g:anint)

  assert_equal(54, 60 - 6)
  assert_equal(50, 60 - g:anint)
  assert_equal(-1, g:alsoint - 5)
  assert_equal(-6, g:alsoint - g:anint)

  assert_equal('hello', 'hel' .. 'lo')
  assert_equal('hello 123', 'hello ' .. 123)
  assert_equal('123 hello', 123 .. ' hello')
  assert_equal('123456', 123 .. 456)

  assert_equal([1, 2, 3, 4], [1, 2] + [3, 4])
  assert_equal(0z11223344, 0z1122 + 0z3344)
  assert_equal(0z112201ab, 0z1122 + g:ablob)
  assert_equal(0z01ab3344, g:ablob + 0z3344)
  assert_equal(0z01ab01ab, g:ablob + g:ablob)
enddef

def Test_expr5_float()
  if !has('float')
    MissingFeature 'float'
  else
    assert_equal(66.0, 60.0 + 6.0)
    assert_equal(66.0, 60.0 + 6)
    assert_equal(66.0, 60 + 6.0)
    assert_equal(5.1, g:afloat + 5)
    assert_equal(8.1, 8 + g:afloat)
    assert_equal(10.1, g:anint + g:afloat)
    assert_equal(10.1, g:afloat + g:anint)

    assert_equal(54.0, 60.0 - 6.0)
    assert_equal(54.0, 60.0 - 6)
    assert_equal(54.0, 60 - 6.0)
    assert_equal(-4.9, g:afloat - 5)
    assert_equal(7.9, 8 - g:afloat)
    assert_equal(9.9, g:anint - g:afloat)
    assert_equal(-9.9, g:afloat - g:anint)
  endif
enddef

func Test_expr5_fails()
  let msg = "white space required before and after '+'"
  call CheckDefFailure("let x = 1+2", msg)
  call CheckDefFailure("let x = 1 +2", msg)
  call CheckDefFailure("let x = 1+ 2", msg)

  let msg = "white space required before and after '-'"
  call CheckDefFailure("let x = 1-2", msg)
  call CheckDefFailure("let x = 1 -2", msg)
  call CheckDefFailure("let x = 1- 2", msg)

  let msg = "white space required before and after '..'"
  call CheckDefFailure("let x = '1'..'2'", msg)
  call CheckDefFailure("let x = '1' ..'2'", msg)
  call CheckDefFailure("let x = '1'.. '2'", msg)

  call CheckDefFailure("let x = 0z1122 + 33", 'E1035')
  call CheckDefFailure("let x = 0z1122 + [3]", 'E1035')
  call CheckDefFailure("let x = 0z1122 + 'asd'", 'E1035')
  call CheckDefFailure("let x = 33 + 0z1122", 'E1035')
  call CheckDefFailure("let x = [3] + 0z1122", 'E1035')
  call CheckDefFailure("let x = 'asdf' + 0z1122", 'E1035')
  call CheckDefFailure("let x = 6 + xxx", 'E1001')
endfunc

" test multiply, divide, modulo
def Test_expr6()
  assert_equal(36, 6 * 6)
  assert_equal(24, 6 * g:alsoint)
  assert_equal(24, g:alsoint * 6)
  assert_equal(40, g:anint * g:alsoint)

  assert_equal(10, 60 / 6)
  assert_equal(6, 60 / g:anint)
  assert_equal(1, g:anint / 6)
  assert_equal(2, g:anint / g:alsoint)

  assert_equal(5, 11 % 6)
  assert_equal(4, g:anint % 6)
  assert_equal(3, 13 % g:anint)
  assert_equal(2, g:anint % g:alsoint)

  assert_equal(4, 6 * 4 / 6)

  let x = [2]
  let y = [3]
  assert_equal(5, x[0] + y[0])
  assert_equal(6, x[0] * y[0])
  if has('float')
    let xf = [2.0]
    let yf = [3.0]
    assert_equal(5.0, xf[0] + yf[0])
    assert_equal(6.0, xf[0] * yf[0])
  endif

  call CheckDefFailure("let x = 6 * xxx", 'E1001')
enddef

def Test_expr6_float()
  if !has('float')
    MissingFeature 'float'
  else
    assert_equal(36.0, 6.0 * 6)
    assert_equal(36.0, 6 * 6.0)
    assert_equal(36.0, 6.0 * 6.0)
    assert_equal(1.0, g:afloat * g:anint)

    assert_equal(10.0, 60 / 6.0)
    assert_equal(10.0, 60.0 / 6)
    assert_equal(10.0, 60.0 / 6.0)
    assert_equal(0.01, g:afloat / g:anint)

    assert_equal(4.0, 6.0 * 4 / 6)
    assert_equal(4.0, 6 * 4.0 / 6)
    assert_equal(4.0, 6 * 4 / 6.0)
    assert_equal(4.0, 6.0 * 4.0 / 6)
    assert_equal(4.0, 6 * 4.0 / 6.0)
    assert_equal(4.0, 6.0 * 4 / 6.0)
    assert_equal(4.0, 6.0 * 4.0 / 6.0)

    assert_equal(4.0, 6.0 * 4.0 / 6.0)
  endif
enddef

func Test_expr6_fails()
  let msg = "white space required before and after '*'"
  call CheckDefFailure("let x = 1*2", msg)
  call CheckDefFailure("let x = 1 *2", msg)
  call CheckDefFailure("let x = 1* 2", msg)

  let msg = "white space required before and after '/'"
  call CheckDefFailure("let x = 1/2", msg)
  call CheckDefFailure("let x = 1 /2", msg)
  call CheckDefFailure("let x = 1/ 2", msg)

  let msg = "white space required before and after '%'"
  call CheckDefFailure("let x = 1%2", msg)
  call CheckDefFailure("let x = 1 %2", msg)
  call CheckDefFailure("let x = 1% 2", msg)

  call CheckDefFailure("let x = '1' * '2'", 'E1036:')
  call CheckDefFailure("let x = '1' / '2'", 'E1036:')
  call CheckDefFailure("let x = '1' % '2'", 'E1035:')

  call CheckDefFailure("let x = 0z01 * 0z12", 'E1036:')
  call CheckDefFailure("let x = 0z01 / 0z12", 'E1036:')
  call CheckDefFailure("let x = 0z01 % 0z12", 'E1035:')

  call CheckDefFailure("let x = [1] * [2]", 'E1036:')
  call CheckDefFailure("let x = [1] / [2]", 'E1036:')
  call CheckDefFailure("let x = [1] % [2]", 'E1035:')

  call CheckDefFailure("let x = #{one: 1} * #{two: 2}", 'E1036:')
  call CheckDefFailure("let x = #{one: 1} / #{two: 2}", 'E1036:')
  call CheckDefFailure("let x = #{one: 1} % #{two: 2}", 'E1035:')

  call CheckDefFailure("let x = 0xff[1]", 'E714:')
  if has('float')
    call CheckDefFailure("let x = 0.7[1]", 'E714:')
  endif
endfunc

func Test_expr6_float_fails()
  CheckFeature float
  call CheckDefFailure("let x = 1.0 % 2", 'E1035:')
endfunc

" define here to use old style parsing
if has('float')
  let g:float_zero = 0.0
  let g:float_neg = -9.8
  let g:float_big = 9.9e99
endif
let g:blob_empty = 0z
let g:blob_one = 0z01
let g:blob_long = 0z0102.0304

let g:string_empty = ''
let g:string_short = 'x'
let g:string_long = 'abcdefghijklm'
let g:string_special = "ab\ncd\ref\ekk"

let g:special_true = v:true
let g:special_false = v:false
let g:special_null = v:null
let g:special_none = v:none

let g:list_empty = []
let g:list_mixed = [1, 'b', v:false]

let g:dict_empty = {}
let g:dict_one = #{one: 1}

let $TESTVAR = 'testvar'

" test low level expression
def Test_expr7_number()
  " number constant
  assert_equal(0, 0)
  assert_equal(654, 0654)

  assert_equal(6, 0x6)
  assert_equal(15, 0xf)
  assert_equal(255, 0xff)
enddef

def Test_expr7_float()
  " float constant
  if !has('float')
    MissingFeature 'float'
  else
    assert_equal(g:float_zero, .0)
    assert_equal(g:float_zero, 0.0)
    assert_equal(g:float_neg, -9.8)
    assert_equal(g:float_big, 9.9e99)
  endif
enddef

def Test_expr7_blob()
  " blob constant
  assert_equal(g:blob_empty, 0z)
  assert_equal(g:blob_one, 0z01)
  assert_equal(g:blob_long, 0z0102.0304)

  call CheckDefFailure("let x = 0z123", 'E973:')
enddef

def Test_expr7_string()
  " string constant
  assert_equal(g:string_empty, '')
  assert_equal(g:string_empty, "")
  assert_equal(g:string_short, 'x')
  assert_equal(g:string_short, "x")
  assert_equal(g:string_long, 'abcdefghijklm')
  assert_equal(g:string_long, "abcdefghijklm")
  assert_equal(g:string_special, "ab\ncd\ref\ekk")

  call CheckDefFailure('let x = "abc', 'E114:')
  call CheckDefFailure("let x = 'abc", 'E115:')
enddef

def Test_expr7_special()
  " special constant
  assert_equal(g:special_true, true)
  assert_equal(g:special_false, false)
  assert_equal(g:special_null, v:null)
  assert_equal(g:special_none, v:none)
enddef

def Test_expr7_list()
  " list
  assert_equal(g:list_empty, [])
  assert_equal(g:list_empty, [  ])
  assert_equal(g:list_mixed, [1, 'b', false])
  assert_equal('b', g:list_mixed[1])

  call CheckDefExecFailure("let x = g:anint[3]", 'E714:')
  call CheckDefFailure("let x = g:list_mixed[xxx]", 'E1001:')
  call CheckDefExecFailure("let x = g:list_mixed['xx']", 'E39:')
  call CheckDefFailure("let x = g:list_mixed[0", 'E111:')
  call CheckDefExecFailure("let x = g:list_empty[3]", 'E684:')
enddef

def Test_expr7_lambda()
  " lambda
  let La = { -> 'result'}
  assert_equal('result', La())
  assert_equal([1, 3, 5], [1, 2, 3]->map({key, val -> key + val}))
enddef

def Test_expr7_dict()
  " dictionary
  assert_equal(g:dict_empty, {})
  assert_equal(g:dict_empty, {  })
  assert_equal(g:dict_one, {'one': 1})
  let key = 'one'
  let val = 1
  assert_equal(g:dict_one, {key: val})

  call CheckDefFailure("let x = #{8: 8}", 'E1014:')
  call CheckDefFailure("let x = #{xxx}", 'E720:')
  call CheckDefFailure("let x = #{xxx: 1", 'E722:')
  call CheckDefFailure("let x = #{xxx: 1,", 'E723:')
  call CheckDefFailure("let x = {'a': xxx}", 'E1001:')
  call CheckDefFailure("let x = {xxx: 8}", 'E1001:')
  call CheckDefFailure("let x = #{a: 1, a: 2}", 'E721:')
  call CheckDefFailure("let x = #", 'E1015:')
  call CheckDefFailure("let x += 1", 'E1020:')
  call CheckDefFailure("let x = x + 1", 'E1001:')
  call CheckDefExecFailure("let x = g:anint.member", 'E715:')
  call CheckDefExecFailure("let x = g:dict_empty.member", 'E716:')
enddef

def Test_expr_member()
  assert_equal(1, g:dict_one.one)

  call CheckDefFailure("let x = g:dict_one.#$!", 'E1002:')
enddef

def Test_expr7_option()
  " option
  set ts=11
  assert_equal(11, &ts)
  &ts = 9
  assert_equal(9, &ts)
  set ts=8
  set grepprg=some\ text
  assert_equal('some text', &grepprg)
  &grepprg = test_null_string()
  assert_equal('', &grepprg)
  set grepprg&
enddef

def Test_expr7_environment()
  " environment variable
  assert_equal('testvar', $TESTVAR)
  assert_equal('', $ASDF_ASD_XXX)

  call CheckDefFailure("let x = $$$", 'E1002:')
enddef

def Test_expr7_register()
  @a = 'register a'
  assert_equal('register a', @a)
enddef

def Test_expr7_parens()
  " (expr)
  assert_equal(4, (6 * 4) / 6)
  assert_equal(0, 6 * ( 4 / 6 ))

  assert_equal(6, +6)
  assert_equal(-6, -6)
  assert_equal(6, --6)
  assert_equal(6, -+-6)
  assert_equal(-6, ---6)
enddef

def Test_expr7_negate()
  assert_equal(-99, -99)
  assert_equal(99, --99)
  let nr = 88
  assert_equal(-88, -nr)
  assert_equal(88, --nr)
enddef

def Echo(arg): string
  return arg
enddef

def s:EchoArg(arg): string
  return arg
enddef

def Test_expr7_call()
  assert_equal('yes', 'yes'->Echo())
  assert_equal('yes', 'yes'->s:EchoArg())

  call CheckDefFailure("let x = 'yes'->Echo", 'E107:')
enddef


def Test_expr7_not()
  assert_equal(true, !'')
  assert_equal(true, ![])
  assert_equal(false, !'asdf')
  assert_equal(false, ![2])
  assert_equal(true, !!'asdf')
  assert_equal(true, !![2])

  assert_equal(true, !test_null_partial())
  assert_equal(false, !{-> 'yes'})

  assert_equal(true, !test_null_dict())
  assert_equal(true, !{})
  assert_equal(false, !{'yes': 'no'})

  if has('channel')
    assert_equal(true, !test_null_job())
    assert_equal(true, !test_null_channel())
  endif

  assert_equal(true, !test_null_blob())
  assert_equal(true, !0z)
  assert_equal(false, !0z01)

  assert_equal(true, !test_void())
  assert_equal(true, !test_unknown())
enddef

func Test_expr7_fails()
  call CheckDefFailure("let x = (12", "E110:")

  call CheckDefFailure("let x = -'xx'", "E1030:")
  call CheckDefFailure("let x = +'xx'", "E1030:")
  call CheckDefFailure("let x = -0z12", "E974:")
  call CheckDefExecFailure("let x = -[8]", "E39:")
  call CheckDefExecFailure("let x = -{'a': 1}", "E39:")

  call CheckDefFailure("let x = @", "E1002:")
  call CheckDefFailure("let x = @<", "E354:")

  call CheckDefFailure("let x = [1, 2", "E697:")
  call CheckDefFailure("let x = [notfound]", "E1001:")

  call CheckDefFailure("let x = { -> 123) }", "E451:")
  call CheckDefFailure("let x = 123->{x -> x + 5) }", "E451:")

  call CheckDefFailure("let x = &notexist", 'E113:')
  call CheckDefExecFailure("&grepprg = [343]", 'E1051:')

  call CheckDefExecFailure("echo s:doesnt_exist", 'E121:')
  call CheckDefExecFailure("echo g:doesnt_exist", 'E121:')

  call CheckDefFailure("echo a:somevar", 'E1075:')
  call CheckDefFailure("echo l:somevar", 'E1075:')
  call CheckDefFailure("echo x:somevar", 'E1075:')

  " TODO
  call CheckDefFailure("echo b:somevar", 'not supported yet')
  call CheckDefFailure("echo w:somevar", 'not supported yet')
  call CheckDefFailure("echo t:somevar", 'not supported yet')

  call CheckDefExecFailure("let x = +g:astring", 'E1030:')
  call CheckDefExecFailure("let x = +g:ablob", 'E974:')
  call CheckDefExecFailure("let x = +g:alist", 'E745:')
  call CheckDefExecFailure("let x = +g:adict", 'E728:')

  call CheckDefFailureMult(["let x = ''", "let y = x.memb"], 'E715:')

  call CheckDefExecFailure("[1, 2->len()", 'E492:')
  call CheckDefExecFailure("#{a: 1->len()", 'E488:')
  call CheckDefExecFailure("{'a': 1->len()", 'E492:')
endfunc

let g:Funcrefs = [function('add')]

func CallMe(arg)
  return a:arg
endfunc

func CallMe2(one, two)
  return a:one .. a:two
endfunc

def Test_expr7_trailing()
  " user function call
  assert_equal(123, CallMe(123))
  assert_equal(123, CallMe(  123))
  assert_equal(123, CallMe(123  ))
  assert_equal('yesno', CallMe2('yes', 'no'))
  assert_equal('yesno', CallMe2( 'yes', 'no' ))
  assert_equal('nothing', CallMe('nothing'))

  " partial call
  let Part = function('CallMe')
  assert_equal('yes', Part('yes'))

  " funcref call, using list index
  let l = []
  g:Funcrefs[0](l, 2)
  assert_equal([2], l)

  " method call
  l = [2, 5, 6]
  l->map({k, v -> k + v})
  assert_equal([2, 6, 8], l)

  " lambda method call
  l = [2, 5]
  l->{l -> add(l, 8)}()
  assert_equal([2, 5, 8], l)

  " dict member
  let d = #{key: 123}
  assert_equal(123, d.key)
enddef

func Test_expr7_trailing_fails()
  call CheckDefFailureList(['let l = [2]', 'l->{l -> add(l, 8)}'], 'E107')
  call CheckDefFailureList(['let l = [2]', 'l->{l -> add(l, 8)} ()'], 'E274')
endfunc

func Test_expr_fails()
  call CheckDefFailure("let x = '1'is2", 'E488:')
  call CheckDefFailure("let x = '1'isnot2", 'E488:')

  call CheckDefExecFailure("CallMe ('yes')", 'E492:')
  call CheckDefFailure("CallMe2('yes','no')", 'E1069:')
  call CheckDefFailure("CallMe2('yes' , 'no')", 'E1068:')

  call CheckDefFailure("v:nosuch += 3", 'E1001:')
  call CheckDefFailure("let v:version = 3", 'E1064:')
  call CheckDefFailure("let asdf = v:nosuch", 'E1001:')

  call CheckDefFailure("echo len('asdf'", 'E110:')
  call CheckDefFailure("echo Func0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789()", 'E1011:')
  call CheckDefFailure("echo doesnotexist()", 'E117:')
endfunc
