" Tests for Vim9 script expressions

source check.vim
source vim9.vim


let g:cond = v:false
def FuncOne(arg: number): string
  return 'yes'
enddef
def FuncTwo(arg: number): number
  return 123
enddef

" test cond ? expr : expr
def Test_expr1()
  assert_equal('one', true ? 'one' : 'two')
  assert_equal('one', 1 ?
			'one' :
			'two')
  if has('float')
    assert_equal('one', 0.1 ? 'one' : 'two')
  endif
  assert_equal('one', 'x' ? 'one' : 'two')
  assert_equal('one', 'x'
  			? 'one'
			: 'two')
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

  let Some: func = function('len')
  let Other: func = function('winnr')
  let Res: func = g:atrue ? Some : Other
  assert_equal(function('len'), Res)

  let RetOne: func(string): number = function('len')
  let RetTwo: func(string): number = function('winnr')
  let RetThat: func = g:atrue ? RetOne : RetTwo
  assert_equal(function('len'), RetThat)

  let x = FuncOne
  let y = FuncTwo
  let Z = g:cond ? FuncOne : FuncTwo
  assert_equal(123, Z(3))
enddef

def Test_expr1_vimscript()
  # only checks line continuation
  let lines =<< trim END
      vim9script
      let var = 1
      		? 'yes'
		: 'no'
      assert_equal('yes', var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = v:false
      		? 'yes'
		: 'no'
      assert_equal('no', var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = v:false ?
      		'yes' :
		'no'
      assert_equal('no', var)
  END
  CheckScriptSuccess(lines)
enddef

func Test_expr1_fails()
  call CheckDefFailure(["let x = 1 ? 'one'"], "Missing ':' after '?'")
  call CheckDefFailure(["let x = 1 ? 'one' : xxx"], "E1001:")

  let msg = "white space required before and after '?'"
  call CheckDefFailure(["let x = 1? 'one' : 'two'"], msg)
  call CheckDefFailure(["let x = 1 ?'one' : 'two'"], msg)
  call CheckDefFailure(["let x = 1?'one' : 'two'"], msg)

  let msg = "white space required before and after ':'"
  call CheckDefFailure(["let x = 1 ? 'one': 'two'"], msg)
  call CheckDefFailure(["let x = 1 ? 'one' :'two'"], msg)
  call CheckDefFailure(["let x = 1 ? 'one':'two'"], msg)

  " missing argument detected even when common type is used
  call CheckDefFailure([
	\ 'let x = FuncOne',
	\ 'let y = FuncTwo',
	\ 'let Z = g:cond ? FuncOne : FuncTwo',
	\ 'Z()'], 'E119:')
endfunc

" TODO: define inside test function
def Record(val: any): any
  g:vals->add(val)
  return val
enddef

" test ||
def Test_expr2()
  assert_equal(2, 2 || 0)
  assert_equal(7, 0 ||
		    0 ||
		    7)
  assert_equal(0, 0 || 0)
  assert_equal(0, 0
  		    || 0)
  assert_equal('', 0 || '')

  g:vals = []
  assert_equal(3, Record(3) || Record(1))
  assert_equal([3], g:vals)

  g:vals = []
  assert_equal(5, Record(0) || Record(5))
  assert_equal([0, 5], g:vals)

  g:vals = []
  assert_equal(4, Record(0)
		      || Record(4)
		      || Record(0))
  assert_equal([0, 4], g:vals)

  g:vals = []
  assert_equal(0, Record([]) || Record('') || Record(0))
  assert_equal([[], '', 0], g:vals)
enddef

def Test_expr2_vimscript()
  # check line continuation
  let lines =<< trim END
      vim9script
      let var = 0
      		|| 1
      assert_equal(1, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = v:false
      		|| v:true
      		|| v:false
      assert_equal(v:true, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = v:false ||
      		v:true ||
		v:false
      assert_equal(v:true, var)
  END
  CheckScriptSuccess(lines)

  # check keeping the value
  lines =<< trim END
      vim9script
      assert_equal(2, 2 || 0)
      assert_equal(7, 0 ||
			0 ||
			7)
      assert_equal(0, 0 || 0)
      assert_equal(0, 0
			|| 0)
      assert_equal('', 0 || '')

      g:vals = []
      assert_equal(3, Record(3) || Record(1))
      assert_equal([3], g:vals)

      g:vals = []
      assert_equal(5, Record(0) || Record(5))
      assert_equal([0, 5], g:vals)

      g:vals = []
      assert_equal(4, Record(0)
			  || Record(4)
			  || Record(0))
      assert_equal([0, 4], g:vals)

      g:vals = []
      assert_equal(0, Record([]) || Record('') || Record(0))
      assert_equal([[], '', 0], g:vals)
  END
  CheckScriptSuccess(lines)
enddef

func Test_expr2_fails()
  let msg = "white space required before and after '||'"
  call CheckDefFailure(["let x = 1||2"], msg)
  call CheckDefFailure(["let x = 1 ||2"], msg)
  call CheckDefFailure(["let x = 1|| 2"], msg)

  call CheckDefFailure(["let x = 1 || xxx"], 'E1001:')
endfunc

" test &&
def Test_expr3()
  assert_equal(0, 2 && 0)
  assert_equal(0, 0 &&
		0 &&
		7)
  assert_equal(7, 2
  		    && 3
		    && 7)
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

def Test_expr3_vimscript()
  # check line continuation
  let lines =<< trim END
      vim9script
      let var = 0
      		&& 1
      assert_equal(0, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = v:true
      		&& v:true
      		&& v:true
      assert_equal(v:true, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = v:true &&
      		v:true &&
      		v:true
      assert_equal(v:true, var)
  END
  CheckScriptSuccess(lines)

  # check keeping the value
  lines =<< trim END
      vim9script
      assert_equal(0, 2 && 0)
      assert_equal(0, 0 &&
		    0 &&
		    7)
      assert_equal(7, 2
			&& 3
			&& 7)
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
  END
  CheckScriptSuccess(lines)
enddef

func Test_expr3_fails()
  let msg = "white space required before and after '&&'"
  call CheckDefFailure(["let x = 1&&2"], msg)
  call CheckDefFailure(["let x = 1 &&2"], msg)
  call CheckDefFailure(["let x = 1&& 2"], msg)
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
  let trueVar = true
  let falseVar = false
  assert_equal(true, true == true)
  assert_equal(false, true ==
			false)
  assert_equal(true, true
			== trueVar)
  assert_equal(false, true == falseVar)
  assert_equal(true, true == g:atrue)
  assert_equal(false, g:atrue == false)

  assert_equal(true, v:none == v:none)
  assert_equal(false, v:none == v:null)
  assert_equal(true, g:anone == v:none)
  assert_equal(false, v:none == g:anull)

  let nr0 = 0
  let nr61 = 61
  assert_equal(false, 2 == 0)
  assert_equal(false, 2 == nr0)
  assert_equal(true, 61 == 61)
  assert_equal(true, 61 == nr61)
  assert_equal(true, g:anint == 10)
  assert_equal(false, 61 == g:anint)

  if has('float')
    let ff = 0.3
    assert_equal(true, ff == 0.3)
    assert_equal(false, 0.4 == ff)
    assert_equal(true, 0.1 == g:afloat)
    assert_equal(false, g:afloat == 0.3)

    ff = 3.0
    assert_equal(true, ff == 3)
    assert_equal(true, 3 == ff)
    ff = 3.1
    assert_equal(false, ff == 3)
    assert_equal(false, 3 == ff)
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

  call CheckDefFailure(["let x = 'a' == xxx"], 'E1001:')

  let bb = 0z3f
  assert_equal(true, 0z3f == bb)
  assert_equal(false, bb == 0z4f)
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

  assert_equal(true, function('g:Test_expr4_equal') == function('g:Test_expr4_equal'))
  assert_equal(false, function('g:Test_expr4_equal') == function('g:Test_expr4_is'))

  assert_equal(true, function('g:Test_expr4_equal', [123]) == function('g:Test_expr4_equal', [123]))
  assert_equal(false, function('g:Test_expr4_equal', [123]) == function('g:Test_expr4_is', [123]))
  assert_equal(false, function('g:Test_expr4_equal', [123]) == function('g:Test_expr4_equal', [999]))

  let OneFunc: func
  let TwoFunc: func
  OneFunc = function('len')
  TwoFunc = function('len')
  assert_equal(true, OneFunc('abc') == TwoFunc('123'))
enddef

" test != comperator
def Test_expr4_notequal()
  let trueVar = true
  let falseVar = false
  assert_equal(false, true != true)
  assert_equal(true, true !=
			false)
  assert_equal(false, true
  			!= trueVar)
  assert_equal(true, true != falseVar)
  assert_equal(false, true != g:atrue)
  assert_equal(true, g:atrue != false)

  assert_equal(false, v:none != v:none)
  assert_equal(true, v:none != v:null)
  assert_equal(false, g:anone != v:none)
  assert_equal(true, v:none != g:anull)

  let nr55 = 55
  let nr0 = 55
  assert_equal(true, 2 != 0)
  assert_equal(true, 2 != nr0)
  assert_equal(false, 55 != 55)
  assert_equal(false, 55 != nr55)
  assert_equal(false, g:anint != 10)
  assert_equal(true, 61 != g:anint)

  if has('float')
    let ff = 0.3
    assert_equal(false, 0.3 != ff)
    assert_equal(true, 0.4 != ff)
    assert_equal(false, 0.1 != g:afloat)
    assert_equal(true, g:afloat != 0.3)

    ff = 3.0
    assert_equal(false, ff != 3)
    assert_equal(false, 3 != ff)
    ff = 3.1
    assert_equal(true, ff != 3)
    assert_equal(true, 3 != ff)
  endif

  assert_equal(false, 'abc' != 'abc')
  assert_equal(true, 'xyz' != 'abc')
  assert_equal(false, g:astring != 'asdf')
  assert_equal(true, 'xyz' != g:astring)

  assert_equal(true, 'abc' != 'ABC')
  set ignorecase
  assert_equal(true, 'abc' != 'ABC')
  set noignorecase

  let bb = 0z3f
  assert_equal(false, 0z3f != bb)
  assert_equal(true, bb != 0z4f)
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

  assert_equal(false, function('g:Test_expr4_equal') != function('g:Test_expr4_equal'))
  assert_equal(true, function('g:Test_expr4_equal') != function('g:Test_expr4_is'))

  assert_equal(false, function('g:Test_expr4_equal', [123]) != function('g:Test_expr4_equal', [123]))
  assert_equal(true, function('g:Test_expr4_equal', [123]) != function('g:Test_expr4_is', [123]))
  assert_equal(true, function('g:Test_expr4_equal', [123]) != function('g:Test_expr4_equal', [999]))
enddef

" test > comperator
def Test_expr4_greater()
  assert_true(2 > 0)
  assert_true(2 >
		1)
  assert_false(2 > 2)
  assert_false(2 > 3)
  let nr2 = 2
  assert_true(nr2 > 0)
  assert_true(nr2 >
		1)
  assert_false(nr2 > 2)
  assert_false(nr2
  		    > 3)
  if has('float')
    let ff = 2.0
    assert_true(ff > 0.0)
    assert_true(ff > 1.0)
    assert_false(ff > 2.0)
    assert_false(ff > 3.0)
  endif
enddef

" test >= comperator
def Test_expr4_greaterequal()
  assert_true(2 >= 0)
  assert_true(2 >=
			2)
  assert_false(2 >= 3)
  let nr2 = 2
  assert_true(nr2 >= 0)
  assert_true(nr2 >= 2)
  assert_false(nr2 >= 3)
  if has('float')
    let ff = 2.0
    assert_true(ff >= 0.0)
    assert_true(ff >= 2.0)
    assert_false(ff >= 3.0)
  endif
enddef

" test < comperator
def Test_expr4_smaller()
  assert_false(2 < 0)
  assert_false(2 <
			2)
  assert_true(2
  		< 3)
  let nr2 = 2
  assert_false(nr2 < 0)
  assert_false(nr2 < 2)
  assert_true(nr2 < 3)
  if has('float')
    let ff = 2.0
    assert_false(ff < 0.0)
    assert_false(ff < 2.0)
    assert_true(ff < 3.0)
  endif
enddef

" test <= comperator
def Test_expr4_smallerequal()
  assert_false(2 <= 0)
  assert_false(2 <=
			1)
  assert_true(2
  		<= 2)
  assert_true(2 <= 3)
  let nr2 = 2
  assert_false(nr2 <= 0)
  assert_false(nr2 <= 1)
  assert_true(nr2 <= 2)
  assert_true(nr2 <= 3)
  if has('float')
    let ff = 2.0
    assert_false(ff <= 0.0)
    assert_false(ff <= 1.0)
    assert_true(ff <= 2.0)
    assert_true(ff <= 3.0)
  endif
enddef

" test =~ comperator
def Test_expr4_match()
  assert_equal(false, '2' =~ '0')
  assert_equal(false, ''
  			 =~ '0')
  assert_equal(true, '2' =~
			'[0-9]')
enddef

" test !~ comperator
def Test_expr4_nomatch()
  assert_equal(true, '2' !~ '0')
  assert_equal(true, ''
  			!~ '0')
  assert_equal(false, '2' !~
			'[0-9]')
enddef

" test is comperator
def Test_expr4_is()
  let mylist = [2]
  assert_false(mylist is [2])
  let other = mylist
  assert_true(mylist is
		other)

  let myblob = 0z1234
  assert_false(myblob
  			is 0z1234)
  let otherblob = myblob
  assert_true(myblob is otherblob)
enddef

" test isnot comperator
def Test_expr4_isnot()
  let mylist = [2]
  assert_true('2' isnot '0')
  assert_true(mylist isnot [2])
  let other = mylist
  assert_false(mylist isnot
			other)

  let myblob = 0z1234
  assert_true(myblob
  		isnot 0z1234)
  let otherblob = myblob
  assert_false(myblob isnot otherblob)
enddef

def RetVoid()
  let x = 1
enddef

def Test_expr4_vimscript()
  # check line continuation
  let lines =<< trim END
      vim9script
      let var = 0
      		< 1
      assert_equal(true, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = 123
      		!= 123
      assert_equal(false, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = 123 ==
      			123
      assert_equal(true, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let list = [1, 2, 3]
      let var = list
      		is list
      assert_equal(true, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let myblob = 0z1234
      let var = myblob
      		isnot 0z11
      assert_equal(true, var)
  END
  CheckScriptSuccess(lines)

  # spot check mismatching types
  lines =<< trim END
      vim9script
      echo '' == 0
  END
  CheckScriptFailure(lines, 'E1072:')

  lines =<< trim END
      vim9script
      echo v:true > v:false
  END
  CheckScriptFailure(lines, 'Cannot compare bool with bool')

  lines =<< trim END
      vim9script
      echo 123 is 123
  END
  CheckScriptFailure(lines, 'Cannot use "is" with number')

  # check 'ignorecase' not being used
  lines =<< trim END
    vim9script
    set ignorecase
    assert_equal(false, 'abc' == 'ABC')
    assert_equal(false, 'abc' ==# 'ABC')
    assert_equal(true, 'abc' ==? 'ABC')

    assert_equal(true, 'abc' != 'ABC')
    assert_equal(true, 'abc' !=# 'ABC')
    assert_equal(false, 'abc' !=? 'ABC')

    assert_equal(false, 'abc' =~ 'ABC')
    assert_equal(false, 'abc' =~# 'ABC')
    assert_equal(true, 'abc' =~? 'ABC')
    set noignorecase
  END
  CheckScriptSuccess(lines)
enddef

func Test_expr4_fails()
  let msg = "white space required before and after '>'"
  call CheckDefFailure(["let x = 1>2"], msg)
  call CheckDefFailure(["let x = 1 >2"], msg)
  call CheckDefFailure(["let x = 1> 2"], msg)

  let msg = "white space required before and after '=='"
  call CheckDefFailure(["let x = 1==2"], msg)
  call CheckDefFailure(["let x = 1 ==2"], msg)
  call CheckDefFailure(["let x = 1== 2"], msg)

  let msg = "white space required before and after 'is'"
  call CheckDefFailure(["let x = '1'is'2'"], msg)
  call CheckDefFailure(["let x = '1' is'2'"], msg)
  call CheckDefFailure(["let x = '1'is '2'"], msg)

  let msg = "white space required before and after 'isnot'"
  call CheckDefFailure(["let x = '1'isnot'2'"], msg)
  call CheckDefFailure(["let x = '1' isnot'2'"], msg)
  call CheckDefFailure(["let x = '1'isnot '2'"], msg)

  call CheckDefFailure(["let x = 1 is# 2"], 'E15:')
  call CheckDefFailure(["let x = 1 is? 2"], 'E15:')
  call CheckDefFailure(["let x = 1 isnot# 2"], 'E15:')
  call CheckDefFailure(["let x = 1 isnot? 2"], 'E15:')

  call CheckDefFailure(["let x = 1 == '2'"], 'Cannot compare number with string')
  call CheckDefFailure(["let x = '1' == 2"], 'Cannot compare string with number')
  call CheckDefFailure(["let x = 1 == RetVoid()"], 'Cannot compare number with void')
  call CheckDefFailure(["let x = RetVoid() == 1"], 'Cannot compare void with number')

  call CheckDefFailure(["let x = true > false"], 'Cannot compare bool with bool')
  call CheckDefFailure(["let x = true >= false"], 'Cannot compare bool with bool')
  call CheckDefFailure(["let x = true < false"], 'Cannot compare bool with bool')
  call CheckDefFailure(["let x = true <= false"], 'Cannot compare bool with bool')
  call CheckDefFailure(["let x = true =~ false"], 'Cannot compare bool with bool')
  call CheckDefFailure(["let x = true !~ false"], 'Cannot compare bool with bool')
  call CheckDefFailure(["let x = true is false"], 'Cannot use "is" with bool')
  call CheckDefFailure(["let x = true isnot false"], 'Cannot use "isnot" with bool')

  call CheckDefFailure(["let x = v:none is v:null"], 'Cannot use "is" with special')
  call CheckDefFailure(["let x = v:none isnot v:null"], 'Cannot use "isnot" with special')
  call CheckDefFailure(["let x = 123 is 123"], 'Cannot use "is" with number')
  call CheckDefFailure(["let x = 123 isnot 123"], 'Cannot use "isnot" with number')
  if has('float')
    call CheckDefFailure(["let x = 1.3 is 1.3"], 'Cannot use "is" with float')
    call CheckDefFailure(["let x = 1.3 isnot 1.3"], 'Cannot use "isnot" with float')
  endif

  call CheckDefFailure(["let x = 0za1 > 0z34"], 'Cannot compare blob with blob')
  call CheckDefFailure(["let x = 0za1 >= 0z34"], 'Cannot compare blob with blob')
  call CheckDefFailure(["let x = 0za1 < 0z34"], 'Cannot compare blob with blob')
  call CheckDefFailure(["let x = 0za1 <= 0z34"], 'Cannot compare blob with blob')
  call CheckDefFailure(["let x = 0za1 =~ 0z34"], 'Cannot compare blob with blob')
  call CheckDefFailure(["let x = 0za1 !~ 0z34"], 'Cannot compare blob with blob')

  call CheckDefFailure(["let x = [13] > [88]"], 'Cannot compare list with list')
  call CheckDefFailure(["let x = [13] >= [88]"], 'Cannot compare list with list')
  call CheckDefFailure(["let x = [13] < [88]"], 'Cannot compare list with list')
  call CheckDefFailure(["let x = [13] <= [88]"], 'Cannot compare list with list')
  call CheckDefFailure(["let x = [13] =~ [88]"], 'Cannot compare list with list')
  call CheckDefFailure(["let x = [13] !~ [88]"], 'Cannot compare list with list')

  call CheckDefFailure(['let j: job', 'let chan: channel', 'let r = j == chan'], 'Cannot compare job with channel')
  call CheckDefFailure(['let j: job', 'let x: list<any>', 'let r = j == x'], 'Cannot compare job with list')
  call CheckDefFailure(['let j: job', 'let Xx: func', 'let r = j == Xx'], 'Cannot compare job with func')
  call CheckDefFailure(['let j: job', 'let Xx: func', 'let r = j == Xx'], 'Cannot compare job with func')
endfunc

" test addition, subtraction, concatenation
def Test_expr5()
  assert_equal(66, 60 + 6)
  assert_equal(70, 60 +
			g:anint)
  assert_equal(9, g:alsoint
  			+ 5)
  assert_equal(14, g:alsoint + g:anint)
  assert_equal([1, 2, 3, 4], [1] + g:alist)

  assert_equal(54, 60 - 6)
  assert_equal(50, 60 -
		    g:anint)
  assert_equal(-1, g:alsoint
  			- 5)
  assert_equal(-6, g:alsoint - g:anint)

  assert_equal('hello', 'hel' .. 'lo')
  assert_equal('hello 123', 'hello ' ..
					123)
  assert_equal('hello 123', 'hello '
  				..  123)
  assert_equal('123 hello', 123 .. ' hello')
  assert_equal('123456', 123 .. 456)

  assert_equal([1, 2, 3, 4], [1, 2] + [3, 4])
  assert_equal(0z11223344, 0z1122 + 0z3344)
  assert_equal(0z112201ab, 0z1122
  				+ g:ablob)
  assert_equal(0z01ab3344, g:ablob + 0z3344)
  assert_equal(0z01ab01ab, g:ablob + g:ablob)
enddef

def Test_expr5_vim9script()
  # only checks line continuation
  let lines =<< trim END
      vim9script
      let var = 11
      		+ 77
		- 22
      assert_equal(66, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = 'one'
      		.. 'two'
      assert_equal('onetwo', var)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr5_float()
  if !has('float')
    MissingFeature 'float'
  else
    assert_equal(66.0, 60.0 + 6.0)
    assert_equal(66.0, 60.0 + 6)
    assert_equal(66.0, 60 +
			 6.0)
    assert_equal(5.1, g:afloat
    			+ 5)
    assert_equal(8.1, 8 + g:afloat)
    assert_equal(10.1, g:anint + g:afloat)
    assert_equal(10.1, g:afloat + g:anint)

    assert_equal(54.0, 60.0 - 6.0)
    assert_equal(54.0, 60.0
    			    - 6)
    assert_equal(54.0, 60 - 6.0)
    assert_equal(-4.9, g:afloat - 5)
    assert_equal(7.9, 8 - g:afloat)
    assert_equal(9.9, g:anint - g:afloat)
    assert_equal(-9.9, g:afloat - g:anint)
  endif
enddef

func Test_expr5_fails()
  let msg = "white space required before and after '+'"
  call CheckDefFailure(["let x = 1+2"], msg)
  call CheckDefFailure(["let x = 1 +2"], msg)
  call CheckDefFailure(["let x = 1+ 2"], msg)

  let msg = "white space required before and after '-'"
  call CheckDefFailure(["let x = 1-2"], msg)
  call CheckDefFailure(["let x = 1 -2"], msg)
  call CheckDefFailure(["let x = 1- 2"], msg)

  let msg = "white space required before and after '..'"
  call CheckDefFailure(["let x = '1'..'2'"], msg)
  call CheckDefFailure(["let x = '1' ..'2'"], msg)
  call CheckDefFailure(["let x = '1'.. '2'"], msg)

  call CheckDefFailure(["let x = 0z1122 + 33"], 'E1051')
  call CheckDefFailure(["let x = 0z1122 + [3]"], 'E1051')
  call CheckDefFailure(["let x = 0z1122 + 'asd'"], 'E1051')
  call CheckDefFailure(["let x = 33 + 0z1122"], 'E1051')
  call CheckDefFailure(["let x = [3] + 0z1122"], 'E1051')
  call CheckDefFailure(["let x = 'asdf' + 0z1122"], 'E1051')
  call CheckDefFailure(["let x = 6 + xxx"], 'E1001')
endfunc

" test multiply, divide, modulo
def Test_expr6()
  assert_equal(36, 6 * 6)
  assert_equal(24, 6 *
			g:alsoint)
  assert_equal(24, g:alsoint
  			* 6)
  assert_equal(40, g:anint * g:alsoint)

  assert_equal(10, 60 / 6)
  assert_equal(6, 60 /
			g:anint)
  assert_equal(1, g:anint / 6)
  assert_equal(2, g:anint
  			/ g:alsoint)

  assert_equal(5, 11 % 6)
  assert_equal(4, g:anint % 6)
  assert_equal(3, 13 %
			g:anint)
  assert_equal(2, g:anint
  			% g:alsoint)

  assert_equal(4, 6 * 4 / 6)

  let x = [2]
  let y = [3]
  assert_equal(5, x[0] + y[0])
  assert_equal(6, x[0] * y[0])
  if has('float')
    let xf = [2.0]
    let yf = [3.0]
    assert_equal(5.0, xf[0]
    			+ yf[0])
    assert_equal(6.0, xf[0]
    			* yf[0])
  endif

  call CheckDefFailure(["let x = 6 * xxx"], 'E1001')
enddef

def Test_expr6_vim9script()
  # only checks line continuation
  let lines =<< trim END
      vim9script
      let var = 11
      		* 22
		/ 3
      assert_equal(80, var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let var = 25
      		% 10
      assert_equal(5, var)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr6_float()
  if !has('float')
    MissingFeature 'float'
  else
    assert_equal(36.0, 6.0 * 6)
    assert_equal(36.0, 6 *
			   6.0)
    assert_equal(36.0, 6.0 * 6.0)
    assert_equal(1.0, g:afloat * g:anint)

    assert_equal(10.0, 60 / 6.0)
    assert_equal(10.0, 60.0 /
			6)
    assert_equal(10.0, 60.0 / 6.0)
    assert_equal(0.01, g:afloat / g:anint)

    assert_equal(4.0, 6.0 * 4 / 6)
    assert_equal(4.0, 6 *
			4.0 /
			6)
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
  call CheckDefFailure(["let x = 1*2"], msg)
  call CheckDefFailure(["let x = 1 *2"], msg)
  call CheckDefFailure(["let x = 1* 2"], msg)

  let msg = "white space required before and after '/'"
  call CheckDefFailure(["let x = 1/2"], msg)
  call CheckDefFailure(["let x = 1 /2"], msg)
  call CheckDefFailure(["let x = 1/ 2"], msg)

  let msg = "white space required before and after '%'"
  call CheckDefFailure(["let x = 1%2"], msg)
  call CheckDefFailure(["let x = 1 %2"], msg)
  call CheckDefFailure(["let x = 1% 2"], msg)

  call CheckDefFailure(["let x = '1' * '2'"], 'E1036:')
  call CheckDefFailure(["let x = '1' / '2'"], 'E1036:')
  call CheckDefFailure(["let x = '1' % '2'"], 'E1035:')

  call CheckDefFailure(["let x = 0z01 * 0z12"], 'E1036:')
  call CheckDefFailure(["let x = 0z01 / 0z12"], 'E1036:')
  call CheckDefFailure(["let x = 0z01 % 0z12"], 'E1035:')

  call CheckDefFailure(["let x = [1] * [2]"], 'E1036:')
  call CheckDefFailure(["let x = [1] / [2]"], 'E1036:')
  call CheckDefFailure(["let x = [1] % [2]"], 'E1035:')

  call CheckDefFailure(["let x = #{one: 1} * #{two: 2}"], 'E1036:')
  call CheckDefFailure(["let x = #{one: 1} / #{two: 2}"], 'E1036:')
  call CheckDefFailure(["let x = #{one: 1} % #{two: 2}"], 'E1035:')

  call CheckDefFailure(["let x = 0xff[1]"], 'E1090:')
  if has('float')
    call CheckDefFailure(["let x = 0.7[1]"], 'E1090:')
  endif
endfunc

func Test_expr6_float_fails()
  CheckFeature float
  call CheckDefFailure(["let x = 1.0 % 2"], 'E1035:')
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
  # number constant
  assert_equal(0, 0)
  assert_equal(654, 0654)

  assert_equal(6, 0x6)
  assert_equal(15, 0xf)
  assert_equal(255, 0xff)
enddef

def Test_expr7_float()
  # float constant
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
  # blob constant
  assert_equal(g:blob_empty, 0z)
  assert_equal(g:blob_one, 0z01)
  assert_equal(g:blob_long, 0z0102.0304)

  call CheckDefFailure(["let x = 0z123"], 'E973:')
enddef

def Test_expr7_string()
  # string constant
  assert_equal(g:string_empty, '')
  assert_equal(g:string_empty, "")
  assert_equal(g:string_short, 'x')
  assert_equal(g:string_short, "x")
  assert_equal(g:string_long, 'abcdefghijklm')
  assert_equal(g:string_long, "abcdefghijklm")
  assert_equal(g:string_special, "ab\ncd\ref\ekk")

  call CheckDefFailure(['let x = "abc'], 'E114:')
  call CheckDefFailure(["let x = 'abc"], 'E115:')
enddef

def Test_expr7_vimvar()
  let old: list<string> = v:oldfiles
  let compl: dict<any> = v:completed_item

  call CheckDefFailure(["let old: list<number> = v:oldfiles"], 'E1013: type mismatch, expected list<number> but got list<string>')
  call CheckDefFailure(["let old: dict<number> = v:completed_item"], 'E1013: type mismatch, expected dict<number> but got dict<any>')
enddef

def Test_expr7_special()
  # special constant
  assert_equal(g:special_true, true)
  assert_equal(g:special_false, false)
  assert_equal(g:special_true, v:true)
  assert_equal(g:special_false, v:false)
  assert_equal(g:special_null, v:null)
  assert_equal(g:special_none, v:none)

  call CheckDefFailure(['v:true = true'], 'E46:')
  call CheckDefFailure(['v:true = false'], 'E46:')
  call CheckDefFailure(['v:false = true'], 'E46:')
  call CheckDefFailure(['v:null = 11'], 'E46:')
  call CheckDefFailure(['v:none = 22'], 'E46:')
enddef

def Test_expr7_special_vim9script()
  let lines =<< trim END
      vim9script
      let t = true
      let f = false
      assert_equal(v:true, true)
      assert_equal(true, t)
      assert_equal(v:false, false)
      assert_equal(false, f)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_list()
  # list
  assert_equal(g:list_empty, [])
  assert_equal(g:list_empty, [  ])
  assert_equal(g:list_mixed, [1, 'b', false,])
  assert_equal('b', g:list_mixed[1])

  call CheckDefExecFailure(["let x = g:anint[3]"], 'E714:')
  call CheckDefFailure(["let x = g:list_mixed[xxx]"], 'E1001:')
  call CheckDefFailure(["let x = [1,2,3]"], 'E1069:')
  call CheckDefExecFailure(["let x = g:list_mixed['xx']"], 'E1029:')
  call CheckDefFailure(["let x = g:list_mixed["], 'E1097:')
  call CheckDefFailure(["let x = g:list_mixed[0"], 'E1097:')
  call CheckDefExecFailure(["let x = g:list_empty[3]"], 'E684:')
enddef

def Test_expr7_list_vim9script()
  let lines =<< trim END
      vim9script
      let l = [
		11,
		22,
		]
      assert_equal([11, 22], l)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let l = [11,
		22]
      assert_equal([11, 22], l)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let l = [11,22]
  END
  CheckScriptFailure(lines, 'E1069:')
enddef

def Test_expr7_lambda()
  let La = { -> 'result'}
  assert_equal('result', La())
  assert_equal([1, 3, 5], [1, 2, 3]->map({key, val -> key + val}))

  # line continuation inside lambda with "cond ? expr : expr" works
  let ll = range(3)
  map(ll, {k, v -> v % 2 ? {
	    '111': 111 } : {}
	})
  assert_equal([{}, {'111': 111}, {}], ll)

  ll = range(3)
  map(ll, {k, v -> v == 8 || v
		== 9
		|| v % 2 ? 111 : 222
	})
  assert_equal([222, 111, 222], ll)

  ll = range(3)
  map(ll, {k, v -> v != 8 && v
		!= 9
		&& v % 2 == 0 ? 111 : 222
	})
  assert_equal([111, 222, 111], ll)

  let dl = [{'key': 0}, {'key': 22}]->filter({ _, v -> v['key'] })
  assert_equal([{'key': 22}], dl)

  dl = [{'key': 12}, {'foo': 34}]
  assert_equal([{'key': 12}], filter(dl,
	{_, v -> has_key(v, 'key') ? v['key'] == 12 : 0}))

  call CheckDefFailure(["filter([1, 2], {k,v -> 1})"], 'E1069:')
enddef

def Test_expr7_lambda_vim9script()
  let lines =<< trim END
      vim9script
      let v = 10->{a ->
	    a
	      + 2
	  }()
      assert_equal(12, v)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_dict()
  # dictionary
  assert_equal(g:dict_empty, {})
  assert_equal(g:dict_empty, {  })
  assert_equal(g:dict_one, {'one': 1})
  let key = 'one'
  let val = 1
  assert_equal(g:dict_one, {key: val})

  call CheckDefFailure(["let x = #{8: 8}"], 'E1014:')
  call CheckDefFailure(["let x = #{xxx}"], 'E720:')
  call CheckDefFailure(["let x = #{xxx: 1", "let y = 2"], 'E722:')
  call CheckDefFailure(["let x = #{xxx: 1,"], 'E723:')
  call CheckDefFailure(["let x = {'a': xxx}"], 'E1001:')
  call CheckDefFailure(["let x = {xxx: 8}"], 'E1001:')
  call CheckDefFailure(["let x = #{a: 1, a: 2}"], 'E721:')
  call CheckDefFailure(["let x = #"], 'E1015:')
  call CheckDefFailure(["let x += 1"], 'E1020:')
  call CheckDefFailure(["let x = x + 1"], 'E1001:')
  call CheckDefExecFailure(["let x = g:anint.member"], 'E715:')
  call CheckDefExecFailure(["let x = g:dict_empty.member"], 'E716:')
enddef

def Test_expr7_dict_vim9script()
  let lines =<< trim END
      vim9script
      let d = {
		'one':
		   1,
		'two': 2,
		   }
      assert_equal({'one': 1, 'two': 2}, d)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let d = { "one": "one", "two": "two", }
      assert_equal({'one': 'one', 'two': 'two'}, d)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let d = #{one: 1,
		two: 2,
	       }
      assert_equal({'one': 1, 'two': 2}, d)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let d = #{one:1, two: 2}
  END
  CheckScriptFailure(lines, 'E1069:')

  lines =<< trim END
      vim9script
      let d = #{one: 1,two: 2}
  END
  CheckScriptFailure(lines, 'E1069:')
enddef

let g:oneString = 'one'

def Test_expr_member()
  assert_equal(1, g:dict_one.one)
  let d: dict<number> = g:dict_one
  assert_equal(1, d['one'])
  assert_equal(1, d[
		  'one'
		  ])
  assert_equal(1, d
  	.one)

  # getting the one member should clear the dict after getting the item
  assert_equal('one', #{one: 'one'}.one)
  assert_equal('one', #{one: 'one'}[g:oneString])

  call CheckDefFailure(["let x = g:dict_one.#$!"], 'E1002:')
  call CheckDefExecFailure(["let d: dict<any>", "echo d['a']"], 'E716:')
  call CheckDefExecFailure(["let d: dict<number>", "d = g:list_empty"], 'E1029: Expected dict but got list')
enddef

def Test_expr_index()
  # getting the one member should clear the list only after getting the item
  assert_equal('bbb', ['aaa', 'bbb', 'ccc'][1])
enddef

def Test_expr_member_vim9script()
  let lines =<< trim END
      vim9script
      let d = #{one:
      		'one',
		two: 'two'}
      assert_equal('one', d.one)
      assert_equal('one', d
                            .one)
      assert_equal('one', d[
			    'one'
			    ])
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      let l = [1,
		  2,
		  3, 4
		  ]
      assert_equal(2, l[
			    1
			    ])
      assert_equal([2, 3], l[1 : 2])
      assert_equal([1, 2, 3], l[
				:
				2
				])
      assert_equal([3, 4], l[
				2
				:
				])
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_option()
  # option
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
  # environment variable
  assert_equal('testvar', $TESTVAR)
  assert_equal('', $ASDF_ASD_XXX)

  call CheckDefFailure(["let x = $$$"], 'E1002:')
enddef

def Test_expr7_register()
  @a = 'register a'
  assert_equal('register a', @a)
enddef

def Test_expr7_namespace()
  g:some_var = 'some'
  assert_equal('some', get(g:, 'some_var'))
  assert_equal('some', get(g:, 'some_var', 'xxx'))
  assert_equal('xxx', get(g:, 'no_var', 'xxx'))
  unlet g:some_var

  b:some_var = 'some'
  assert_equal('some', get(b:, 'some_var'))
  assert_equal('some', get(b:, 'some_var', 'xxx'))
  assert_equal('xxx', get(b:, 'no_var', 'xxx'))
  unlet b:some_var

  w:some_var = 'some'
  assert_equal('some', get(w:, 'some_var'))
  assert_equal('some', get(w:, 'some_var', 'xxx'))
  assert_equal('xxx', get(w:, 'no_var', 'xxx'))
  unlet w:some_var

  t:some_var = 'some'
  assert_equal('some', get(t:, 'some_var'))
  assert_equal('some', get(t:, 'some_var', 'xxx'))
  assert_equal('xxx', get(t:, 'no_var', 'xxx'))
  unlet t:some_var
enddef

def Test_expr7_parens()
  # (expr)
  assert_equal(4, (6 * 4) / 6)
  assert_equal(0, 6 * ( 4 / 6 ))

  assert_equal(6, +6)
  assert_equal(-6, -6)
  assert_equal(6, --6)
  assert_equal(6, -+-6)
  assert_equal(-6, ---6)
  assert_equal(false, !-3)
  assert_equal(true, !+-+0)
enddef

def Test_expr7_parens_vim9script()
  let lines =<< trim END
      vim9script
      let s = (
		'one'
		..
		'two'
		)
      assert_equal('onetwo', s)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_negate()
  assert_equal(-99, -99)
  assert_equal(99, --99)
  let nr = 88
  assert_equal(-88, -nr)
  assert_equal(88, --nr)
enddef

def Echo(arg: any): string
  return arg
enddef

def s:EchoArg(arg: any): string
  return arg
enddef

def Test_expr7_call()
  assert_equal('yes', 'yes'->Echo())
  assert_equal('yes', 'yes'
  			->s:EchoArg())
  assert_equal(1, !range(5)->empty())
  assert_equal([0, 1, 2], --3->range())

  call CheckDefFailure(["let x = 'yes'->Echo"], 'E107:')
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
  call CheckDefFailure(["let x = (12"], "E110:")

  call CheckDefFailure(["let x = -'xx'"], "E1030:")
  call CheckDefFailure(["let x = +'xx'"], "E1030:")
  call CheckDefFailure(["let x = -0z12"], "E974:")
  call CheckDefExecFailure(["let x = -[8]"], "E39:")
  call CheckDefExecFailure(["let x = -{'a': 1}"], "E39:")

  call CheckDefFailure(["let x = @"], "E1002:")
  call CheckDefFailure(["let x = @<"], "E354:")

  call CheckDefFailure(["let x = [1, 2"], "E697:")
  call CheckDefFailure(["let x = [notfound]"], "E1001:")

  call CheckDefFailure(["let x = { -> 123) }"], "E451:")
  call CheckDefFailure(["let x = 123->{x -> x + 5) }"], "E451:")

  call CheckDefFailure(["let x = &notexist"], 'E113:')
  call CheckDefFailure(["&grepprg = [343]"], 'E1013:')

  call CheckDefExecFailure(["echo s:doesnt_exist"], 'E121:')
  call CheckDefExecFailure(["echo g:doesnt_exist"], 'E121:')

  call CheckDefFailure(["echo a:somevar"], 'E1075:')
  call CheckDefFailure(["echo l:somevar"], 'E1075:')
  call CheckDefFailure(["echo x:somevar"], 'E1075:')

  call CheckDefExecFailure(["let x = +g:astring"], 'E1030:')
  call CheckDefExecFailure(["let x = +g:ablob"], 'E974:')
  call CheckDefExecFailure(["let x = +g:alist"], 'E745:')
  call CheckDefExecFailure(["let x = +g:adict"], 'E728:')

  call CheckDefFailure(["let x = ''", "let y = x.memb"], 'E715:')

  call CheckDefFailure(["'yes'->", "Echo()"], 'E488: Trailing characters: ->')

  call CheckDefExecFailure(["[1, 2->len()"], 'E697:')
  call CheckDefExecFailure(["#{a: 1->len()"], 'E488:')
  call CheckDefExecFailure(["{'a': 1->len()"], 'E723:')
endfunc

let g:Funcrefs = [function('add')]

func CallMe(arg)
  return a:arg
endfunc

func CallMe2(one, two)
  return a:one .. a:two
endfunc

def Test_expr7_trailing()
  # user function call
  assert_equal(123, g:CallMe(123))
  assert_equal(123, g:CallMe(  123))
  assert_equal(123, g:CallMe(123  ))
  assert_equal('yesno', g:CallMe2('yes', 'no'))
  assert_equal('yesno', g:CallMe2( 'yes', 'no' ))
  assert_equal('nothing', g:CallMe('nothing'))

  # partial call
  let Part = function('g:CallMe')
  assert_equal('yes', Part('yes'))

  # funcref call, using list index
  let l = []
  g:Funcrefs[0](l, 2)
  assert_equal([2], l)

  # method call
  l = [2, 5, 6]
  l->map({k, v -> k + v})
  assert_equal([2, 6, 8], l)

  # lambda method call
  l = [2, 5]
  l->{l -> add(l, 8)}()
  assert_equal([2, 5, 8], l)

  # dict member
  let d = #{key: 123}
  assert_equal(123, d.key)
enddef

def Test_expr7_subscript()
  let text = 'abcdef'
  assert_equal('', text[-1])
  assert_equal('a', text[0])
  assert_equal('e', text[4])
  assert_equal('f', text[5])
  assert_equal('', text[6])
enddef

def Test_expr7_subscript_linebreak()
  let range = range(
  		3)
  let l = range
	->map('string(v:key)')
  assert_equal(['0', '1', '2'], l)

  l = range
  	->map('string(v:key)')
  assert_equal(['0', '1', '2'], l)

  l = range # comment
  	->map('string(v:key)')
  assert_equal(['0', '1', '2'], l)

  l = range

  	->map('string(v:key)')
  assert_equal(['0', '1', '2'], l)

  l = range
	# comment
  	->map('string(v:key)')
  assert_equal(['0', '1', '2'], l)

  assert_equal('1', l[
	1])

  let d = #{one: 33}
  assert_equal(33, d.
	one)
enddef

def Test_expr7_method_call()
  new
  setline(1, ['first', 'last'])
  eval 'second'->append(1)
  assert_equal(['first', 'second', 'last'], getline(1, '$'))
  bwipe!

  let bufnr = bufnr()
  let loclist = [#{bufnr: bufnr, lnum: 42, col: 17, text: 'wrong'}]
  loclist->setloclist(0)
  assert_equal([#{bufnr: bufnr,
  		lnum: 42,
		col: 17,
		text: 'wrong',
		pattern: '',
		valid: 1,
		vcol: 0,
		nr: 0,
		type: '',
		module: ''}
		], getloclist(0))
enddef

func Test_expr7_trailing_fails()
  call CheckDefFailure(['let l = [2]', 'l->{l -> add(l, 8)}'], 'E107')
  call CheckDefFailure(['let l = [2]', 'l->{l -> add(l, 8)} ()'], 'E274')
endfunc

func Test_expr_fails()
  call CheckDefFailure(["let x = '1'is2"], 'E488:')
  call CheckDefFailure(["let x = '1'isnot2"], 'E488:')

  call CheckDefFailure(["CallMe ('yes')"], 'E476:')
  call CheckDefFailure(["CallMe2('yes','no')"], 'E1069:')
  call CheckDefFailure(["CallMe2('yes' , 'no')"], 'E1068:')

  call CheckDefFailure(["v:nosuch += 3"], 'E1001:')
  call CheckDefFailure(["let v:statusmsg = ''"], 'E1016: Cannot declare a v: variable:')
  call CheckDefFailure(["let asdf = v:nosuch"], 'E1001:')

  call CheckDefFailure(["echo len('asdf'"], 'E110:')
  call CheckDefFailure(["echo Func0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789()"], 'E1011:')
  call CheckDefFailure(["echo doesnotexist()"], 'E117:')
endfunc
