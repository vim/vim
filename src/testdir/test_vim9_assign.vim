" Test Vim9 assignments

source check.vim
source vim9.vim

let s:appendToMe = 'xxx'
let s:addToMe = 111
let g:existing = 'yes'
let g:inc_counter = 1
let $SOME_ENV_VAR = 'some'
let g:alist = [7]
let g:astring = 'text'

def Test_assignment_bool()
  var bool1: bool = true
  assert_equal(v:true, bool1)
  var bool2: bool = false
  assert_equal(v:false, bool2)

  var bool3: bool = 0
  assert_equal(false, bool3)
  var bool4: bool = 1
  assert_equal(true, bool4)

  var bool5: bool = 1 && true
  assert_equal(true, bool5)
  var bool6: bool = 0 && 1
  assert_equal(false, bool6)
  var bool7: bool = 0 || 1 && true
  assert_equal(true, bool7)

  var lines =<< trim END
    vim9script
    def GetFlag(): bool
      var flag: bool = 1
      return flag
    enddef
    var flag: bool = GetFlag()
    assert_equal(true, flag)
    flag = 0
    assert_equal(false, flag)
    flag = 1
    assert_equal(true, flag)
    flag = 1 || true
    assert_equal(true, flag)
    flag = 1 && false
    assert_equal(false, flag)
  END
  CheckScriptSuccess(lines)
  CheckDefAndScriptFailure(['var x: bool = 2'], 'E1012:')
  CheckDefAndScriptFailure(['var x: bool = -1'], 'E1012:')
  CheckDefAndScriptFailure(['var x: bool = [1]'], 'E1012:')
  CheckDefAndScriptFailure(['var x: bool = {}'], 'E1012:')
  CheckDefAndScriptFailure(['var x: bool = "x"'], 'E1012:')
enddef

def Test_syntax()
  var name = 234
  var other: list<string> = ['asdf']
enddef

