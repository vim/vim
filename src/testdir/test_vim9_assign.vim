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
  let bool1: bool = true
  assert_equal(v:true, bool1)
  let bool2: bool = false
  assert_equal(v:false, bool2)

  let bool3: bool = 0
  assert_equal(false, bool3)
  let bool4: bool = 1
  assert_equal(true, bool4)

  let bool5: bool = 'yes' && 'no'
  assert_equal(true, bool5)
  let bool6: bool = [] && 99
  assert_equal(false, bool6)
  let bool7: bool = [] || #{a: 1} && 99
  assert_equal(true, bool7)

  let lines =<< trim END
    vim9script
    def GetFlag(): bool
      let flag: bool = 1
      return flag
    enddef
    let flag: bool = GetFlag()
    assert_equal(true, flag)
    flag = 0
    assert_equal(false, flag)
    flag = 1
    assert_equal(true, flag)
    flag = 99 || 123
    assert_equal(true, flag)
    flag = 'yes' && []
    assert_equal(false, flag)
  END
  CheckScriptSuccess(lines)
  CheckDefAndScriptFailure(['let x: bool = 2'], 'E1012:')
  CheckDefAndScriptFailure(['let x: bool = -1'], 'E1012:')
  CheckDefAndScriptFailure(['let x: bool = [1]'], 'E1012:')
  CheckDefAndScriptFailure(['let x: bool = {}'], 'E1012:')
  CheckDefAndScriptFailure(['let x: bool = "x"'], 'E1012:')
enddef

def Test_syntax()
  let var = 234
  let other: list<string> = ['asdf']
enddef

def Test_assignment()
  CheckDefFailure(['let x:string'], 'E1069:')
  CheckDefFailure(['let x:string = "x"'], 'E1069:')
  CheckDefFailure(['let a:string = "x"'], 'E1069:')
  CheckDefFailure(['let lambda = {-> "lambda"}'], 'E704:')

  let nr: number = 1234
  CheckDefFailure(['let nr: number = "asdf"'], 'E1012:')

  let a: number = 6 #comment
  assert_equal(6, a)

  if has('channel')
    let chan1: channel
    let job1: job
    let job2: job = job_start('willfail')
  endif
  if has('float')
    let float1: float = 3.4
  endif
  let Funky1: func
  let Funky2: func = function('len')
  let Party2: func = funcref('g:Test_syntax')

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

  let lines =<< trim END
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
    let f100: float = 100.0
    f100 /= 5
    assert_equal(20.0, f100)

    let f200: float = 200.0
    f200 /= 5.0
    assert_equal(40.0, f200)

    CheckDefFailure(['let nr: number = 200', 'nr /= 5.0'], 'E1012:')
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
  CheckDefFailure(['&ts = 3', 'let asdf'], 'E1022:')
  &ts = 8

  lines =<< trim END
    let save_TI = &t_TI
    &t_TI = ''
    assert_equal('', &t_TI)
    &t_TI = 'xxx'
    assert_equal('xxx', &t_TI)
    &t_TI = save_TI
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefFailure(['&t_TI = 123'], 'E1012:')
  CheckScriptFailure(['vim9script', '&t_TI = 123'], 'E928:')

  CheckDefFailure(['let s:var = 123'], 'E1101:')
  CheckDefFailure(['let s:var: number'], 'E1101:')

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

  # this should not leak
  if 0
    let text =<< trim END
      some text
    END
  endif
enddef

