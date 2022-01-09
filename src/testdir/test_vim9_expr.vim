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
  var lines =<< trim END
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
      assert_equal('one', !!{x: 0} ? 'one' : 'two')
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

      echo ['a'] + (1 ? ['b'] : ['c']
                )
      echo ['a'] + (1 ? ['b'] : ['c'] # comment
                )

      # with constant condition expression is not evaluated 
      assert_equal('one', 1 ? 'one' : xxx)

      var Some: func = function('len')
      var Other: func = function('winnr')
      var Res: func = g:atrue ? Some : Other
      assert_equal(function('len'), Res)

      var RetOne: func(string): number = function('len')
      var RetTwo: func(string): number = function('charcol')
      var RetThat: func = g:atrue ? RetOne : RetTwo
      assert_equal(function('len'), RetThat)

      var X = FuncOne
      var Y = FuncTwo
      var Z = g:cond ? FuncOne : FuncTwo
      assert_equal(123, Z(3))
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr1_trinary_vimscript()
  # check line continuation
  var lines =<< trim END
      var name = 1
      		? 'yes'
		: 'no'
      assert_equal('yes', name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:false
      		? 'yes'
		: 'no'
      assert_equal('no', name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:false ?
      		'yes' :
		'no'
      assert_equal('no', name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:false ?  # comment
      		'yes' :
                # comment
		'no' # comment
      assert_equal('no', name)
  END
  CheckDefAndScriptSuccess(lines)

  # check white space
  lines =<< trim END
      var name = v:true?1:2
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''?'' at "?1:2"', 1)

  lines =<< trim END
      var name = v:true? 1 : 2
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      var name = v:true ?1 : 2
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      var name = v:true ? 1: 2
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after '':'' at ": 2"', 1)

  lines =<< trim END
      var name = v:true ? 1 :2
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      var name = 'x' ? 1 : 2
  END
  CheckDefAndScriptFailure(lines, 'E1135:', 1)

  lines =<< trim END
      var name = [] ? 1 : 2
  END
  CheckDefExecAndScriptFailure(lines, 'E745:', 1)

  lines =<< trim END
      var name = {} ? 1 : 2
  END
  CheckDefExecAndScriptFailure(lines, 'E728:', 1)

  # check after failure eval_flags is reset
  lines =<< trim END
      try
        eval('0 ? 1: 2')
      catch
      endtry
      assert_equal(v:true, eval(string(v:true)))
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      try
        eval('0 ? 1 :2')
      catch
      endtry
      assert_equal(v:true, eval(string(v:true)))
  END
  CheckDefAndScriptSuccess(lines)
enddef

func Test_expr1_trinary_fails()
  call CheckDefAndScriptFailure(["var x = 1 ? 'one'"], "Missing ':' after '?'", 1)

  let msg = "White space required before and after '?'"
  call CheckDefAndScriptFailure(["var x = 1? 'one' : 'two'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 ?'one' : 'two'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1?'one' : 'two'"], msg, 1)
  let lines =<< trim END
    var x = 1
     ?'one' : 'two'
     # comment
  END
  call CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''?'' at "?''one'' : ''two''"', 2)

  let msg = "White space required before and after ':'"
  call CheckDefAndScriptFailure(["var x = 1 ? 'one': 'two'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 ? 'one' :'two'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 ? 'one':'two'"], msg, 1)
  let lines =<< trim END
    var x = 1 ? 'one'
          :'two'
          # Comment
  END
  call CheckDefAndScriptFailure(lines, 'E1004: White space required before and after '':'' at ":''two''"', 2)

  call CheckDefAndScriptFailure(["var x = 'x' ? 'one' : 'two'"], 'E1135:', 1)
  call CheckDefAndScriptFailure(["var x = 0z1234 ? 'one' : 'two'"], 'E974:', 1)
  call CheckDefExecAndScriptFailure(["var x = [] ? 'one' : 'two'"], 'E745:', 1)
  call CheckDefExecAndScriptFailure(["var x = {} ? 'one' : 'two'"], 'E728:', 1)

  call CheckDefExecFailure(["var x = false ? "], 'E1097:', 3)
  call CheckScriptFailure(['vim9script', "var x = false ? "], 'E15:', 2)
  call CheckDefExecFailure(["var x = false ? 'one' : "], 'E1097:', 3)
  call CheckScriptFailure(['vim9script', "var x = false ? 'one' : "], 'E15:', 2)

  call CheckDefExecAndScriptFailure(["var x = true ? xxx : 'foo'"], ['E1001:', 'E121:'], 1)
  call CheckDefExecAndScriptFailure(["var x = false ? 'foo' : xxx"], ['E1001:', 'E121:'], 1)

  if has('float')
    call CheckDefAndScriptFailure(["var x = 0.1 ? 'one' : 'two'"], 'E805:', 1)
  endif

  " missing argument detected even when common type is used
  call CheckDefAndScriptFailure([
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
      assert_equal({one: 1}, {one: 1} ?? 456)
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
  call CheckDefAndScriptFailure(["var x = 1?? 'one' : 'two'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 ??'one' : 'two'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1??'one' : 'two'"], msg, 1)
  lines =<< trim END
    var x = 1
      ??'one' : 'two'
      #comment
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''??'' at "??''one'' : ''two''"', 2)
enddef

def Record(val: any): any
  g:vals->add(val)
  return val
enddef

" test ||
def Test_expr2()
  var lines =<< trim END
      assert_equal(true, 1 || 0)
      assert_equal(true, 0 ||
                        0 ||
                        1)
      assert_equal(true, 0 ||
			0 ||
			!!7)
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
      assert_equal(true, Record(0) || Record(true))
      assert_equal([0, true], g:vals)

      g:vals = []
      assert_equal(true, Record(0)
                          || Record(1)
                          || Record(0))
      assert_equal([0, 1], g:vals)

      g:vals = []
      assert_equal(true, Record(0)
			  || Record(true)
			  || Record(0))
      assert_equal([0, true], g:vals)

      g:vals = []
      assert_equal(true, Record(true) || Record(false))
      assert_equal([true], g:vals)

      g:vals = []
      assert_equal(false, Record(0) || Record(false) || Record(0))
      assert_equal([0, false, 0], g:vals)

      g:vals = []
      var x = 1
      if x || true
        g:vals = [1]
      endif
      assert_equal([1], g:vals)

      g:vals = []
      x = 3
      if true || x
        g:vals = [1]
      endif
      assert_equal([1], g:vals)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr2_vimscript()
  # check line continuation
  var lines =<< trim END
      var name = 0
      		|| 1
      assert_equal(true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:false
      		|| v:true
      		|| v:false
      assert_equal(v:true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:false ||
      		v:true ||
		v:false
      assert_equal(v:true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:false || # comment
                # comment
      		v:true ||
                # comment
		v:false # comment
      assert_equal(v:true, name)
  END
  CheckDefAndScriptSuccess(lines)

  # check white space
  lines =<< trim END
      var name = v:true||v:true
  END
  CheckDefExecAndScriptFailure(lines, 'E1004: White space required before and after ''||'' at "||v:true"', 1)

  lines =<< trim END
      var name = v:true ||v:true
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      var name = v:true|| v:true
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)
enddef

def Test_expr2_fails()
  var msg = "White space required before and after '||'"
  call CheckDefAndScriptFailure(["var x = 1||0"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 ||0"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1|| 0"], msg, 1)

  call CheckDefFailure(["var x = false || "], 'E1097:', 3)
  call CheckScriptFailure(['vim9script', "var x = false || "], 'E15:', 2)

  # script does not fail, the second expression is skipped
  call CheckDefFailure(["var x = 1 || xxx"], 'E1001:', 1)

  call CheckDefAndScriptFailure(["var x = [] || false"], ['E1012:', 'E745:'], 1)

  call CheckDefAndScriptFailure(["if 'yes' || 0", 'echo 0', 'endif'], ['E1012: Type mismatch; expected bool but got string', 'E1135: Using a String as a Bool'], 1)

  call CheckDefAndScriptFailure(["var x = 3 || false"], ['E1012:', 'E1023:'], 1)
  call CheckDefAndScriptFailure(["var x = false || 3"], ['E1012:', 'E1023:'], 1)

  call CheckDefAndScriptFailure(["if 3"], 'E1023:', 1)
  call CheckDefExecAndScriptFailure(['var x = 3', 'if x', 'endif'], 'E1023:', 2)

  call CheckDefAndScriptFailure(["var x = [] || false"], ['E1012: Type mismatch; expected bool but got list<unknown>', 'E745:'], 1)

  var lines =<< trim END
    vim9script
    echo false
      ||true
    # comment
  END
  CheckScriptFailure(lines, 'E1004: White space required before and after ''||'' at "||true"', 3)

  lines =<< trim END
      var x = false
              || false
              || a.b
  END
  CheckDefFailure(lines, 'E1001:', 3)
enddef

" test &&
def Test_expr3()
  var lines =<< trim END
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
      assert_equal(true, Record(1) && Record(true))
      assert_equal([1, true], g:vals)

      g:vals = []
      assert_equal(false, Record(0) && Record(1))
      assert_equal([0], g:vals)

      g:vals = []
      assert_equal(false, Record(0) && Record(1) && Record(0))
      assert_equal([0], g:vals)

      g:vals = []
      assert_equal(false, Record(0) && Record(4) && Record(0))
      assert_equal([0], g:vals)

      g:vals = []
      assert_equal(false, Record(1) && Record(true) && Record(0))
      assert_equal([1, true, 0], g:vals)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr3_vimscript()
  # check line continuation
  var lines =<< trim END
      var name = 0
      		&& 1
      assert_equal(false, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:true
      		&& v:true
      		&& v:true
      assert_equal(v:true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:true &&
      		v:true &&
      		v:true
      assert_equal(v:true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = v:true &&  # comment
                # comment
      		v:true &&
                # comment
      		v:true
      assert_equal(v:true, name)
  END
  CheckDefAndScriptSuccess(lines)

  # check white space
  lines =<< trim END
      var name = v:true&&v:true
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      var name = v:true &&v:true
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''&&'' at "&&v:true"', 1)

  lines =<< trim END
      var name = v:true&& v:true
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)
enddef

def Test_expr3_fails()
  var msg = "White space required before and after '&&'"
  CheckDefAndScriptFailure(["var x = 1&&0"], msg, 1)
  CheckDefAndScriptFailure(["var x = 1 &&0"], msg, 1)
  CheckDefAndScriptFailure(["var x = 1&& 0"], msg, 1)
  var lines =<< trim END
    var x = 1
      &&0
    # comment
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''&&'' at "&&0"', 2)

  g:vals = []
  CheckDefAndScriptFailure(["if 'yes' && 0", 'echo 0', 'endif'], ['E1012: Type mismatch; expected bool but got string', 'E1135: Using a String as a Bool'], 1)

  CheckDefExecAndScriptFailure(['assert_equal(false, Record(1) && Record(4) && Record(0))'], 'E1023: Using a Number as a Bool: 4', 1)

  lines =<< trim END
      if 3
          && true
      endif
  END
  CheckDefAndScriptFailure(lines, ['E1012:', 'E1023:'], 1)

  lines =<< trim END
      if true
          && 3
      endif
  END
  CheckDefAndScriptFailure(lines, ['E1012:', 'E1023:'], 2)

  lines =<< trim END
      if 'yes'
          && true
      endif
  END
  CheckDefAndScriptFailure(lines, ['E1012:', 'E1135: Using a String as a Bool'], 1)
enddef

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
  var lines =<< trim END
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
      assert_equal(true, null == v:null)
      assert_equal(true, null == g:anull)
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
      assert_equal(true, 'abc' ==? 'ABC')
      set noignorecase

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

      assert_equal(true, {one: 1, two: 2} == {one: 1, two: 2})
      assert_equal(false, {one: 1, two: 2} == {one: 2, two: 2})
      assert_equal(false, {one: 1, two: 2} == {two: 2})
      assert_equal(false, {one: 1, two: 2} == {})
      assert_equal(true, g:adict == {bbb: 8, aaa: 2})
      assert_equal(false, {ccc: 9, aaa: 2} == g:adict)

      assert_equal(true, function('g:Test_expr4_equal') == function('g:Test_expr4_equal'))
      assert_equal(false, function('g:Test_expr4_equal') == function('g:Test_expr4_is'))

      assert_equal(true, function('g:Test_expr4_equal', [123]) == function('g:Test_expr4_equal', [123]))
      assert_equal(false, function('g:Test_expr4_equal', [123]) == function('g:Test_expr4_is', [123]))
      assert_equal(false, function('g:Test_expr4_equal', [123]) == function('g:Test_expr4_equal', [999]))

      if true
        var OneFunc: func
        var TwoFunc: func
        OneFunc = function('len')
        TwoFunc = function('len')
        assert_equal(true, OneFunc('abc') == TwoFunc('123'))
      endif

      # check this doesn't fail when skipped
      if false
        var OneFunc: func
        var TwoFunc: func
        OneFunc = function('len')
        TwoFunc = function('len')
        assert_equal(true, OneFunc('abc') == TwoFunc('123'))
      endif
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var x = 'a' == xxx"], ['E1001:', 'E121:'], 1)
  CheckDefFailure(["var x = 'a' == "], 'E1097:', 3)
  CheckScriptFailure(['vim9script', "var x = 'a' == "], 'E15:', 2)

  CheckDefExecAndScriptFailure(['var items: any', 'eval 1 + 1', 'eval 2 + 2', 'if items == []', 'endif'], ['E691:', 'E1072:'], 4)

  CheckDefExecAndScriptFailure(['var x: any = "a"', 'echo x == true'], 'E1072: Cannot compare string with bool', 2)
  CheckDefExecAndScriptFailure(["var x: any = true", 'echo x == ""'], 'E1072: Cannot compare bool with string', 2)
  CheckDefExecAndScriptFailure(["var x: any = 99", 'echo x == true'], ['E1138', 'E1072:'], 2)
  CheckDefExecAndScriptFailure(["var x: any = 'a'", 'echo x == 99'], ['E1030:', 'E1072:'], 2)

  lines =<< trim END
      vim9script
      var n: any = 2
      def Compare()
        eval n == '3'
        g:notReached = false
      enddef
      g:notReached = true
      Compare()
  END
  CheckScriptFailure(lines, 'E1030: Using a String as a Number: "3"')
  assert_true(g:notReached)

  if has('float')
    lines =<< trim END
        vim9script
        var n: any = 2.2
        def Compare()
          eval n == '3'
          g:notReached = false
        enddef
        g:notReached = true
        Compare()
    END
    CheckScriptFailure(lines, 'E892: Using a String as a Float')
    assert_true(g:notReached)
  endif

  unlet g:notReached
enddef

def Test_expr4_wrong_type()
  for op in ['>', '>=', '<', '<=', '=~', '!~']
    CheckDefExecAndScriptFailure([
        "var a: any = 'a'",
        'var b: any = true',
        'echo a ' .. op .. ' b'], 'E1072:', 3)
  endfor
  for op in ['>', '>=', '<', '<=']
    CheckDefExecAndScriptFailure([
        "var n: any = 2",
        'echo n ' .. op .. ' "3"'], ['E1030:', 'E1072:'], 2)
  endfor
  for op in ['=~', '!~']
    CheckDefExecAndScriptFailure([
        "var n: any = 2",
        'echo n ' .. op .. ' "3"'], 'E1072:', 2)
  endfor

  CheckDefAndScriptFailure([
      'echo v:none == true'], 'E1072:', 1)
  CheckDefAndScriptFailure([
      'echo false >= true'], 'E1072:', 1)
  CheckDefExecAndScriptFailure([
      "var n: any = v:none",
      'echo n == true'], 'E1072:', 2)
  CheckDefExecAndScriptFailure([
      "var n: any = v:none",
      'echo n < true'], 'E1072:', 2)
enddef

" test != comperator
def Test_expr4_notequal()
  var lines =<< trim END
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
      assert_equal(true, 'abc' !=# 'ABC')
      assert_equal(false, 'abc' !=? 'ABC')
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

      assert_equal(false, {one: 1, two: 2} != {one: 1, two: 2})
      assert_equal(true, {one: 1, two: 2} != {one: 2, two: 2})
      assert_equal(true, {one: 1, two: 2} != {two: 2})
      assert_equal(true, {one: 1, two: 2} != {})
      assert_equal(false, g:adict != {bbb: 8, aaa: 2})
      assert_equal(true, {ccc: 9, aaa: 2} != g:adict)

      assert_equal(false, function('g:Test_expr4_equal') != function('g:Test_expr4_equal'))
      assert_equal(true, function('g:Test_expr4_equal') != function('g:Test_expr4_is'))

      assert_equal(false, function('g:Test_expr4_equal', [123]) != function('g:Test_expr4_equal', [123]))
      assert_equal(true, function('g:Test_expr4_equal', [123]) != function('g:Test_expr4_is', [123]))
      assert_equal(true, function('g:Test_expr4_equal', [123]) != function('g:Test_expr4_equal', [999]))
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test > comperator
def Test_expr4_greater()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test >= comperator
def Test_expr4_greaterequal()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test < comperator
def Test_expr4_smaller()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test <= comperator
def Test_expr4_smallerequal()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test =~ comperator
def Test_expr4_match()
  var lines =<< trim END
      assert_equal(false, '2' =~ '0')
      assert_equal(false, ''
                             =~ '0')
      assert_equal(true, '2' =~
                            '[0-9]')
      set ignorecase
      assert_equal(false, 'abc' =~ 'ABC')
      assert_equal(false, 'abc' =~# 'ABC')
      assert_equal(true, 'abc' =~? 'ABC')
      set noignorecase
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test !~ comperator
def Test_expr4_nomatch()
  var lines =<< trim END
      assert_equal(true, '2' !~ '0')
      assert_equal(true, ''
                            !~ '0')
      assert_equal(false, '2' !~
                            '[0-9]')
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test is comperator
def Test_expr4_is()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test isnot comperator
def Test_expr4_isnot()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)
enddef

def RetVoid()
  var x = 1
enddef

def Test_expr4_vim9script()
  # check line continuation
  var lines =<< trim END
      var name = 0
      		< 1
      assert_equal(true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 123
                # comment
      		!= 123
      assert_equal(false, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 123 ==
      			123
      assert_equal(true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var list = [1, 2, 3]
      var name = list
      		is list
      assert_equal(true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var list = [1, 2, 3]
      var name = list # comment
                 # comment
      		is list
      assert_equal(true, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var myblob = 0z1234
      var name = myblob
      		isnot 0z11
      assert_equal(true, name)
  END
  CheckDefAndScriptSuccess(lines)

  # spot check mismatching types
  lines =<< trim END
      echo '' == 0
  END
  CheckDefAndScriptFailure(lines, 'E1072:', 1)

  lines =<< trim END
      echo v:true > v:false
  END
  CheckDefAndScriptFailure(lines, 'Cannot compare bool with bool', 1)

  lines =<< trim END
      echo 123 is 123
  END
  CheckDefAndScriptFailure(lines, 'Cannot use "is" with number', 1)

  # check missing white space
  lines =<< trim END
    echo 2>3
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''>'' at ">3"', 1)

  lines =<< trim END
    echo 2 >3
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
    echo 2> 3
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
    echo 2!=3
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
    echo 2 !=3
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''!='' at "!=3"', 1)

  lines =<< trim END
    echo 2!= 3
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  for op in ['==', '>', '>=', '<', '<=', '=~', '!~', 'is', 'isnot']
    lines = ["echo 'aaa'", op .. "'bbb'", '# comment']
    var msg = printf("E1004: White space required before and after '%s'", op)
    CheckDefAndScriptFailure(lines, msg, 2)
  endfor

  lines =<< trim END
    echo len('xxx') == 3
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
    var line = 'abc'
    echo line[1] =~ '\w'
  END
  CheckDefAndScriptSuccess(lines)
enddef

func Test_expr4_fails()
  let msg = "White space required before and after '>'"
  call CheckDefAndScriptFailure(["var x = 1>2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 >2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1> 2"], msg, 1)

  let msg = "White space required before and after '=='"
  call CheckDefAndScriptFailure(["var x = 1==2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 ==2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1== 2"], msg, 1)

  let msg = "White space required before and after 'is'"
  call CheckDefAndScriptFailure(["var x = '1'is'2'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = '1' is'2'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = '1'is '2'"], msg, 1)

  let msg = "White space required before and after 'isnot'"
  call CheckDefAndScriptFailure(["var x = '1'isnot'2'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = '1' isnot'2'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = '1'isnot '2'"], msg, 1)

  call CheckDefAndScriptFailure(["var x = 1 is# 2"], 'E15:', 1)
  call CheckDefAndScriptFailure(["var x = 1 is? 2"], 'E15:', 1)
  call CheckDefAndScriptFailure(["var x = 1 isnot# 2"], 'E15:', 1)
  call CheckDefAndScriptFailure(["var x = 1 isnot? 2"], 'E15:', 1)

  call CheckDefAndScriptFailure(["var x = 1 == '2'"], 'Cannot compare number with string', 1)
  call CheckDefAndScriptFailure(["var x = '1' == 2"], 'Cannot compare string with number', 1)
  call CheckDefAndScriptFailure(["var x = 1 == RetVoid()"], 'Cannot compare number with void', 1)
  call CheckDefAndScriptFailure(["var x = RetVoid() == 1"], 'Cannot compare void with number', 1)

  call CheckDefAndScriptFailure(["var x = true > false"], 'Cannot compare bool with bool', 1)
  call CheckDefAndScriptFailure(["var x = true >= false"], 'Cannot compare bool with bool', 1)
  call CheckDefAndScriptFailure(["var x = true < false"], 'Cannot compare bool with bool', 1)
  call CheckDefAndScriptFailure(["var x = true <= false"], 'Cannot compare bool with bool', 1)
  call CheckDefAndScriptFailure(["var x = true =~ false"], 'Cannot compare bool with bool', 1)
  call CheckDefAndScriptFailure(["var x = true !~ false"], 'Cannot compare bool with bool', 1)
  call CheckDefAndScriptFailure(["var x = true is false"], 'Cannot use "is" with bool', 1)
  call CheckDefAndScriptFailure(["var x = true isnot false"], 'Cannot use "isnot" with bool', 1)

  call CheckDefAndScriptFailure(["var x = v:none is v:null"], 'Cannot use "is" with special', 1)
  call CheckDefAndScriptFailure(["var x = v:none isnot v:null"], 'Cannot use "isnot" with special', 1)
  call CheckDefAndScriptFailure(["var x = 123 is 123"], 'Cannot use "is" with number', 1)
  call CheckDefAndScriptFailure(["var x = 123 isnot 123"], 'Cannot use "isnot" with number', 1)
  if has('float')
    call CheckDefAndScriptFailure(["var x = 1.3 is 1.3"], 'Cannot use "is" with float', 1)
    call CheckDefAndScriptFailure(["var x = 1.3 isnot 1.3"], 'Cannot use "isnot" with float', 1)
  endif

  call CheckDefAndScriptFailure(["var x = 0za1 > 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefAndScriptFailure(["var x = 0za1 >= 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefAndScriptFailure(["var x = 0za1 < 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefAndScriptFailure(["var x = 0za1 <= 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefAndScriptFailure(["var x = 0za1 =~ 0z34"], 'Cannot compare blob with blob', 1)
  call CheckDefAndScriptFailure(["var x = 0za1 !~ 0z34"], 'Cannot compare blob with blob', 1)

  call CheckDefAndScriptFailure(["var x = [13] > [88]"], 'Cannot compare list with list', 1)
  call CheckDefAndScriptFailure(["var x = [13] >= [88]"], 'Cannot compare list with list', 1)
  call CheckDefAndScriptFailure(["var x = [13] < [88]"], 'Cannot compare list with list', 1)
  call CheckDefAndScriptFailure(["var x = [13] <= [88]"], 'Cannot compare list with list', 1)
  call CheckDefAndScriptFailure(["var x = [13] =~ [88]"], 'Cannot compare list with list', 1)
  call CheckDefAndScriptFailure(["var x = [13] !~ [88]"], 'Cannot compare list with list', 1)

  call CheckDefAndScriptFailure(['var j: job', 'var chan: channel', 'var r = j == chan'], 'Cannot compare job with channel', 3)
  call CheckDefAndScriptFailure(['var j: job', 'var x: list<any>', 'var r = j == x'], 'Cannot compare job with list', 3)
  call CheckDefAndScriptFailure(['var j: job', 'var Xx: func', 'var r = j == Xx'], 'Cannot compare job with func', 3)
  call CheckDefAndScriptFailure(['var j: job', 'var Xx: func', 'var r = j == Xx'], 'Cannot compare job with func', 3)
endfunc

" test addition, subtraction, concatenation
def Test_expr5()
  var lines =<< trim END
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

      assert_equal('atrue', 'a' .. true)
      assert_equal('afalse', 'a' .. false)
      assert_equal('anull', 'a' .. v:null)
      assert_equal('av:none', 'a' .. v:none)
      if has('float')
        assert_equal('a0.123', 'a' .. 0.123)
      endif

      assert_equal(3, 1 + [2, 3, 4][0])
      assert_equal(5, 2 + {key: 3}['key'])

      set digraph
      assert_equal('val: true', 'val: ' .. &digraph)
      set nodigraph
      assert_equal('val: false', 'val: ' .. &digraph)

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

      assert_equal('val', '' .. {key: 'val'}['key'])
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr5_vim9script()
  # check line continuation
  var lines =<< trim END
      var name = 11
      		+ 77
		- 22
      assert_equal(66, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 11 +
		  77 -
		  22
      assert_equal(66, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 11 +  # comment
		  77 -
                  # comment
		  22
      assert_equal(66, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 'one'
      		.. 'two'
      assert_equal('onetwo', name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      echo 'abc' is# 'abc'
  END
  CheckDefAndScriptFailure(lines, 'E15:', 1)

  lines =<< trim END
      echo {} - 22
  END
  CheckDefAndScriptFailure(lines, ['E1036:', 'E728:'], 1)

  lines =<< trim END
      echo [] - 33
  END
  CheckDefAndScriptFailure(lines, ['E1036:', 'E745:'], 1)

  lines =<< trim END
      echo 0z1234 - 44
  END
  CheckDefAndScriptFailure(lines, ['E1036', 'E974:'], 1)

  lines =<< trim END
      echo 'abc' is? 'abc'
  END
  CheckDefAndScriptFailure(lines, 'E15:', 1)

  lines =<< trim END
      echo 'abc' isnot# 'abc'
  END
  CheckDefAndScriptFailure(lines, 'E15:', 1)

  lines =<< trim END
      echo 'abc' isnot? 'abc'
  END
  CheckDefAndScriptFailure(lines, 'E15:', 1)

  # check white space
  lines =<< trim END
      echo 5+6
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)
  lines =<< trim END
      echo 5 +6
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      echo 5+ 6
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      echo 'a'..'b'
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''..'' at "..''b''"', 1)

  lines =<< trim END
      echo 'a' ..'b'
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      echo 'a'.. 'b'
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''..'' at ".. ''b''"', 1)

  lines =<< trim END
      echo 'a'
          ..'b'
      # comment
  END
  CheckDefAndScriptFailure(lines, 'E1004: White space required before and after ''..'' at "..''b''"', 2)

  # check invalid string concatenation
  lines =<< trim END
      echo 'a' .. [1]
  END
  CheckDefAndScriptFailure(lines, ['E1105:', 'E730:'], 1)

  lines =<< trim END
      echo 'a' .. {a: 1}
  END
  CheckDefAndScriptFailure(lines, ['E1105:', 'E731:'], 1)

  lines =<< trim END
      echo 'a' .. test_void()
  END
  CheckDefAndScriptFailure(lines, ['E1105:', 'E908:'], 1)

  lines =<< trim END
      echo 'a' .. 0z33
  END
  CheckDefAndScriptFailure(lines, ['E1105:', 'E976:'], 1)

  lines =<< trim END
      echo 'a' .. function('len')
  END
  CheckDefAndScriptFailure(lines, ['E1105:', 'E729:'], 1)

  lines =<< trim END
      new
      ['']->setline(1)
      /pattern

      eval 0
      bwipe!
  END
  CheckDefAndScriptFailure(lines, "E1004: White space required before and after '/' at \"/pattern", 3)

  for op in ['+', '-']
    lines = ['var x = 1', op .. '2', '# comment']
    var msg = printf("E1004: White space required before and after '%s' at \"%s2\"", op, op)
    CheckDefAndScriptFailure(lines, msg, 2)
  endfor
enddef

def Test_expr5_vim9script_channel()
  if !has('channel')
    MissingFeature 'channel'
  else
    var lines =<< trim END
        echo 'a' .. test_null_job()
    END
    CheckDefAndScriptFailure(lines, ['E1105:', 'E908:'], 1)
    lines =<< trim END
        echo 'a' .. test_null_channel()
    END
    CheckDefAndScriptFailure(lines, ['E1105:', 'E908:'], 1)
  endif
enddef

def Test_expr5_float()
  if !has('float')
    MissingFeature 'float'
  else
    var lines =<< trim END
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
    END
    CheckDefAndScriptSuccess(lines)
  endif
enddef

func Test_expr5_fails()
  let msg = "White space required before and after '+'"
  call CheckDefAndScriptFailure(["var x = 1+2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 +2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1+ 2"], msg, 1)

  let msg = "White space required before and after '-'"
  call CheckDefAndScriptFailure(["var x = 1-2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 -2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1- 2"], msg, 1)

  let msg = "White space required before and after '..'"
  call CheckDefAndScriptFailure(["var x = '1'..'2'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = '1' ..'2'"], msg, 1)
  call CheckDefAndScriptFailure(["var x = '1'.. '2'"], msg, 1)

  call CheckDefAndScriptFailure(["var x = 0z1122 + 33"], ['E1051:', 'E974:'], 1)
  call CheckDefAndScriptFailure(["var x = 0z1122 + [3]"], ['E1051:', 'E974:'], 1)
  call CheckDefAndScriptFailure(["var x = 0z1122 + 'asd'"], ['E1051:', 'E974:'], 1)
  call CheckDefAndScriptFailure(["var x = 33 + 0z1122"], ['E1051:', 'E974:'], 1)
  call CheckDefAndScriptFailure(["var x = [3] + 0z1122"], ['E1051:', 'E745:'], 1)
  call CheckDefAndScriptFailure(["var x = 'asdf' + 0z1122"], ['E1051:', 'E1030:'], 1)
  call CheckDefAndScriptFailure(["var x = 6 + xxx"], ['E1001:', 'E121:'], 1)

  call CheckDefAndScriptFailure(["var x = 'a' .. [1]"], ['E1105:', 'E730:'], 1)
  call CheckDefAndScriptFailure(["var x = 'a' .. {a: 1}"], ['E1105:', 'E731:'], 1)
  call CheckDefAndScriptFailure(["var x = 'a' .. test_void()"], ['E1105:', 'E908:'], 1)
  call CheckDefAndScriptFailure(["var x = 'a' .. 0z32"], ['E1105:', 'E976:'], 1)
  call CheckDefAndScriptFailure(["var x = 'a' .. function('len')"], ['E1105:', 'E729:'], 1)
  call CheckDefAndScriptFailure(["var x = 'a' .. function('len', ['a'])"], ['E1105:', 'E729:'], 1)

  call CheckDefAndScriptFailure(['var x = 1 + v:none'], ['E1051:', 'E611:'], 1)
  call CheckDefAndScriptFailure(['var x = 1 + v:null'], ['E1051:', 'E611:'], 1)
  call CheckDefAndScriptFailure(['var x = 1 + v:true'], ['E1051:', 'E1138:'], 1)
  call CheckDefAndScriptFailure(['var x = 1 + v:false'], ['E1051:', 'E1138:'], 1)
  call CheckDefAndScriptFailure(['var x = 1 + true'], ['E1051:', 'E1138:'], 1)
  call CheckDefAndScriptFailure(['var x = 1 + false'], ['E1051:', 'E1138:'], 1)
endfunc

func Test_expr5_fails_channel()
  CheckFeature channel
  call CheckDefAndScriptFailure(["var x = 'a' .. test_null_job()"], ['E1105:', 'E908:'], 1)
  call CheckDefAndScriptFailure(["var x = 'a' .. test_null_channel()"], ['E1105:', 'E908:'], 1)
endfunc

def Test_expr5_list_add()
  var lines =<< trim END
      # concatenating two lists with same member types is OK
      var d = {}
      for i in ['a'] + ['b']
        d = {[i]: 0}
      endfor

      # concatenating two lists with different member types results in "any"
      var dany = {}
      for i in ['a'] + [12]
        dany[i] = i
      endfor
      assert_equal({a: 'a', 12: 12}, dany)

      # result of glob() is "any", runtime type check
      var sl: list<string> = glob('*.txt', false, true) + ['']
  END
  CheckDefAndScriptSuccess(lines)
enddef

" test multiply, divide, modulo
def Test_expr6()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var x = 6 * xxx"], ['E1001:', 'E121:'], 1)
  CheckDefFailure(["var d = 6 * "], 'E1097:', 3)
  CheckScriptFailure(['vim9script', "var d = 6 * "], 'E15:', 2)

  CheckDefExecAndScriptFailure(['echo 1 / 0'], 'E1154', 1)
  CheckDefExecAndScriptFailure(['echo 1 % 0'], 'E1154', 1)

  lines =<< trim END
    var n = 0
    eval 1 / n
  END
  CheckDefExecAndScriptFailure(lines, 'E1154', 2)

  lines =<< trim END
    var n = 0
    eval 1 % n
  END
  CheckDefExecAndScriptFailure(lines, 'E1154', 2)
enddef

def Test_expr6_vim9script()
  # check line continuation
  var lines =<< trim END
      var name = 11
      		* 22
		/ 3
      assert_equal(80, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 25
      		% 10
      assert_equal(5, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 25
                # comment

                # comment
      		% 10
      assert_equal(5, name)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var name = 11 *
      		22 /
		3
      assert_equal(80, name)
  END
  CheckDefAndScriptSuccess(lines)

  # check white space
  lines =<< trim END
      echo 5*6
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      echo 5 *6
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)

  lines =<< trim END
      echo 5* 6
  END
  CheckDefAndScriptFailure(lines, 'E1004:', 1)
enddef

def Test_expr6_float()
  if !has('float')
    MissingFeature 'float'
  else
    var lines =<< trim END
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
    END
    CheckDefAndScriptSuccess(lines)
  endif
enddef

func Test_expr6_fails()
  let msg = "White space required before and after '*'"
  call CheckDefAndScriptFailure(["var x = 1*2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 *2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1* 2"], msg, 1)

  let msg = "White space required before and after '/'"
  call CheckDefAndScriptFailure(["var x = 1/2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 /2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1/ 2"], msg, 1)

  let msg = "White space required before and after '%'"
  call CheckDefAndScriptFailure(["var x = 1%2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1 %2"], msg, 1)
  call CheckDefAndScriptFailure(["var x = 1% 2"], msg, 1)

  call CheckDefAndScriptFailure(["var x = '1' * '2'"], ['E1036:', 'E1030:'], 1)
  call CheckDefAndScriptFailure(["var x = '1' / '2'"], ['E1036:', 'E1030:'], 1)
  call CheckDefAndScriptFailure(["var x = '1' % '2'"], ['E1035:', 'E1030:'], 1)

  call CheckDefAndScriptFailure(["var x = 0z01 * 0z12"], ['E1036:', 'E974:'], 1)
  call CheckDefAndScriptFailure(["var x = 0z01 / 0z12"], ['E1036:', 'E974:'], 1)
  call CheckDefAndScriptFailure(["var x = 0z01 % 0z12"], ['E1035:', 'E974:'], 1)

  call CheckDefAndScriptFailure(["var x = [1] * [2]"], ['E1036:', 'E745:'], 1)
  call CheckDefAndScriptFailure(["var x = [1] / [2]"], ['E1036:', 'E745:'], 1)
  call CheckDefAndScriptFailure(["var x = [1] % [2]"], ['E1035:', 'E745:'], 1)

  call CheckDefAndScriptFailure(["var x = {one: 1} * {two: 2}"], ['E1036:', 'E728:'], 1)
  call CheckDefAndScriptFailure(["var x = {one: 1} / {two: 2}"], ['E1036:', 'E728:'], 1)
  call CheckDefAndScriptFailure(["var x = {one: 1} % {two: 2}"], ['E1035:', 'E728:'], 1)

  call CheckDefAndScriptFailure(["var x = 0xff[1]"], ['E1107:', 'E1062:'], 1)
  if has('float')
    call CheckDefAndScriptFailure(["var x = 0.7[1]"], ['E1107:', 'E806:'], 1)
  endif

  for op in ['*', '/', '%']
    let lines = ['var x = 1', op .. '2', '# comment']
    let msg = printf("E1004: White space required before and after '%s' at \"%s2\"", op, op)
    call CheckDefAndScriptFailure(lines, msg, 2)
  endfor
endfunc

func Test_expr6_float_fails()
  CheckFeature float
  call CheckDefAndScriptFailure(["var x = 1.0 % 2"], ['E1035:', 'E804:'], 1)
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
  var lines =<< trim END
      var ls: list<string> = ['a', <string>g:string_empty]
      var ln: list<number> = [<number>g:anint, <number>g:thefour]
      var nr = <number>234
      assert_equal(234, nr)
      var b: bool = <bool>1
      assert_equal(true, b)
      var text =
            <string>
              'text'
      if false
        text = <number>'xxx'
      endif
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var x = <nr>123"], 'E1010:', 1)
  CheckDefFailure(["var x = <number>"], 'E1097:', 3)
  CheckDefFailure(["var x = <number>string(1)"], 'E1012:', 1)
  CheckScriptFailure(['vim9script', "var x = <number>"], 'E15:', 2)
  CheckDefAndScriptFailure(["var x = <number >123"], 'E1068:', 1)
  CheckDefAndScriptFailure(["var x = <number 123"], 'E1104:', 1)
enddef

" test low level expression
def Test_expr7_number()
  # number constant
  var lines =<< trim END
      assert_equal(0, 0)
      assert_equal(654, 0654)

      assert_equal(6, 0x6)
      assert_equal(15, 0xf)
      assert_equal(255, 0xff)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_float()
  # float constant
  if !has('float')
    MissingFeature 'float'
  else
    var lines =<< trim END
        assert_equal(g:float_zero, .0)
        assert_equal(g:float_zero, 0.0)
        assert_equal(g:float_neg, -9.8)
        assert_equal(g:float_big, 9.9e99)
    END
    CheckDefAndScriptSuccess(lines)
  endif
enddef

def Test_expr7_blob()
  # blob constant
  var lines =<< trim END
      assert_equal(g:blob_empty, 0z)
      assert_equal(g:blob_one, 0z01)
      assert_equal(g:blob_long, 0z0102.0304)

      var testblob = 0z010203
      assert_equal(0x01, testblob[0])
      assert_equal(0x02, testblob[1])
      assert_equal(0x03, testblob[-1])
      assert_equal(0x02, testblob[-2])

      assert_equal(0z01, testblob[0 : 0])
      assert_equal(0z0102, testblob[0 : 1])
      assert_equal(0z010203, testblob[0 : 2])
      assert_equal(0z010203, testblob[0 : ])
      assert_equal(0z0203, testblob[1 : ])
      assert_equal(0z0203, testblob[1 : 2])
      assert_equal(0z0203, testblob[1 : -1])
      assert_equal(0z03, testblob[-1 : -1])
      assert_equal(0z02, testblob[-2 : -2])

      # blob slice accepts out of range
      assert_equal(0z, testblob[3 : 3])
      assert_equal(0z, testblob[0 : -4])
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var x = 0z123"], 'E973:', 1)
enddef

def Test_expr7_string()
  # string constant
  var lines =<< trim END
      assert_equal(g:string_empty, '')
      assert_equal(g:string_empty, "")
      assert_equal(g:string_short, 'x')
      assert_equal(g:string_short, "x")
      assert_equal(g:string_long, 'abcdefghijklm')
      assert_equal(g:string_long, "abcdefghijklm")
      assert_equal(g:string_special, "ab\ncd\ref\ekk")
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(['var x = "abc'], 'E114:', 1)
  CheckDefAndScriptFailure(["var x = 'abc"], 'E115:', 1)
enddef

def Test_expr7_vimvar()
  v:errors = []
  var errs: list<string> = v:errors
  CheckDefFailure(['var errs: list<number> = v:errors'], 'E1012:')

  var old: list<string> = v:oldfiles
  CheckDefFailure(['var old: list<number> = v:oldfiles'], 'E1012:')

  var compl: dict<string> = v:completed_item
  CheckDefFailure(['var compl: dict<number> = v:completed_item'], 'E1012:')

  var args: list<string> = v:argv
  CheckDefFailure(['var args: list<number> = v:argv'], 'E1012:')

  var colors: dict<string> = v:colornames
  CheckDefFailure(['var colors: dict<number> = v:colornames'], 'E1012:')

  CheckDefFailure(["var old: list<number> = v:oldfiles"], 'E1012: Type mismatch; expected list<number> but got list<string>', 1)
  CheckScriptFailure(['vim9script', 'v:oldfiles = ["foo"]', "var old: list<number> = v:oldfiles"], 'E1012: Type mismatch; expected list<number> but got list<string>', 3)
  new
  exec "normal! afoo fo\<C-N>\<Esc>"
  CheckDefExecAndScriptFailure(["var old: dict<number> = v:completed_item"], 'E1012: Type mismatch; expected dict<number> but got dict<string>', 1)
  bwipe!
enddef

def Test_expr7_special()
  # special constant
  var lines =<< trim END
      assert_equal(g:special_true, true)
      assert_equal(g:special_false, false)
      assert_equal(g:special_true, v:true)
      assert_equal(g:special_false, v:false)
      assert_equal(v:true, true)
      assert_equal(v:false, false)

      assert_equal(true, !false)
      assert_equal(false, !true)
      assert_equal(true, !0)
      assert_equal(false, !1)
      assert_equal(false, !!false)
      assert_equal(true, !!true)
      assert_equal(false, !!0)
      assert_equal(true, !!1)

      var t = true
      var f = false
      assert_equal(true, t)
      assert_equal(false, f)

      assert_equal(g:special_null, v:null)
      assert_equal(g:special_null, null)
      assert_equal(g:special_none, v:none)
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(['v:true = true'], 'E46:', 1)
  CheckDefAndScriptFailure(['v:true = false'], 'E46:', 1)
  CheckDefAndScriptFailure(['v:false = true'], 'E46:', 1)
  CheckDefAndScriptFailure(['v:null = 11'], 'E46:', 1)
  CheckDefAndScriptFailure(['v:none = 22'], 'E46:', 1)
enddef

def Test_expr7_list()
  # list
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)

  var rangelist: list<number> = range(3)
  g:rangelist = range(3)
  CheckDefExecAndScriptFailure(["var x: list<string> = g:rangelist"], 'E1012: Type mismatch; expected list<string> but got list<number>', 1)

  CheckDefAndScriptFailure(["var x = 1234[3]"], ['E1107:', 'E1062:'], 1)
  CheckDefExecAndScriptFailure(["var x = g:anint[3]"], 'E1062:', 1)

  CheckDefAndScriptFailure(["var x = g:list_mixed[xxx]"], ['E1001:', 'E121:'], 1)

  CheckDefAndScriptFailure(["var x = [1,2,3]"], 'E1069:', 1)
  CheckDefAndScriptFailure(["var x = [1 ,2, 3]"], 'E1068:', 1)

  CheckDefExecAndScriptFailure(["echo 1", "var x = [][0]", "echo 3"], 'E684:', 2)

  CheckDefExecAndScriptFailure(["var x = g:list_mixed['xx']"], ['E1012:', 'E1030:'], 1)
  CheckDefFailure(["var x = g:list_mixed["], 'E1097:', 3)
  CheckScriptFailure(['vim9script', "var x = g:list_mixed["], 'E15:', 2)
  CheckDefFailure(["var x = g:list_mixed[0"], 'E1097:', 3)
  CheckScriptFailure(['vim9script', "var x = g:list_mixed[0"], 'E111:', 2)
  CheckDefExecAndScriptFailure(["var x = g:list_empty[3]"], 'E684:', 1)
  CheckDefExecAndScriptFailure(["var l: list<number> = [234, 'x']"], 'E1012:', 1)
  CheckDefExecAndScriptFailure(["var l: list<number> = ['x', 234]"], 'E1012:', 1)
  CheckDefExecAndScriptFailure(["var l: list<string> = [234, 'x']"], 'E1012:', 1)
  CheckDefExecAndScriptFailure(["var l: list<string> = ['x', 123]"], 'E1012:', 1)

  lines =<< trim END
      var datalist: list<string>
      def Main()
        datalist += ['x'.
      enddef
      Main()
  END
  CheckDefAndScriptFailure(lines, 'E1127:')

  lines =<< trim END
      var numbers = [1, 2, 3, 4]
      var a = 1
      var b = 2
  END
  CheckDefAndScriptFailure(lines + ['echo numbers[1:b]'],
      'E1004: White space required before and after '':'' at ":b]"', 4)
  CheckDefAndScriptFailure(lines + ['echo numbers[1: b]'], 'E1004:', 4)
  CheckDefAndScriptFailure(lines + ['echo numbers[a :b]'], 'E1004:', 4)
enddef

def Test_expr7_list_vim9script()
  var lines =<< trim END
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
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var l = [11,
		22]
      assert_equal([11, 22], l)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var l = [11,22]
  END
  CheckDefAndScriptFailure(lines, 'E1069:', 1)

  lines =<< trim END
      var l = [11 , 22]
  END
  CheckDefAndScriptFailure(lines, 'E1068:', 1)

  lines =<< trim END
    var l: list<number> = [234, 'x']
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
    var l: list<number> = ['x', 234]
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
    var l: list<string> = ['x', 234]
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
    var l: list<string> = [234, 'x']
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
      def Failing()
        job_stop()
      enddef
      var list = [Failing]
  END
  if has('channel')
    CheckDefAndScriptFailure(lines, 'E119:', 0)
  else
    CheckDefAndScriptFailure(lines, 'E117:', 0)
  endif
enddef

def LambdaWithComments(): func
  return (x) =>
            # some comment
            x == 1
            # some comment
            ||
            x == 2
enddef

def LambdaUsingArg(x: number): func
  return () =>
            # some comment
            x == 1
            # some comment
            ||
            x == 2
enddef

def Test_expr7_lambda()
  var lines =<< trim END
      var La = () => 'result'
      # comment
      assert_equal('result', La())
      assert_equal([1, 3, 5], [1, 2, 3]->map((key, val) => key + val))

      # line continuation inside lambda with "cond ? expr : expr" works
      var ll = range(3)
      var dll = mapnew(ll, (k, v) => v % 2 ? {
                ['111']: 111 } : {}
            )
      assert_equal([{}, {111: 111}, {}], dll)

      # comment halfway an expression
      var Ref = () => 4
      # comment
      + 6
      assert_equal(10, Ref())

      ll = range(3)
      map(ll, (k, v) => v == 8 || v
                    == 9
                    || v % 2 ? 111 : 222
            )
      assert_equal([222, 111, 222], ll)

      ll = range(3)
      map(ll, (k, v) => v != 8 && v
                    != 9
                    && v % 2 == 0 ? 111 : 222
            )
      assert_equal([111, 222, 111], ll)

      var dl = [{key: 0}, {key: 22}]->filter(( _, v) => !!v['key'] )
      assert_equal([{key: 22}], dl)

      dl = [{key: 12}, {['foo']: 34}]
      assert_equal([{key: 12}], filter(dl,
            (_, v) => has_key(v, 'key') ? v['key'] == 12 : 0))

      assert_equal(false, LambdaWithComments()(0))
      assert_equal(true, LambdaWithComments()(1))
      assert_equal(true, LambdaWithComments()(2))
      assert_equal(false, LambdaWithComments()(3))

      assert_equal(false, LambdaUsingArg(0)())
      assert_equal(true, LambdaUsingArg(1)())

      var res = map([1, 2, 3], (i: number, v: number) => i + v)
      assert_equal([1, 3, 5], res)
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var Ref = (a)=>a + 1"], 'E1004:')
  CheckDefAndScriptFailure(["var Ref = (a)=> a + 1"], 'E1004: White space required before and after ''=>'' at "=> a + 1"')
  CheckDefAndScriptFailure(["var Ref = (a) =>a + 1"], 'E1004:')
  CheckDefAndScriptFailure(["var Ref = (a) =< a + 1"], ['E1001:', 'E121:'])
  CheckDefAndScriptFailure(["var Ref = (a: int) => a + 1"], 'E1010:')
  CheckDefAndScriptFailure(["var Ref = (a): int => a + 1"], 'E1010:')

  CheckDefAndScriptFailure(["filter([1, 2], (k,v) => 1)"], 'E1069:', 1)
  # error is in first line of the lambda
  CheckDefAndScriptFailure(["var L = (a) => a + b"], 'E1001:', 0)

  assert_equal('xxxyyy', 'xxx'->((a, b) => a .. b)('yyy'))

  CheckDefExecFailure(["var s = 'asdf'->((a) => a)('x')"], 'E118:')
  CheckDefExecFailure(["var s = 'asdf'->((a) => a)('x', 'y')"], 'E118:')
  CheckDefAndScriptFailure(["echo 'asdf'->((a) => a)(x)"], ['E1001:', 'E121:'], 1)

  CheckDefAndScriptSuccess(['var Fx = (a) => ({k1: 0,', ' k2: 1})'])
  CheckDefAndScriptFailure(['var Fx = (a) => ({k1: 0', ' k2: 1})'], 'E722:', 2)
  CheckDefAndScriptFailure(['var Fx = (a) => ({k1: 0,', ' k2 1})'], 'E720:', 2)

  CheckDefAndScriptSuccess(['var Fx = (a) => [0,', ' 1]'])
  CheckDefAndScriptFailure(['var Fx = (a) => [0', ' 1]'], 'E696:', 2)

  # no error for existing script variable when checking for lambda
  lines =<< trim END
    var name = 0
    eval (name + 2) / 3
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_lambda_block()
  var lines =<< trim END
      var Func = (s: string): string => {
                      return 'hello ' .. s
                    }
      assert_equal('hello there', Func('there'))

      var ll = range(3)
      var dll = mapnew(ll, (k, v): string => {
          if v % 2
            return 'yes'
          endif
          return 'no'
        })
      assert_equal(['no', 'yes', 'no'], dll)

      # ignored_inline(0, (_) => {
      #   echo 'body'
      # })

      sandbox var Safe = (nr: number): number => {
          return nr + 7
        }
      assert_equal(10, Safe(3))
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      map([1, 2], (k, v) => { redrawt })
  END
  CheckDefAndScriptFailure(lines, 'E488')

  lines =<< trim END
      var Func = (nr: int) => {
              echo nr
            }
  END
  CheckDefAndScriptFailure(lines, 'E1010', 1)

  lines =<< trim END
      var Func = (nr: number): int => {
              return nr
            }
  END
  CheckDefAndScriptFailure(lines, 'E1010', 1)

  lines =<< trim END
      var Func = (nr: number): int => {
              return nr
  END
  CheckDefFailure(lines, 'E1171', 0)  # line nr is function start
  CheckScriptFailure(['vim9script'] + lines, 'E1171', 2)

  lines =<< trim END
      var Func = (nr: number): int => {
          var ll =<< ENDIT
             nothing
  END
  CheckDefFailure(lines, 'E1145: Missing heredoc end marker: ENDIT', 0)
  CheckScriptFailure(['vim9script'] + lines, 'E1145: Missing heredoc end marker: ENDIT', 2)
enddef

def NewLambdaWithComments(): func
  return (x) =>
            # some comment
            x == 1
            # some comment
            ||
            x == 2
enddef

def NewLambdaUsingArg(x: number): func
  return () =>
            # some comment
            x == 1
            # some comment
            ||
            x == 2
enddef

def Test_expr7_new_lambda()
  var lines =<< trim END
      var La = () => 'result'
      assert_equal('result', La())
      assert_equal([1, 3, 5], [1, 2, 3]->map((key, val) => key + val))

      # line continuation inside lambda with "cond ? expr : expr" works
      var ll = range(3)
      var dll = mapnew(ll, (k, v) => v % 2 ? {
                ['111']: 111 } : {}
            )
      assert_equal([{}, {111: 111}, {}], dll)

      ll = range(3)
      map(ll, (k, v) => v == 8 || v
                    == 9
                    || v % 2 ? 111 : 222
            )
      assert_equal([222, 111, 222], ll)

      ll = range(3)
      map(ll, (k, v) => v != 8 && v
                    != 9
                    && v % 2 == 0 ? 111 : 222
            )
      assert_equal([111, 222, 111], ll)

      var dl = [{key: 0}, {key: 22}]->filter(( _, v) => !!v['key'] )
      assert_equal([{key: 22}], dl)

      dl = [{key: 12}, {['foo']: 34}]
      assert_equal([{key: 12}], filter(dl,
            (_, v) => has_key(v, 'key') ? v['key'] == 12 : 0))

      assert_equal(false, NewLambdaWithComments()(0))
      assert_equal(true, NewLambdaWithComments()(1))
      assert_equal(true, NewLambdaWithComments()(2))
      assert_equal(false, NewLambdaWithComments()(3))

      assert_equal(false, NewLambdaUsingArg(0)())
      assert_equal(true, NewLambdaUsingArg(1)())

      var res = map([1, 2, 3], (i: number, v: number) => i + v)
      assert_equal([1, 3, 5], res)

      # Lambda returning a dict
      var Lmb = () => ({key: 42})
      assert_equal({key: 42}, Lmb())

      var RefOne: func(number): string = (a: number): string => 'x'
      var RefTwo: func(number): any = (a: number): any => 'x'

      var Fx = (a) => ({k1: 0,
                         k2: 1})
      var Fy = (a) => [0,
                       1]
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var Ref = (a)=>a + 1"], 'E1004:')
  CheckDefAndScriptFailure(["var Ref = (a)=> a + 1"], 'E1004:')
  CheckDefAndScriptFailure(["var Ref = (a) =>a + 1"],
      'E1004: White space required before and after ''=>'' at " =>a + 1"')

  CheckDefAndScriptFailure(["var Ref: func(number): number = (a: number): string => 'x'"], 'E1012:')
  CheckDefAndScriptFailure(["var Ref: func(number): string = (a: number): string => 99"], 'E1012:')

  CheckDefAndScriptFailure(["filter([1, 2], (k,v) => 1)"], 'E1069:', 1)
  # error is in first line of the lambda
  CheckDefAndScriptFailure(["var L = (a) -> a + b"], ['E1001:', 'E121:'], 1)

  assert_equal('xxxyyy', 'xxx'->((a, b) => a .. b)('yyy'))

  CheckDefExecFailure(["var s = 'asdf'->((a) => a)('x')"],
        'E118: Too many arguments for function:')
  CheckDefExecFailure(["var s = 'asdf'->((a) => a)('x', 'y')"],
        'E118: Too many arguments for function:')
  CheckDefFailure(["echo 'asdf'->((a) => a)(x)"], 'E1001:', 1)

  CheckDefAndScriptFailure(['var Fx = (a) => ({k1: 0', ' k2: 1})'], 'E722:', 2)
  CheckDefAndScriptFailure(['var Fx = (a) => ({k1: 0,', ' k2 1})'], 'E720:', 2)

  CheckDefAndScriptFailure(['var Fx = (a) => [0', ' 1]'], 'E696:', 2)
enddef

def Test_expr7_lambda_vim9script()
  var lines =<< trim END
      var v = 10->((a) =>
	    a
	      + 2
            )()
      assert_equal(12, v)
  END
  CheckDefAndScriptSuccess(lines)

  # nested lambda with line breaks
  lines =<< trim END
      search('"', 'cW', 0, 0, () =>
	synstack('.', col('.'))
          ->mapnew((_, v) => synIDattr(v, 'name'))->len())
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_funcref()
  var lines =<< trim END
      def RetNumber(): number
        return 123
      enddef
      var FuncRef = RetNumber
      assert_equal(123, FuncRef())
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      vim9script
      func g:GlobalFunc()
        return 'global'
      endfunc
      func s:ScriptFunc()
        return 'script'
      endfunc
      def Test()
        var Ref = g:GlobalFunc
        assert_equal('global', Ref())
        Ref = GlobalFunc
        assert_equal('global', Ref())

        Ref = s:ScriptFunc
        assert_equal('script', Ref())
        Ref = ScriptFunc
        assert_equal('script', Ref())
      enddef
      Test()
  END
  CheckScriptSuccess(lines)
enddef

let g:test_space_dict = {'': 'empty', ' ': 'space'}
let g:test_hash_dict = #{one: 1, two: 2}

def Test_expr7_dict()
  # dictionary
  var lines =<< trim END
      assert_equal(g:dict_empty, {})
      assert_equal(g:dict_empty, {  })
      assert_equal(g:dict_one, {['one']: 1})
      var key = 'one'
      var val = 1
      assert_equal(g:dict_one, {[key]: val})

      var numbers: dict<number> = {a: 1, b: 2, c: 3}
      numbers = {a: 1}
      numbers = {}

      var strings: dict<string> = {a: 'a', b: 'b', c: 'c'}
      strings = {a: 'x'}
      strings = {}

      var dash = {xx-x: 8}
      assert_equal({['xx-x']: 8}, dash)

      var dnr = {8: 8}
      assert_equal({['8']: 8}, dnr)

      var mixed: dict<any> = {a: 'a', b: 42}
      mixed = {a: 'x'}
      mixed = {a: 234}
      mixed = {}

      var dictlist: dict<list<string>> = {absent: [], present: ['hi']}
      dictlist = {absent: ['hi'], present: []}
      dictlist = {absent: [], present: []}

      var dictdict: dict<dict<string>> = {one: {a: 'text'}, two: {}}
      dictdict = {one: {}, two: {a: 'text'}}
      dictdict = {one: {}, two: {}}

      assert_equal({['']: 0}, {[matchstr('string', 'wont match')]: 0})

      assert_equal(g:test_space_dict, {['']: 'empty', [' ']: 'space'})
      assert_equal(g:test_hash_dict, {one: 1, two: 2})

      assert_equal({['a a']: 1, ['b/c']: 2}, {'a a': 1, "b/c": 2})

      var d = {a: () => 3, b: () => 7}
      assert_equal(3, d.a())
      assert_equal(7, d.b())

      var cd = { # comment
                key: 'val' # comment
               }

      # different types used for the key
      var dkeys = {['key']: 'string',
                   [12]: 'numberexpr',
                   34: 'number',
                   [true]: 'bool'} 
      assert_equal('string', dkeys['key'])
      assert_equal('numberexpr', dkeys[12])
      assert_equal('number', dkeys[34])
      assert_equal('bool', dkeys[true])
      if has('float')
        dkeys = {[1.2]: 'floatexpr', [3.4]: 'float'}
        assert_equal('floatexpr', dkeys[1.2])
        assert_equal('float', dkeys[3.4])
      endif

      # automatic conversion from number to string
      var n = 123
      var dictnr = {[n]: 1}

      # comment to start fold is OK
      var x1: number #{{ fold
      var x2 = 9 #{{ fold
  END
  CheckDefAndScriptSuccess(lines)
 
  # legacy syntax doesn't work
  CheckDefAndScriptFailure(["var x = #{key: 8}"], 'E1170:', 1)
  CheckDefAndScriptFailure(["var x = 'a' #{a: 1}"], 'E1170:', 1)
  CheckDefAndScriptFailure(["var x = 'a' .. #{a: 1}"], 'E1170:', 1)
  CheckDefAndScriptFailure(["var x = true ? #{a: 1}"], 'E1170:', 1)

  CheckDefAndScriptFailure(["var x = {a:8}"], 'E1069:', 1)
  CheckDefAndScriptFailure(["var x = {a : 8}"], 'E1068:', 1)
  CheckDefAndScriptFailure(["var x = {a :8}"], 'E1068:', 1)
  CheckDefAndScriptFailure(["var x = {a: 8 , b: 9}"], 'E1068:', 1)
  CheckDefAndScriptFailure(["var x = {a: 1,b: 2}"], 'E1069:', 1)

  CheckDefAndScriptFailure(["var x = {xxx}"], 'E720:', 1)
  CheckDefAndScriptFailure(["var x = {xxx: 1", "var y = 2"], 'E722:', 2)
  CheckDefFailure(["var x = {xxx: 1,"], 'E723:', 2)
  CheckScriptFailure(['vim9script', "var x = {xxx: 1,"], 'E723:', 2)
  CheckDefAndScriptFailure(["var x = {['a']: xxx}"], ['E1001:', 'E121:'], 1)
  CheckDefAndScriptFailure(["var x = {a: 1, a: 2}"], 'E721:', 1)
  CheckDefExecAndScriptFailure(["var x = g:anint.member"], ['E715:', 'E488:'], 1)
  CheckDefExecAndScriptFailure(["var x = g:dict_empty.member"], 'E716:', 1)

  CheckDefExecAndScriptFailure(['var x: dict<number> = {a: 234, b: "1"}'], 'E1012:', 1)
  CheckDefExecAndScriptFailure(['var x: dict<number> = {a: "x", b: 134}'], 'E1012:', 1)
  CheckDefExecAndScriptFailure(['var x: dict<string> = {a: 234, b: "1"}'], 'E1012:', 1)
  CheckDefExecAndScriptFailure(['var x: dict<string> = {a: "x", b: 134}'], 'E1012:', 1)

  # invalid types for the key
  CheckDefAndScriptFailure(["var x = {[[1, 2]]: 0}"], ['E1105:', 'E730:'], 1)

  CheckDefFailure(['var x = ({'], 'E723:', 2)
  CheckScriptFailure(['vim9script', 'var x = ({'], 'E723:', 2)
  CheckDefExecAndScriptFailure(['{}[getftype("file")]'], 'E716: Key not present in Dictionary: ""', 1)
enddef

def Test_expr7_dict_vim9script()
  var lines =<< trim END
      var d = {
		['one']:
		   1,
		['two']: 2,
		   }
      assert_equal({one: 1, two: 2}, d)

      d = {  # comment
		['one']:
                # comment

		   1,
                # comment
                # comment
		['two']: 2,
		   }
      assert_equal({one: 1, two: 2}, d)

      var dd = {k: 123->len()}
      assert_equal(3, dd.k)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var d = { ["one"]: "one", ["two"]: "two", }
      assert_equal({one: 'one', two: 'two'}, d)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var d = {one: 1,
		two: 2,
	       }
      assert_equal({one: 1, two: 2}, d)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var d = {one:1, two: 2}
  END
  CheckDefAndScriptFailure(lines, 'E1069:', 1)

  lines =<< trim END
      var d = {one: 1,two: 2}
  END
  CheckDefAndScriptFailure(lines, 'E1069:', 1)

  lines =<< trim END
      var d = {one : 1}
  END
  CheckDefAndScriptFailure(lines, 'E1068:', 1)

  lines =<< trim END
      var d = {one:1}
  END
  CheckDefAndScriptFailure(lines, 'E1069:', 1)

  lines =<< trim END
      var d = {one: 1 , two: 2}
  END
  CheckDefAndScriptFailure(lines, 'E1068:', 1)

  lines =<< trim END
    var l: dict<number> = {a: 234, b: 'x'}
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
    var l: dict<number> = {a: 'x', b: 234}
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
    var l: dict<string> = {a: 'x', b: 234}
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
    var l: dict<string> = {a: 234, b: 'x'}
  END
  CheckDefAndScriptFailure(lines, 'E1012:', 1)

  lines =<< trim END
    var d = {['a']: 234, ['b': 'x'}
  END
  CheckDefAndScriptFailure(lines, 'E1139:', 1)

  lines =<< trim END
    def Func()
      var d = {['a']: 234, ['b': 'x'}
    enddef
    defcompile
  END
  CheckDefAndScriptFailure(lines, 'E1139:', 0)

  lines =<< trim END
    var d = {'a':
  END
  CheckDefFailure(lines, 'E723:', 2)
  CheckScriptFailure(['vim9script'] + lines, 'E15:', 2)

  lines =<< trim END
    def Func()
      var d = {'a':
    enddef
    defcompile
  END
  CheckDefAndScriptFailure(lines, 'E723:', 0)

  lines =<< trim END
      def Failing()
        job_stop()
      enddef
      var dict = {name: Failing}
  END
  if has('channel')
    CheckDefAndScriptFailure(lines, 'E119:', 0)
  else
    CheckDefAndScriptFailure(lines, 'E117:', 0)
  endif

  lines =<< trim END
      vim9script
      var x = 99
      assert_equal({x: 99}, s:)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_dict_in_block()
  var lines =<< trim END
      vim9script
      command MyCommand {
          echo {
              k: 0, }
      }
      MyCommand

      command YourCommand {
         g:global = {
           key: 'value' }
         }
      YourCommand
      assert_equal({key: 'value'}, g:global)
      unlet g:global
  END
  CheckScriptSuccess(lines)

  delcommand MyCommand
  delcommand YourCommand
enddef

def Test_expr7_call_2bool()
  var lines =<< trim END
      vim9script

      def BrokenCall(nr: number, mode: bool, use: string): void
        assert_equal(3, nr)
        assert_equal(false, mode)
        assert_equal('ab', use)
      enddef

      def TestBrokenCall(): void
        BrokenCall(3, 0, 'ab')
      enddef

      TestBrokenCall()
  END
  CheckScriptSuccess(lines)
enddef

let g:oneString = 'one'

def Test_expr_member()
  var lines =<< trim END
      assert_equal(1, g:dict_one.one)
      var d: dict<number> = g:dict_one
      assert_equal(1, d['one'])
      assert_equal(1, d[
                      'one'
                      ])
      assert_equal(1, d
            .one)
      d = {1: 1, _: 2}
      assert_equal(1, d
            .1)
      assert_equal(2, d
            ._)

      # getting the one member should clear the dict after getting the item
      assert_equal('one', {one: 'one'}.one)
      assert_equal('one', {one: 'one'}[g:oneString])
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var x = g:dict_one.#$!"], ['E1002:', 'E15:'], 1)
  CheckDefExecAndScriptFailure(["var d: dict<any>", "echo d['a']"], 'E716:', 2)
  CheckDefExecAndScriptFailure(["var d: dict<number>", "d = g:list_empty"], 'E1012: Type mismatch; expected dict<number> but got list<unknown>', 2)
enddef

def Test_expr7_any_index_slice()
  var lines =<< trim END
    # getting the one member should clear the list only after getting the item
    assert_equal('bbb', ['aaa', 'bbb', 'ccc'][1])

    # string is permissive, index out of range accepted
    g:teststring = 'abcdef'
    assert_equal('b', g:teststring[1])
    assert_equal('f', g:teststring[-1])
    assert_equal('', g:teststring[99])

    assert_equal('b', g:teststring[1 : 1])
    assert_equal('bcdef', g:teststring[1 :])
    assert_equal('abcd', g:teststring[: 3])
    assert_equal('cdef', g:teststring[-4 :])
    assert_equal('abcdef', g:teststring[-9 :])
    assert_equal('abcd', g:teststring[: -3])
    assert_equal('', g:teststring[: -9])

    # composing characters are included
    g:teststring = 'àéû'
    assert_equal('à', g:teststring[0])
    assert_equal('é', g:teststring[1])
    assert_equal('û', g:teststring[2])
    assert_equal('', g:teststring[3])
    assert_equal('', g:teststring[4])

    assert_equal('û', g:teststring[-1])
    assert_equal('é', g:teststring[-2])
    assert_equal('à', g:teststring[-3])
    assert_equal('', g:teststring[-4])
    assert_equal('', g:teststring[-5])

    assert_equal('à', g:teststring[0 : 0])
    assert_equal('é', g:teststring[1 : 1])
    assert_equal('àé', g:teststring[0 : 1])
    assert_equal('àéû', g:teststring[0 : -1])
    assert_equal('àé', g:teststring[0 : -2])
    assert_equal('à', g:teststring[0 : -3])
    assert_equal('', g:teststring[0 : -4])
    assert_equal('', g:teststring[0 : -5])
    assert_equal('àéû', g:teststring[ : ])
    assert_equal('àéû', g:teststring[0 : ])
    assert_equal('éû', g:teststring[1 : ])
    assert_equal('û', g:teststring[2 : ])
    assert_equal('', g:teststring[3 : ])
    assert_equal('', g:teststring[4 : ])

    # blob index cannot be out of range
    g:testblob = 0z01ab
    assert_equal(0x01, g:testblob[0])
    assert_equal(0xab, g:testblob[1])
    assert_equal(0xab, g:testblob[-1])
    assert_equal(0x01, g:testblob[-2])

    # blob slice accepts out of range
    assert_equal(0z01ab, g:testblob[0 : 1])
    assert_equal(0z01, g:testblob[0 : 0])
    assert_equal(0z01, g:testblob[-2 : -2])
    assert_equal(0zab, g:testblob[1 : 1])
    assert_equal(0zab, g:testblob[-1 : -1])
    assert_equal(0z, g:testblob[2 : 2])
    assert_equal(0z, g:testblob[0 : -3])

    # list index cannot be out of range
    g:testlist = [0, 1, 2, 3]
    assert_equal(0, g:testlist[0])
    assert_equal(1, g:testlist[1])
    assert_equal(3, g:testlist[3])
    assert_equal(3, g:testlist[-1])
    assert_equal(0, g:testlist[-4])
    assert_equal(1, g:testlist[g:theone])

    # list slice accepts out of range
    assert_equal([0], g:testlist[0 : 0])
    assert_equal([3], g:testlist[3 : 3])
    assert_equal([0, 1], g:testlist[0 : 1])
    assert_equal([0, 1, 2, 3], g:testlist[0 : 3])
    assert_equal([0, 1, 2, 3], g:testlist[0 : 9])
    assert_equal([], g:testlist[-1 : 1])
    assert_equal([1], g:testlist[-3 : 1])
    assert_equal([0, 1], g:testlist[-4 : 1])
    assert_equal([0, 1], g:testlist[-9 : 1])
    assert_equal([1, 2, 3], g:testlist[1 : -1])
    assert_equal([1], g:testlist[1 : -3])
    assert_equal([], g:testlist[1 : -4])
    assert_equal([], g:testlist[1 : -9])

    g:testdict = {a: 1, b: 2}
    assert_equal(1, g:testdict['a'])
    assert_equal(2, g:testdict['b'])
  END

  CheckDefAndScriptSuccess(lines)

  CheckDefExecAndScriptFailure(['echo g:testblob[2]'], 'E979:', 1)
  CheckDefExecAndScriptFailure(['echo g:testblob[-3]'], 'E979:', 1)

  CheckDefExecAndScriptFailure(['echo g:testlist[4]'], 'E684: list index out of range: 4', 1)
  CheckDefExecAndScriptFailure(['echo g:testlist[-5]'], 'E684:', 1)

  CheckDefExecAndScriptFailure(['echo g:testdict["a" : "b"]'], 'E719:', 1)
  CheckDefExecAndScriptFailure(['echo g:testdict[1]'], 'E716:', 1)

  unlet g:teststring
  unlet g:testblob
  unlet g:testlist
enddef

def Test_expr_member_vim9script()
  var lines =<< trim END
      var d = {one:
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
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
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
  CheckDefAndScriptSuccess(lines)
enddef

def SetSomeVar()
  b:someVar = &fdm
enddef

def Test_expr7_option()
  var lines =<< trim END
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

      # check matching type
      var bval: bool = &tgc
      var nval: number = &ts
      var sval: string = &path

      # check v_lock is cleared (requires using valgrind, doesn't always show)
      SetSomeVar()
      b:someVar = 0
      unlet b:someVar
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_environment()
  var lines =<< trim END
      # environment variable
      assert_equal('testvar', $TESTVAR)
      assert_equal('', $ASDF_ASD_XXX)
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["var x = $$$"], ['E1002:', 'E15:'], 1)
  CheckDefAndScriptFailure(["$"], ['E1002:', 'E15:'], 1)
enddef

def Test_expr7_register()
  var lines =<< trim END
      @a = 'register a'
      assert_equal('register a', @a)

      var fname = expand('%')
      assert_equal(fname, @%)

      feedkeys(":echo 'some'\<CR>", "xt")
      assert_equal("echo 'some'", @:)

      normal axyz
      assert_equal("xyz", @.)

      @/ = 'slash'
      assert_equal('slash', @/)

      @= = 'equal'
      assert_equal('equal', @=)
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefAndScriptFailure(["@. = 'yes'"], ['E354:', 'E488:'], 1)
enddef

" This is slow when run under valgrind.
def Test_expr7_namespace()
  var lines =<< trim END
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
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_namespace_loop_def()
  var lines =<< trim END
      # check using g: in a for loop more than DO_NOT_FREE_CNT times
      var exists = 0
      var exists_not = 0
      for i in range(100000)
        if has_key(g:, 'does-not-exist')
          exists += 1
        else
          exists_not += 1
        endif
      endfor
      assert_equal(0, exists)
      assert_equal(100000, exists_not)
  END
  CheckDefSuccess(lines)
enddef

" NOTE: this is known to be slow.  To skip use:
"   :let $TEST_SKIP_PAT = 'Test_expr7_namespace_loop_script'
def Test_expr7_namespace_loop_script()
  var lines =<< trim END
      vim9script
      # check using g: in a for loop more than DO_NOT_FREE_CNT times
      var exists = 0
      var exists_not = 0
      for i in range(100000)
        if has_key(g:, 'does-not-exist')
          exists += 1
        else
          exists_not += 1
        endif
      endfor
      assert_equal(0, exists)
      assert_equal(100000, exists_not)
  END
  CheckScriptSuccess(lines)
enddef

def Test_expr7_parens()
  # (expr)
  var lines =<< trim END
      assert_equal(4, (6 * 4) / 6)
      assert_equal(0, 6 * ( 4 / 6 ))

      assert_equal(6, +6)
      assert_equal(-6, -6)
      assert_equal(false, !-3)
      assert_equal(true, !+0)

      assert_equal(7, 5 + (
                    2))
      assert_equal(7, 5 + (
                    2
                    ))
      assert_equal(7, 5 + ( # comment
                    2))
      assert_equal(7, 5 + ( # comment
                    # comment
                    2))

      var s = (
		'one'
		..
		'two'
		)
      assert_equal('onetwo', s)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_negate_add()
  var lines =<< trim END
      assert_equal(-99, -99)
      assert_equal(-99, - 99)
      assert_equal(99, +99)

      var nr = 88
      assert_equal(-88, -nr)
      assert_equal(-88, - nr)
      assert_equal(88, + nr)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
    var n = 12
    echo ++n
  END
  CheckDefAndScriptFailure(lines, 'E15:')
  lines =<< trim END
    var n = 12
    echo --n
  END
  CheckDefAndScriptFailure(lines, 'E15:')
  lines =<< trim END
    var n = 12
    echo +-n
  END
  CheckDefAndScriptFailure(lines, 'E15:')
  lines =<< trim END
    var n = 12
    echo -+n
  END
  CheckDefAndScriptFailure(lines, 'E15:')
  lines =<< trim END
    var n = 12
    echo - -n
  END
  CheckDefAndScriptFailure(lines, 'E15:')
  lines =<< trim END
    var n = 12
    echo + +n
  END
  CheckDefAndScriptFailure(lines, 'E15:')
enddef

def LegacyReturn(): string
  legacy return #{key: 'ok'}.key
enddef

def Test_expr7_legacy_script()
  var lines =<< trim END
      let s:legacy = 'legacy'
      def GetLocal(): string
        return legacy
      enddef
      def GetLocalPrefix(): string
        return s:legacy
      enddef
      call assert_equal('legacy', GetLocal())
      call assert_equal('legacy', GetLocalPrefix())
  END
  CheckScriptSuccess(lines)

  assert_equal('ok', LegacyReturn())

  lines =<< trim END
      vim9script 
      def GetNumber(): number   
          legacy return range(3)->map('v:val + 1') 
      enddef 
      echo GetNumber()
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected number but got list<number>')
enddef

def Echo(arg: any): string
  return arg
enddef

def s:Echo4Arg(arg: any): string
  return arg
enddef

def Test_expr7_call()
  var lines =<< trim END
      assert_equal('yes', 'yes'->Echo())
      assert_equal(true, !range(5)->empty())
      assert_equal([0, 1, 2], 3->range())
  END
  CheckDefAndScriptSuccess(lines)

  assert_equal('yes', 'yes'
                        ->s:Echo4Arg())

  CheckDefAndScriptFailure(["var x = 'yes'->Echo"], 'E107:', 1)
  CheckDefAndScriptFailure([
       "var x = substitute ('x', 'x', 'x', 'x')"
       ], ['E1001:', 'E121:'], 1)
  CheckDefAndScriptFailure(["var Ref = function('len' [1, 2])"], ['E1123:', 'E116:'], 1)

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

def Test_expr7_method_call()
  var lines =<< trim END
      new
      setline(1, ['first', 'last'])
      'second'->append(1)
      "third"->append(2)
      assert_equal(['first', 'second', 'third', 'last'], getline(1, '$'))
      bwipe!

      var bufnr = bufnr()
      var loclist = [{bufnr: bufnr, lnum: 42, col: 17, text: 'wrong'}]
      loclist->setloclist(0)
      assert_equal([{bufnr: bufnr,
                    lnum: 42,
                    end_lnum: 0,
                    col: 17,
                    end_col: 0,
                    text: 'wrong',
                    pattern: '',
                    valid: 1,
                    vcol: 0,
                    nr: 0,
                    type: '',
                    module: ''}
                    ], getloclist(0))

      var result: bool = get({n: 0}, 'n', 0)
      assert_equal(false, result)

      assert_equal('+string+', 'string'->((s) => '+' .. s .. '+')())
      assert_equal('-text-', 'text'->((s, c) => c .. s .. c)('-'))

      var Join = (l) => join(l, 'x')
      assert_equal('axb', ['a', 'b']->(Join)())
      
      var sorted = [3, 1, 2]
                    -> sort()
      assert_equal([1, 2, 3], sorted)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
    def RetVoid()
    enddef
    RetVoid()->byteidx(3)
  END
  CheckDefExecFailure(lines, 'E1013:')
enddef


def Test_expr7_not()
  var lines =<< trim END
      assert_equal(true, !'')
      assert_equal(true, ![])
      assert_equal(false, !'asdf')
      assert_equal(false, ![2])
      assert_equal(true, !!'asdf')
      assert_equal(true, !![2])

      assert_equal(true, ! false)
      assert_equal(true, !! true)
      assert_equal(true, ! ! true)
      assert_equal(true, !!! false)
      assert_equal(true, ! ! ! false)

      g:true = true
      g:false = false
      assert_equal(true, ! g:false)
      assert_equal(true, !! g:true)
      assert_equal(true, ! ! g:true)
      assert_equal(true, !!! g:false)
      assert_equal(true, ! ! ! g:false)
      unlet g:true
      unlet g:false

      assert_equal(true, !test_null_partial())
      assert_equal(false, !() => 'yes')

      assert_equal(true, !test_null_dict())
      assert_equal(true, !{})
      assert_equal(false, !{yes: 'no'})

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
  CheckDefAndScriptSuccess(lines)
enddef

let g:anumber = 42

def Test_expr7_negate()
  var lines =<< trim END
      var nr = 1
      assert_equal(-1, -nr)
      assert_equal(-42, -g:anumber)
  END
  CheckDefAndScriptSuccess(lines)
enddef

func Test_expr7_fails()
  call CheckDefFailure(["var x = (12"], "E1097:", 3)
  call CheckScriptFailure(['vim9script', "var x = (12"], 'E110:', 2)

  call CheckDefAndScriptFailure(["var x = -'xx'"], "E1030:", 1)
  call CheckDefAndScriptFailure(["var x = +'xx'"], "E1030:", 1)
  call CheckDefAndScriptFailure(["var x = -0z12"], "E974:", 1)
  call CheckDefExecAndScriptFailure(["var x = -[8]"], ["E1012:", 'E745:'], 1)
  call CheckDefExecAndScriptFailure(["var x = -{a: 1}"], ["E1012:", 'E728:'], 1)

  call CheckDefAndScriptFailure(["var x = @"], "E1002:", 1)
  call CheckDefAndScriptFailure(["var x = @<"], "E354:", 1)

  call CheckDefFailure(["var x = [1, 2"], "E697:", 2)
  call CheckScriptFailure(['vim9script', "var x = [1, 2"], 'E696:', 2)

  call CheckDefAndScriptFailure(["var x = [notfound]"], ["E1001:", 'E121:'], 1)

  call CheckDefAndScriptFailure(["var X = () => 123)"], 'E488:', 1)
  call CheckDefAndScriptFailure(["var x = 123->((x) => x + 5)"], "E107:", 1)

  call CheckDefAndScriptFailure(["var x = &notexist"], 'E113:', 1)
  call CheckDefAndScriptFailure(["&grepprg = [343]"], ['E1012:', 'E730:'], 1)

  call CheckDefExecAndScriptFailure(["echo s:doesnt_exist"], 'E121:', 1)
  call CheckDefExecAndScriptFailure(["echo g:doesnt_exist"], 'E121:', 1)

  call CheckDefAndScriptFailure(["echo a:somevar"], ['E1075:', 'E121:'], 1)
  call CheckDefAndScriptFailure(["echo l:somevar"], ['E1075:', 'E121:'], 1)
  call CheckDefAndScriptFailure(["echo x:somevar"], ['E1075:', 'E121:'], 1)

  call CheckDefExecAndScriptFailure(["var x = +g:astring"], ['E1012:', 'E1030:'], 1)
  call CheckDefExecAndScriptFailure(["var x = +g:ablob"], ['E1012:', 'E974:'], 1)
  call CheckDefExecAndScriptFailure(["var x = +g:alist"], ['E1012:', 'E745:'], 1)
  call CheckDefExecAndScriptFailure(["var x = +g:adict"], ['E1012:', 'E728:'], 1)

  call CheckDefAndScriptFailure(["var x = ''", "var y = x.memb"], ['E1229: Expected dictionary for using key "memb", but got string', 'E488:'], 2)

  call CheckDefAndScriptFailure(["'yes'->", "Echo()"], ['E488: Trailing characters: ->', 'E260: Missing name after ->'], 1)

  call CheckDefExecFailure(["[1, 2->len()"], 'E697:', 2)
  call CheckScriptFailure(['vim9script', "[1, 2->len()"], 'E696:', 2)

  call CheckDefFailure(["{a: 1->len()"], 'E723:', 2)
  call CheckScriptFailure(['vim9script', "{a: 1->len()"], 'E722:', 2)

  call CheckDefExecFailure(["{['a']: 1->len()"], 'E723:', 2)
  call CheckScriptFailure(['vim9script', "{['a']: 1->len()"], 'E722:', 2)
endfunc

let g:Funcrefs = [function('add')]

func CallMe(arg)
  return a:arg
endfunc

func CallMe2(one, two)
  return a:one .. a:two
endfunc

def Test_expr7_trailing()
  var lines =<< trim END
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
      l->map((k, v) => k + v)
      assert_equal([2, 6, 8], l)

      # lambda method call
      l = [2, 5]
      l->((ll) => add(ll, 8))()
      assert_equal([2, 5, 8], l)

      # dict member
      var d = {key: 123}
      assert_equal(123, d.key)
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_string_subscript()
  var lines =<< trim END
    var text = 'abcdef'
    assert_equal('f', text[-1])
    assert_equal('a', text[0])
    assert_equal('e', text[4])
    assert_equal('f', text[5])
    assert_equal('', text[6])

    text = 'ábçdë'
    assert_equal('ë', text[-1])
    assert_equal('d', text[-2])
    assert_equal('ç', text[-3])
    assert_equal('b', text[-4])
    assert_equal('á', text[-5])
    assert_equal('', text[-6])

    text = 'ábçdëf'
    assert_equal('', text[-999])
    assert_equal('f', text[-1])
    assert_equal('á', text[0])
    assert_equal('b', text[1])
    assert_equal('ç', text[2])
    assert_equal('d', text[3])
    assert_equal('ë', text[4])
    assert_equal('f', text[5])
    assert_equal('', text[6])
    assert_equal('', text[999])

    assert_equal('ábçdëf', text[0 : -1])
    assert_equal('ábçdëf', text[0 : -1])
    assert_equal('ábçdëf', text[0 : -1])
    assert_equal('ábçdëf', text[0 : -1])
    assert_equal('ábçdëf', text[0
                  : -1])
    assert_equal('ábçdëf', text[0 :
                  -1])
    assert_equal('ábçdëf', text[0 : -1
                  ])
    assert_equal('bçdëf', text[1 : -1])
    assert_equal('çdëf', text[2 : -1])
    assert_equal('dëf', text[3 : -1])
    assert_equal('ëf', text[4 : -1])
    assert_equal('f', text[5 : -1])
    assert_equal('', text[6 : -1])
    assert_equal('', text[999 : -1])

    assert_equal('ábçd', text[: 3])
    assert_equal('bçdëf', text[1 :])
    assert_equal('ábçdëf', text[:])

    assert_equal('a', g:astring[0])
    assert_equal('sd', g:astring[1 : 2])
    assert_equal('asdf', g:astring[:])
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var d = 'asdf'[1 :
  END
  CheckDefFailure(lines, 'E1097:', 3)
  CheckScriptFailure(['vim9script'] + lines, 'E15:', 2)

  lines =<< trim END
      var d = 'asdf'[1 : xxx]
  END
  CheckDefAndScriptFailure(lines, ['E1001:', 'E121:'], 1)

  lines =<< trim END
      var d = 'asdf'[1 : 2
  END
  CheckDefFailure(lines, 'E1097:', 3)
  CheckScriptFailure(['vim9script'] + lines, 'E111:', 2)

  lines =<< trim END
      var d = 'asdf'[1 : 2
      echo d
  END
  CheckDefAndScriptFailure(lines, 'E111:', 2)

  lines =<< trim END
      var d = 'asdf'['1']
      echo d
  END
  CheckDefAndScriptFailure(lines, ['E1012: Type mismatch; expected number but got string', 'E1030: Using a String as a Number: "1"'], 1)

  lines =<< trim END
      var d = 'asdf'['1' : 2]
      echo d
  END
  CheckDefAndScriptFailure(lines, ['E1012: Type mismatch; expected number but got string', 'E1030: Using a String as a Number: "1"'], 1)

  lines =<< trim END
      var d = 'asdf'[1 : '2']
      echo d
  END
  CheckDefAndScriptFailure(lines, ['E1012: Type mismatch; expected number but got string', 'E1030: Using a String as a Number: "2"'], 1)
enddef

def Test_expr7_list_subscript()
  var lines =<< trim END
      var list = [0, 1, 2, 3, 4]
      assert_equal(0, list[0])
      assert_equal(4, list[4])
      assert_equal(4, list[-1])
      assert_equal(0, list[-5])

      assert_equal([0, 1, 2, 3, 4], list[0 : 4])
      assert_equal([0, 1, 2, 3, 4], list[:])
      assert_equal([1, 2, 3, 4], list[1 :])
      assert_equal([2, 3, 4], list[2 : -1])
      assert_equal([4], list[4 : -1])
      assert_equal([], list[5 : -1])
      assert_equal([], list[999 : -1])
      assert_equal([1, 2, 3, 4], list[g:theone : g:thefour])

      assert_equal([0, 1, 2, 3], list[0 : 3])
      assert_equal([0], list[0 : 0])
      assert_equal([0, 1, 2, 3, 4], list[0 : -1])
      assert_equal([0, 1, 2], list[0 : -3])
      assert_equal([0], list[0 : -5])
      assert_equal([], list[0 : -6])
      assert_equal([], list[0 : -99])

      assert_equal(2, g:alist[0])
      assert_equal([2, 3, 4], g:alist[:])
  END
  CheckDefAndScriptSuccess(lines)

  lines = ['var l = [0, 1, 2]', 'echo l[g:astring : g:theone]']
  CheckDefExecAndScriptFailure(lines, ['E1012:', 'E1030:'], 2)

  lines =<< trim END
      var ld = []
      def Func()
        eval ld[0].key
      enddef
      defcompile
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_dict_subscript()
  var lines =<< trim END
      var l = [{lnum: 2}, {lnum: 1}]
      var res = l[0].lnum > l[1].lnum
      assert_true(res)

      assert_equal(2, g:adict['aaa'])
      assert_equal(8, g:adict.bbb)

      var dd = {}
      def Func1()
        eval dd.key1.key2
      enddef
      def Func2()
        eval dd['key1'].key2
      enddef
      defcompile
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_blob_subscript()
  var lines =<< trim END
      var b = 0z112233
      assert_equal(0x11, b[0])
      assert_equal(0z112233, b[:])

      assert_equal(0x01, g:ablob[0])
      assert_equal(0z01ab, g:ablob[:])
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_expr7_subscript_linebreak()
  var lines =<< trim END
      var range = range(
                    3)
      var l = range
            ->mapnew('string(v:key)')
      assert_equal(['0', '1', '2'], l)

      l = range
            ->mapnew('string(v:key)')
      assert_equal(['0', '1', '2'], l)

      l = range # comment
            ->mapnew('string(v:key)')
      assert_equal(['0', '1', '2'], l)

      l = range

            ->mapnew('string(v:key)')
      assert_equal(['0', '1', '2'], l)

      l = range
            # comment
            ->mapnew('string(v:key)')
      assert_equal(['0', '1', '2'], l)

      assert_equal('1', l[
            1])

      var d = {one: 33}
      assert_equal(33, d
            .one)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var d = {one: 33}
      assert_equal(33, d.
            one)
  END
  CheckDefAndScriptFailure(lines, ['E1127:', 'E116:'], 2)
enddef

func Test_expr7_trailing_fails()
  call CheckDefAndScriptFailure(['var l = [2]', 'l->((ll) => add(ll, 8))'], 'E107:', 2)
  call CheckDefAndScriptFailure(['var l = [2]', 'l->((ll) => add(ll, 8)) ()'], 'E274:', 2)
endfunc

func Test_expr_fails()
  call CheckDefAndScriptFailure(["var x = '1'is2"], 'E488:', 1)
  call CheckDefAndScriptFailure(["var x = '1'isnot2"], 'E488:', 1)

  call CheckDefAndScriptFailure(["CallMe ('yes')"], ['E476:', 'E492:'], 1)

  call CheckDefAndScriptFailure(["CallMe2('yes','no')"], 'E1069:', 1)

  call CheckDefAndScriptFailure(["v:nosuch += 3"], ['E1001:', 'E121:'], 1)
  call CheckDefAndScriptFailure(["var v:statusmsg = ''"], 'E1016: Cannot declare a v: variable:', 1)
  call CheckDefAndScriptFailure(["var asdf = v:nosuch"], ['E1001:', 'E121:'], 1)

  call CheckDefFailure(["echo len('asdf'"], 'E110:', 2)
  call CheckScriptFailure(['vim9script', "echo len('asdf'"], 'E116:', 2)

  call CheckDefAndScriptFailure(["echo Func0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789()"], ['E1011:', 'E117:'], 1)
  call CheckDefAndScriptFailure(["echo doesnotexist()"], 'E117:', 1)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