def Test_assignment()
  CheckDefFailure(['var x:string'], 'E1069:')
  CheckDefFailure(['var x:string = "x"'], 'E1069:')
  CheckDefFailure(['var a:string = "x"'], 'E1069:')
  CheckDefFailure(['var lambda = {-> "lambda"}'], 'E704:')
  CheckScriptFailure(['var x = "x"'], 'E1124:')

  var nr: number = 1234
  CheckDefFailure(['var nr: number = "asdf"'], 'E1012:')

  var a: number = 6 #comment
  assert_equal(6, a)

  if has('channel')
    var chan1: channel
    assert_equal('fail', ch_status(chan1))

    var job1: job
    assert_equal('fail', job_status(job1))

    # calling job_start() is in test_vim9_fails.vim, it causes leak reports
  endif
  if has('float')
    var float1: float = 3.4
  endif
  var Funky1: func
  var Funky2: func = function('len')
  var Party2: func = funcref('g:Test_syntax')

  g:newvar = 'new'  #comment
  assert_equal('new', g:newvar)

  assert_equal('yes', g:existing)
  g:existing = 'no'
  assert_equal('no', g:existing)

  v:char = 'abc'
  assert_equal('abc', v:char)

  $ENVVAR = 'foobar'
  assert_equal('foobar', $ENVVAR)
  $ENVVAR = ''

  var lines =<< trim END
    vim9script
    $ENVVAR = 'barfoo'
    assert_equal('barfoo', $ENVVAR)
    $ENVVAR = ''
  END
  CheckScriptSuccess(lines)

  s:appendToMe ..= 'yyy'
  assert_equal('xxxyyy', s:appendToMe)
  s:addToMe += 222
  assert_equal(333, s:addToMe)
  s:newVar = 'new'
  assert_equal('new', s:newVar)

  set ts=7
  &ts += 1
  assert_equal(8, &ts)
  &ts -= 3
  assert_equal(5, &ts)
  &ts *= 2
  assert_equal(10, &ts)
  &ts /= 3
  assert_equal(3, &ts)
  set ts=10
  &ts %= 4
  assert_equal(2, &ts)

  if has('float')
    var f100: float = 100.0
    f100 /= 5
    assert_equal(20.0, f100)

    var f200: float = 200.0
    f200 /= 5.0
    assert_equal(40.0, f200)

    CheckDefFailure(['var nr: number = 200', 'nr /= 5.0'], 'E1012:')
  endif

  lines =<< trim END
    &ts = 6
    &ts += 3
    assert_equal(9, &ts)

    &l:ts = 6
    assert_equal(6, &ts)
    &l:ts += 2
    assert_equal(8, &ts)

    &g:ts = 6
    assert_equal(6, &g:ts)
    &g:ts += 2
    assert_equal(8, &g:ts)
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefFailure(['&notex += 3'], 'E113:')
  CheckDefFailure(['&ts ..= "xxx"'], 'E1019:')
  CheckDefFailure(['&ts = [7]'], 'E1012:')
  CheckDefExecFailure(['&ts = g:alist'], 'E1012: Type mismatch; expected number but got list<number>')
  CheckDefFailure(['&ts = "xx"'], 'E1012:')
  CheckDefExecFailure(['&ts = g:astring'], 'E1012: Type mismatch; expected number but got string')
  CheckDefFailure(['&path += 3'], 'E1012:')
  CheckDefExecFailure(['&bs = "asdf"'], 'E474:')
  # test freeing ISN_STOREOPT
  CheckDefFailure(['&ts = 3', 'var asdf'], 'E1022:')
  &ts = 8

  lines =<< trim END
    var save_TI = &t_TI
    &t_TI = ''
    assert_equal('', &t_TI)
    &t_TI = 'xxx'
    assert_equal('xxx', &t_TI)
    &t_TI = save_TI
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefFailure(['&t_TI = 123'], 'E1012:')
  CheckScriptFailure(['vim9script', '&t_TI = 123'], 'E928:')

  CheckDefFailure(['var s:var = 123'], 'E1101:')
  CheckDefFailure(['var s:var: number'], 'E1101:')

  lines =<< trim END
    vim9script
    def SomeFunc()
      s:var = 123
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E1089:')

  g:inc_counter += 1
  assert_equal(2, g:inc_counter)

  $SOME_ENV_VAR ..= 'more'
  assert_equal('somemore', $SOME_ENV_VAR)
  CheckDefFailure(['$SOME_ENV_VAR += "more"'], 'E1051:')
  CheckDefFailure(['$SOME_ENV_VAR += 123'], 'E1012:')

  lines =<< trim END
    @c = 'areg'
    @c ..= 'add'
    assert_equal('aregadd', @c)
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefFailure(['@a += "more"'], 'E1051:')
  CheckDefFailure(['@a += 123'], 'E1012:')

  v:errmsg = 'none'
  v:errmsg ..= 'again'
  assert_equal('noneagain', v:errmsg)
  CheckDefFailure(['v:errmsg += "more"'], 'E1051:')
  CheckDefFailure(['v:errmsg += 123'], 'E1012:')

  var text =<< trim END
    some text
  END
enddef

def Test_assign_linebreak()
  var nr: number
  nr =
      123
  assert_equal(123, nr)

  var n2: number
  [nr, n2] =
     [12, 34]
  assert_equal(12, nr)
  assert_equal(34, n2)

  CheckDefFailure(["var x = #"], 'E1097:', 2)
enddef