def Test_extend_list()
  let lines =<< trim END
      vim9script
      let l: list<number>
      l += [123]
      assert_equal([123], l)

      let d: dict<number>
      d['one'] = 1
      assert_equal(#{one: 1}, d)
  END
  CheckScriptSuccess(lines)
enddef

def Test_single_letter_vars()
  # single letter variables
  let a: number = 123
  a = 123
  assert_equal(123, a)
  let b: number
  b = 123
  assert_equal(123, b)
  let g: number
  g = 123
  assert_equal(123, g)
  let s: number
  s = 123
  assert_equal(123, s)
  let t: number
  t = 123
  assert_equal(123, t)
  let v: number
  v = 123
  assert_equal(123, v)
  let w: number
  w = 123
  assert_equal(123, w)
enddef

def Test_vim9_single_char_vars()
  let lines =<< trim END
      vim9script

      # single character variable declarations work
      let a: string
      let b: number
      let l: list<any>
      let s: string
      let t: number
      let v: number
      let w: number

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
  let list1: list<bool> = [false, true, false]
  let list2: list<number> = [1, 2, 3]
  let list3: list<string> = ['sdf', 'asdf']
  let list4: list<any> = ['yes', true, 1234]
  let list5: list<blob> = [0z01, 0z02]

  let listS: list<string> = []
  let listN: list<number> = []

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

  CheckDefExecFailure(['let ll = [1, 2, 3]', 'll[-4] = 6'], 'E684:')
  CheckDefExecFailure(['let [v1, v2] = [1, 2]'], 'E1092:')

  # type becomes list<any>
  let somelist = rand() > 0 ? [1, 2, 3] : ['a', 'b', 'c']
enddef

def Test_assignment_list_vim9script()
  let lines =<< trim END
    vim9script
    let v1: number
    let v2: number
    let v3: number
    [v1, v2, v3] = [1, 2, 3]
    assert_equal([1, 2, 3], [v1, v2, v3])
  END
  CheckScriptSuccess(lines)
enddef

def Test_assignment_dict()
  let dict1: dict<bool> = #{one: false, two: true}
  let dict2: dict<number> = #{one: 1, two: 2}
  let dict3: dict<string> = #{key: 'value'}
  let dict4: dict<any> = #{one: 1, two: '2'}
  let dict5: dict<blob> = #{one: 0z01, two: 0z02}

  # overwrite
  dict3['key'] = 'another'

  # empty key can be used
  let dd = {}
  dd[""] = 6
  assert_equal({'': 6}, dd)

  # type becomes dict<any>
  let somedict = rand() > 0 ? #{a: 1, b: 2} : #{a: 'a', b: 'b'}

  # assignment to script-local dict
  let lines =<< trim END
    vim9script
    let test: dict<any> = {}
    def FillDict(): dict<any>
      test['a'] = 43
      return test
    enddef
    assert_equal(#{a: 43}, FillDict())
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    let test: dict<any>
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
  let script_lines: list<string> =<< trim END
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
  let thebool: bool
  assert_equal(v:false, thebool)

  let thenumber: number
  assert_equal(0, thenumber)

  if has('float')
    let thefloat: float
    assert_equal(0.0, thefloat)
  endif

  let thestring: string
  assert_equal('', thestring)

  let theblob: blob
  assert_equal(0z, theblob)

  let Thefunc: func
  assert_equal(test_null_function(), Thefunc)

  let thelist: list<any>
  assert_equal([], thelist)

  let thedict: dict<any>
  assert_equal({}, thedict)

  if has('channel')
    let thejob: job
    assert_equal(test_null_job(), thejob)

    let thechannel: channel
    assert_equal(test_null_channel(), thechannel)

    if has('unix') && executable('cat')
      # check with non-null job and channel, types must match
      thejob = job_start("cat ", #{})
      thechannel = job_getchannel(thejob)
      job_stop(thejob, 'kill')
    endif
  endif

  let nr = 1234 | nr = 5678
  assert_equal(5678, nr)
enddef

def Test_assignment_var_list()
  let v1: string
  let v2: string
  let vrem: list<string>
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
enddef

def Test_assignment_vim9script()
  let lines =<< trim END
    vim9script
    def Func(): list<number>
      return [1, 2]
    enddef
    let var1: number
    let var2: number
    [var1, var2] =
          Func()
    assert_equal(1, var1)
    assert_equal(2, var2)
    let ll =
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

    let a: number = 123
    assert_equal(123, a)
    let s: string = 'yes'
    assert_equal('yes', s)
    let b: number = 42
    assert_equal(42, b)
    let w: number = 43
    assert_equal(43, w)
    let t: number = 44
    assert_equal(44, t)
  END
  CheckScriptSuccess(lines)
enddef

def Mess(): string
  v:foldstart = 123
  return 'xxx'
enddef

def Test_assignment_failure()
  CheckDefFailure(['let var=234'], 'E1004:')
  CheckDefFailure(['let var =234'], 'E1004:')
  CheckDefFailure(['let var= 234'], 'E1004:')

  CheckScriptFailure(['vim9script', 'let var=234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'let var=234'], "before and after '='")
  CheckScriptFailure(['vim9script', 'let var =234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'let var= 234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'let var = 234', 'var+=234'], 'E1004:')
  CheckScriptFailure(['vim9script', 'let var = 234', 'var+=234'], "before and after '+='")
  CheckScriptFailure(['vim9script', 'let var = "x"', 'var..="y"'], 'E1004:')
  CheckScriptFailure(['vim9script', 'let var = "x"', 'var..="y"'], "before and after '..='")

  CheckDefFailure(['let true = 1'], 'E1034:')
  CheckDefFailure(['let false = 1'], 'E1034:')

  CheckDefFailure(['[a; b; c] = g:list'], 'E452:')
  CheckDefExecFailure(['let a: number',
                       '[a] = test_null_list()'], 'E1093:')
  CheckDefExecFailure(['let a: number',
                       '[a] = []'], 'E1093:')
  CheckDefExecFailure(['let x: number',
                       'let y: number',
                       '[x, y] = [1]'], 'E1093:')
  CheckDefExecFailure(['let x: number',
                       'let y: number',
                       'let z: list<number>',
                       '[x, y; z] = [1]'], 'E1093:')

  CheckDefFailure(['let somevar'], "E1022:")
  CheckDefFailure(['let &tabstop = 4'], 'E1052:')
  CheckDefFailure(['&g:option = 5'], 'E113:')
  CheckScriptFailure(['vim9script', 'let &tabstop = 4'], 'E1052:')

  CheckDefFailure(['let $VAR = 5'], 'E1016: Cannot declare an environment variable:')
  CheckScriptFailure(['vim9script', 'let $ENV = "xxx"'], 'E1016:')

  if has('dnd')
    CheckDefFailure(['let @~ = 5'], 'E1066:')
  else
    CheckDefFailure(['let @~ = 5'], 'E354:')
    CheckDefFailure(['@~ = 5'], 'E354:')
  endif
  CheckDefFailure(['let @a = 5'], 'E1066:')
  CheckDefFailure(['let @/ = "x"'], 'E1066:')
  CheckScriptFailure(['vim9script', 'let @a = "abc"'], 'E1066:')

  CheckDefFailure(['let g:var = 5'], 'E1016: Cannot declare a global variable:')
  CheckDefFailure(['let w:var = 5'], 'E1016: Cannot declare a window variable:')
  CheckDefFailure(['let b:var = 5'], 'E1016: Cannot declare a buffer variable:')
  CheckDefFailure(['let t:var = 5'], 'E1016: Cannot declare a tab variable:')

  CheckDefFailure(['let anr = 4', 'anr ..= "text"'], 'E1019:')
  CheckDefFailure(['let xnr += 4'], 'E1020:', 1)
  CheckScriptFailure(['vim9script', 'let xnr += 4'], 'E1020:')
  CheckDefFailure(["let xnr = xnr + 1"], 'E1001:', 1)
  CheckScriptFailure(['vim9script', 'let xnr = xnr + 4'], 'E121:')

  CheckScriptFailure(['vim9script', 'def Func()', 'let dummy = s:notfound', 'enddef', 'defcompile'], 'E1108:')

  CheckDefFailure(['let var: list<string> = [123]'], 'expected list<string> but got list<number>')
  CheckDefFailure(['let var: list<number> = ["xx"]'], 'expected list<number> but got list<string>')

  CheckDefFailure(['let var: dict<string> = #{key: 123}'], 'expected dict<string> but got dict<number>')
  CheckDefFailure(['let var: dict<number> = #{key: "xx"}'], 'expected dict<number> but got dict<string>')

  CheckDefFailure(['let var = feedkeys("0")'], 'E1031:')
  CheckDefFailure(['let var: number = feedkeys("0")'], 'expected number but got void')

  CheckDefFailure(['let var: dict <number>'], 'E1068:')
  CheckDefFailure(['let var: dict<number'], 'E1009:')

  assert_fails('s/^/\=Mess()/n', 'E794:')
  CheckDefFailure(['let var: dict<number'], 'E1009:')

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
  let l: list<string> = []
  l[0] = 'value'
  assert_equal('value', l[0])

  l[1] = 'asdf'
  assert_equal('value', l[0])
  assert_equal('asdf', l[1])
  assert_equal('asdf', l[-1])
  assert_equal('value', l[-2])

  let nrl: list<number> = []
  for i in range(5)
    nrl[i] = i
  endfor
  assert_equal([0, 1, 2, 3, 4], nrl)
enddef

def Test_assign_dict()
  let d: dict<string> = {}
  d['key'] = 'value'
  assert_equal('value', d['key'])

  d[123] = 'qwerty'
  assert_equal('qwerty', d[123])
  assert_equal('qwerty', d['123'])

  let nrd: dict<number> = {}
  for i in range(3)
    nrd[i] = i
  endfor
  assert_equal({'0': 0, '1': 1, '2': 2}, nrd)
enddef

def Test_assign_dict_unknown_type()
  let lines =<< trim END
      vim9script
      let mylist = []
      mylist += [#{one: 'one'}]
      def Func()
        let dd = mylist[0]
        assert_equal('one', dd.one)
      enddef
      Func()
  END
  CheckScriptSuccess(lines)

  # doesn't work yet
  #lines =<< trim END
  #    vim9script
  #    let mylist = [[]]
  #    mylist[0] += [#{one: 'one'}]
  #    def Func()
  #      let dd = mylist[0][0]
  #      assert_equal('one', dd.one)
  #    enddef
  #    Func()
  #END
  #CheckScriptSuccess(lines)
enddef

def Test_assign_lambda()
  # check if assign a lambda to a variable which type is func or any.
  let lines =<< trim END
      vim9script
      let FuncRef = {->123}
      assert_equal(123, FuncRef())
      let FuncRef_Func: func = {->123}
      assert_equal(123, FuncRef_Func())
      let FuncRef_Any: any = {->123}
      assert_equal(123, FuncRef_Any())
  END
  CheckScriptSuccess(lines)
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
