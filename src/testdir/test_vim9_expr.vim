" Tests for Vim9 script expressions

source check.vim

" Check that "line" inside ":def" results in an "error" message.
func CheckDefFailure(line, error)
  call writefile(['def Func()', a:line, 'enddef'], 'Xdef')
  call assert_fails('so Xdef', a:error, a:line)
  call delete('Xdef')
endfunc

func CheckDefFailureList(lines, error)
  call writefile(['def Func()'] + a:lines + ['enddef'], 'Xdef')
  call assert_fails('so Xdef', a:error, string(a:lines))
  call delete('Xdef')
endfunc

" test cond ? expr : expr
def Test_expr1()
  assert_equal('one', v:true ? 'one' : 'two')
  assert_equal('one', 1 ? 'one' : 'two')
  assert_equal('one', 0.1 ? 'one' : 'two')
  assert_equal('one', 'x' ? 'one' : 'two')
"  assert_equal('one', 0z1234 ? 'one' : 'two')
  assert_equal('one', [0] ? 'one' : 'two')
"  assert_equal('one', #{x: 0} ? 'one' : 'two')
  let var = 1
  assert_equal('one', var ? 'one' : 'two')

  assert_equal('two', v:false ? 'one' : 'two')
  assert_equal('two', 0 ? 'one' : 'two')
  assert_equal('two', 0.0 ? 'one' : 'two')
  assert_equal('two', '' ? 'one' : 'two')
"  assert_equal('one', 0z ? 'one' : 'two')
  assert_equal('two', [] ? 'one' : 'two')
"  assert_equal('two', {} ? 'one' : 'two')
  var = 0
  assert_equal('two', var ? 'one' : 'two')
enddef

func Test_expr1_fails()
  call CheckDefFailure("let x = 1 ? 'one'", "Missing ':' after '?'")

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

" test comperator
def Test_expr4()
  assert_equal(v:false, 2 == 0)
  assert_equal(v:true, 61 == 61)

  assert_equal(v:true, 2 != 0)
  assert_equal(v:false, 55 != 55)

  assert_equal(v:true, 2 > 0)
  assert_equal(v:true, 2 > 1)
  assert_equal(v:false, 2 > 2)
  assert_equal(v:false, 2 > 3)

  assert_equal(v:true, 2 >= 0)
  assert_equal(v:true, 2 >= 2)
  assert_equal(v:false, 2 >= 3)

  assert_equal(v:false, 2 < 0)
  assert_equal(v:false, 2 < 2)
  assert_equal(v:true, 2 < 3)

  assert_equal(v:false, 2 <= 0)
  assert_equal(v:false, 2 <= 1)
  assert_equal(v:true, 2 <= 2)
  assert_equal(v:true, 2 <= 3)

  assert_equal(v:false, '2' =~ '0')
  assert_equal(v:true, '2' =~ '[0-9]')

  assert_equal(v:true, '2' !~ '0')
  assert_equal(v:false, '2' !~ '[0-9]')

  assert_equal(v:false, 'abc' == 'ABC')
  set ignorecase
  assert_equal(v:false, 'abc' == 'ABC')
  set noignorecase

  let mylist = [2]
  assert_equal(v:false, mylist is [2])
  let other = mylist
  assert_equal(v:true, mylist is other)

  assert_equal(v:true, '2' isnot '0')
  assert_equal(v:true, mylist isnot [2])
  assert_equal(v:false, mylist isnot other)
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
  call CheckDefFailure("let x = 1 is'2'", msg)
  call CheckDefFailure("let x = '1'is 2", msg)

  let msg = "white space required before and after 'isnot'"
  call CheckDefFailure("let x = '1'isnot'2'", msg)
  call CheckDefFailure("let x = 1 isnot'2'", msg)
  call CheckDefFailure("let x = '1'isnot 2", msg)

  call CheckDefFailure("let x = 1 is# 2", 'E15:')
  call CheckDefFailure("let x = 1 is? 2", 'E15:')
  call CheckDefFailure("let x = 1 isnot# 2", 'E15:')
  call CheckDefFailure("let x = 1 isnot? 2", 'E15:')