def Test_assign_index()
  # list of list
  var l1: list<number>
  l1[0] = 123
  assert_equal([123], l1)

  var l2: list<list<number>>
  l2[0] = []
  l2[0][0] = 123
  assert_equal([[123]], l2)

  var l3: list<list<list<number>>>
  l3[0] = []
  l3[0][0] = []
  l3[0][0][0] = 123
  assert_equal([[[123]]], l3)

  var lines =<< trim END
      var l3: list<list<number>>
      l3[0] = []
      l3[0][0] = []
  END
  CheckDefFailure(lines, 'E1012: Type mismatch; expected number but got list<unknown>', 3)

  # dict of dict
  var d1: dict<number>
  d1.one = 1
  assert_equal({one: 1}, d1)

  var d2: dict<dict<number>>
  d2.one = {}
  d2.one.two = 123
  assert_equal({one: {two: 123}}, d2)

  var d3: dict<dict<dict<number>>>
  d3.one = {}
  d3.one.two = {}
  d3.one.two.three = 123
  assert_equal({one: {two: {three: 123}}}, d3)

  lines =<< trim END
      var d3: dict<dict<number>>
      d3.one = {}
      d3.one.two = {}
  END
  CheckDefFailure(lines, 'E1012: Type mismatch; expected number but got dict<unknown>', 3)

  # list of dict
  var ld: list<dict<number>>
  ld[0] = {}
  ld[0].one = 123
  assert_equal([{one: 123}], ld)

  lines =<< trim END
      var ld: list<dict<number>>
      ld[0] = []
  END
  CheckDefFailure(lines, 'E1012: Type mismatch; expected dict<number> but got list<unknown>', 2)

  # dict of list
  var dl: dict<list<number>>
  dl.one = []
  dl.one[0] = 123
  assert_equal({one: [123]}, dl)

  lines =<< trim END
      var dl: dict<list<number>>
      dl.one = {}
  END
  CheckDefFailure(lines, 'E1012: Type mismatch; expected list<number> but got dict<unknown>', 2)
enddef

def Test_extend_list()
  var lines =<< trim END
      vim9script
      var l: list<number>
      l += [123]
      assert_equal([123], l)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var list: list<string>
      extend(list, ['x'])
      assert_equal(['x'], list)
  END
  CheckScriptSuccess(lines)

  # appending to NULL list from a function
  lines =<< trim END
      vim9script
      var list: list<string>
      def Func()
        list += ['a', 'b']
      enddef
      Func()
      assert_equal(['a', 'b'], list)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var l: list<string> = test_null_list()
      extend(l, ['x'])
      assert_equal(['x'], l)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      extend(test_null_list(), ['x'])
  END
  CheckScriptFailure(lines, 'E1134:', 2)
enddef

