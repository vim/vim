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
def Test_expr1_trinary()
  assert_equal('one', true ? 'one' : 'two')
  assert_equal('one', 1 ?
			'one' :
			'two')
  if has('float')
    assert_equal('one', !!0.1 ? 'one' : 'two')
  endif
  assert_equal('one', !!'x' ? 'one' : 'two')
  assert_equal('one', !!'x'
  			? 'one'
			: 'two')
  assert_equal('one', !!0z1234 ? 'one' : 'two')
  assert_equal('one', !![0] ? 'one' : 'two')
  assert_equal('one', !!#{x: 0} ? 'one' : 'two')
  var name = 1
  assert_equal('one', name ? 'one' : 'two')

  assert_equal('two', false ? 'one' : 'two')
  assert_equal('two', 0 ? 'one' : 'two')
  if has('float')
    assert_equal('two', !!0.0 ? 'one' : 'two')
  endif
  assert_equal('two', !!'' ? 'one' : 'two')
  assert_equal('two', !!0z ? 'one' : 'two')
  assert_equal('two', !![] ? 'one' : 'two')
  assert_equal('two', !!{} ? 'one' : 'two')
  name = 0
  assert_equal('two', name ? 'one' : 'two')

  # with constant condition expression is not evaluated 
  assert_equal('one', 1 ? 'one' : xxx)

  var Some: func = function('len')
  var Other: func = function('winnr')
  var Res: func = g:atrue ? Some : Other
  assert_equal(function('len'), Res)

  var RetOne: func(string): number = function('len')
  var RetTwo: func(string): number = function('winnr')
  var RetThat: func = g:atrue ? RetOne : RetTwo
  assert_equal(function('len'), RetThat)

  var X = FuncOne
  var Y = FuncTwo
  var Z = g:cond ? FuncOne : FuncTwo
  assert_equal(123, Z(3))
enddef

def Test_expr1_trinary_vimscript()
  # check line continuation
  var lines =<< trim END
      vim9script
      var name = 1
      		? 'yes'
		: 'no'
      assert_equal('yes', name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:false
      		? 'yes'
		: 'no'
      assert_equal('no', name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:false ?
      		'yes' :
		'no'
      assert_equal('no', name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:false ?  # comment
      		'yes' :
                # comment
		'no' # comment
      assert_equal('no', name)
  END
  CheckScriptSuccess(lines)

  # check white space
  lines =<< trim END
      vim9script
      var name = v:true?1:2
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true? 1 : 2
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true ?1 : 2
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true ? 1: 2
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true ? 1 :2
  END
  CheckScriptFailure(lines, 'E1004:', 2)

  lines =<< trim END
      vim9script
      var name = 'x' ? 1 : 2
  END
  CheckScriptFailure(lines, 'E1030:', 2)

  lines =<< trim END
      vim9script
      var name = [] ? 1 : 2
  END
  CheckScriptFailure(lines, 'E745:', 2)

  lines =<< trim END
      vim9script
      var name = {} ? 1 : 2
  END
  CheckScriptFailure(lines, 'E728:', 2)

  # check after failure eval_flags is reset
  lines =<< trim END
      vim9script
      try
        eval('0 ? 1: 2')
      catch
      endtry
      assert_equal(v:true, eval(string(v:true)))
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      try
        eval('0 ? 1 :2')
      catch
      endtry
      assert_equal(v:true, eval(string(v:true)))
  END
  CheckScriptSuccess(lines)
enddef

func Test_expr1_trinary_fails()
  call CheckDefFailure(["var x = 1 ? 'one'"], "Missing ':' after '?'", 1)

  let msg = "White space required before and after '?'"
  call CheckDefFailure(["var x = 1? 'one' : 'two'"], msg, 1)
  call CheckDefFailure(["var x = 1 ?'one' : 'two'"], msg, 1)
  call CheckDefFailure(["var x = 1?'one' : 'two'"], msg, 1)

  let msg = "White space required before and after ':'"
  call CheckDefFailure(["var x = 1 ? 'one': 'two'"], msg, 1)
  call CheckDefFailure(["var x = 1 ? 'one' :'two'"], msg, 1)
  call CheckDefFailure(["var x = 1 ? 'one':'two'"], msg, 1)

  call CheckDefFailure(["var x = 'x' ? 'one' : 'two'"], 'E1030:', 1)
  call CheckDefFailure(["var x = 0z1234 ? 'one' : 'two'"], 'E974:', 1)
  call CheckDefExecFailure(["var x = [] ? 'one' : 'two'"], 'E745:', 1)
  call CheckDefExecFailure(["var x = {} ? 'one' : 'two'"], 'E728:', 1)

  if has('float')
    call CheckDefFailure(["var x = 0.1 ? 'one' : 'two'"], 'E805:', 1)
  endif

  " missing argument detected even when common type is used
  call CheckDefFailure([
	\ 'var X = FuncOne',
	\ 'var Y = FuncTwo',
	\ 'var Z = g:cond ? FuncOne : FuncTwo',
	\ 'Z()'], 'E119:', 4)
endfunc

def Test_expr1_falsy()
  var lines =<< trim END
      assert_equal(v:true, v:true ?? 456)
      assert_equal(123, 123 ?? 456)
      assert_equal('yes', 'yes' ?? 456)
      assert_equal([1], [1] ?? 456)
      assert_equal(#{one: 1}, #{one: 1} ?? 456)
      if has('float')
        assert_equal(0.1, 0.1 ?? 456)
      endif

      assert_equal(456, v:false ?? 456)
      assert_equal(456, 0 ?? 456)
      assert_equal(456, '' ?? 456)
      assert_equal(456, [] ?? 456)
      assert_equal(456, {} ?? 456)
      if has('float')
        assert_equal(456, 0.0 ?? 456)
      endif
  END
  CheckDefAndScriptSuccess(lines)

  var msg = "White space required before and after '??'"
  call CheckDefFailure(["var x = 1?? 'one' : 'two'"], msg, 1)
  call CheckDefFailure(["var x = 1 ??'one' : 'two'"], msg, 1)
  call CheckDefFailure(["var x = 1??'one' : 'two'"], msg, 1)
enddef

" TODO: define inside test function
def Record(val: any): any
  g:vals->add(val)
  return val
enddef

" test ||
def Test_expr2()
  assert_equal(true, 1 || 0)
  assert_equal(true, 0 ||
		    0 ||
		    1)
  assert_equal(false, 0 || 0)
  assert_equal(false, 0
  		    || 0)
  assert_equal(false, 0 || false)

  g:vals = []
  assert_equal(true, Record(1) || Record(3))
  assert_equal([1], g:vals)

  g:vals = []
  assert_equal(true, Record(0) || Record(1))
  assert_equal([0, 1], g:vals)

  g:vals = []
  assert_equal(true, Record(0)
		      || Record(1)
		      || Record(0))
  assert_equal([0, 1], g:vals)

  g:vals = []
  assert_equal(false, Record(0) || Record(false) || Record(0))
  assert_equal([0, false, 0], g:vals)
enddef

def Test_expr2_vimscript()
  # check line continuation
  var lines =<< trim END
      vim9script
      var name = 0
      		|| 1
      assert_equal(true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:false
      		|| v:true
      		|| v:false
      assert_equal(v:true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:false ||
      		v:true ||
		v:false
      assert_equal(v:true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:false || # comment
                # comment
      		v:true ||
                # comment
		v:false # comment
      assert_equal(v:true, name)
  END
  CheckScriptSuccess(lines)

  # check white space
  lines =<< trim END
      vim9script
      var name = v:true||v:true
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true ||v:true
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true|| v:true
  END
  CheckScriptFailure(lines, 'E1004:', 2)

  # check evaluating to bool
  lines =<< trim END
      assert_equal(true, 1 || 0)
      assert_equal(true, 0 ||
			0 ||
			!!7)
      assert_equal(false, 0 || 0)
      assert_equal(false, 0
			|| 0)
      assert_equal(false, 0 || false)

      g:vals = []
      assert_equal(true, Record(true) || Record(false))
      assert_equal([true], g:vals)

      g:vals = []
      assert_equal(true, Record(0) || Record(true))
      assert_equal([0, true], g:vals)

      g:vals = []
      assert_equal(true, Record(0)
			  || Record(true)
			  || Record(0))
      assert_equal([0, true], g:vals)

      g:vals = []
      assert_equal(false, Record(0) || Record(false) || Record(0))
      assert_equal([0, false, 0], g:vals)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr2_fails()
  var msg = "White space required before and after '||'"
  call CheckDefFailure(["var x = 1||2"], msg, 1)
  call CheckDefFailure(["var x = 1 ||2"], msg, 1)
  call CheckDefFailure(["var x = 1|| 2"], msg, 1)

  call CheckDefFailure(["var x = 1 || xxx"], 'E1001:', 1)

  # TODO: should fail at compile time
  call CheckDefExecFailure(["var x = 3 || 7"], 'E1023:', 1)
  call CheckScriptFailure(["vim9script", "var x = 3 || 7"], 'E1023:', 2)
  call CheckDefExecFailure(["var x = [] || false"], 'E745:', 1)
  call CheckScriptFailure(["vim9script", "var x = [] || false"], 'E745:', 2)
enddef

" test &&
def Test_expr3()
  assert_equal(false, 1 && 0)
  assert_equal(false, 0 &&
		0 &&
		1)
  assert_equal(true, 1
  		    && true
		    && 1)
  assert_equal(false, 0 && 0)
  assert_equal(false, 0 && false)
  assert_equal(true, 1 && true)

  g:vals = []
  assert_equal(true, Record(true) && Record(1))
  assert_equal([true, 1], g:vals)

  g:vals = []
  assert_equal(false, Record(0) && Record(1))
  assert_equal([0], g:vals)

  g:vals = []
  assert_equal(false, Record(0) && Record(4) && Record(0))
  assert_equal([0], g:vals)

  g:vals = []
  assert_equal(false, Record(1) && Record(true) && Record(0))
  assert_equal([1, true, 0], g:vals)

  g:vals = []
  assert_equal(false, Record(1) && Record(true) && Record(0))
  assert_equal([1, true, 0], g:vals)
enddef

def Test_expr3_vimscript()
  # check line continuation
  var lines =<< trim END
      vim9script
      var name = 0
      		&& 1
      assert_equal(false, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:true
      		&& v:true
      		&& v:true
      assert_equal(v:true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:true &&
      		v:true &&
      		v:true
      assert_equal(v:true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = v:true &&  # comment
                # comment
      		v:true &&
                # comment
      		v:true
      assert_equal(v:true, name)
  END
  CheckScriptSuccess(lines)

  # check white space
  lines =<< trim END
      vim9script
      var name = v:true&&v:true
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true &&v:true
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      var name = v:true&& v:true
  END
  CheckScriptFailure(lines, 'E1004:', 2)

  # check keeping the value
  lines =<< trim END
      vim9script
      assert_equal(false, 1 && 0)
      assert_equal(false, 0 &&
		    0 &&
		    1)
      assert_equal(true, 1
			&& true
			&& 1)
      assert_equal(false, 0 && 0)
      assert_equal(false, 0 && false)
      assert_equal(false, 1 && 0)

      g:vals = []
      assert_equal(true, Record(1) && Record(true))
      assert_equal([1, true], g:vals)

      g:vals = []
      assert_equal(false, Record(0) && Record(1))
      assert_equal([0], g:vals)

      g:vals = []
      assert_equal(false, Record(0) && Record(1) && Record(0))
      assert_equal([0], g:vals)

      g:vals = []
      assert_equal(false, Record(1) && Record(true) && Record(0))
      assert_equal([1, true, 0], g:vals)
  END
  CheckScriptSuccess(lines)
enddef

func Test_expr3_fails()
  let msg = "White space required before and after '&&'"
  call CheckDefFailure(["var x = 1&&2"], msg, 1)
  call CheckDefFailure(["var x = 1 &&2"], msg, 1)
  call CheckDefFailure(["var x = 1&& 2"], msg, 1)
endfunc

" global variables to use for tests with the "any" type
let atrue = v:true
let afalse = v:false
let anone = v:none
let anull = v:null
let anint = 10
let theone = 1
let thefour = 4
if has('float')
  let afloat = 0.1
endif
let astring = 'asdf'
let ablob = 0z01ab
let alist = [2, 3, 4]
let adict = #{aaa: 2, bbb: 8}

" test == comperator
def Test_expr4_equal()
  var trueVar = true
  var falseVar = false
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

  var nr0 = 0
  var nr61 = 61
  assert_equal(false, 2 == 0)
  assert_equal(false, 2 == nr0)
  assert_equal(true, 61 == 61)
  assert_equal(true, 61 == nr61)
  assert_equal(true, g:anint == 10)
  assert_equal(false, 61 == g:anint)

  if has('float')
    var ff = 0.3
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

  CheckDefFailure(["var x = 'a' == xxx"], 'E1001:', 1)
  CheckDefExecFailure(['var items: any', 'eval 1', 'eval 2', 'if items == []', 'endif'], 'E691:', 4)

  var bb = 0z3f
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

  var OneFunc: func
  var TwoFunc: func
  OneFunc = function('len')
  TwoFunc = function('len')
  assert_equal(true, OneFunc('abc') == TwoFunc('123'))
enddef

" test != comperator
def Test_expr4_notequal()
  var trueVar = true
  var falseVar = false
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

  var nr55 = 55
  var nr0 = 55
  assert_equal(true, 2 != 0)
  assert_equal(true, 2 != nr0)
  assert_equal(false, 55 != 55)
  assert_equal(false, 55 != nr55)
  assert_equal(false, g:anint != 10)
  assert_equal(true, 61 != g:anint)

  if has('float')
    var ff = 0.3
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

  var bb = 0z3f
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
  var nr2 = 2
  assert_true(nr2 > 0)
  assert_true(nr2 >
		1)
  assert_false(nr2 > 2)
  assert_false(nr2
  		    > 3)
  if has('float')
    var ff = 2.0
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
  var nr2 = 2
  assert_true(nr2 >= 0)
  assert_true(nr2 >= 2)
  assert_false(nr2 >= 3)
  if has('float')
    var ff = 2.0
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
  var nr2 = 2
  assert_false(nr2 < 0)
  assert_false(nr2 < 2)
  assert_true(nr2 < 3)
  if has('float')
    var ff = 2.0
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
  var nr2 = 2
  assert_false(nr2 <= 0)
  assert_false(nr2 <= 1)
  assert_true(nr2 <= 2)
  assert_true(nr2 <= 3)
  if has('float')
    var ff = 2.0
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
  var mylist = [2]
  assert_false(mylist is [2])
  var other = mylist
  assert_true(mylist is
		other)

  var myblob = 0z1234
  assert_false(myblob
  			is 0z1234)
  var otherblob = myblob
  assert_true(myblob is otherblob)
enddef

" test isnot comperator
def Test_expr4_isnot()
  var mylist = [2]
  assert_true('2' isnot '0')
  assert_true(mylist isnot [2])
  var other = mylist
  assert_false(mylist isnot
			other)

  var myblob = 0z1234
  assert_true(myblob
  		isnot 0z1234)
  var otherblob = myblob
  assert_false(myblob isnot otherblob)
enddef

def RetVoid()
  var x = 1
enddef

def Test_expr4_vim9script()
  # check line continuation
  var lines =<< trim END
      vim9script
      var name = 0
      		< 1
      assert_equal(true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 123
                # comment
      		!= 123
      assert_equal(false, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 123 ==
      			123
      assert_equal(true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var list = [1, 2, 3]
      var name = list
      		is list
      assert_equal(true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var list = [1, 2, 3]
      var name = list # comment
                 # comment
      		is list
      assert_equal(true, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var myblob = 0z1234
      var name = myblob
      		isnot 0z11
      assert_equal(true, name)
  END
  CheckScriptSuccess(lines)

  # spot check mismatching types
  lines =<< trim END
      vim9script
      echo '' == 0
  END
  CheckScriptFailure(lines, 'E1072:', 2)

  lines =<< trim END
      vim9script
      echo v:true > v:false
  END
  CheckScriptFailure(lines, 'Cannot compare bool with bool', 2)

  lines =<< trim END
      vim9script
      echo 123 is 123
  END
  CheckScriptFailure(lines, 'Cannot use "is" with number', 2)

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

  # check missing white space
  lines =<< trim END
    vim9script
    echo 2>3
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
    vim9script
    echo 2 >3
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
    vim9script
    echo 2> 3
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
    vim9script
    echo 2!=3
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
    vim9script
    echo 2 !=3
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
    vim9script
    echo 2!= 3
  END
  CheckScriptFailure(lines, 'E1004:', 2)

  lines =<< trim END
    vim9script
    echo len('xxx') == 3
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    var line = 'abc'
    echo line[1] =~ '\w'
  END
  CheckScriptSuccess(lines)
enddef

func Test_expr4_fails()
  let msg = "White space required before and after '>'"
  call CheckDefFailure(["var x = 1>2"], msg, 1)
  call CheckDefFailure(["var x = 1 >2"], msg, 1)
  call CheckDefFailure(["var x = 1> 2"], msg, 1)

  let msg = "White space required before and after '=='"
  call CheckDefFailure(["var x = 1==2"], msg, 1)
  call CheckDefFailure(["var x = 1 ==2"], msg, 1)
  call CheckDefFailure(["var x = 1== 2"], msg, 1)

  let msg = "White space required before and after 'is'"
  call CheckDefFailure(["var x = '1'is'2'"], msg, 1)
  call CheckDefFailure(["var x = '1' is'2'"], msg, 1)
  call CheckDefFailure(["var x = '1'is '2'"], msg, 1)

  let msg = "White space required before and after 'isnot'"
  call CheckDefFailure(["var x = '1'isnot'2'"], msg, 1)
  call CheckDefFailure(["var x = '1' isnot'2'"], msg, 1)
  call CheckDefFailure(["var x = '1'isnot '2'"], msg, 1)

  call CheckDefFailure(["var x = 1 is# 2"], 'E15:', 1)
  call CheckDefFailure(["var x = 1 is? 2"], 'E15:', 1)
  call CheckDefFailure(["var x = 1 isnot# 2"], 'E15:', 1)
  call CheckDefFailure(["var x = 1 isnot? 2"], 'E15:', 1)

  call CheckDefFailure(["var x = 1 == '2'"], 'Cannot compare number with string', 1)
  call CheckDefFailure(["var x = '1' == 2"], 'Cannot compare string with number', 1)
  call CheckDefFailure(["var x = 1 == RetVoid()"], 'Cannot compare number with void', 1)
  call CheckDefFailure(["var x = RetVoid() == 1"], 'Cannot compare void with number', 1)

  call CheckDefFailure(["var x = true > false"], 'Cannot compare bool with bool', 1)
  call CheckDefFailure(["var x = true >= false"], 'Cannot compare bool with bool', 1)
  call CheckDefFailure(["var x = true < false"], 'Cannot compare bool with bool', 1)
  call CheckDefFailure(["var x = true <= false"], 'Cannot compare bool with bool', 1)
  call CheckDefFailure(["var x = true =~ false"], 'Cannot compare bool with bool', 1)
  call CheckDefFailure(["var x = true !~ false"], 'Cannot compare bool with bool', 1)
  call CheckDefFailure(["var x = true is false"], 'Cannot use "is" with bool', 1)
  call CheckDefFailure(["var x = true isnot false"], 'Cannot use "isnot" with bool', 1)

  call CheckDefFailure(["var x = v:none is v:null"], 'Cannot use "is" with special', 1)
  call CheckDefFailure(["var x = v:none isnot v:null"], 'Cannot use "isnot" with special', 1)
  call CheckDefFailure(["var x = 123 is 123"], 'Cannot use "is" with number', 1)
  call CheckDefFailure(["var x = 123 isnot 123"], 'Cannot use "isnot" with number', 1)
  if has('float')
    call CheckDefFailure(["var x = 1.3 is 1.3"], 'Cannot use "is" with float', 1)
    call CheckDefFailure(["var x = 1.3 isnot 1.3"], 'Cannot use "isnot" with float', 1)
  endif

  call CheckDefFailure(["var x = 0za1 > 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefFailure(["var x = 0za1 >= 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefFailure(["var x = 0za1 < 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefFailure(["var x = 0za1 <= 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefFailure(["var x = 0za1 =~ 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefFailure(["var x = 0za1 !~ 0z34"], 'Cannot compare blob with blob', 1)

  call CheckDefFailure(["var x = [13] > [88]"], 'Cannot compare list with list', 1)
  call CheckDefFailure(["var x = [13] >= [88]"], 'Cannot compare list with list', 1)
  call CheckDefFailure(["var x = [13] < [88]"], 'Cannot compare list with list', 1)
  call CheckDefFailure(["var x = [13] <= [88]"], 'Cannot compare list with list', 1)
  call CheckDefFailure(["var x = [13] =~ [88]"], 'Cannot compare list with list', 1)
  call CheckDefFailure(["var x = [13] !~ [88]"], 'Cannot compare list with list', 1)

  call CheckDefFailure(['var j: job', 'var chan: channel', 'var r = j == chan'], 'Cannot compare job with channel', 3)
  call CheckDefFailure(['var j: job', 'var x: list<any>', 'var r = j == x'], 'Cannot compare job with list', 3)
  call CheckDefFailure(['var j: job', 'var Xx: func', 'var r = j == Xx'], 'Cannot compare job with func', 3)
  call CheckDefFailure(['var j: job', 'var Xx: func', 'var r = j == Xx'], 'Cannot compare job with func', 3)
endfunc

" test addition, subtraction, concatenation
def Test_expr5()
  assert_equal(66, 60 + 6)
  assert_equal(70, 60 +
			g:anint)
  assert_equal(9, g:thefour
  			+ 5)
  assert_equal(14, g:thefour + g:anint)
  assert_equal([1, 2, 3, 4], [1] + g:alist)

  assert_equal(54, 60 - 6)
  assert_equal(50, 60 -
		    g:anint)
  assert_equal(-1, g:thefour
  			- 5)
  assert_equal(-6, g:thefour - g:anint)

  assert_equal('hello', 'hel' .. 'lo')
  assert_equal('hello 123', 'hello ' ..
					123)
  assert_equal('hello 123', 'hello '
  				..  123)
  assert_equal('123 hello', 123 .. ' hello')
  assert_equal('123456', 123 .. 456)

  assert_equal('av:true', 'a' .. true)
  assert_equal('av:false', 'a' .. false)
  assert_equal('av:null', 'a' .. v:null)
  assert_equal('av:none', 'a' .. v:none)
  if has('float')
    assert_equal('a0.123', 'a' .. 0.123)
  endif

  assert_equal([1, 2, 3, 4], [1, 2] + [3, 4])
  assert_equal(0z11223344, 0z1122 + 0z3344)
  assert_equal(0z112201ab, 0z1122
  				+ g:ablob)
  assert_equal(0z01ab3344, g:ablob + 0z3344)
  assert_equal(0z01ab01ab, g:ablob + g:ablob)

  # concatenate non-constant to constant
  var save_path = &path
  &path = 'b'
  assert_equal('ab', 'a' .. &path)
  &path = save_path

  @b = 'b'
  assert_equal('ab', 'a' .. @b)

  $ENVVAR = 'env'
  assert_equal('aenv', 'a' .. $ENVVAR)
enddef

def Test_expr5_vim9script()
  # check line continuation
  var lines =<< trim END
      vim9script
      var name = 11
      		+ 77
		- 22
      assert_equal(66, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 11 +
		  77 -
		  22
      assert_equal(66, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 11 +  # comment
		  77 -
                  # comment
		  22
      assert_equal(66, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 'one'
      		.. 'two'
      assert_equal('onetwo', name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      echo 'abc' is# 'abc'
  END
  CheckScriptFailure(lines, 'E15:', 2)

  lines =<< trim END
      vim9script
      echo 'abc' is? 'abc'
  END
  CheckScriptFailure(lines, 'E15:', 2)

  lines =<< trim END
      vim9script
      echo 'abc' isnot# 'abc'
  END
  CheckScriptFailure(lines, 'E15:', 2)

  lines =<< trim END
      vim9script
      echo 'abc' isnot? 'abc'
  END
  CheckScriptFailure(lines, 'E15:', 2)

  # check white space
  lines =<< trim END
      vim9script
      echo 5+6
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      echo 5 +6
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      echo 5+ 6
  END
  CheckScriptFailure(lines, 'E1004:', 2)

  lines =<< trim END
      vim9script
      echo 'a'..'b'
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      echo 'a' ..'b'
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      echo 'a'.. 'b'
  END
  CheckScriptFailure(lines, 'E1004:', 2)

  # check valid string concatenation
  lines =<< trim END
      vim9script
      assert_equal('one123', 'one' .. 123)
      assert_equal('onev:true', 'one' .. true)
      assert_equal('onev:null', 'one' .. v:null)
      assert_equal('onev:none', 'one' .. v:none)
      if has('float')
        assert_equal('a0.123', 'a' .. 0.123)
      endif
  END
  CheckScriptSuccess(lines)

  # check invalid string concatenation
  lines =<< trim END
      vim9script
      echo 'a' .. [1]
  END
  CheckScriptFailure(lines, 'E730:', 2)
  lines =<< trim END
      vim9script
      echo 'a' .. #{a: 1}
  END
  CheckScriptFailure(lines, 'E731:', 2)
  lines =<< trim END
      vim9script
      echo 'a' .. test_void()
  END
  CheckScriptFailure(lines, 'E908:', 2)
  lines =<< trim END
      vim9script
      echo 'a' .. 0z33
  END
  CheckScriptFailure(lines, 'E976:', 2)
  lines =<< trim END
      vim9script
      echo 'a' .. function('len')
  END
  CheckScriptFailure(lines, 'E729:', 2)
enddef

def Test_expr5_vim9script_channel()
  if !has('channel')
    MissingFeature 'float'
  else
    var lines =<< trim END
        vim9script
        echo 'a' .. test_null_job()
    END
    CheckScriptFailure(lines, 'E908:', 2)
    lines =<< trim END
        vim9script
        echo 'a' .. test_null_channel()
    END
    CheckScriptFailure(lines, 'E908:', 2)
  endif
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
  let msg = "White space required before and after '+'"
  call CheckDefFailure(["var x = 1+2"], msg, 1)
  call CheckDefFailure(["var x = 1 +2"], msg, 1)
  call CheckDefFailure(["var x = 1+ 2"], msg, 1)

  let msg = "White space required before and after '-'"
  call CheckDefFailure(["var x = 1-2"], msg, 1)
  call CheckDefFailure(["var x = 1 -2"], msg, 1)
  call CheckDefFailure(["var x = 1- 2"], msg, 1)

  let msg = "White space required before and after '..'"
  call CheckDefFailure(["var x = '1'..'2'"], msg, 1)
  call CheckDefFailure(["var x = '1' ..'2'"], msg, 1)
  call CheckDefFailure(["var x = '1'.. '2'"], msg, 1)

  call CheckDefFailure(["var x = 0z1122 + 33"], 'E1051', 1)
  call CheckDefFailure(["var x = 0z1122 + [3]"], 'E1051', 1)
  call CheckDefFailure(["var x = 0z1122 + 'asd'"], 'E1051', 1)
  call CheckDefFailure(["var x = 33 + 0z1122"], 'E1051', 1)
  call CheckDefFailure(["var x = [3] + 0z1122"], 'E1051', 1)
  call CheckDefFailure(["var x = 'asdf' + 0z1122"], 'E1051', 1)
  call CheckDefFailure(["var x = 6 + xxx"], 'E1001', 1)

  call CheckDefFailure(["var x = 'a' .. [1]"], 'E1105', 1)
  call CheckDefFailure(["var x = 'a' .. #{a: 1}"], 'E1105', 1)
  call CheckDefFailure(["var x = 'a' .. test_void()"], 'E1105', 1)
  call CheckDefFailure(["var x = 'a' .. 0z32"], 'E1105', 1)
  call CheckDefFailure(["var x = 'a' .. function('len')"], 'E1105', 1)
  call CheckDefFailure(["var x = 'a' .. function('len', ['a'])"], 'E1105', 1)
endfunc

func Test_expr5_fails_channel()
  CheckFeature channel
  call CheckDefFailure(["var x = 'a' .. test_null_job()"], 'E1105', 1)
  call CheckDefFailure(["var x = 'a' .. test_null_channel()"], 'E1105', 1)
endfunc

" test multiply, divide, modulo
def Test_expr6()
  assert_equal(36, 6 * 6)
  assert_equal(24, 6 *
			g:thefour)
  assert_equal(24, g:thefour
  			* 6)
  assert_equal(40, g:anint * g:thefour)

  assert_equal(10, 60 / 6)
  assert_equal(6, 60 /
			g:anint)
  assert_equal(1, g:anint / 6)
  assert_equal(2, g:anint
  			/ g:thefour)

  assert_equal(5, 11 % 6)
  assert_equal(4, g:anint % 6)
  assert_equal(3, 13 %
			g:anint)
  assert_equal(2, g:anint
  			% g:thefour)

  assert_equal(4, 6 * 4 / 6)

  var x = [2]
  var y = [3]
  assert_equal(5, x[0] + y[0])
  assert_equal(6, x[0] * y[0])
  if has('float')
    var xf = [2.0]
    var yf = [3.0]
    assert_equal(5.0, xf[0]
    			+ yf[0])
    assert_equal(6.0, xf[0]
    			* yf[0])
  endif

  CheckDefFailure(["var x = 6 * xxx"], 'E1001', 1)
enddef

def Test_expr6_vim9script()
  # check line continuation
  var lines =<< trim END
      vim9script
      var name = 11
      		* 22
		/ 3
      assert_equal(80, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 25
      		% 10
      assert_equal(5, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 25
                # comment

                # comment
      		% 10
      assert_equal(5, name)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var name = 11 *
      		22 /
		3
      assert_equal(80, name)
  END
  CheckScriptSuccess(lines)

  # check white space
  lines =<< trim END
      vim9script
      echo 5*6
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      echo 5 *6
  END
  CheckScriptFailure(lines, 'E1004:', 2)
  lines =<< trim END
      vim9script
      echo 5* 6
  END
  CheckScriptFailure(lines, 'E1004:', 2)
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
  let msg = "White space required before and after '*'"
  call CheckDefFailure(["var x = 1*2"], msg, 1)
  call CheckDefFailure(["var x = 1 *2"], msg, 1)
  call CheckDefFailure(["var x = 1* 2"], msg, 1)

  let msg = "White space required before and after '/'"
  call CheckDefFailure(["var x = 1/2"], msg, 1)
  call CheckDefFailure(["var x = 1 /2"], msg, 1)
  call CheckDefFailure(["var x = 1/ 2"], msg, 1)

  let msg = "White space required before and after '%'"
  call CheckDefFailure(["var x = 1%2"], msg, 1)
  call CheckDefFailure(["var x = 1 %2"], msg, 1)
  call CheckDefFailure(["var x = 1% 2"], msg, 1)

  call CheckDefFailure(["var x = '1' * '2'"], 'E1036:', 1)
  call CheckDefFailure(["var x = '1' / '2'"], 'E1036:', 1)
  call CheckDefFailure(["var x = '1' % '2'"], 'E1035:', 1)

  call CheckDefFailure(["var x = 0z01 * 0z12"], 'E1036:', 1)
  call CheckDefFailure(["var x = 0z01 / 0z12"], 'E1036:', 1)
  call CheckDefFailure(["var x = 0z01 % 0z12"], 'E1035:', 1)

  call CheckDefFailure(["var x = [1] * [2]"], 'E1036:', 1)
  call CheckDefFailure(["var x = [1] / [2]"], 'E1036:', 1)
  call CheckDefFailure(["var x = [1] % [2]"], 'E1035:', 1)

  call CheckDefFailure(["var x = #{one: 1} * #{two: 2}"], 'E1036:', 1)
  call CheckDefFailure(["var x = #{one: 1} / #{two: 2}"], 'E1036:', 1)
  call CheckDefFailure(["var x = #{one: 1} % #{two: 2}"], 'E1035:', 1)

  call CheckDefFailure(["var x = 0xff[1]"], 'E1107:', 1)
  if has('float')
    call CheckDefFailure(["var x = 0.7[1]"], 'E1107:', 1)
  endif
endfunc

func Test_expr6_float_fails()
  CheckFeature float
  call CheckDefFailure(["var x = 1.0 % 2"], 'E1035:', 1)
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

" type casts
def Test_expr7t()
  var ls: list<string> = ['a', <string>g:string_empty]
  var ln: list<number> = [<number>g:anint, <number>g:thefour]
  var nr = <number>234
  assert_equal(234, nr)

  CheckDefFailure(["var x = <nr>123"], 'E1010:', 1)
  CheckDefFailure(["var x = <number >123"], 'E1068:', 1)
  CheckDefFailure(["var x = <number 123"], 'E1104:', 1)
enddef

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

  CheckDefFailure(["var x = 0z123"], 'E973:', 1)
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

  CheckDefFailure(['var x = "abc'], 'E114:', 1)
  CheckDefFailure(["var x = 'abc"], 'E115:', 1)
enddef

def Test_expr7_vimvar()
  var old: list<string> = v:oldfiles
  var compl: dict<any> = v:completed_item

  CheckDefFailure(["var old: list<number> = v:oldfiles"], 'E1012: Type mismatch; expected list<number> but got list<string>', 1)
  new
  exec "normal! afoo fo\<C-N>\<Esc>"
  CheckDefExecFailure(["var old: dict<number> = v:completed_item"], 'E1012: Type mismatch; expected dict<number> but got dict<string>', 1)
  bwipe!
enddef

def Test_expr7_special()
  # special constant
  assert_equal(g:special_true, true)
  assert_equal(g:special_false, false)
  assert_equal(g:special_true, v:true)
  assert_equal(g:special_false, v:false)

  assert_equal(true, !false)
  assert_equal(false, !true)
  assert_equal(true, !0)
  assert_equal(false, !1)
  assert_equal(false, !!false)
  assert_equal(true, !!true)
  assert_equal(false, !!0)
  assert_equal(true, !!1)

  assert_equal(g:special_null, v:null)
  assert_equal(g:special_none, v:none)

  CheckDefFailure(['v:true = true'], 'E46:', 1)
  CheckDefFailure(['v:true = false'], 'E46:', 1)
  CheckDefFailure(['v:false = true'], 'E46:', 1)
  CheckDefFailure(['v:null = 11'], 'E46:', 1)
  CheckDefFailure(['v:none = 22'], 'E46:', 1)
enddef

def Test_expr7_special_vim9script()
  var lines =<< trim END
      vim9script
      var t = true
      var f = false
      assert_equal(v:true, true)
      assert_equal(true, t)
      assert_equal(v:false, false)
      assert_equal(false, f)
      assert_equal(true, !false)
      assert_equal(false, !true)
      assert_equal(true, !0)
      assert_equal(false, !1)
      assert_equal(false, !!false)
      assert_equal(true, !!true)
      assert_equal(false, !!0)
      assert_equal(true, !!1)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_list()
  # list
  assert_equal(g:list_empty, [])
  assert_equal(g:list_empty, [  ])

  var numbers: list<number> = [1, 2, 3]
  numbers = [1]
  numbers = []

  var strings: list<string> = ['a', 'b', 'c']
  strings = ['x']
  strings = []

  var mixed: list<any> = [1, 'b', false,]
  assert_equal(g:list_mixed, mixed)
  assert_equal('b', mixed[1])

  echo [1,
  	2] [3,
		4]

  var llstring: list<list<string>> = [['text'], []]
  llstring = [[], ['text']]
  llstring = [[], []]

  var rangelist: list<number> = range(3)
  g:rangelist = range(3)
  CheckDefExecFailure(["var x: list<string> = g:rangelist"], 'E1012: Type mismatch; expected list<string> but got list<number>', 1)

  CheckDefFailure(["var x = 1234[3]"], 'E1107:', 1)
  CheckDefExecFailure(["var x = g:anint[3]"], 'E1062:', 1)

  CheckDefFailure(["var x = g:list_mixed[xxx]"], 'E1001:', 1)

  CheckDefFailure(["var x = [1,2,3]"], 'E1069:', 1)
  CheckDefFailure(["var x = [1 ,2, 3]"], 'E1068:', 1)

  CheckDefExecFailure(["echo 1", "var x = [][0]", "echo 3"], 'E684:', 2)

  CheckDefExecFailure(["var x = g:list_mixed['xx']"], 'E1012:', 1)
  CheckDefFailure(["var x = g:list_mixed["], 'E1097:', 2)
  CheckDefFailure(["var x = g:list_mixed[0"], 'E1097:', 2)
  CheckDefExecFailure(["var x = g:list_empty[3]"], 'E684:', 1)
  CheckDefExecFailure(["var l: list<number> = [234, 'x']"], 'E1012:', 1)
  CheckDefExecFailure(["var l: list<number> = ['x', 234]"], 'E1012:', 1)
  CheckDefExecFailure(["var l: list<string> = [234, 'x']"], 'E1012:', 1)
  CheckDefExecFailure(["var l: list<string> = ['x', 123]"], 'E1012:', 1)

  var lines =<< trim END
      vim9script
      var datalist: list<string>
      def Main()
        datalist += ['x'.
      enddef
      Main()
  END
  CheckScriptFailure(lines, 'E1127:')
enddef

def Test_expr7_list_vim9script()
  var lines =<< trim END
      vim9script
      var l = [
		11,
		22,
		]
      assert_equal([11, 22], l)

      echo [1,
	    2] [3,
		    4]

      echo [1, # comment
            # comment
	    2] [3,
            # comment
		    4]
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var l = [11,
		22]
      assert_equal([11, 22], l)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var l = [11,22]
  END
  CheckScriptFailure(lines, 'E1069:', 2)

  lines =<< trim END
      vim9script
      var l = [11 , 22]
  END
  CheckScriptFailure(lines, 'E1068:', 2)

  lines =<< trim END
    vim9script
    var l: list<number> = [234, 'x']
  END
  CheckScriptFailure(lines, 'E1012:', 2)
  lines =<< trim END
    vim9script
    var l: list<number> = ['x', 234]
  END
  CheckScriptFailure(lines, 'E1012:', 2)
  lines =<< trim END
    vim9script
    var l: list<string> = ['x', 234]
  END
  CheckScriptFailure(lines, 'E1012:', 2)
  lines =<< trim END
    vim9script
    var l: list<string> = [234, 'x']
  END
  CheckScriptFailure(lines, 'E1012:', 2)
enddef

def LambdaWithComments(): func
  return {x ->
            # some comment
            x == 1
            # some comment
            ||
            x == 2
        }
enddef

def LambdaUsingArg(x: number): func
  return {->
            # some comment
            x == 1
            # some comment
            ||
            x == 2
        }
enddef

def Test_expr7_lambda()
  var La = { -> 'result'}
  assert_equal('result', La())
  assert_equal([1, 3, 5], [1, 2, 3]->map({key, val -> key + val}))

  # line continuation inside lambda with "cond ? expr : expr" works
  var ll = range(3)
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

  var dl = [{'key': 0}, {'key': 22}]->filter({ _, v -> v['key'] })
  assert_equal([{'key': 22}], dl)

  dl = [{'key': 12}, {'foo': 34}]
  assert_equal([{'key': 12}], filter(dl,
	{_, v -> has_key(v, 'key') ? v['key'] == 12 : 0}))

  assert_equal(false, LambdaWithComments()(0))
  assert_equal(true, LambdaWithComments()(1))
  assert_equal(true, LambdaWithComments()(2))
  assert_equal(false, LambdaWithComments()(3))

  assert_equal(false, LambdaUsingArg(0)())
  assert_equal(true, LambdaUsingArg(1)())

  CheckDefFailure(["filter([1, 2], {k,v -> 1})"], 'E1069:', 1)
  # error is in first line of the lambda
  CheckDefFailure(["var L = {a -> a + b}"], 'E1001:', 0)

  assert_equal('xxxyyy', 'xxx'->{a, b -> a .. b}('yyy'))

  CheckDefExecFailure(["var s = 'asdf'->{a -> a}('x')"],
        'E1106: One argument too many')
  CheckDefExecFailure(["var s = 'asdf'->{a -> a}('x', 'y')"],
        'E1106: 2 arguments too many')
  CheckDefFailure(["echo 'asdf'->{a -> a}(x)"], 'E1001:', 1)
enddef

def Test_expr7_lambda_vim9script()
  var lines =<< trim END
      vim9script
      var v = 10->{a ->
	    a
	      + 2
	  }()
      assert_equal(12, v)
  END
  CheckScriptSuccess(lines)

  # nested lambda with line breaks
  lines =<< trim END
      vim9script
      search('"', 'cW', 0, 0, {->
	synstack('.', col('.'))
	->map({_, v -> synIDattr(v, 'name')})->len()})
  END
  CheckScriptSuccess(lines)
enddef

def Test_epxr7_funcref()
  var lines =<< trim END
    def RetNumber(): number
      return 123
    enddef
    var FuncRef = RetNumber
    assert_equal(123, FuncRef())
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_dict()
  # dictionary
  assert_equal(g:dict_empty, {})
  assert_equal(g:dict_empty, {  })
  assert_equal(g:dict_one, {'one': 1})
  var key = 'one'
  var val = 1
  assert_equal(g:dict_one, {key: val})

  var numbers: dict<number> = #{a: 1, b: 2, c: 3}
  numbers = #{a: 1}
  numbers = #{}

  var strings: dict<string> = #{a: 'a', b: 'b', c: 'c'}
  strings = #{a: 'x'}
  strings = #{}

  var mixed: dict<any> = #{a: 'a', b: 42}
  mixed = #{a: 'x'}
  mixed = #{a: 234}
  mixed = #{}

  var dictlist: dict<list<string>> = #{absent: [], present: ['hi']}
  dictlist = #{absent: ['hi'], present: []}
  dictlist = #{absent: [], present: []}

  var dictdict: dict<dict<string>> = #{one: #{a: 'text'}, two: #{}}
  dictdict = #{one: #{}, two: #{a: 'text'}}
  dictdict = #{one: #{}, two: #{}}
 
  CheckDefFailure(["var x = #{a:8}"], 'E1069:', 1)
  CheckDefFailure(["var x = #{a : 8}"], 'E1068:', 1)
  CheckDefFailure(["var x = #{a :8}"], 'E1068:', 1)
  CheckDefFailure(["var x = #{a: 8 , b: 9}"], 'E1068:', 1)

  CheckDefFailure(["var x = #{8: 8}"], 'E1014:', 1)
  CheckDefFailure(["var x = #{xxx}"], 'E720:', 1)
  CheckDefFailure(["var x = #{xxx: 1", "var y = 2"], 'E722:', 2)
  CheckDefFailure(["var x = #{xxx: 1,"], 'E723:', 2)
  CheckDefFailure(["var x = {'a': xxx}"], 'E1001:', 1)
  CheckDefFailure(["var x = {xxx: 8}"], 'E1001:', 1)
  CheckDefFailure(["var x = #{a: 1, a: 2}"], 'E721:', 1)
  CheckDefFailure(["var x = #"], 'E1015:', 1)
  CheckDefExecFailure(["var x = g:anint.member"], 'E715:', 1)
  CheckDefExecFailure(["var x = g:dict_empty.member"], 'E716:', 1)

  CheckDefExecFailure(['var x: dict<number> = #{a: 234, b: "1"}'], 'E1012:', 1)
  CheckDefExecFailure(['var x: dict<number> = #{a: "x", b: 134}'], 'E1012:', 1)
  CheckDefExecFailure(['var x: dict<string> = #{a: 234, b: "1"}'], 'E1012:', 1)
  CheckDefExecFailure(['var x: dict<string> = #{a: "x", b: 134}'], 'E1012:', 1)

  CheckDefFailure(['var x = ({'], 'E723:', 2)
enddef

def Test_expr7_dict_vim9script()
  var lines =<< trim END
      vim9script
      var d = {
		'one':
		   1,
		'two': 2,
		   }
      assert_equal({'one': 1, 'two': 2}, d)

      d = {  # comment
		'one':
                # comment

		   1,
                # comment
                # comment
		'two': 2,
		   }
      assert_equal({'one': 1, 'two': 2}, d)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var d = { "one": "one", "two": "two", }
      assert_equal({'one': 'one', 'two': 'two'}, d)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var d = #{one: 1,
		two: 2,
	       }
      assert_equal({'one': 1, 'two': 2}, d)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var d = #{one:1, two: 2}
  END
  CheckScriptFailure(lines, 'E1069:', 2)

  lines =<< trim END
      vim9script
      var d = #{one: 1,two: 2}
  END
  CheckScriptFailure(lines, 'E1069:', 2)

  lines =<< trim END
      vim9script
      var d = #{one : 1}
  END
  CheckScriptFailure(lines, 'E1068:', 2)

  lines =<< trim END
      vim9script
      var d = #{one:1}
  END
  CheckScriptFailure(lines, 'E1069:', 2)

  lines =<< trim END
      vim9script
      var d = #{one: 1 , two: 2}
  END
  CheckScriptFailure(lines, 'E1068:', 2)

  lines =<< trim END
    vim9script
    var l: dict<number> = #{a: 234, b: 'x'}
  END
  CheckScriptFailure(lines, 'E1012:', 2)
  lines =<< trim END
    vim9script
    var l: dict<number> = #{a: 'x', b: 234}
  END
  CheckScriptFailure(lines, 'E1012:', 2)
  lines =<< trim END
    vim9script
    var l: dict<string> = #{a: 'x', b: 234}
  END
  CheckScriptFailure(lines, 'E1012:', 2)
  lines =<< trim END
    vim9script
    var l: dict<string> = #{a: 234, b: 'x'}
  END
  CheckScriptFailure(lines, 'E1012:', 2)
enddef

let g:oneString = 'one'

def Test_expr_member()
  assert_equal(1, g:dict_one.one)
  var d: dict<number> = g:dict_one
  assert_equal(1, d['one'])
  assert_equal(1, d[
		  'one'
		  ])
  assert_equal(1, d
  	.one)
  d = {'1': 1, '_': 2}
  assert_equal(1, d
  	.1)
  assert_equal(2, d
  	._)

  # getting the one member should clear the dict after getting the item
  assert_equal('one', #{one: 'one'}.one)
  assert_equal('one', #{one: 'one'}[g:oneString])

  CheckDefFailure(["var x = g:dict_one.#$!"], 'E1002:', 1)
  CheckDefExecFailure(["var d: dict<any>", "echo d['a']"], 'E716:', 2)
  CheckDefExecFailure(["var d: dict<number>", "d = g:list_empty"], 'E1012: Type mismatch; expected dict<number> but got list<unknown>', 2)
enddef

def Test_expr7_any_index_slice()
  var lines =<< trim END
    # getting the one member should clear the list only after getting the item
    assert_equal('bbb', ['aaa', 'bbb', 'ccc'][1])

    # string is permissive, index out of range accepted
    g:teststring = 'abcdef'
    assert_equal('b', g:teststring[1])
    assert_equal('', g:teststring[-1])
    assert_equal('', g:teststring[99])

    assert_equal('b', g:teststring[1:1])
    assert_equal('bcdef', g:teststring[1:])
    assert_equal('abcd', g:teststring[:3])
    assert_equal('cdef', g:teststring[-4:])
    assert_equal('abcdef', g:teststring[-9:])
    assert_equal('abcd', g:teststring[:-3])
    assert_equal('', g:teststring[:-9])

    # blob index cannot be out of range
    g:testblob = 0z01ab
    assert_equal(0x01, g:testblob[0])
    assert_equal(0xab, g:testblob[1])
    assert_equal(0xab, g:testblob[-1])
    assert_equal(0x01, g:testblob[-2])

    # blob slice accepts out of range
    assert_equal(0z01ab, g:testblob[0:1])
    assert_equal(0z01, g:testblob[0:0])
    assert_equal(0z01, g:testblob[-2:-2])
    assert_equal(0zab, g:testblob[1:1])
    assert_equal(0zab, g:testblob[-1:-1])
    assert_equal(0z, g:testblob[2:2])
    assert_equal(0z, g:testblob[0:-3])

    # list index cannot be out of range
    g:testlist = [0, 1, 2, 3]
    assert_equal(0, g:testlist[0])
    assert_equal(1, g:testlist[1])
    assert_equal(3, g:testlist[3])
    assert_equal(3, g:testlist[-1])
    assert_equal(0, g:testlist[-4])
    assert_equal(1, g:testlist[g:theone])

    # list slice accepts out of range
    assert_equal([0], g:testlist[0:0])
    assert_equal([3], g:testlist[3:3])
    assert_equal([0, 1], g:testlist[0:1])
    assert_equal([0, 1, 2, 3], g:testlist[0:3])
    assert_equal([0, 1, 2, 3], g:testlist[0:9])
    assert_equal([], g:testlist[-1:1])
    assert_equal([1], g:testlist[-3:1])
    assert_equal([0, 1], g:testlist[-4:1])
    assert_equal([0, 1], g:testlist[-9:1])
    assert_equal([1, 2, 3], g:testlist[1:-1])
    assert_equal([1], g:testlist[1:-3])
    assert_equal([], g:testlist[1:-4])
    assert_equal([], g:testlist[1:-9])

    g:testdict = #{a: 1, b: 2}
    assert_equal(1, g:testdict['a'])
    assert_equal(2, g:testdict['b'])
  END

  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)

  CheckDefExecFailure(['echo g:testblob[2]'], 'E979:', 1)
  CheckScriptFailure(['vim9script', 'echo g:testblob[2]'], 'E979:', 2)
  CheckDefExecFailure(['echo g:testblob[-3]'], 'E979:', 1)
  CheckScriptFailure(['vim9script', 'echo g:testblob[-3]'], 'E979:', 2)

  CheckDefExecFailure(['echo g:testlist[4]'], 'E684:', 1)
  CheckScriptFailure(['vim9script', 'echo g:testlist[4]'], 'E684:', 2)
  CheckDefExecFailure(['echo g:testlist[-5]'], 'E684:', 1)
  CheckScriptFailure(['vim9script', 'echo g:testlist[-5]'], 'E684:', 2)

  CheckDefExecFailure(['echo g:testdict["a":"b"]'], 'E719:', 1)
  CheckScriptFailure(['vim9script', 'echo g:testdict["a":"b"]'], 'E719:', 2)
  CheckDefExecFailure(['echo g:testdict[1]'], 'E716:', 1)
  CheckScriptFailure(['vim9script', 'echo g:testdict[1]'], 'E716:', 2)

  unlet g:teststring
  unlet g:testblob
  unlet g:testlist
enddef

def Test_expr_member_vim9script()
  var lines =<< trim END
      vim9script
      var d = #{one:
      		'one',
		two: 'two',
		1: 1,
		_: 2}
      assert_equal('one', d.one)
      assert_equal('one', d
                            .one)
      assert_equal(1, d
                            .1)
      assert_equal(2, d
                            ._)
      assert_equal('one', d[
			    'one'
			    ])
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var l = [1,
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

  CheckDefFailure(["var x = $$$"], 'E1002:', 1)
enddef

def Test_expr7_register()
  @a = 'register a'
  assert_equal('register a', @a)

  var fname = expand('%')
  assert_equal(fname, @%)

  feedkeys(":echo 'some'\<CR>", "xt")
  assert_equal("echo 'some'", @:)

  normal axyz
  assert_equal("xyz", @.)
  CheckDefFailure(["@. = 'yes'"], 'E354:', 1)

  @/ = 'slash'
  assert_equal('slash', @/)

  @= = 'equal'
  assert_equal('equal', @=)
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
  var lines =<< trim END
      vim9script
      var s = (
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
  var nr = 88
  assert_equal(-88, -nr)
  assert_equal(88, --nr)
enddef

def Echo(arg: any): string
  return arg
enddef

def s:Echo4Arg(arg: any): string
  return arg
enddef

def Test_expr7_call()
  assert_equal('yes', 'yes'->Echo())
  assert_equal('yes', 'yes'
  			->s:Echo4Arg())
  assert_equal(true, !range(5)->empty())
  assert_equal([0, 1, 2], --3->range())

  CheckDefFailure(["var x = 'yes'->Echo"], 'E107:', 1)
  CheckScriptFailure([
   "vim9script",
   "var x = substitute ('x', 'x', 'x', 'x')"
   ], 'E121:', 2)
  CheckDefFailure(["var Ref = function('len' [1, 2])"], 'E1123:', 1)

  var auto_lines =<< trim END
      def g:some#func(): string
	return 'found'
      enddef
  END
  mkdir('Xruntime/autoload', 'p')
  writefile(auto_lines, 'Xruntime/autoload/some.vim')
  var save_rtp = &rtp
  &rtp = getcwd() .. '/Xruntime,' .. &rtp
  assert_equal('found', g:some#func())
  assert_equal('found', some#func())

  &rtp = save_rtp
  delete('Xruntime', 'rf')
enddef


def Test_expr7_not()
  var lines =<< trim END
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

      assert_equal(false, ![1, 2, 3]->reverse())
      assert_equal(true, ![]->reverse())
  END
  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)
enddef

func Test_expr7_fails()
  call CheckDefFailure(["var x = (12"], "E110:", 1)

  call CheckDefFailure(["var x = -'xx'"], "E1030:", 1)
  call CheckDefFailure(["var x = +'xx'"], "E1030:", 1)
  call CheckDefFailure(["var x = -0z12"], "E974:", 1)
  call CheckDefExecFailure(["var x = -[8]"], "E39:", 1)
  call CheckDefExecFailure(["var x = -{'a': 1}"], "E39:", 1)

  call CheckDefFailure(["var x = @"], "E1002:", 1)
  call CheckDefFailure(["var x = @<"], "E354:", 1)

  call CheckDefFailure(["var x = [1, 2"], "E697:", 2)
  call CheckDefFailure(["var x = [notfound]"], "E1001:", 1)

  call CheckDefFailure(["var x = { -> 123) }"], "E451:", 1)
  call CheckDefFailure(["var x = 123->{x -> x + 5) }"], "E451:", 1)

  call CheckDefFailure(["var x = &notexist"], 'E113:', 1)
  call CheckDefFailure(["&grepprg = [343]"], 'E1012:', 1)

  call CheckDefExecFailure(["echo s:doesnt_exist"], 'E121:', 1)
  call CheckDefExecFailure(["echo g:doesnt_exist"], 'E121:', 1)

  call CheckDefFailure(["echo a:somevar"], 'E1075:', 1)
  call CheckDefFailure(["echo l:somevar"], 'E1075:', 1)
  call CheckDefFailure(["echo x:somevar"], 'E1075:', 1)

  call CheckDefExecFailure(["var x = +g:astring"], 'E1030:', 1)
  call CheckDefExecFailure(["var x = +g:ablob"], 'E974:', 1)
  call CheckDefExecFailure(["var x = +g:alist"], 'E745:', 1)
  call CheckDefExecFailure(["var x = +g:adict"], 'E728:', 1)

  call CheckDefFailure(["var x = ''", "var y = x.memb"], 'E715:', 2)

  call CheckDefFailure(["'yes'->", "Echo()"], 'E488: Trailing characters: ->', 1)

  call CheckDefExecFailure(["[1, 2->len()"], 'E697:', 2)
  call CheckDefExecFailure(["#{a: 1->len()"], 'E488:', 1)
  call CheckDefExecFailure(["{'a': 1->len()"], 'E723:', 2)
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
  var Part = function('g:CallMe')
  assert_equal('yes', Part('yes'))

  # funcref call, using list index
  var l = []
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
  var d = #{key: 123}
  assert_equal(123, d.key)
enddef

def Test_expr7_string_subscript()
  var lines =<< trim END
    var text = 'abcdef'
    assert_equal('', text[-1])
    assert_equal('a', text[0])
    assert_equal('e', text[4])
    assert_equal('f', text[5])
    assert_equal('', text[6])

    text = 'ábçdëf'
    assert_equal('', text[-999])
    assert_equal('', text[-1])
    assert_equal('á', text[0])
    assert_equal('b', text[1])
    assert_equal('ç', text[2])
    assert_equal('d', text[3])
    assert_equal('ë', text[4])
    assert_equal('f', text[5])
    assert_equal('', text[6])
    assert_equal('', text[999])

    assert_equal('ábçdëf', text[0:-1])
    assert_equal('ábçdëf', text[0 :-1])
    assert_equal('ábçdëf', text[0: -1])
    assert_equal('ábçdëf', text[0 : -1])
    assert_equal('ábçdëf', text[0
                  :-1])
    assert_equal('ábçdëf', text[0:
                  -1])
    assert_equal('ábçdëf', text[0 : -1
                  ])
    assert_equal('bçdëf', text[1:-1])
    assert_equal('çdëf', text[2:-1])
    assert_equal('dëf', text[3:-1])
    assert_equal('ëf', text[4:-1])
    assert_equal('f', text[5:-1])
    assert_equal('', text[6:-1])
    assert_equal('', text[999:-1])

    assert_equal('ábçd', text[:3])
    assert_equal('bçdëf', text[1:])
    assert_equal('ábçdëf', text[:])
  END
  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)
enddef

def Test_expr7_list_subscript()
  var lines =<< trim END
    var list = [0, 1, 2, 3, 4]
    assert_equal(0, list[0])
    assert_equal(4, list[4])
    assert_equal(4, list[-1])
    assert_equal(0, list[-5])

    assert_equal([0, 1, 2, 3, 4], list[0:4])
    assert_equal([0, 1, 2, 3, 4], list[:])
    assert_equal([1, 2, 3, 4], list[1:])
    assert_equal([2, 3, 4], list[2:-1])
    assert_equal([4], list[4:-1])
    assert_equal([], list[5:-1])
    assert_equal([], list[999:-1])
    assert_equal([1, 2, 3, 4], list[g:theone:g:thefour])

    assert_equal([0, 1, 2, 3], list[0:3])
    assert_equal([0], list[0:0])
    assert_equal([0, 1, 2, 3, 4], list[0:-1])
    assert_equal([0, 1, 2], list[0:-3])
    assert_equal([0], list[0:-5])
    assert_equal([], list[0:-6])
    assert_equal([], list[0:-99])
  END
  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)

  lines = ['var l = [0, 1, 2]', 'echo l[g:astring : g:theone]']
  CheckDefExecFailure(lines, 'E1012:')
  CheckScriptFailure(['vim9script'] + lines, 'E1030:', 3)
enddef

def Test_expr7_dict_subscript()
  var lines =<< trim END
      vim9script
      var l = [#{lnum: 2}, #{lnum: 1}]
      var res = l[0].lnum > l[1].lnum
      assert_true(res)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_subscript_linebreak()
  var range = range(
  		3)
  var l = range
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

  var d = #{one: 33}
  assert_equal(33, d.
	one)
enddef

def Test_expr7_method_call()
  new
  setline(1, ['first', 'last'])
  'second'->append(1)
  "third"->append(2)
  assert_equal(['first', 'second', 'third', 'last'], getline(1, '$'))
  bwipe!

  var bufnr = bufnr()
  var loclist = [#{bufnr: bufnr, lnum: 42, col: 17, text: 'wrong'}]
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

  var result: bool = get(#{n: 0}, 'n', 0)
  assert_equal(false, result)
enddef

func Test_expr7_trailing_fails()
  call CheckDefFailure(['var l = [2]', 'l->{l -> add(l, 8)}'], 'E107:', 2)
  call CheckDefFailure(['var l = [2]', 'l->{l -> add(l, 8)} ()'], 'E274:', 2)
endfunc

func Test_expr_fails()
  call CheckDefFailure(["var x = '1'is2"], 'E488:', 1)
  call CheckDefFailure(["var x = '1'isnot2"], 'E488:', 1)

  call CheckDefFailure(["CallMe ('yes')"], 'E476:', 1)
  call CheckScriptFailure(["CallMe ('yes')"], 'E492:', 1)
  call CheckDefAndScriptFailure(["CallMe2('yes','no')"], 'E1069:', 1)
  call CheckDefFailure(["CallMe2('yes' , 'no')"], 'E1068:', 1)

  call CheckDefFailure(["v:nosuch += 3"], 'E1001:', 1)
  call CheckDefFailure(["var v:statusmsg = ''"], 'E1016: Cannot declare a v: variable:', 1)
  call CheckDefFailure(["var asdf = v:nosuch"], 'E1001:', 1)

  call CheckDefFailure(["echo len('asdf'"], 'E110:', 2)
  call CheckDefFailure(["echo Func0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789()"], 'E1011:', 1)
  call CheckDefFailure(["echo doesnotexist()"], 'E117:', 1)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