endfunc

let afloat = 0.1
let anint = 10
let alsoint = 4

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
enddef

def Test_expr5_float()
  CheckFeature float
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
enddef

def Test_expr6_float()
  CheckFeature float

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

let @a = 'register a'

" test low level expression
def Test_expr7()
  " number constant
  assert_equal(0, 0)
  assert_equal(654, 0654)

  assert_equal(6, 0x6)
  assert_equal(15, 0xf)
  assert_equal(255, 0xff)

  " float constant
  if has('float')
    assert_equal(g:float_zero, .0)
    assert_equal(g:float_zero, 0.0)
    assert_equal(g:float_neg, -9.8)
    assert_equal(g:float_big, 9.9e99)
  endif

  " blob constant
  assert_equal(g:blob_empty, 0z)
  assert_equal(g:blob_one, 0z01)
  assert_equal(g:blob_long, 0z0102.0304)

  " string constant
  assert_equal(g:string_empty, '')
  assert_equal(g:string_empty, "")
  assert_equal(g:string_short, 'x')
  assert_equal(g:string_short, "x")
  assert_equal(g:string_long, 'abcdefghijklm')
  assert_equal(g:string_long, "abcdefghijklm")
  assert_equal(g:string_special, "ab\ncd\ref\ekk")

  " special constant
  assert_equal(g:special_true, v:true)
  assert_equal(g:special_false, v:false)
  assert_equal(g:special_null, v:null)
  assert_equal(g:special_none, v:none)

  " list
  assert_equal(g:list_empty, [])
  assert_equal(g:list_empty, [  ])
  assert_equal(g:list_mixed, [1, 'b', v:false])

  " lambda
  let La = { -> 'result'}
  assert_equal('result', La())
  assert_equal([1, 3, 5], [1, 2, 3]->map({key, val -> key + val}))

  " dictionary
  assert_equal(g:dict_empty, {})
  assert_equal(g:dict_empty, {  })
  assert_equal(g:dict_one, {'one': 1})
  let key = 'one'
  let val = 1
  assert_equal(g:dict_one, {key: val})

  " option
  set ts=11
  assert_equal(11, &ts)
  set ts=8
  set grepprg=some\ text
  assert_equal('some text', &grepprg)
  set grepprg&

  " environment variable
  assert_equal('testvar', $TESTVAR)
  assert_equal('', $ASDF_ASD_XXX)

  " register
  assert_equal('register a', @a)

  " (expr)
  assert_equal(4, (6 * 4) / 6)
  assert_equal(0, 6 * ( 4 / 6 ))

  assert_equal(6, +6)
  assert_equal(-6, -6)
  assert_equal(6, --6)
  assert_equal(6, -+-6)
  assert_equal(-6, ---6)

  assert_equal(v:true, !'')
  assert_equal(v:true, ![])
  assert_equal(v:false, !'asdf')
  assert_equal(v:false, ![2])
  assert_equal(v:true, !!'asdf')
  assert_equal(v:true, !![2])
enddef

func Test_expr7_fails()
  call CheckDefFailure("let x = (12", "E110:")

  call CheckDefFailure("let x = -'xx'", "E1030:")
  call CheckDefFailure("let x = +'xx'", "E1030:")

  call CheckDefFailure("let x = @", "E1002:")
  call CheckDefFailure("let x = @<", "E354:")
endfunc

let g:Funcrefs = [function('add')]

func CallMe(arg)
  return a:arg
endfunc

def Test_expr7_trailing()
  " user function call
  assert_equal(123, CallMe(123))
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
endfunc

func Test_expr_fails()
  call CheckDefFailure("let x = '1'is2", 'E488:')
  call CheckDefFailure("let x = '1'isnot2", 'E488:')
endfunc