def Test_extend_dict()
  var lines =<< trim END
      vim9script
      var d: dict<number>
      extend(d, #{a: 1})
      assert_equal(#{a: 1}, d)

      var d2: dict<number>
      d2['one'] = 1
      assert_equal(#{one: 1}, d2)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var d: dict<string> = test_null_dict()
      extend(d, #{a: 'x'})
      assert_equal(#{a: 'x'}, d)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      extend(test_null_dict(), #{a: 'x'})
  END
  CheckScriptFailure(lines, 'E1133:', 2)
enddef

def Test_single_letter_vars()
  # single letter variables
  var a: number = 123
  a = 123
  assert_equal(123, a)
  var b: number
  b = 123
  assert_equal(123, b)
  var g: number
  g = 123
  assert_equal(123, g)
  var s: number
  s = 123
  assert_equal(123, s)
  var t: number
  t = 123
  assert_equal(123, t)
  var v: number
  v = 123
  assert_equal(123, v)
  var w: number
  w = 123
  assert_equal(123, w)
enddef

def Test_vim9_single_char_vars()
  var lines =<< trim END
      vim9script

      # single character variable declarations work
      var a: string
      var b: number
      var l: list<any>
      var s: string
      var t: number
      var v: number
      var w: number

      # script-local variables can be used without s: prefix
      a = 'script-a'
      b = 111
      l = [1, 2, 3]
      s = 'script-s'
      t = 222
      v = 333
      w = 444

      assert_equal('script-a', a)
      assert_equal(111, b)
      assert_equal([1, 2, 3], l)
      assert_equal('script-s', s)
      assert_equal(222, t)
      assert_equal(333, v)
      assert_equal(444, w)
  END
  writefile(lines, 'Xsinglechar')
  source Xsinglechar
  delete('Xsinglechar')
enddef

def Test_assignment_list()
  var list1: list<bool> = [false, true, false]
  var list2: list<number> = [1, 2, 3]
  var list3: list<string> = ['sdf', 'asdf']
  var list4: list<any> = ['yes', true, 1234]
  var list5: list<blob> = [0z01, 0z02]

  var listS: list<string> = []
  var listN: list<number> = []

  assert_equal([1, 2, 3], list2)
  list2[-1] = 99
  assert_equal([1, 2, 99], list2)
  list2[-2] = 88
  assert_equal([1, 88, 99], list2)
  list2[-3] = 77
  assert_equal([77, 88, 99], list2)
  list2 += [100]
  assert_equal([77, 88, 99, 100], list2)

  list3 += ['end']
  assert_equal(['sdf', 'asdf', 'end'], list3)

  CheckDefExecFailure(['var ll = [1, 2, 3]', 'll[-4] = 6'], 'E684:')
  CheckDefExecFailure(['var [v1, v2] = [1, 2]'], 'E1092:')

  # type becomes list<any>
  var somelist = rand() > 0 ? [1, 2, 3] : ['a', 'b', 'c']
enddef

def Test_assignment_list_vim9script()
  var lines =<< trim END
    vim9script
    var v1: number
    var v2: number
    var v3: number
    [v1, v2, v3] = [1, 2, 3]
    assert_equal([1, 2, 3], [v1, v2, v3])
  END
  CheckScriptSuccess(lines)
enddef

def Test_assignment_dict()
  var dict1: dict<bool> = #{one: false, two: true}
  var dict2: dict<number> = #{one: 1, two: 2}
  var dict3: dict<string> = #{key: 'value'}
  var dict4: dict<any> = #{one: 1, two: '2'}
  var dict5: dict<blob> = #{one: 0z01, two: 0z02}

  # overwrite
  dict3['key'] = 'another'
  assert_equal(dict3, #{key: 'another'})
  dict3.key = 'yet another'
  assert_equal(dict3, #{key: 'yet another'})

  var lines =<< trim END
    var dd = #{one: 1}
    dd.one) = 2
  END
  CheckDefFailure(lines, 'E15:', 2)

  # empty key can be used
  var dd = {}
  dd[""] = 6
  assert_equal({'': 6}, dd)

  # type becomes dict<any>
  var somedict = rand() > 0 ? #{a: 1, b: 2} : #{a: 'a', b: 'b'}

  # assignment to script-local dict
  lines =<< trim END
    vim9script
    var test: dict<any> = {}
    def FillDict(): dict<any>
      test['a'] = 43
      return test
    enddef
    assert_equal(#{a: 43}, FillDict())
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    var test: dict<any>
    def FillDict(): dict<any>
      test['a'] = 43
      return test
    enddef
    FillDict()
  END
  CheckScriptFailure(lines, 'E1103:')

  # assignment to global dict
  lines =<< trim END
    vim9script
    g:test = {}
    def FillDict(): dict<any>
      g:test['a'] = 43
      return g:test
    enddef
    assert_equal(#{a: 43}, FillDict())
  END
  CheckScriptSuccess(lines)

  # assignment to buffer dict
  lines =<< trim END
    vim9script
    b:test = {}
    def FillDict(): dict<any>
      b:test['a'] = 43
      return b:test
    enddef
    assert_equal(#{a: 43}, FillDict())
  END
  CheckScriptSuccess(lines)
enddef

def Test_assignment_local()
  # Test in a separated file in order not to the current buffer/window/tab is
  # changed.
  var script_lines: list<string> =<< trim END
    let b:existing = 'yes'
    let w:existing = 'yes'
    let t:existing = 'yes'

    def Test_assignment_local_internal()
      b:newvar = 'new'
      assert_equal('new', b:newvar)
      assert_equal('yes', b:existing)
      b:existing = 'no'
      assert_equal('no', b:existing)
      b:existing ..= 'NO'
      assert_equal('noNO', b:existing)

      w:newvar = 'new'
      assert_equal('new', w:newvar)
      assert_equal('yes', w:existing)
      w:existing = 'no'
      assert_equal('no', w:existing)
      w:existing ..= 'NO'
      assert_equal('noNO', w:existing)

      t:newvar = 'new'
      assert_equal('new', t:newvar)
      assert_equal('yes', t:existing)
      t:existing = 'no'
      assert_equal('no', t:existing)
      t:existing ..= 'NO'
      assert_equal('noNO', t:existing)
    enddef
    call Test_assignment_local_internal()
  END
  CheckScriptSuccess(script_lines)
enddef

def Test_assignment_default()
  # Test default values.
  var thebool: bool
  assert_equal(v:false, thebool)

  var thenumber: number
  assert_equal(0, thenumber)

  if has('float')
    var thefloat: float
    assert_equal(0.0, thefloat)
  endif

  var thestring: string
  assert_equal('', thestring)

  var theblob: blob
  assert_equal(0z, theblob)

  var Thefunc: func
  assert_equal(test_null_function(), Thefunc)

  var thelist: list<any>
  assert_equal([], thelist)

  var thedict: dict<any>
  assert_equal({}, thedict)

  if has('channel')
    var thejob: job
    assert_equal(test_null_job(), thejob)

    var thechannel: channel
    assert_equal(test_null_channel(), thechannel)

    if has('unix') && executable('cat')
      # check with non-null job and channel, types must match
      thejob = job_start("cat ", #{})
      thechannel = job_getchannel(thejob)
      job_stop(thejob, 'kill')
    endif
  endif

  var nr = 1234 | nr = 5678
  assert_equal(5678, nr)
enddef

def Test_assignment_var_list()
  var lines =<< trim END
      var v1: string
      var v2: string
      var vrem: list<string>
      [v1] = ['aaa']
      assert_equal('aaa', v1)

      [v1, v2] = ['one', 'two']
      assert_equal('one', v1)
      assert_equal('two', v2)

      [v1, v2; vrem] = ['one', 'two']
      assert_equal('one', v1)
      assert_equal('two', v2)
      assert_equal([], vrem)

      [v1, v2; vrem] = ['one', 'two', 'three']
      assert_equal('one', v1)
      assert_equal('two', v2)
      assert_equal(['three'], vrem)

      [&ts, &sw] = [3, 4]
      assert_equal(3, &ts)
      assert_equal(4, &sw)
      set ts=8 sw=4

      [@a, @z] = ['aa', 'zz']
      assert_equal('aa', @a)
      assert_equal('zz', @z)

      [$SOME_VAR, $OTHER_VAR] = ['some', 'other']
      assert_equal('some', $SOME_VAR)
      assert_equal('other', $OTHER_VAR)

      [g:globalvar, s:scriptvar, b:bufvar, w:winvar, t:tabvar, v:errmsg] =
            ['global', 'script', 'buf', 'win', 'tab', 'error']
      assert_equal('global', g:globalvar)
      assert_equal('script', s:scriptvar)
      assert_equal('buf', b:bufvar)
      assert_equal('win', w:winvar)
      assert_equal('tab', t:tabvar)
      assert_equal('error', v:errmsg)
      unlet g:globalvar
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_assignment_vim9script()
  var lines =<< trim END
    vim9script
    def Func(): list<number>
      return [1, 2]
    enddef
    var name1: number
    var name2: number
    [name1, name2] =
          Func()
    assert_equal(1, name1)
    assert_equal(2, name2)
    var ll =
          Func()
    assert_equal([1, 2], ll)

    @/ = 'text'
    assert_equal('text', @/)
    @0 = 'zero'
    assert_equal('zero', @0)
    @1 = 'one'
    assert_equal('one', @1)
    @9 = 'nine'
    assert_equal('nine', @9)
    @- = 'minus'
    assert_equal('minus', @-)
    if has('clipboard_working')
      @* = 'star'
      assert_equal('star', @*)
      @+ = 'plus'
      assert_equal('plus', @+)
    endif

    var a: number = 123
    assert_equal(123, a)
    var s: string = 'yes'
    assert_equal('yes', s)
    var b: number = 42
    assert_equal(42, b)
    var w: number = 43
    assert_equal(43, w)
    var t: number = 44
    assert_equal(44, t)

    var to_var = 0
    to_var = 3
    assert_equal(3, to_var)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var n: number
      def Func()
        n = 'string'
      enddef
      defcompile
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected number but got string')
enddef

def Mess(): string
  v:foldstart = 123
  return 'xxx'
enddef

def Test_assignment_failure()
  CheckDefFailure(['var name=234'], 'E1004:')
  CheckDefFailure(['var name =234'], 'E1004:')
  CheckDefFailure(['var name= 234'], 'E1004:')

  CheckScriptFailure(['vim9script', 'var name=234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'var name=234'], "before and after '='")
  CheckScriptFailure(['vim9script', 'var name =234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'var name= 234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'var name = 234', 'name+=234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'var name = 234', 'name+=234'], "before and after '+='")
  CheckScriptFailure(['vim9script', 'var name = "x"', 'name..="y"'], 'E1004:')
  CheckScriptFailure(['vim9script', 'var name = "x"', 'name..="y"'], "before and after '..='")

  CheckDefFailure(['var true = 1'], 'E1034:')
  CheckDefFailure(['var false = 1'], 'E1034:')

  CheckDefFailure(['[a; b; c] = g:list'], 'E452:')
  CheckDefExecFailure(['var a: number',
                       '[a] = test_null_list()'], 'E1093:')
  CheckDefExecFailure(['var a: number',
                       '[a] = []'], 'E1093:')
  CheckDefExecFailure(['var x: number',
                       'var y: number',
                       '[x, y] = [1]'], 'E1093:')
  CheckDefExecFailure(['var x: string',
                       'var y: string',
                       '[x, y] = ["x"]'], 'E1093:')
  CheckDefExecFailure(['var x: number',
                       'var y: number',
                       'var z: list<number>',
                       '[x, y; z] = [1]'], 'E1093:')

  CheckDefFailure(['var somevar'], "E1022:")
  CheckDefFailure(['var &tabstop = 4'], 'E1052:')
  CheckDefFailure(['&g:option = 5'], 'E113:')
  CheckScriptFailure(['vim9script', 'var &tabstop = 4'], 'E1052:')

  CheckDefFailure(['var $VAR = 5'], 'E1016: Cannot declare an environment variable:')
  CheckScriptFailure(['vim9script', 'var $ENV = "xxx"'], 'E1016:')

  if has('dnd')
    CheckDefFailure(['var @~ = 5'], 'E1066:')
  else
    CheckDefFailure(['var @~ = 5'], 'E354:')
    CheckDefFailure(['@~ = 5'], 'E354:')
  endif
  CheckDefFailure(['var @a = 5'], 'E1066:')
  CheckDefFailure(['var @/ = "x"'], 'E1066:')
  CheckScriptFailure(['vim9script', 'var @a = "abc"'], 'E1066:')

  CheckDefFailure(['var g:var = 5'], 'E1016: Cannot declare a global variable:')
  CheckDefFailure(['var w:var = 5'], 'E1016: Cannot declare a window variable:')
  CheckDefFailure(['var b:var = 5'], 'E1016: Cannot declare a buffer variable:')
  CheckDefFailure(['var t:var = 5'], 'E1016: Cannot declare a tab variable:')

  CheckDefFailure(['var anr = 4', 'anr ..= "text"'], 'E1019:')
  CheckDefFailure(['var xnr += 4'], 'E1020:', 1)
  CheckScriptFailure(['vim9script', 'var xnr += 4'], 'E1020:')
  CheckDefFailure(["var xnr = xnr + 1"], 'E1001:', 1)
  CheckScriptFailure(['vim9script', 'var xnr = xnr + 4'], 'E121:')

  CheckScriptFailure(['vim9script', 'def Func()', 'var dummy = s:notfound', 'enddef', 'defcompile'], 'E1108:')

  CheckDefFailure(['var name: list<string> = [123]'], 'expected list<string> but got list<number>')
  CheckDefFailure(['var name: list<number> = ["xx"]'], 'expected list<number> but got list<string>')

  CheckDefFailure(['var name: dict<string> = #{key: 123}'], 'expected dict<string> but got dict<number>')
  CheckDefFailure(['var name: dict<number> = #{key: "xx"}'], 'expected dict<number> but got dict<string>')

  CheckDefFailure(['var name = feedkeys("0")'], 'E1031:')
  CheckDefFailure(['var name: number = feedkeys("0")'], 'expected number but got void')

  CheckDefFailure(['var name: dict <number>'], 'E1068:')
  CheckDefFailure(['var name: dict<number'], 'E1009:')

  assert_fails('s/^/\=Mess()/n', 'E794:')
  CheckDefFailure(['var name: dict<number'], 'E1009:')

  CheckDefFailure(['w:foo: number = 10'],
                  'E488: Trailing characters: : number = 1')
  CheckDefFailure(['t:foo: bool = true'],
                  'E488: Trailing characters: : bool = true')
  CheckDefFailure(['b:foo: string = "x"'],
                  'E488: Trailing characters: : string = "x"')
  CheckDefFailure(['g:foo: number = 123'],
                  'E488: Trailing characters: : number = 123')
enddef

def Test_assign_list()
  var l: list<string> = []
  l[0] = 'value'
  assert_equal('value', l[0])

  l[1] = 'asdf'
  assert_equal('value', l[0])
  assert_equal('asdf', l[1])
  assert_equal('asdf', l[-1])
  assert_equal('value', l[-2])

  var nrl: list<number> = []
  for i in range(5)
    nrl[i] = i
  endfor
  assert_equal([0, 1, 2, 3, 4], nrl)

  CheckDefFailure(["var l: list<number> = ['', true]"], 'E1012: Type mismatch; expected list<number> but got list<any>', 1)
  CheckDefFailure(["var l: list<list<number>> = [['', true]]"], 'E1012: Type mismatch; expected list<list<number>> but got list<list<any>>', 1)
enddef

def Test_assign_dict()
  var d: dict<string> = {}
  d['key'] = 'value'
  assert_equal('value', d['key'])

  d[123] = 'qwerty'
  assert_equal('qwerty', d[123])
  assert_equal('qwerty', d['123'])

  var nrd: dict<number> = {}
  for i in range(3)
    nrd[i] = i
  endfor
  assert_equal({'0': 0, '1': 1, '2': 2}, nrd)

  CheckDefFailure(["var d: dict<number> = #{a: '', b: true}"], 'E1012: Type mismatch; expected dict<number> but got dict<any>', 1)
  CheckDefFailure(["var d: dict<dict<number>> = #{x: #{a: '', b: true}}"], 'E1012: Type mismatch; expected dict<dict<number>> but got dict<dict<any>>', 1)
enddef

def Test_assign_dict_unknown_type()
  var lines =<< trim END
      vim9script
      var mylist = []
      mylist += [#{one: 'one'}]
      def Func()
        var dd = mylist[0]
        assert_equal('one', dd.one)
      enddef
      Func()
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      var mylist = [[]]
      mylist[0] += [#{one: 'one'}]
      def Func()
        var dd = mylist[0][0]
        assert_equal('one', dd.one)
      enddef
      Func()
  END
  CheckScriptSuccess(lines)
enddef

def Test_assign_lambda()
  # check if assign a lambda to a variable which type is func or any.
  var lines =<< trim END
      vim9script
      var FuncRef = {->123}
      assert_equal(123, FuncRef())
      var FuncRef_Func: func = {->123}
      assert_equal(123, FuncRef_Func())
      var FuncRef_Any: any = {->123}
      assert_equal(123, FuncRef_Any())
  END
  CheckScriptSuccess(lines)
enddef

def Test_heredoc()
  var lines =<< trim END # comment
    text
  END
  assert_equal(['text'], lines)

  CheckDefFailure(['var lines =<< trim END X', 'END'], 'E488:')
  CheckDefFailure(['var lines =<< trim END " comment', 'END'], 'E488:')

  lines =<< trim [END]
      def Func()
        var&lines =<< trim END
        x
        x
        x
        x
        x
        x
        x
        x
      enddef
      call Func()
  [END]
  CheckScriptFailure(lines, 'E990:')
enddef

def Test_let_func_call()
  var lines =<< trim END
    vim9script
    func GetValue()
      if exists('g:count')
        let g:count += 1
      else
        let g:count = 1
      endif
      return 'this'
    endfunc
    var val: string = GetValue() 
    # env var is always a string
    var env = $TERM
  END
  writefile(lines, 'Xfinished')
  source Xfinished
  # GetValue() is not called during discovery phase
  assert_equal(1, g:count)

  unlet g:count
  delete('Xfinished')
enddef

def Test_let_missing_type()
  var lines =<< trim END
    vim9script
    var name = g:unknown
  END
  CheckScriptFailure(lines, 'E121:')

  lines =<< trim END
    vim9script
    var nr: number = 123
    var name = nr
  END
  CheckScriptSuccess(lines)
enddef

def Test_let_declaration()
  var lines =<< trim END
    vim9script
    var name: string
    g:var_uninit = name
    name = 'text'
    g:var_test = name
    # prefixing s: is optional
    s:name = 'prefixed'
    g:var_prefixed = s:name

    var s:other: number
    other = 1234
    g:other_var = other

    # type is inferred
    s:dict = {'a': 222}
    def GetDictVal(key: any)
      g:dict_val = s:dict[key]
    enddef
    GetDictVal('a')
  END
  CheckScriptSuccess(lines)
  assert_equal('', g:var_uninit)
  assert_equal('text', g:var_test)
  assert_equal('prefixed', g:var_prefixed)
  assert_equal(1234, g:other_var)
  assert_equal(222, g:dict_val)

  unlet g:var_uninit
  unlet g:var_test
  unlet g:var_prefixed
  unlet g:other_var
enddef

def Test_let_declaration_fails()
  var lines =<< trim END
    vim9script
    final var: string
  END
  CheckScriptFailure(lines, 'E1125:')

  lines =<< trim END
    vim9script
    const var: string
  END
  CheckScriptFailure(lines, 'E1021:')

  lines =<< trim END
    vim9script
    var 9var: string
  END
  CheckScriptFailure(lines, 'E475:')
enddef

def Test_let_type_check()
  var lines =<< trim END
    vim9script
    var name: string
    name = 1234
  END
  CheckScriptFailure(lines, 'E1012:')

  lines =<< trim END
    vim9script
    var name:string
  END
  CheckScriptFailure(lines, 'E1069:')

  lines =<< trim END
    vim9script
    var name: asdf
  END
  CheckScriptFailure(lines, 'E1010:')

  lines =<< trim END
    vim9script
    var s:l: list<number>
    s:l = []
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    var s:d: dict<number>
    s:d = {}
  END
  CheckScriptSuccess(lines)
enddef

let g:dict_number = #{one: 1, two: 2}

def Test_let_list_dict_type()
  var ll: list<number>
  ll = [1, 2, 2, 3, 3, 3]->uniq()
  ll->assert_equal([1, 2, 3])

  var dd: dict<number>
  dd = g:dict_number
  dd->assert_equal(g:dict_number)

  var lines =<< trim END
      var ll: list<number>
      ll = [1, 2, 3]->map('"one"')
  END
  CheckDefExecFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>')
enddef

def Test_unlet()
  g:somevar = 'yes'
  assert_true(exists('g:somevar'))
  unlet g:somevar
  assert_false(exists('g:somevar'))
  unlet! g:somevar

  # also works for script-local variable in legacy Vim script
  s:somevar = 'legacy'
  assert_true(exists('s:somevar'))
  unlet s:somevar
  assert_false(exists('s:somevar'))
  unlet! s:somevar

  CheckScriptFailure([
   'vim9script',
   'var svar = 123',
   'unlet svar',
   ], 'E1081:')
  CheckScriptFailure([
   'vim9script',
   'var svar = 123',
   'unlet s:svar',
   ], 'E1081:')
  CheckScriptFailure([
   'vim9script',
   'var svar = 123',
   'def Func()',
   '  unlet svar',
   'enddef',
   'defcompile',
   ], 'E1081:')
  CheckScriptFailure([
   'vim9script',
   'var svar = 123',
   'def Func()',
   '  unlet s:svar',
   'enddef',
   'defcompile',
   ], 'E1081:')

  $ENVVAR = 'foobar'
  assert_equal('foobar', $ENVVAR)
  unlet $ENVVAR
  assert_equal('', $ENVVAR)
enddef


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
