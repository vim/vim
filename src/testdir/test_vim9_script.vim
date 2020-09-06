" Test various aspects of the Vim9 script language.

source check.vim
source term_util.vim
source view_util.vim
source vim9.vim
source shared.vim

def Test_syntax()
  let var = 234
  let other: list<string> = ['asdf']
enddef

def Test_range_only()
  new
  setline(1, ['blah', 'Blah'])
  :/Blah/
  assert_equal(2, getcurpos()[1])
  bwipe!

  # without range commands use current line
  new
  setline(1, ['one', 'two', 'three'])
  :2
  print
  assert_equal('two', Screenline(&lines))
  :3
  list
  assert_equal('three$', Screenline(&lines))
  bwipe!
enddef

let s:appendToMe = 'xxx'
let s:addToMe = 111
let g:existing = 'yes'
let g:inc_counter = 1
let $SOME_ENV_VAR = 'some'
let g:alist = [7]
let g:astring = 'text'
let g:anumber = 123

def Test_assignment()
  let bool1: bool = true
  assert_equal(v:true, bool1)
  let bool2: bool = false
  assert_equal(v:false, bool2)

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
  CheckDefExecFailure(['&ts = g:alist'], 'E1029: Expected number but got list')
  CheckDefFailure(['&ts = "xx"'], 'E1012:')
  CheckDefExecFailure(['&ts = g:astring'], 'E1029: Expected number but got string')
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
  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)

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

  # single letter variables
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
  call CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    let test: dict<any>
    def FillDict(): dict<any>
      test['a'] = 43
      return test
    enddef
    FillDict()
  END
  call CheckScriptFailure(lines, 'E1103:')

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
  call CheckScriptSuccess(lines)

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
  call CheckScriptSuccess(lines)
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
  call CheckScriptSuccess(script_lines)
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
   'let svar = 123',
   'unlet svar',
   ], 'E1081:')
  CheckScriptFailure([
   'vim9script',
   'let svar = 123',
   'unlet s:svar',
   ], 'E1081:')
  CheckScriptFailure([
   'vim9script',
   'let svar = 123',
   'def Func()',
   '  unlet svar',
   'enddef',
   'defcompile',
   ], 'E1081:')
  CheckScriptFailure([
   'vim9script',
   'let svar = 123',
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

def Test_delfunction()
  # Check function is defined in script namespace
  CheckScriptSuccess([
      'vim9script',
      'func CheckMe()',
      '  return 123',
      'endfunc',
      'assert_equal(123, s:CheckMe())',
      ])

  # Check function in script namespace cannot be deleted
  CheckScriptFailure([
      'vim9script',
      'func DeleteMe1()',
      'endfunc',
      'delfunction DeleteMe1',
      ], 'E1084:')
  CheckScriptFailure([
      'vim9script',
      'func DeleteMe2()',
      'endfunc',
      'def DoThat()',
      '  delfunction DeleteMe2',
      'enddef',
      'DoThat()',
      ], 'E1084:')
  CheckScriptFailure([
      'vim9script',
      'def DeleteMe3()',
      'enddef',
      'delfunction DeleteMe3',
      ], 'E1084:')
  CheckScriptFailure([
      'vim9script',
      'def DeleteMe4()',
      'enddef',
      'def DoThat()',
      '  delfunction DeleteMe4',
      'enddef',
      'DoThat()',
      ], 'E1084:')

  # Check that global :def function can be replaced and deleted
  let lines =<< trim END
      vim9script
      def g:Global(): string
        return "yes"
      enddef
      assert_equal("yes", g:Global())
      def! g:Global(): string
        return "no"
      enddef
      assert_equal("no", g:Global())
      delfunc g:Global
      assert_false(exists('*g:Global'))
  END
  CheckScriptSuccess(lines)

  # Check that global function can be replaced by a :def function and deleted
  lines =<< trim END
      vim9script
      func g:Global()
        return "yes"
      endfunc
      assert_equal("yes", g:Global())
      def! g:Global(): string
        return "no"
      enddef
      assert_equal("no", g:Global())
      delfunc g:Global
      assert_false(exists('*g:Global'))
  END
  CheckScriptSuccess(lines)

  # Check that global :def function can be replaced by a function and deleted
  lines =<< trim END
      vim9script
      def g:Global(): string
        return "yes"
      enddef
      assert_equal("yes", g:Global())
      func! g:Global()
        return "no"
      endfunc
      assert_equal("no", g:Global())
      delfunc g:Global
      assert_false(exists('*g:Global'))
  END
  CheckScriptSuccess(lines)
enddef

func Test_wrong_type()
  call CheckDefFailure(['let var: list<nothing>'], 'E1010:')
  call CheckDefFailure(['let var: list<list<nothing>>'], 'E1010:')
  call CheckDefFailure(['let var: dict<nothing>'], 'E1010:')
  call CheckDefFailure(['let var: dict<dict<nothing>>'], 'E1010:')

  call CheckDefFailure(['let var: dict<number'], 'E1009:')
  call CheckDefFailure(['let var: dict<list<number>'], 'E1009:')

  call CheckDefFailure(['let var: ally'], 'E1010:')
  call CheckDefFailure(['let var: bram'], 'E1010:')
  call CheckDefFailure(['let var: cathy'], 'E1010:')
  call CheckDefFailure(['let var: dom'], 'E1010:')
  call CheckDefFailure(['let var: freddy'], 'E1010:')
  call CheckDefFailure(['let var: john'], 'E1010:')
  call CheckDefFailure(['let var: larry'], 'E1010:')
  call CheckDefFailure(['let var: ned'], 'E1010:')
  call CheckDefFailure(['let var: pam'], 'E1010:')
  call CheckDefFailure(['let var: sam'], 'E1010:')
  call CheckDefFailure(['let var: vim'], 'E1010:')

  call CheckDefFailure(['let Ref: number', 'Ref()'], 'E1085:')
  call CheckDefFailure(['let Ref: string', 'let res = Ref()'], 'E1085:')
endfunc

func Test_const()
  call CheckDefFailure(['const var = 234', 'var = 99'], 'E1018:')
  call CheckDefFailure(['const one = 234', 'let one = 99'], 'E1017:')
  call CheckDefFailure(['const two'], 'E1021:')
  call CheckDefFailure(['const &option'], 'E996:')
endfunc

def Test_range_no_colon()
  CheckDefFailure(['%s/a/b/'], 'E1050:')
  CheckDefFailure(['+ s/a/b/'], 'E1050:')
  CheckDefFailure(['- s/a/b/'], 'E1050:')
  CheckDefFailure(['. s/a/b/'], 'E1050:')
enddef


def Test_block()
  let outer = 1
  {
    let inner = 2
    assert_equal(1, outer)
    assert_equal(2, inner)
  }
  assert_equal(1, outer)
enddef

func Test_block_failure()
  call CheckDefFailure(['{', 'let inner = 1', '}', 'echo inner'], 'E1001:')
  call CheckDefFailure(['}'], 'E1025:')
  call CheckDefFailure(['{', 'echo 1'], 'E1026:')
endfunc

func g:NoSuchFunc()
  echo 'none'
endfunc

def Test_try_catch()
  let l = []
  try # comment
    add(l, '1')
    throw 'wrong'
    add(l, '2')
  catch # comment
    add(l, v:exception)
  finally # comment
    add(l, '3')
  endtry # comment
  assert_equal(['1', 'wrong', '3'], l)

  l = []
  try
    try
      add(l, '1')
      throw 'wrong'
      add(l, '2')
    catch /right/
      add(l, v:exception)
    endtry
  catch /wrong/
    add(l, 'caught')
  finally
    add(l, 'finally')
  endtry
  assert_equal(['1', 'caught', 'finally'], l)

  let n: number
  try
    n = l[3]
  catch /E684:/
    n = 99
  endtry
  assert_equal(99, n)

  try
    # string slice returns a string, not a number
    n = g:astring[3]
  catch /E1029:/
    n = 77
  endtry
  assert_equal(77, n)

  try
    n = l[g:astring]
  catch /E1029:/
    n = 88
  endtry
  assert_equal(88, n)

  try
    n = s:does_not_exist
  catch /E121:/
    n = 111
  endtry
  assert_equal(111, n)

  try
    n = g:does_not_exist
  catch /E121:/
    n = 121
  endtry
  assert_equal(121, n)

  let d = #{one: 1}
  try
    n = d[g:astring]
  catch /E716:/
    n = 222
  endtry
  assert_equal(222, n)

  try
    n = -g:astring
  catch /E39:/
    n = 233
  endtry
  assert_equal(233, n)

  try
    n = +g:astring
  catch /E1030:/
    n = 244
  endtry
  assert_equal(244, n)

  try
    n = +g:alist
  catch /E745:/
    n = 255
  endtry
  assert_equal(255, n)

  let nd: dict<any>
  try
    nd = {g:anumber: 1}
  catch /E1029:/
    n = 266
  endtry
  assert_equal(266, n)

  try
    [n] = [1, 2, 3]
  catch /E1093:/
    n = 277
  endtry
  assert_equal(277, n)

  try
    &ts = g:astring
  catch /E1029:/
    n = 288
  endtry
  assert_equal(288, n)

  try
    &backspace = 'asdf'
  catch /E474:/
    n = 299
  endtry
  assert_equal(299, n)

  l = [1]
  try
    l[3] = 3
  catch /E684:/
    n = 300
  endtry
  assert_equal(300, n)

  try
    unlet g:does_not_exist
  catch /E108:/
    n = 322
  endtry
  assert_equal(322, n)

  try
    d = {'text': 1, g:astring: 2}
  catch /E721:/
    n = 333
  endtry
  assert_equal(333, n)

  try
    l = DeletedFunc()
  catch /E933:/
    n = 344
  endtry
  assert_equal(344, n)

  try
    echo len(v:true)
  catch /E701:/
    n = 355
  endtry
  assert_equal(355, n)

  let P = function('g:NoSuchFunc')
  delfunc g:NoSuchFunc
  try
    echo P()
  catch /E117:/
    n = 366
  endtry
  assert_equal(366, n)

  try
    echo g:NoSuchFunc()
  catch /E117:/
    n = 377
  endtry
  assert_equal(377, n)

  try
    echo g:alist + 4
  catch /E745:/
    n = 388
  endtry
  assert_equal(388, n)

  try
    echo 4 + g:alist
  catch /E745:/
    n = 399
  endtry
  assert_equal(399, n)

  try
    echo g:alist.member
  catch /E715:/
    n = 400
  endtry
  assert_equal(400, n)

  try
    echo d.member
  catch /E716:/
    n = 411
  endtry
  assert_equal(411, n)
enddef

def DeletedFunc(): list<any>
  return ['delete me']
enddef
defcompile
delfunc DeletedFunc

def ThrowFromDef()
  throw "getout" # comment
enddef

func CatchInFunc()
  try
    call ThrowFromDef()
  catch
    let g:thrown_func = v:exception
  endtry
endfunc

def CatchInDef()
  try
    ThrowFromDef()
  catch
    g:thrown_def = v:exception
  endtry
enddef

def ReturnFinally(): string
  try
    return 'intry'
  finally
    g:in_finally = 'finally'
  endtry
  return 'end'
enddef

def Test_try_catch_nested()
  CatchInFunc()
  assert_equal('getout', g:thrown_func)

  CatchInDef()
  assert_equal('getout', g:thrown_def)

  assert_equal('intry', ReturnFinally())
  assert_equal('finally', g:in_finally)
enddef

def Test_try_catch_match()
  let seq = 'a'
  try
    throw 'something'
  catch /nothing/
    seq ..= 'x'
  catch /some/
    seq ..= 'b'
  catch /asdf/
    seq ..= 'x'
  catch ?a\?sdf?
    seq ..= 'y'
  finally
    seq ..= 'c'
  endtry
  assert_equal('abc', seq)
enddef

def Test_try_catch_fails()
  CheckDefFailure(['catch'], 'E603:')
  CheckDefFailure(['try', 'echo 0', 'catch', 'catch'], 'E1033:')
  CheckDefFailure(['try', 'echo 0', 'catch /pat'], 'E1067:')
  CheckDefFailure(['finally'], 'E606:')
  CheckDefFailure(['try', 'echo 0', 'finally', 'echo 1', 'finally'], 'E607:')
  CheckDefFailure(['endtry'], 'E602:')
  CheckDefFailure(['while 1', 'endtry'], 'E170:')
  CheckDefFailure(['for i in range(5)', 'endtry'], 'E170:')
  CheckDefFailure(['if 2', 'endtry'], 'E171:')
  CheckDefFailure(['try', 'echo 1', 'endtry'], 'E1032:')

  CheckDefFailure(['throw'], 'E1015:')
  CheckDefFailure(['throw xxx'], 'E1001:')
enddef

def Test_throw_vimscript()
  # only checks line continuation
  let lines =<< trim END
      vim9script
      try
        throw 'one'
              .. 'two'
      catch
        assert_equal('onetwo', v:exception)
      endtry
  END
  CheckScriptSuccess(lines)
enddef

def Test_error_in_nested_function()
  # an error in a nested :function aborts executin in the calling :def function
  let lines =<< trim END
      vim9script
      def Func()
        Error()
        g:test_var = 1
      enddef
      func Error() abort
        eval [][0]
      endfunc
      Func()
  END
  g:test_var = 0
  CheckScriptFailure(lines, 'E684:')
  assert_equal(0, g:test_var)
enddef

def Test_cexpr_vimscript()
  # only checks line continuation
  set errorformat=File\ %f\ line\ %l
  let lines =<< trim END
      vim9script
      cexpr 'File'
                .. ' someFile' ..
                   ' line 19'
      assert_equal(19, getqflist()[0].lnum)
  END
  CheckScriptSuccess(lines)
  set errorformat&
enddef

def Test_statusline_syntax()
  # legacy syntax is used for 'statusline'
  let lines =<< trim END
      vim9script
      func g:Status()
        return '%{"x" is# "x"}'
      endfunc
      set laststatus=2 statusline=%!Status()
      redrawstatus
      set laststatus statusline= 
  END
  CheckScriptSuccess(lines)
enddef

def Test_list_vimscript()
  # checks line continuation and comments
  let lines =<< trim END
      vim9script
      let mylist = [
            'one',
            # comment
            'two', # empty line follows

            'three',
            ]
      assert_equal(['one', 'two', 'three'], mylist)
  END
  CheckScriptSuccess(lines)

  # check all lines from heredoc are kept
  lines =<< trim END
      # comment 1
      two
      # comment 3

      five
      # comment 6
  END
  assert_equal(['# comment 1', 'two', '# comment 3', '', 'five', '# comment 6'], lines)
enddef

if has('channel')
  let someJob = test_null_job()

  def FuncWithError()
    echomsg g:someJob
  enddef

  func Test_convert_emsg_to_exception()
    try
      call FuncWithError()
    catch
      call assert_match('Vim:E908:', v:exception)
    endtry
  endfunc
endif

let s:export_script_lines =<< trim END
  vim9script
  let name: string = 'bob'
  def Concat(arg: string): string
    return name .. arg
  enddef
  g:result = Concat('bie')
  g:localname = name

  export const CONST = 1234
  export let exported = 9876
  export let exp_name = 'John'
  export def Exported(): string
    return 'Exported'
  enddef
END

def Undo_export_script_lines()
  unlet g:result
  unlet g:localname
enddef

def Test_vim9_import_export()
  let import_script_lines =<< trim END
    vim9script
    import {exported, Exported} from './Xexport.vim'
    g:imported = exported
    exported += 3
    g:imported_added = exported
    g:imported_func = Exported()

    def GetExported(): string
      let local_dict = #{ref: Exported}
      return local_dict.ref()
    enddef
    g:funcref_result = GetExported()

    import {exp_name} from './Xexport.vim'
    g:imported_name = exp_name
    exp_name ..= ' Doe'
    g:imported_name_appended = exp_name
    g:imported_later = exported
  END

  writefile(import_script_lines, 'Ximport.vim')
  writefile(s:export_script_lines, 'Xexport.vim')

  source Ximport.vim

  assert_equal('bobbie', g:result)
  assert_equal('bob', g:localname)
  assert_equal(9876, g:imported)
  assert_equal(9879, g:imported_added)
  assert_equal(9879, g:imported_later)
  assert_equal('Exported', g:imported_func)
  assert_equal('Exported', g:funcref_result)
  assert_equal('John', g:imported_name)
  assert_equal('John Doe', g:imported_name_appended)
  assert_false(exists('g:name'))

  Undo_export_script_lines()
  unlet g:imported
  unlet g:imported_added
  unlet g:imported_later
  unlet g:imported_func
  unlet g:imported_name g:imported_name_appended
  delete('Ximport.vim')

  # similar, with line breaks
  let import_line_break_script_lines =<< trim END
    vim9script
    import {
        exported,
        Exported,
        }
        from
        './Xexport.vim'
    g:imported = exported
    exported += 5
    g:imported_added = exported
    g:imported_func = Exported()
  END
  writefile(import_line_break_script_lines, 'Ximport_lbr.vim')
  source Ximport_lbr.vim

  assert_equal(9876, g:imported)
  assert_equal(9881, g:imported_added)
  assert_equal('Exported', g:imported_func)

  # exported script not sourced again
  assert_false(exists('g:result'))
  unlet g:imported
  unlet g:imported_added
  unlet g:imported_func
  delete('Ximport_lbr.vim')

  # import inside :def function
  let import_in_def_lines =<< trim END
    vim9script
    def ImportInDef()
      import exported from './Xexport.vim'
      g:imported = exported
      exported += 7
      g:imported_added = exported
    enddef
    ImportInDef()
  END
  writefile(import_in_def_lines, 'Ximport2.vim')
  source Ximport2.vim
  # TODO: this should be 9879
  assert_equal(9876, g:imported)
  assert_equal(9883, g:imported_added)
  unlet g:imported
  unlet g:imported_added
  delete('Ximport2.vim')

  let import_star_as_lines =<< trim END
    vim9script
    import * as Export from './Xexport.vim'
    def UseExport()
      g:imported = Export.exported
    enddef
    UseExport()
  END
  writefile(import_star_as_lines, 'Ximport.vim')
  source Ximport.vim
  assert_equal(9883, g:imported)

  let import_star_as_lines_no_dot =<< trim END
    vim9script
    import * as Export from './Xexport.vim'
    def Func()
      let dummy = 1
      let imported = Export + dummy
    enddef
    defcompile
  END
  writefile(import_star_as_lines_no_dot, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1060:', '', 2, 'Func')

  let import_star_as_lines_dot_space =<< trim END
    vim9script
    import * as Export from './Xexport.vim'
    def Func()
      let imported = Export . exported
    enddef
    defcompile
  END
  writefile(import_star_as_lines_dot_space, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1074:', '', 1, 'Func')

  let import_star_as_lines_missing_name =<< trim END
    vim9script
    import * as Export from './Xexport.vim'
    def Func()
      let imported = Export.
    enddef
    defcompile
  END
  writefile(import_star_as_lines_missing_name, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1048:', '', 1, 'Func')

  let import_star_as_lbr_lines =<< trim END
    vim9script
    import *
        as Export
        from
        './Xexport.vim'
    def UseExport()
      g:imported = Export.exported
    enddef
    UseExport()
  END
  writefile(import_star_as_lbr_lines, 'Ximport.vim')
  source Ximport.vim
  assert_equal(9883, g:imported)

  let import_star_lines =<< trim END
    vim9script
    import * from './Xexport.vim'
  END
  writefile(import_star_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1045:', '', 2, 'Ximport.vim')

  # try to import something that exists but is not exported
  let import_not_exported_lines =<< trim END
    vim9script
    import name from './Xexport.vim'
  END
  writefile(import_not_exported_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1049:', '', 2, 'Ximport.vim')

  # try to import something that is already defined
  let import_already_defined =<< trim END
    vim9script
    let exported = 'something'
    import exported from './Xexport.vim'
  END
  writefile(import_already_defined, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1073:', '', 3, 'Ximport.vim')

  # try to import something that is already defined
  import_already_defined =<< trim END
    vim9script
    let exported = 'something'
    import * as exported from './Xexport.vim'
  END
  writefile(import_already_defined, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1073:', '', 3, 'Ximport.vim')

  # try to import something that is already defined
  import_already_defined =<< trim END
    vim9script
    let exported = 'something'
    import {exported} from './Xexport.vim'
  END
  writefile(import_already_defined, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1073:', '', 3, 'Ximport.vim')

  # import a very long name, requires making a copy
  let import_long_name_lines =<< trim END
    vim9script
    import name012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 from './Xexport.vim'
  END
  writefile(import_long_name_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1048:', '', 2, 'Ximport.vim')

  let import_no_from_lines =<< trim END
    vim9script
    import name './Xexport.vim'
  END
  writefile(import_no_from_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1070:', '', 2, 'Ximport.vim')

  let import_invalid_string_lines =<< trim END
    vim9script
    import name from Xexport.vim
  END
  writefile(import_invalid_string_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1071:', '', 2, 'Ximport.vim')

  let import_wrong_name_lines =<< trim END
    vim9script
    import name from './XnoExport.vim'
  END
  writefile(import_wrong_name_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1053:', '', 2, 'Ximport.vim')

  let import_missing_comma_lines =<< trim END
    vim9script
    import {exported name} from './Xexport.vim'
  END
  writefile(import_missing_comma_lines, 'Ximport3.vim')
  assert_fails('source Ximport3.vim', 'E1046:', '', 2, 'Ximport3.vim')

  delete('Ximport.vim')
  delete('Ximport3.vim')
  delete('Xexport.vim')

  # Check that in a Vim9 script 'cpo' is set to the Vim default.
  set cpo&vi
  let cpo_before = &cpo
  let lines =<< trim END
    vim9script
    g:cpo_in_vim9script = &cpo
  END
  writefile(lines, 'Xvim9_script')
  source Xvim9_script
  assert_equal(cpo_before, &cpo)
  set cpo&vim
  assert_equal(&cpo, g:cpo_in_vim9script)
  delete('Xvim9_script')
enddef

func g:Trigger()
  source Ximport.vim
  return "echo 'yes'\<CR>"
endfunc

def Test_import_export_expr_map()
  # check that :import and :export work when buffer is locked
  let export_lines =<< trim END
    vim9script
    export def That(): string
      return 'yes'
    enddef
  END
  writefile(export_lines, 'Xexport_that.vim')

  let import_lines =<< trim END
    vim9script
    import That from './Xexport_that.vim'
    assert_equal('yes', That())
  END
  writefile(import_lines, 'Ximport.vim')

  nnoremap <expr> trigger g:Trigger()
  feedkeys('trigger', "xt")

  delete('Xexport_that.vim')
  delete('Ximport.vim')
  nunmap trigger
enddef

def Test_import_in_filetype()
  # check that :import works when the buffer is locked
  mkdir('ftplugin', 'p')
  let export_lines =<< trim END
    vim9script
    export let That = 'yes'
  END
  writefile(export_lines, 'ftplugin/Xexport_ft.vim')

  let import_lines =<< trim END
    vim9script
    import That from './Xexport_ft.vim'
    assert_equal('yes', That)
    g:did_load_mytpe = 1
  END
  writefile(import_lines, 'ftplugin/qf.vim')

  let save_rtp = &rtp
  &rtp = getcwd() .. ',' .. &rtp

  filetype plugin on
  copen
  assert_equal(1, g:did_load_mytpe)

  quit!
  delete('Xexport_ft.vim')
  delete('ftplugin', 'rf')
  &rtp = save_rtp
enddef

def Test_use_import_in_mapping()
  let lines =<< trim END
      vim9script
      export def Funcx()
        g:result = 42
      enddef
  END
  writefile(lines, 'XsomeExport.vim')
  lines =<< trim END
      vim9script
      import Funcx from './XsomeExport.vim'
      nnoremap <F3> :call <sid>Funcx()<cr>
  END
  writefile(lines, 'Xmapscript.vim')

  source Xmapscript.vim
  feedkeys("\<F3>", "xt")
  assert_equal(42, g:result)

  unlet g:result
  delete('XsomeExport.vim')
  delete('Xmapscript.vim')
  nunmap <F3>
enddef

def Test_vim9script_fails()
  CheckScriptFailure(['scriptversion 2', 'vim9script'], 'E1039:')
  CheckScriptFailure(['vim9script', 'scriptversion 2'], 'E1040:')
  CheckScriptFailure(['export let some = 123'], 'E1042:')
  CheckScriptFailure(['import some from "./Xexport.vim"'], 'E1048:')
  CheckScriptFailure(['vim9script', 'export let g:some'], 'E1022:')
  CheckScriptFailure(['vim9script', 'export echo 134'], 'E1043:')

  CheckScriptFailure(['vim9script', 'let str: string', 'str = 1234'], 'E1012:')
  CheckScriptFailure(['vim9script', 'const str = "asdf"', 'str = "xxx"'], 'E46:')

  assert_fails('vim9script', 'E1038:')
  assert_fails('export something', 'E1043:')
enddef

func Test_import_fails_without_script()
  CheckRunVimInTerminal

  " call indirectly to avoid compilation error for missing functions
  call Run_Test_import_fails_on_command_line()
endfunc

def Run_Test_import_fails_on_command_line()
  let export =<< trim END
    vim9script
    export def Foo(): number
        return 0
    enddef
  END
  writefile(export, 'XexportCmd.vim')

  let buf = RunVimInTerminal('-c "import Foo from ''./XexportCmd.vim''"', #{
                rows: 6, wait_for_ruler: 0})
  WaitForAssert({-> assert_match('^E1094:', term_getline(buf, 5))})

  delete('XexportCmd.vim')
  StopVimInTerminal(buf)
enddef

def Test_vim9script_reload_import()
  let lines =<< trim END
    vim9script
    const var = ''
    let valone = 1234
    def MyFunc(arg: string)
       valone = 5678
    enddef
  END
  let morelines =<< trim END
    let valtwo = 222
    export def GetValtwo(): number
      return valtwo
    enddef
  END
  writefile(lines + morelines, 'Xreload.vim')
  source Xreload.vim
  source Xreload.vim
  source Xreload.vim

  let testlines =<< trim END
    vim9script
    def TheFunc()
      import GetValtwo from './Xreload.vim'
      assert_equal(222, GetValtwo())
    enddef
    TheFunc()
  END
  writefile(testlines, 'Ximport.vim')
  source Ximport.vim

  # Test that when not using "morelines" GetValtwo() and valtwo are still
  # defined, because import doesn't reload a script.
  writefile(lines, 'Xreload.vim')
  source Ximport.vim

  # cannot declare a var twice
  lines =<< trim END
    vim9script
    let valone = 1234
    let valone = 5678
  END
  writefile(lines, 'Xreload.vim')
  assert_fails('source Xreload.vim', 'E1041:', '', 3, 'Xreload.vim')

  delete('Xreload.vim')
  delete('Ximport.vim')
enddef

def s:RetSome(): string
  return 'some'
enddef

" Not exported function that is referenced needs to be accessed by the
" script-local name.
def Test_vim9script_funcref()
  let sortlines =<< trim END
      vim9script
      def Compare(i1: number, i2: number): number
        return i2 - i1
      enddef

      export def FastSort(): list<number>
        return range(5)->sort(Compare)
      enddef
  END
  writefile(sortlines, 'Xsort.vim')

  let lines =<< trim END
    vim9script
    import FastSort from './Xsort.vim'
    def Test()
      g:result = FastSort()
    enddef
    Test()
  END
  writefile(lines, 'Xscript.vim')

  source Xscript.vim
  assert_equal([4, 3, 2, 1, 0], g:result)

  unlet g:result
  delete('Xsort.vim')
  delete('Xscript.vim')

  let Funcref = function('s:RetSome')
  assert_equal('some', Funcref())
enddef

" Check that when searching for "FilterFunc" it finds the import in the
" script where FastFilter() is called from, both as a string and as a direct
" function reference.
def Test_vim9script_funcref_other_script()
  let filterLines =<< trim END
    vim9script
    export def FilterFunc(idx: number, val: number): bool
      return idx % 2 == 1
    enddef
    export def FastFilter(): list<number>
      return range(10)->filter('FilterFunc')
    enddef
    export def FastFilterDirect(): list<number>
      return range(10)->filter(FilterFunc)
    enddef
  END
  writefile(filterLines, 'Xfilter.vim')

  let lines =<< trim END
    vim9script
    import {FilterFunc, FastFilter, FastFilterDirect} from './Xfilter.vim'
    def Test()
      let x: list<number> = FastFilter()
    enddef
    Test()
    def TestDirect()
      let x: list<number> = FastFilterDirect()
    enddef
    TestDirect()
  END
  CheckScriptSuccess(lines)
  delete('Xfilter.vim')
enddef

def Test_vim9script_reload_delfunc()
  let first_lines =<< trim END
    vim9script
    def FuncYes(): string
      return 'yes'
    enddef
  END
  let withno_lines =<< trim END
    def FuncNo(): string
      return 'no'
    enddef
    def g:DoCheck(no_exists: bool)
      assert_equal('yes', FuncYes())
      assert_equal('no', FuncNo())
    enddef
  END
  let nono_lines =<< trim END
    def g:DoCheck(no_exists: bool)
      assert_equal('yes', FuncYes())
      assert_fails('FuncNo()', 'E117:', '', 2, 'DoCheck')
    enddef
  END

  # FuncNo() is defined
  writefile(first_lines + withno_lines, 'Xreloaded.vim')
  source Xreloaded.vim
  g:DoCheck(true)

  # FuncNo() is not redefined
  writefile(first_lines + nono_lines, 'Xreloaded.vim')
  source Xreloaded.vim
  g:DoCheck()

  # FuncNo() is back
  writefile(first_lines + withno_lines, 'Xreloaded.vim')
  source Xreloaded.vim
  g:DoCheck()

  delete('Xreloaded.vim')
enddef

def Test_vim9script_reload_delvar()
  # write the script with a script-local variable
  let lines =<< trim END
    vim9script
    let var = 'string'
  END
  writefile(lines, 'XreloadVar.vim')
  source XreloadVar.vim

  # now write the script using the same variable locally - works
  lines =<< trim END
    vim9script
    def Func()
      let var = 'string'
    enddef
  END
  writefile(lines, 'XreloadVar.vim')
  source XreloadVar.vim

  delete('XreloadVar.vim')
enddef

def Test_import_absolute()
  let import_lines = [
        'vim9script',
        'import exported from "' .. escape(getcwd(), '\') .. '/Xexport_abs.vim"',
        'def UseExported()',
        '  g:imported_abs = exported',
        '  exported = 8888',
        '  g:imported_after = exported',
        'enddef',
        'UseExported()',
        'g:import_disassembled = execute("disass UseExported")',
        ]
  writefile(import_lines, 'Ximport_abs.vim')
  writefile(s:export_script_lines, 'Xexport_abs.vim')

  source Ximport_abs.vim

  assert_equal(9876, g:imported_abs)
  assert_equal(8888, g:imported_after)
  assert_match('<SNR>\d\+_UseExported.*' ..
          'g:imported_abs = exported.*' ..
          '0 LOADSCRIPT exported from .*Xexport_abs.vim.*' ..
          '1 STOREG g:imported_abs.*' ..
          'exported = 8888.*' ..
          '3 STORESCRIPT exported in .*Xexport_abs.vim.*' ..
          'g:imported_after = exported.*' ..
          '4 LOADSCRIPT exported from .*Xexport_abs.vim.*' ..
          '5 STOREG g:imported_after.*',
        g:import_disassembled)

  Undo_export_script_lines()
  unlet g:imported_abs
  unlet g:import_disassembled

  delete('Ximport_abs.vim')
  delete('Xexport_abs.vim')
enddef

def Test_import_rtp()
  let import_lines = [
        'vim9script',
        'import exported from "Xexport_rtp.vim"',
        'g:imported_rtp = exported',
        ]
  writefile(import_lines, 'Ximport_rtp.vim')
  mkdir('import')
  writefile(s:export_script_lines, 'import/Xexport_rtp.vim')

  let save_rtp = &rtp
  &rtp = getcwd()
  source Ximport_rtp.vim
  &rtp = save_rtp

  assert_equal(9876, g:imported_rtp)

  Undo_export_script_lines()
  unlet g:imported_rtp
  delete('Ximport_rtp.vim')
  delete('import', 'rf')
enddef

def Test_import_compile_error()
  let export_lines = [
        'vim9script',
        'export def ExpFunc(): string',
        '  return notDefined',
        'enddef',
        ]
  writefile(export_lines, 'Xexported.vim')

  let import_lines = [
        'vim9script',
        'import ExpFunc from "./Xexported.vim"',
        'def ImpFunc()',
        '  echo ExpFunc()',
        'enddef',
        'defcompile',
        ]
  writefile(import_lines, 'Ximport.vim')

  try
    source Ximport.vim
  catch /E1001/
    # Error should be fore the Xexported.vim file.
    assert_match('E1001: variable not found: notDefined', v:exception)
    assert_match('function <SNR>\d\+_ImpFunc\[1\]..<SNR>\d\+_ExpFunc, line 1', v:throwpoint)
  endtry

  delete('Xexported.vim')
  delete('Ximport.vim')
enddef

def Test_func_redefine_error()
  let lines = [
        'vim9script',
        'def Func()',
        '  eval [][0]',
        'enddef',
        'Func()',
        ]
  writefile(lines, 'Xtestscript.vim')

  for count in range(3)
    try
      source Xtestscript.vim
    catch /E684/
      # function name should contain <SNR> every time
      assert_match('E684: list index out of range', v:exception)
      assert_match('function <SNR>\d\+_Func, line 1', v:throwpoint)
    endtry
  endfor

  delete('Xtestscript.vim')
enddef

def Test_func_overrules_import_fails()
  let export_lines =<< trim END
      vim9script
      export def Func()
        echo 'imported'
      enddef
  END
  writefile(export_lines, 'XexportedFunc.vim')

  let lines =<< trim END
    vim9script
    import Func from './XexportedFunc.vim'
    def Func()
      echo 'local to function'
    enddef
  END
  CheckScriptFailure(lines, 'E1073:')

  lines =<< trim END
    vim9script
    import Func from './XexportedFunc.vim'
    def Outer()
      def Func()
        echo 'local to function'
      enddef
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E1073:')

  delete('XexportedFunc.vim')
enddef

def Test_func_redefine_fails()
  let lines =<< trim END
    vim9script
    def Func()
      echo 'one'
    enddef
    def Func()
      echo 'two'
    enddef
  END
  CheckScriptFailure(lines, 'E1073:')

  lines =<< trim END
    vim9script
    def Foo(): string
      return 'foo'
      enddef
    def Func()
      let  Foo = {-> 'lambda'}
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E1073:')
enddef

def Test_fixed_size_list()
  # will be allocated as one piece of memory, check that changes work
  let l = [1, 2, 3, 4]
  l->remove(0)
  l->add(5)
  l->insert(99, 1)
  assert_equal([2, 99, 3, 4, 5], l)
enddef

def Test_no_insert_xit()
  CheckDefExecFailure(['a = 1'], 'E1100:')
  CheckDefExecFailure(['c = 1'], 'E1100:')
  CheckDefExecFailure(['i = 1'], 'E1100:')
  CheckDefExecFailure(['t = 1'], 'E1100:')
  CheckDefExecFailure(['x = 1'], 'E1100:')

  CheckScriptFailure(['vim9script', 'a = 1'], 'E488:')
  CheckScriptFailure(['vim9script', 'a'], 'E1100:')
  CheckScriptFailure(['vim9script', 'c = 1'], 'E488:')
  CheckScriptFailure(['vim9script', 'c'], 'E1100:')
  CheckScriptFailure(['vim9script', 'i = 1'], 'E488:')
  CheckScriptFailure(['vim9script', 'i'], 'E1100:')
  CheckScriptFailure(['vim9script', 't'], 'E1100:')
  CheckScriptFailure(['vim9script', 't = 1'], 'E1100:')
  CheckScriptFailure(['vim9script', 'x = 1'], 'E1100:')
enddef

def IfElse(what: number): string
  let res = ''
  if what == 1
    res = "one"
  elseif what == 2
    res = "two"
  else
    res = "three"
  endif
  return res
enddef

def Test_if_elseif_else()
  assert_equal('one', IfElse(1))
  assert_equal('two', IfElse(2))
  assert_equal('three', IfElse(3))
enddef

def Test_if_elseif_else_fails()
  CheckDefFailure(['elseif true'], 'E582:')
  CheckDefFailure(['else'], 'E581:')
  CheckDefFailure(['endif'], 'E580:')
  CheckDefFailure(['if true', 'elseif xxx'], 'E1001:')
  CheckDefFailure(['if true', 'echo 1'], 'E171:')
enddef

let g:bool_true = v:true
let g:bool_false = v:false

def Test_if_const_expr()
  let res = false
  if true ? true : false
    res = true
  endif
  assert_equal(true, res)

  g:glob = 2
  if false
    execute('g:glob = 3')
  endif
  assert_equal(2, g:glob)
  if true
    execute('g:glob = 3')
  endif
  assert_equal(3, g:glob)

  res = false
  if g:bool_true ? true : false
    res = true
  endif
  assert_equal(true, res)

  res = false
  if true ? g:bool_true : false
    res = true
  endif
  assert_equal(true, res)

  res = false
  if true ? true : g:bool_false
    res = true
  endif
  assert_equal(true, res)

  res = false
  if true ? false : true
    res = true
  endif
  assert_equal(false, res)

  res = false
  if false ? false : true
    res = true
  endif
  assert_equal(true, res)

  res = false
  if false ? true : false
    res = true
  endif
  assert_equal(false, res)

  res = false
  if has('xyz') ? true : false
    res = true
  endif
  assert_equal(false, res)

  res = false
  if true && true
    res = true
  endif
  assert_equal(true, res)

  res = false
  if true && false
    res = true
  endif
  assert_equal(false, res)

  res = false
  if g:bool_true && false
    res = true
  endif
  assert_equal(false, res)

  res = false
  if true && g:bool_false
    res = true
  endif
  assert_equal(false, res)

  res = false
  if false && false
    res = true
  endif
  assert_equal(false, res)

  res = false
  if true || false
    res = true
  endif
  assert_equal(true, res)

  res = false
  if g:bool_true || false
    res = true
  endif
  assert_equal(true, res)

  res = false
  if true || g:bool_false
    res = true
  endif
  assert_equal(true, res)

  res = false
  if false || false
    res = true
  endif
  assert_equal(false, res)

  # with constant "false" expression may be invalid so long as the syntax is OK
  if false | eval 0 | endif
  if false | eval burp + 234 | endif
  if false | echo burp 234 'asd' | endif
  if false
    burp
  endif
enddef

def Test_if_const_expr_fails()
  CheckDefFailure(['if "aaa" == "bbb'], 'E114:')
  CheckDefFailure(["if 'aaa' == 'bbb"], 'E115:')
  CheckDefFailure(["if has('aaa'"], 'E110:')
  CheckDefFailure(["if has('aaa') ? true false"], 'E109:')
enddef

def RunNested(i: number): number
  let x: number = 0
  if i % 2
    if 1
      # comment
    else
      # comment
    endif
    x += 1
  else
    x += 1000
  endif
  return x
enddef

def Test_nested_if()
  assert_equal(1, RunNested(1))
  assert_equal(1000, RunNested(2))
enddef

def Test_execute_cmd()
  new
  setline(1, 'default')
  execute 'setline(1, "execute-string")'
  assert_equal('execute-string', getline(1))

  execute "setline(1, 'execute-string')"
  assert_equal('execute-string', getline(1))

  let cmd1 = 'setline(1,'
  let cmd2 = '"execute-var")'
  execute cmd1 cmd2 # comment
  assert_equal('execute-var', getline(1))

  execute cmd1 cmd2 '|setline(1, "execute-var-string")'
  assert_equal('execute-var-string', getline(1))

  let cmd_first = 'call '
  let cmd_last = 'setline(1, "execute-var-var")'
  execute cmd_first .. cmd_last
  assert_equal('execute-var-var', getline(1))
  bwipe!

  let n = true
  execute 'echomsg' (n ? '"true"' : '"no"')
  assert_match('^true$', Screenline(&lines))

  echomsg [1, 2, 3] #{a: 1, b: 2}
  assert_match('^\[1, 2, 3\] {''a'': 1, ''b'': 2}$', Screenline(&lines))

  CheckDefFailure(['execute xxx'], 'E1001:', 1)
  CheckDefExecFailure(['execute "tabnext " .. 8'], 'E475:', 1)
  CheckDefFailure(['execute "cmd"# comment'], 'E488:', 1)
enddef

def Test_execute_cmd_vimscript()
  # only checks line continuation
  let lines =<< trim END
      vim9script
      execute 'g:someVar'
                .. ' = ' ..
                   '28'
      assert_equal(28, g:someVar)
      unlet g:someVar
  END
  CheckScriptSuccess(lines)
enddef

def Test_echo_cmd()
  echo 'some' # comment
  echon 'thing'
  assert_match('^something$', Screenline(&lines))

  echo "some" # comment
  echon "thing"
  assert_match('^something$', Screenline(&lines))

  let str1 = 'some'
  let str2 = 'more'
  echo str1 str2
  assert_match('^some more$', Screenline(&lines))

  CheckDefFailure(['echo "xxx"# comment'], 'E488:')
enddef

def Test_echomsg_cmd()
  echomsg 'some' 'more' # comment
  assert_match('^some more$', Screenline(&lines))
  echo 'clear'
  :1messages
  assert_match('^some more$', Screenline(&lines))

  CheckDefFailure(['echomsg "xxx"# comment'], 'E488:')
enddef

def Test_echomsg_cmd_vimscript()
  # only checks line continuation
  let lines =<< trim END
      vim9script
      echomsg 'here'
                .. ' is ' ..
                   'a message'
      assert_match('^here is a message$', Screenline(&lines))
  END
  CheckScriptSuccess(lines)
enddef

def Test_echoerr_cmd()
  try
    echoerr 'something' 'wrong' # comment
  catch
    assert_match('something wrong', v:exception)
  endtry
enddef

def Test_echoerr_cmd_vimscript()
  # only checks line continuation
  let lines =<< trim END
      vim9script
      try
        echoerr 'this'
                .. ' is ' ..
                   'wrong'
      catch
        assert_match('this is wrong', v:exception)
      endtry
  END
  CheckScriptSuccess(lines)
enddef

def Test_for_outside_of_function()
  let lines =<< trim END
    vim9script
    new
    for var in range(0, 3)
      append(line('$'), var)
    endfor
    assert_equal(['', '0', '1', '2', '3'], getline(1, '$'))
    bwipe!
  END
  writefile(lines, 'Xvim9for.vim')
  source Xvim9for.vim
  delete('Xvim9for.vim')
enddef

def Test_for_loop()
  let result = ''
  for cnt in range(7)
    if cnt == 4
      break
    endif
    if cnt == 2
      continue
    endif
    result ..= cnt .. '_'
  endfor
  assert_equal('0_1_3_', result)

  let concat = ''
  for str in eval('["one", "two"]')
    concat ..= str
  endfor
  assert_equal('onetwo', concat)
enddef

def Test_for_loop_fails()
  CheckDefFailure(['for # in range(5)'], 'E690:')
  CheckDefFailure(['for i In range(5)'], 'E690:')
  CheckDefFailure(['let x = 5', 'for x in range(5)'], 'E1017:')
  CheckScriptFailure(['def Func(arg: any)', 'for arg in range(5)', 'enddef', 'defcompile'], 'E1006:')
  CheckDefFailure(['for i in "text"'], 'E1012:')
  CheckDefFailure(['for i in xxx'], 'E1001:')
  CheckDefFailure(['endfor'], 'E588:')
  CheckDefFailure(['for i in range(3)', 'echo 3'], 'E170:')
enddef

def Test_while_loop()
  let result = ''
  let cnt = 0
  while cnt < 555
    if cnt == 3
      break
    endif
    cnt += 1
    if cnt == 2
      continue
    endif
    result ..= cnt .. '_'
  endwhile
  assert_equal('1_3_', result)
enddef

def Test_while_loop_fails()
  CheckDefFailure(['while xxx'], 'E1001:')
  CheckDefFailure(['endwhile'], 'E588:')
  CheckDefFailure(['continue'], 'E586:')
  CheckDefFailure(['if true', 'continue'], 'E586:')
  CheckDefFailure(['break'], 'E587:')
  CheckDefFailure(['if true', 'break'], 'E587:')
  CheckDefFailure(['while 1', 'echo 3'], 'E170:')
enddef

def Test_interrupt_loop()
  let caught = false
  let x = 0
  try
    while 1
      x += 1
      if x == 100
        feedkeys("\<C-C>", 'Lt')
      endif
    endwhile
  catch
    caught = true
    assert_equal(100, x)
  endtry
  assert_true(caught, 'should have caught an exception')
  # consume the CTRL-C
  getchar(0)
enddef

def Test_automatic_line_continuation()
  let mylist = [
      'one',
      'two',
      'three',
      ] # comment
  assert_equal(['one', 'two', 'three'], mylist)

  let mydict = {
      'one': 1,
      'two': 2,
      'three':
          3,
      } # comment
  assert_equal({'one': 1, 'two': 2, 'three': 3}, mydict)
  mydict = #{
      one: 1,  # comment
      two:     # comment
           2,  # comment
      three: 3 # comment
      }
  assert_equal(#{one: 1, two: 2, three: 3}, mydict)
  mydict = #{
      one: 1, 
      two: 
           2, 
      three: 3 
      }
  assert_equal(#{one: 1, two: 2, three: 3}, mydict)

  assert_equal(
        ['one', 'two', 'three'],
        split('one two three')
        )
enddef

def Test_vim9_comment()
  CheckScriptSuccess([
      'vim9script',
      '# something',
      ])
  CheckScriptFailure([
      'vim9script',
      ':# something',
      ], 'E488:')
  CheckScriptFailure([
      '# something',
      ], 'E488:')
  CheckScriptFailure([
      ':# something',
      ], 'E488:')

  { # block start
  } # block end
  CheckDefFailure([
      '{# comment',
      ], 'E488:')
  CheckDefFailure([
      '{',
      '}# comment',
      ], 'E488:')

  echo "yes" # comment
  CheckDefFailure([
      'echo "yes"# comment',
      ], 'E488:')
  CheckScriptSuccess([
      'vim9script',
      'echo "yes" # something',
      ])
  CheckScriptFailure([
      'vim9script',
      'echo "yes"# something',
      ], 'E121:')
  CheckScriptFailure([
      'vim9script',
      'echo# something',
      ], 'E121:')
  CheckScriptFailure([
      'echo "yes" # something',
      ], 'E121:')

  exe "echo" # comment
  CheckDefFailure([
      'exe "echo"# comment',
      ], 'E488:')
  CheckScriptSuccess([
      'vim9script',
      'exe "echo" # something',
      ])
  CheckScriptFailure([
      'vim9script',
      'exe "echo"# something',
      ], 'E121:')
  CheckDefFailure([
      'exe # comment',
      ], 'E1015:')
  CheckScriptFailure([
      'vim9script',
      'exe# something',
      ], 'E121:')
  CheckScriptFailure([
      'exe "echo" # something',
      ], 'E121:')

  CheckDefFailure([
      'try# comment',
      '  echo "yes"',
      'catch',
      'endtry',
      ], 'E488:')
  CheckScriptFailure([
      'vim9script',
      'try# comment',
      'echo "yes"',
      ], 'E488:')
  CheckDefFailure([
      'try',
      '  throw#comment',
      'catch',
      'endtry',
      ], 'E1015:')
  CheckDefFailure([
      'try',
      '  throw "yes"#comment',
      'catch',
      'endtry',
      ], 'E488:')
  CheckDefFailure([
      'try',
      '  echo "yes"',
      'catch# comment',
      'endtry',
      ], 'E488:')
  CheckScriptFailure([
      'vim9script',
      'try',
      '  echo "yes"',
      'catch# comment',
      'endtry',
      ], 'E654:')
  CheckDefFailure([
      'try',
      '  echo "yes"',
      'catch /pat/# comment',
      'endtry',
      ], 'E488:')
  CheckDefFailure([
      'try',
      'echo "yes"',
      'catch',
      'endtry# comment',
      ], 'E488:')
  CheckScriptFailure([
      'vim9script',
      'try',
      '  echo "yes"',
      'catch',
      'endtry# comment',
      ], 'E488:')

  CheckScriptSuccess([
      'vim9script',
      'hi # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'hi# comment',
      ], 'E416:')
  CheckScriptSuccess([
      'vim9script',
      'hi Search # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'hi Search# comment',
      ], 'E416:')
  CheckScriptSuccess([
      'vim9script',
      'hi link This Search # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'hi link This That# comment',
      ], 'E413:')
  CheckScriptSuccess([
      'vim9script',
      'hi clear This # comment',
      'hi clear # comment',
      ])
  # not tested, because it doesn't give an error but a warning:
  # hi clear This# comment',
  CheckScriptFailure([
      'vim9script',
      'hi clear# comment',
      ], 'E416:')

  CheckScriptSuccess([
      'vim9script',
      'hi Group term=bold',
      'match Group /todo/ # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'hi Group term=bold',
      'match Group /todo/# comment',
      ], 'E488:')
  CheckScriptSuccess([
      'vim9script',
      'match # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'match# comment',
      ], 'E475:')
  CheckScriptSuccess([
      'vim9script',
      'match none # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'match none# comment',
      ], 'E475:')

  CheckScriptSuccess([
      'vim9script',
      'menutrans clear # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'menutrans clear# comment text',
      ], 'E474:')

  CheckScriptSuccess([
      'vim9script',
      'syntax clear # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax clear# comment text',
      ], 'E28:')
  CheckScriptSuccess([
      'vim9script',
      'syntax keyword Word some',
      'syntax clear Word # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax keyword Word some',
      'syntax clear Word# comment text',
      ], 'E28:')

  CheckScriptSuccess([
      'vim9script',
      'syntax list # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax list# comment text',
      ], 'E28:')

  CheckScriptSuccess([
      'vim9script',
      'syntax match Word /pat/ oneline # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax match Word /pat/ oneline# comment',
      ], 'E475:')

  CheckScriptSuccess([
      'vim9script',
      'syntax keyword Word word # comm[ent',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax keyword Word word# comm[ent',
      ], 'E789:')

  CheckScriptSuccess([
      'vim9script',
      'syntax match Word /pat/ # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax match Word /pat/# comment',
      ], 'E402:')

  CheckScriptSuccess([
      'vim9script',
      'syntax match Word /pat/ contains=Something # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax match Word /pat/ contains=Something# comment',
      ], 'E475:')
  CheckScriptFailure([
      'vim9script',
      'syntax match Word /pat/ contains= # comment',
      ], 'E406:')
  CheckScriptFailure([
      'vim9script',
      'syntax match Word /pat/ contains=# comment',
      ], 'E475:')

  CheckScriptSuccess([
      'vim9script',
      'syntax region Word start=/pat/ end=/pat/ # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax region Word start=/pat/ end=/pat/# comment',
      ], 'E402:')

  CheckScriptSuccess([
      'vim9script',
      'syntax sync # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax sync# comment',
      ], 'E404:')
  CheckScriptSuccess([
      'vim9script',
      'syntax sync ccomment # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax sync ccomment# comment',
      ], 'E404:')

  CheckScriptSuccess([
      'vim9script',
      'syntax cluster Some contains=Word # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'syntax cluster Some contains=Word# comment',
      ], 'E475:')

  CheckScriptSuccess([
      'vim9script',
      'command Echo echo # comment',
      'command Echo # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'command Echo echo# comment',
      'Echo',
      ], 'E121:')
  CheckScriptFailure([
      'vim9script',
      'command Echo# comment',
      ], 'E182:')
  CheckScriptFailure([
      'vim9script',
      'command Echo echo',
      'command Echo# comment',
      ], 'E182:')

  CheckScriptSuccess([
      'vim9script',
      'function # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'function " comment',
      ], 'E129:')
  CheckScriptFailure([
      'vim9script',
      'function# comment',
      ], 'E129:')
  CheckScriptSuccess([
      'vim9script',
      'function CheckScriptSuccess # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'function CheckScriptSuccess# comment',
      ], 'E488:')

  CheckScriptSuccess([
      'vim9script',
      'func g:DeleteMeA()',
      'endfunc',
      'delfunction g:DeleteMeA # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'func g:DeleteMeB()',
      'endfunc',
      'delfunction g:DeleteMeB# comment',
      ], 'E488:')

  CheckScriptSuccess([
      'vim9script',
      'call execute("ls") # comment',
      ])
  CheckScriptFailure([
      'vim9script',
      'call execute("ls")# comment',
      ], 'E488:')

  CheckScriptFailure([
      'def Test() " comment',
      'enddef',
      ], 'E488:')
  CheckScriptFailure([
      'vim9script',
      'def Test() " comment',
      'enddef',
      ], 'E488:')

  CheckScriptSuccess([
      'func Test() " comment',
      'endfunc',
      ])
  CheckScriptSuccess([
      'vim9script',
      'func Test() " comment',
      'endfunc',
      ])

  CheckScriptSuccess([
      'def Test() # comment',
      'enddef',
      ])
  CheckScriptFailure([
      'func Test() # comment',
      'endfunc',
      ], 'E488:')
enddef

def Test_vim9_comment_gui()
  CheckCanRunGui

  CheckScriptFailure([
      'vim9script',
      'gui#comment'
      ], 'E499:')
  CheckScriptFailure([
      'vim9script',
      'gui -f#comment'
      ], 'E499:')
enddef

def Test_vim9_comment_not_compiled()
  au TabEnter *.vim g:entered = 1
  au TabEnter *.x g:entered = 2

  edit test.vim
  doautocmd TabEnter #comment
  assert_equal(1, g:entered)

  doautocmd TabEnter f.x
  assert_equal(2, g:entered)

  g:entered = 0
  doautocmd TabEnter f.x #comment
  assert_equal(2, g:entered)

  assert_fails('doautocmd Syntax#comment', 'E216:')

  au! TabEnter
  unlet g:entered

  CheckScriptSuccess([
      'vim9script',
      'g:var = 123',
      'b:var = 456',
      'w:var = 777',
      't:var = 888',
      'unlet g:var w:var # something',
      ])

  CheckScriptFailure([
      'vim9script',
      'let g:var = 123',
      ], 'E1016: Cannot declare a global variable:')

  CheckScriptFailure([
      'vim9script',
      'let b:var = 123',
      ], 'E1016: Cannot declare a buffer variable:')

  CheckScriptFailure([
      'vim9script',
      'let w:var = 123',
      ], 'E1016: Cannot declare a window variable:')

  CheckScriptFailure([
      'vim9script',
      'let t:var = 123',
      ], 'E1016: Cannot declare a tab variable:')

  CheckScriptFailure([
      'vim9script',
      'let v:version = 123',
      ], 'E1016: Cannot declare a v: variable:')

  CheckScriptFailure([
      'vim9script',
      'let $VARIABLE = "text"',
      ], 'E1016: Cannot declare an environment variable:')

  CheckScriptFailure([
      'vim9script',
      'g:var = 123',
      'unlet g:var# comment1',
      ], 'E108:')

  CheckScriptFailure([
      'let g:var = 123',
      'unlet g:var # something',
      ], 'E488:')

  CheckScriptSuccess([
      'vim9script',
      'if 1 # comment2',
      '  echo "yes"',
      'elseif 2 #comment',
      '  echo "no"',
      'endif',
      ])

  CheckScriptFailure([
      'vim9script',
      'if 1# comment3',
      '  echo "yes"',
      'endif',
      ], 'E15:')

  CheckScriptFailure([
      'vim9script',
      'if 0 # comment4',
      '  echo "yes"',
      'elseif 2#comment',
      '  echo "no"',
      'endif',
      ], 'E15:')

  CheckScriptSuccess([
      'vim9script',
      'let v = 1 # comment5',
      ])

  CheckScriptFailure([
      'vim9script',
      'let v = 1# comment6',
      ], 'E15:')

  CheckScriptSuccess([
      'vim9script',
      'new'
      'setline(1, ["# define pat", "last"])',
      ':$',
      'dsearch /pat/ #comment',
      'bwipe!',
      ])

  CheckScriptFailure([
      'vim9script',
      'new'
      'setline(1, ["# define pat", "last"])',
      ':$',
      'dsearch /pat/#comment',
      'bwipe!',
      ], 'E488:')

  CheckScriptFailure([
      'vim9script',
      'func! SomeFunc()',
      ], 'E477:')
enddef

def Test_finish()
  let lines =<< trim END
    vim9script
    g:res = 'one'
    if v:false | finish | endif
    g:res = 'two'
    finish
    g:res = 'three'
  END
  writefile(lines, 'Xfinished')
  source Xfinished
  assert_equal('two', g:res)

  unlet g:res
  delete('Xfinished')
enddef

def Test_let_func_call()
  let lines =<< trim END
    vim9script
    func GetValue()
      if exists('g:count')
        let g:count += 1
      else
        let g:count = 1
      endif
      return 'this'
    endfunc
    let val: string = GetValue() 
    # env var is always a string
    let env = $TERM
  END
  writefile(lines, 'Xfinished')
  source Xfinished
  # GetValue() is not called during discovery phase
  assert_equal(1, g:count)

  unlet g:count
  delete('Xfinished')
enddef

def Test_let_missing_type()
  let lines =<< trim END
    vim9script
    let var = g:unknown
  END
  CheckScriptFailure(lines, 'E121:')

  lines =<< trim END
    vim9script
    let nr: number = 123
    let var = nr
  END
  CheckScriptSuccess(lines)
enddef

def Test_let_declaration()
  let lines =<< trim END
    vim9script
    let var: string
    g:var_uninit = var
    var = 'text'
    g:var_test = var
    # prefixing s: is optional
    s:var = 'prefixed'
    g:var_prefixed = s:var

    let s:other: number
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
  let lines =<< trim END
    vim9script
    const var: string
  END
  CheckScriptFailure(lines, 'E1021:')

  lines =<< trim END
    vim9script
    let 9var: string
  END
  CheckScriptFailure(lines, 'E475:')
enddef

def Test_let_type_check()
  let lines =<< trim END
    vim9script
    let var: string
    var = 1234
  END
  CheckScriptFailure(lines, 'E1012:')

  lines =<< trim END
    vim9script
    let var:string
  END
  CheckScriptFailure(lines, 'E1069:')

  lines =<< trim END
    vim9script
    let var: asdf
  END
  CheckScriptFailure(lines, 'E1010:')

  lines =<< trim END
    vim9script
    let s:l: list<number>
    s:l = []
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    let s:d: dict<number>
    s:d = {}
  END
  CheckScriptSuccess(lines)
enddef

def Test_forward_declaration()
  let lines =<< trim END
    vim9script
    def GetValue(): string
      return theVal
    enddef
    let theVal = 'something'
    g:initVal = GetValue()
    theVal = 'else'
    g:laterVal = GetValue()
  END
  writefile(lines, 'Xforward')
  source Xforward
  assert_equal('something', g:initVal)
  assert_equal('else', g:laterVal)

  unlet g:initVal
  unlet g:laterVal
  delete('Xforward')
enddef

def Test_source_vim9_from_legacy()
  let legacy_lines =<< trim END
    source Xvim9_script.vim

    call assert_false(exists('local'))
    call assert_false(exists('exported'))
    call assert_false(exists('s:exported'))
    call assert_equal('global', global)
    call assert_equal('global', g:global)

    " imported variable becomes script-local
    import exported from './Xvim9_script.vim'
    call assert_equal('exported', s:exported)
    call assert_false(exists('exported'))

    " imported function becomes script-local
    import GetText from './Xvim9_script.vim'
    call assert_equal('text', s:GetText())
    call assert_false(exists('*GetText'))
  END
  writefile(legacy_lines, 'Xlegacy_script.vim')

  let vim9_lines =<< trim END
    vim9script
    let local = 'local'
    g:global = 'global'
    export let exported = 'exported'
    export def GetText(): string
       return 'text'
    enddef
  END
  writefile(vim9_lines, 'Xvim9_script.vim')

  source Xlegacy_script.vim

  assert_equal('global', g:global)
  unlet g:global

  delete('Xlegacy_script.vim')
  delete('Xvim9_script.vim')
enddef

func Test_vim9script_not_global()
  " check that items defined in Vim9 script are script-local, not global
  let vim9lines =<< trim END
    vim9script
    let var = 'local'
    func TheFunc()
      echo 'local'
    endfunc
    def DefFunc()
      echo 'local'
    enddef
  END
  call writefile(vim9lines, 'Xvim9script.vim')
  source Xvim9script.vim
  try
    echo g:var
    assert_report('did not fail')
  catch /E121:/
    " caught
  endtry
  try
    call TheFunc()
    assert_report('did not fail')
  catch /E117:/
    " caught
  endtry
  try
    call DefFunc()
    assert_report('did not fail')
  catch /E117:/
    " caught
  endtry

  call delete('Xvim9script.vim')
endfunc

def Test_vim9_copen()
  # this was giving an error for setting w:quickfix_title
  copen
  quit
enddef

" test using a vim9script that is auto-loaded from an autocmd
def Test_vim9_autoload()
  let lines =<< trim END
     vim9script
     def foo#test()
         echomsg getreg('"')
     enddef
  END

  mkdir('Xdir/autoload', 'p')
  writefile(lines, 'Xdir/autoload/foo.vim')
  let save_rtp = &rtp
  exe 'set rtp^=' .. getcwd() .. '/Xdir'
  augroup test
    autocmd TextYankPost * call foo#test()
  augroup END

  normal Y

  augroup test
    autocmd!
  augroup END
  delete('Xdir', 'rf')
  &rtp = save_rtp
enddef

def Test_script_var_in_autocmd()
  # using a script variable from an autocommand, defined in a :def function in a
  # legacy Vim script, cannot check the variable type.
  let lines =<< trim END
    let s:counter = 1
    def s:Func()
      au! CursorHold
      au CursorHold * s:counter += 1
    enddef
    call s:Func()
    doau CursorHold
    call assert_equal(2, s:counter)
    au! CursorHold
  END
  CheckScriptSuccess(lines)
enddef

def Test_cmdline_win()
  # if the Vim syntax highlighting uses Vim9 constructs they can be used from
  # the command line window.
  mkdir('rtp/syntax', 'p')
  let export_lines =<< trim END
    vim9script
    export let That = 'yes'
  END
  writefile(export_lines, 'rtp/syntax/Xexport.vim')
  let import_lines =<< trim END
    vim9script
    import That from './Xexport.vim'
  END
  writefile(import_lines, 'rtp/syntax/vim.vim')
  let save_rtp = &rtp
  &rtp = getcwd() .. '/rtp' .. ',' .. &rtp
  syntax on
  augroup CmdWin
    autocmd CmdwinEnter * g:got_there = 'yes'
  augroup END
  # this will open and also close the cmdline window
  feedkeys('q:', 'xt')
  assert_equal('yes', g:got_there)

  augroup CmdWin
    au!
  augroup END
  &rtp = save_rtp
  delete('rtp', 'rf')
enddef

def Test_invalid_sid()
  assert_fails('func <SNR>1234_func', 'E123:')

  if RunVim([], ['wq Xdidit'], '+"func <SNR>1_func"')
    assert_equal([], readfile('Xdidit'))
  endif
  delete('Xdidit')
enddef

" Keep this last, it messes up highlighting.
def Test_substitute_cmd()
  new
  setline(1, 'something')
  :substitute(some(other(
  assert_equal('otherthing', getline(1))
  bwipe!

  # also when the context is Vim9 script
  let lines =<< trim END
    vim9script
    new
    setline(1, 'something')
    :substitute(some(other(
    assert_equal('otherthing', getline(1))
    bwipe!
  END
  writefile(lines, 'Xvim9lines')
  source Xvim9lines

  delete('Xvim9lines')
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
