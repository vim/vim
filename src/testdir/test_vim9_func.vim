" Test various aspects of the Vim9 script language.

source check.vim
source term_util.vim
source view_util.vim
source vim9.vim
source screendump.vim

func Test_def_basic()
  def SomeFunc(): string
    return 'yes'
  enddef
  call SomeFunc()->assert_equal('yes')
endfunc

func Test_compiling_error()
  " use a terminal to see the whole error message
  CheckRunVimInTerminal

  call TestCompilingError()
  call TestCompilingErrorInTry()
endfunc

def TestCompilingError()
  var lines =<< trim END
    vim9script
    def Fails()
      echo nothing
    enddef
    defcompile
  END
  writefile(lines, 'XTest_compile_error')
  var buf = RunVimInTerminal('-S XTest_compile_error',
              {rows: 10, wait_for_ruler: 0})
  WaitForAssert(() => assert_match('Error detected while compiling command line.*Fails.*Variable not found: nothing',
                     Term_getlines(buf, range(1, 9))))

  # clean up
  StopVimInTerminal(buf)
  delete('XTest_compile_error')
enddef

def TestCompilingErrorInTry()
  var dir = 'Xdir/autoload'
  mkdir(dir, 'p')

  var lines =<< trim END
      vim9script
      def script#OnlyCompiled()
        g:runtime = 'yes'
        invalid
      enddef
  END
  writefile(lines, dir .. '/script.vim')

  lines =<< trim END
      vim9script
      todo
      try
        script#OnlyCompiled()
      catch /nothing/
      endtry
  END
  lines[1] = 'set rtp=' .. getcwd() .. '/Xdir'
  writefile(lines, 'XTest_compile_error')

  var buf = RunVimInTerminal('-S XTest_compile_error', {rows: 10, wait_for_ruler: 0})
  WaitForAssert(() => assert_match('Error detected while compiling command line.*function script#OnlyCompiled.*Invalid command: invalid',
                     Term_getlines(buf, range(1, 9))))

  # clean up
  StopVimInTerminal(buf)
  delete('XTest_compile_error')
  delete('Xdir', 'rf')
enddef

def Test_compile_error_in_called_function()
  var lines =<< trim END
      vim9script
      var n: number
      def Foo()
        &hls = n
      enddef
      def Bar()
        Foo()
      enddef
      silent! Foo()
      Bar()
  END
  CheckScriptFailureList(lines, ['E1012:', 'E1191:'])
enddef

def Test_wrong_function_name()
  var lines =<< trim END
      vim9script
      func _Foo()
        echo 'foo'
      endfunc
  END
  CheckScriptFailure(lines, 'E128:')

  lines =<< trim END
      vim9script
      def _Foo()
        echo 'foo'
      enddef
  END
  CheckScriptFailure(lines, 'E128:')
enddef

def Test_autoload_name_mismatch()
  var dir = 'Xdir/autoload'
  mkdir(dir, 'p')

  var lines =<< trim END
      vim9script
      def scriptX#Function()
        # comment
        g:runtime = 'yes'
      enddef
  END
  writefile(lines, dir .. '/script.vim')

  var save_rtp = &rtp
  exe 'set rtp=' .. getcwd() .. '/Xdir'
  lines =<< trim END
      call script#Function()
  END
  CheckScriptFailure(lines, 'E746:', 2)

  &rtp = save_rtp
  delete(dir, 'rf')
enddef

def Test_autoload_names()
  var dir = 'Xdir/autoload'
  mkdir(dir, 'p')

  var lines =<< trim END
      func foobar#function()
        return 'yes'
      endfunc
      let foobar#var = 'no'
  END
  writefile(lines, dir .. '/foobar.vim')

  var save_rtp = &rtp
  exe 'set rtp=' .. getcwd() .. '/Xdir'

  lines =<< trim END
      assert_equal('yes', foobar#function())
      var Function = foobar#function
      assert_equal('yes', Function())

      assert_equal('no', foobar#var)
  END
  CheckDefAndScriptSuccess(lines)

  &rtp = save_rtp
  delete(dir, 'rf')
enddef

def Test_autoload_error_in_script()
  var dir = 'Xdir/autoload'
  mkdir(dir, 'p')

  var lines =<< trim END
      func scripterror#function()
        let g:called_function = 'yes'
      endfunc
      let 0 = 1
  END
  writefile(lines, dir .. '/scripterror.vim')

  var save_rtp = &rtp
  exe 'set rtp=' .. getcwd() .. '/Xdir'

  g:called_function = 'no'
  # The error in the autoload script cannot be checked with assert_fails(), use
  # CheckDefSuccess() instead of CheckDefFailure()
  try
    CheckDefSuccess(['scripterror#function()'])
  catch
    assert_match('E121: Undefined variable: 0', v:exception)
  endtry
  assert_equal('no', g:called_function)

  lines =<< trim END
      func scriptcaught#function()
        let g:called_function = 'yes'
      endfunc
      try
        let 0 = 1
      catch
        let g:caught = v:exception
      endtry
  END
  writefile(lines, dir .. '/scriptcaught.vim')

  g:called_function = 'no'
  CheckDefSuccess(['scriptcaught#function()'])
  assert_match('E121: Undefined variable: 0', g:caught)
  assert_equal('yes', g:called_function)

  &rtp = save_rtp
  delete(dir, 'rf')
enddef

def CallRecursive(n: number): number
  return CallRecursive(n + 1)
enddef

def CallMapRecursive(l: list<number>): number
  return map(l, (_, v) => CallMapRecursive([v]))[0]
enddef

def Test_funcdepth_error()
  set maxfuncdepth=10

  var caught = false
  try
    CallRecursive(1)
  catch /E132:/
    caught = true
  endtry
  assert_true(caught)

  caught = false
  try
    CallMapRecursive([1])
  catch /E132:/
    caught = true
  endtry
  assert_true(caught)

  set maxfuncdepth&
enddef

def Test_endfunc_enddef()
  var lines =<< trim END
    def Test()
      echo 'test'
      endfunc
    enddef
  END
  CheckScriptFailure(lines, 'E1151:', 3)

  lines =<< trim END
    def Test()
      func Nested()
        echo 'test'
      enddef
    enddef
  END
  CheckScriptFailure(lines, 'E1152:', 4)

  lines =<< trim END
    def Ok()
      echo 'hello'
    enddef | echo 'there'
    def Bad()
      echo 'hello'
    enddef there
  END
  CheckScriptFailure(lines, 'E1173: Text found after enddef: there', 6)
enddef

def Test_missing_endfunc_enddef()
  var lines =<< trim END
    vim9script
    def Test()
      echo 'test'
    endef
  END
  CheckScriptFailure(lines, 'E1057:', 2)

  lines =<< trim END
    vim9script
    func Some()
      echo 'test'
    enfffunc
  END
  CheckScriptFailure(lines, 'E126:', 2)
enddef

def Test_white_space_before_paren()
  var lines =<< trim END
    vim9script
    def Test ()
      echo 'test'
    enddef
  END
  CheckScriptFailure(lines, 'E1068:', 2)

  lines =<< trim END
    vim9script
    func Test ()
      echo 'test'
    endfunc
  END
  CheckScriptFailure(lines, 'E1068:', 2)

  lines =<< trim END
    def Test ()
      echo 'test'
    enddef
  END
  CheckScriptFailure(lines, 'E1068:', 1)

  lines =<< trim END
    func Test ()
      echo 'test'
    endfunc
  END
  CheckScriptSuccess(lines)
enddef

def Test_enddef_dict_key()
  var d = {
    enddef: 'x',
    endfunc: 'y',
  }
  assert_equal({enddef: 'x', endfunc: 'y'}, d)
enddef

def ReturnString(): string
  return 'string'
enddef

def ReturnNumber(): number
  return 123
enddef

let g:notNumber = 'string'

def ReturnGlobal(): number
  return g:notNumber
enddef

def Test_return_something()
  ReturnString()->assert_equal('string')
  ReturnNumber()->assert_equal(123)
  assert_fails('ReturnGlobal()', 'E1012: Type mismatch; expected number but got string', '', 1, 'ReturnGlobal')
enddef

def Test_check_argument_type()
  var lines =<< trim END
      vim9script
      def Val(a: number, b: number): number
        return 0
      enddef
      def Func()
        var x: any = true
        Val(0, x)
      enddef
      disass Func
      Func()
  END
  CheckScriptFailure(lines, 'E1013: Argument 2: type mismatch, expected number but got bool', 2)
enddef

def Test_missing_return()
  CheckDefFailure(['def Missing(): number',
                   '  if g:cond',
                   '    echo "no return"',
                   '  else',
                   '    return 0',
                   '  endif'
                   'enddef'], 'E1027:')
  CheckDefFailure(['def Missing(): number',
                   '  if g:cond',
                   '    return 1',
                   '  else',
                   '    echo "no return"',
                   '  endif'
                   'enddef'], 'E1027:')
  CheckDefFailure(['def Missing(): number',
                   '  if g:cond',
                   '    return 1',
                   '  else',
                   '    return 2',
                   '  endif'
                   '  return 3'
                   'enddef'], 'E1095:')
enddef

def Test_return_bool()
  var lines =<< trim END
      vim9script
      def MenuFilter(id: number, key: string): bool
        return popup_filter_menu(id, key)
      enddef
      def YesnoFilter(id: number, key: string): bool
        return popup_filter_yesno(id, key)
      enddef
      defcompile
  END
  CheckScriptSuccess(lines)
enddef

let s:nothing = 0
def ReturnNothing()
  s:nothing = 1
  if true
    return
  endif
  s:nothing = 2
enddef

def Test_return_nothing()
  ReturnNothing()
  s:nothing->assert_equal(1)
enddef

def Test_return_invalid()
  var lines =<< trim END
    vim9script
    def Func(): invalid
      return xxx
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E1010:', 2)

  lines =<< trim END
      vim9script
      def Test(Fun: func(number): number): list<number>
          return map([1, 2, 3], (_, i) => Fun(i))
      enddef
      defcompile
      def Inc(nr: number): nr
        return nr + 2
      enddef
      echo Test(Inc)
  END
  # doing this twice was leaking memory
  CheckScriptFailure(lines, 'E1010:')
  CheckScriptFailure(lines, 'E1010:')
enddef

def Test_return_list_any()
  # This used to fail but now the actual list type is checked, and since it has
  # an item of type string it can be used as list<string>.
  var lines =<< trim END
      vim9script
      def Func(): list<string>
        var l: list<any>
        l->add('string')
        return l
      enddef
      echo Func()
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Func(): list<string>
        var l: list<any>
        l += ['string']
        return l
      enddef
      echo Func()
  END
  CheckScriptSuccess(lines)
enddef

func Increment()
  let g:counter += 1
endfunc

def Test_call_ufunc_count()
  g:counter = 1
  Increment()
  Increment()
  Increment()
  # works with and without :call
  g:counter->assert_equal(4)
  eval g:counter->assert_equal(4)
  unlet g:counter
enddef

def MyVarargs(arg: string, ...rest: list<string>): string
  var res = arg
  for s in rest
    res ..= ',' .. s
  endfor
  return res
enddef

def Test_call_varargs()
  MyVarargs('one')->assert_equal('one')
  MyVarargs('one', 'two')->assert_equal('one,two')
  MyVarargs('one', 'two', 'three')->assert_equal('one,two,three')
enddef

def Test_call_white_space()
  CheckDefAndScriptFailure(["call Test ('text')"], ['E476:', 'E1068:'])
enddef

def MyDefaultArgs(name = 'string'): string
  return name
enddef

def MyDefaultSecond(name: string, second: bool  = true): string
  return second ? name : 'none'
enddef


def Test_call_default_args()
  MyDefaultArgs()->assert_equal('string')
  MyDefaultArgs(v:none)->assert_equal('string')
  MyDefaultArgs('one')->assert_equal('one')
  assert_fails('MyDefaultArgs("one", "two")', 'E118:', '', 4, 'Test_call_default_args')

  MyDefaultSecond('test')->assert_equal('test')
  MyDefaultSecond('test', true)->assert_equal('test')
  MyDefaultSecond('test', false)->assert_equal('none')

  var lines =<< trim END
      def MyDefaultThird(name: string, aa = 'aa', bb = 'bb'): string
        return name .. aa .. bb
      enddef

      MyDefaultThird('->')->assert_equal('->aabb')
      MyDefaultThird('->', v:none)->assert_equal('->aabb')
      MyDefaultThird('->', 'xx')->assert_equal('->xxbb')
      MyDefaultThird('->', v:none, v:none)->assert_equal('->aabb')
      MyDefaultThird('->', 'xx', v:none)->assert_equal('->xxbb')
      MyDefaultThird('->', v:none, 'yy')->assert_equal('->aayy')
      MyDefaultThird('->', 'xx', 'yy')->assert_equal('->xxyy')

      def DefArg(mandatory: any, optional = mandatory): string
        return mandatory .. optional
      enddef
      DefArg(1234)->assert_equal('12341234')
      DefArg("ok")->assert_equal('okok')
  END
  CheckDefAndScriptSuccess(lines)

  CheckScriptFailure(['def Func(arg: number = asdf)', 'enddef', 'defcompile'], 'E1001:')
  delfunc g:Func
  CheckScriptFailure(['def Func(arg: number = "text")', 'enddef', 'defcompile'], 'E1013: Argument 1: type mismatch, expected number but got string')
  delfunc g:Func
  CheckDefFailure(['def Func(x: number = )', 'enddef'], 'E15:')

  lines =<< trim END
      vim9script
      def Func(a = b == 0 ? 1 : 2, b = 0)
      enddef
      defcompile
  END
  CheckScriptFailure(lines, 'E1001: Variable not found: b')

  # using script variable requires matching type or type cast when executed
  lines =<< trim END
      vim9script
      var a: any
      def Func(arg: string = a)
        echo arg
      enddef
      defcompile
  END
  CheckScriptSuccess(lines + ['a = "text"', 'Func()'])
  CheckScriptFailure(lines + ['a = 123', 'Func()'], 'E1013: Argument 1: type mismatch, expected string but got number')

  # using global variable does not require type cast
  lines =<< trim END
      vim9script
      def Func(arg: string = g:str)
        echo arg
      enddef
      g:str = 'works'
      Func()
  END
  CheckScriptSuccess(lines)
enddef

def FuncWithComment(  # comment
  a: number, #comment
  b: bool, # comment
  c: string) #comment
  assert_equal(4, a)
  assert_equal(true, b)
  assert_equal('yes', c)
enddef

def Test_func_with_comments()
  FuncWithComment(4, true, 'yes')

  var lines =<< trim END
      def Func(# comment
        arg: string)
      enddef
  END
  CheckScriptFailure(lines, 'E125:', 1)

  lines =<< trim END
      def Func(
        arg: string# comment
        )
      enddef
  END
  CheckScriptFailure(lines, 'E475:', 2)

  lines =<< trim END
      def Func(
        arg: string
        )# comment
      enddef
  END
  CheckScriptFailure(lines, 'E488:', 3)
enddef

def Test_nested_function()
  def NestedDef(arg: string): string
    return 'nested ' .. arg
  enddef
  NestedDef(':def')->assert_equal('nested :def')

  func NestedFunc(arg)
    return 'nested ' .. a:arg
  endfunc
  NestedFunc(':func')->assert_equal('nested :func')

  CheckDefFailure(['def Nested()', 'enddef', 'Nested(66)'], 'E118:')
  CheckDefFailure(['def Nested(arg: string)', 'enddef', 'Nested()'], 'E119:')

  CheckDefFailure(['def s:Nested()', 'enddef'], 'E1075:')
  CheckDefFailure(['def b:Nested()', 'enddef'], 'E1075:')

  var lines =<< trim END
      def Outer()
        def Inner()
          # comment
        enddef
        def Inner()
        enddef
      enddef
  END
  CheckDefFailure(lines, 'E1073:')

  lines =<< trim END
      def Outer()
        def Inner()
          # comment
        enddef
        def! Inner()
        enddef
      enddef
  END
  CheckDefFailure(lines, 'E1117:')

  lines =<< trim END
      vim9script
      def Outer()
        def Inner()
          g:result = 'ok'
        enddef
        Inner()
      enddef
      Outer()
      Inner()
  END
  CheckScriptFailure(lines, 'E117: Unknown function: Inner')
  assert_equal('ok', g:result)
  unlet g:result

  # nested function inside conditional
  lines =<< trim END
      vim9script
      var thecount = 0
      if true
        def Test(): number
          def TheFunc(): number
            thecount += 1
            return thecount
          enddef
          return TheFunc()
        enddef
      endif
      defcompile
      assert_equal(1, Test())
      assert_equal(2, Test())
  END
  CheckScriptSuccess(lines)

  # also works when "thecount" is inside the "if" block
  lines =<< trim END
      vim9script
      if true
        var thecount = 0
        def Test(): number
          def TheFunc(): number
            thecount += 1
            return thecount
          enddef
          return TheFunc()
        enddef
      endif
      defcompile
      assert_equal(1, Test())
      assert_equal(2, Test())
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Outer()
        def Inner()
          echo 'hello'
        enddef burp
      enddef
      defcompile
  END
  CheckScriptFailure(lines, 'E1173: Text found after enddef: burp', 3)
enddef

def Test_not_nested_function()
  echo printf('%d',
      function('len')('xxx'))
enddef

func Test_call_default_args_from_func()
  call MyDefaultArgs()->assert_equal('string')
  call MyDefaultArgs('one')->assert_equal('one')
  call assert_fails('call MyDefaultArgs("one", "two")', 'E118:', '', 3, 'Test_call_default_args_from_func')
endfunc

def Test_nested_global_function()
  var lines =<< trim END
      vim9script
      def Outer()
          def g:Inner(): string
              return 'inner'
          enddef
      enddef
      defcompile
      Outer()
      g:Inner()->assert_equal('inner')
      delfunc g:Inner
      Outer()
      g:Inner()->assert_equal('inner')
      delfunc g:Inner
      Outer()
      g:Inner()->assert_equal('inner')
      delfunc g:Inner
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Outer()
          func g:Inner()
            return 'inner'
          endfunc
      enddef
      defcompile
      Outer()
      g:Inner()->assert_equal('inner')
      delfunc g:Inner
      Outer()
      g:Inner()->assert_equal('inner')
      delfunc g:Inner
      Outer()
      g:Inner()->assert_equal('inner')
      delfunc g:Inner
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Outer()
          def g:Inner(): string
              return 'inner'
          enddef
      enddef
      defcompile
      Outer()
      Outer()
  END
  CheckScriptFailure(lines, "E122:")
  delfunc g:Inner

  lines =<< trim END
      vim9script
      def Outer()
        def g:Inner()
          echo map([1, 2, 3], (_, v) => v + 1)
        enddef
        g:Inner()
      enddef
      Outer()
  END
  CheckScriptSuccess(lines)
  delfunc g:Inner

  lines =<< trim END
      vim9script
      def Func()
        echo 'script'
      enddef
      def Outer()
        def Func()
          echo 'inner'
        enddef
      enddef
      defcompile
  END
  CheckScriptFailure(lines, "E1073:", 1)

  lines =<< trim END
      vim9script
      def Func()
        echo 'script'
      enddef
      def Func()
        echo 'script'
      enddef
  END
  CheckScriptFailure(lines, "E1073:", 5)
enddef

def DefListAll()
  def
enddef

def DefListOne()
  def DefListOne
enddef

def DefListMatches()
  def /DefList
enddef

def Test_nested_def_list()
  var funcs = split(execute('call DefListAll()'), "\n")
  assert_true(len(funcs) > 10)
  assert_true(funcs->index('def DefListAll()') >= 0)

  funcs = split(execute('call DefListOne()'), "\n")
  assert_equal(['   def DefListOne()', '1    def DefListOne', '   enddef'], funcs)

  funcs = split(execute('call DefListMatches()'), "\n")
  assert_true(len(funcs) >= 3)
  assert_true(funcs->index('def DefListAll()') >= 0)
  assert_true(funcs->index('def DefListOne()') >= 0)
  assert_true(funcs->index('def DefListMatches()') >= 0)

  var lines =<< trim END
    vim9script
    def Func()
      def +Func+
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E476:', 1)
enddef

def Test_global_local_function()
  var lines =<< trim END
      vim9script
      def g:Func(): string
          return 'global'
      enddef
      def Func(): string
          return 'local'
      enddef
      g:Func()->assert_equal('global')
      Func()->assert_equal('local')
      delfunc g:Func
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def g:Funcy()
        echo 'funcy'
      enddef
      s:Funcy()
  END
  CheckScriptFailure(lines, 'E117:')
enddef

def Test_local_function_shadows_global()
  var lines =<< trim END
      vim9script
      def g:Gfunc(): string
        return 'global'
      enddef
      def AnotherFunc(): number
        var Gfunc = function('len')
        return Gfunc('testing')
      enddef
      g:Gfunc()->assert_equal('global')
      AnotherFunc()->assert_equal(7)
      delfunc g:Gfunc
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def g:Func(): string
        return 'global'
      enddef
      def AnotherFunc()
        g:Func = function('len')
      enddef
      AnotherFunc()
  END
  CheckScriptFailure(lines, 'E705:')
  delfunc g:Func

  # global function is found without g: prefix
  lines =<< trim END
      vim9script
      def g:Func(): string
        return 'global'
      enddef
      def AnotherFunc(): string
        return Func()
      enddef
      assert_equal('global', AnotherFunc())
    delfunc g:Func
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def g:Func(): string
        return 'global'
      enddef
      assert_equal('global', Func())
      delfunc g:Func
  END
  CheckScriptSuccess(lines)

  # This does not shadow "i" which is visible only inside the for loop
  lines =<< trim END
      vim9script

      def Foo(i: number)
        echo i
      enddef

      for i in range(3)
        # Foo() is compiled here
        Foo(i)
      endfor
  END
  CheckScriptSuccess(lines)
enddef

func TakesOneArg(arg)
  echo a:arg
endfunc

def Test_call_wrong_args()
  CheckDefFailure(['TakesOneArg()'], 'E119:')
  CheckDefFailure(['TakesOneArg(11, 22)'], 'E118:')
  CheckDefFailure(['bufnr(xxx)'], 'E1001:')
  CheckScriptFailure(['def Func(Ref: func(s: string))'], 'E475:')

  var lines =<< trim END
    vim9script
    def Func(s: string)
      echo s
    enddef
    Func([])
  END
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got list<unknown>', 5)

  # argument name declared earlier is found when declaring a function
  lines =<< trim END
    vim9script
    var name = 'piet'
    def FuncOne(name: string)
      echo nr
    enddef
  END
  CheckScriptFailure(lines, 'E1168:')

  # argument name declared later is only found when compiling
  lines =<< trim END
    vim9script
    def FuncOne(name: string)
      echo nr
    enddef
    var name = 'piet'
  END
  CheckScriptSuccess(lines)
  CheckScriptFailure(lines + ['defcompile'], 'E1168:')

  lines =<< trim END
    vim9script
    def FuncOne(nr: number)
      echo nr
    enddef
    def FuncTwo()
      FuncOne()
    enddef
    defcompile
  END
  writefile(lines, 'Xscript')
  var didCatch = false
  try
    source Xscript
  catch
    assert_match('E119: Not enough arguments for function: <SNR>\d\+_FuncOne', v:exception)
    assert_match('Xscript\[8\]..function <SNR>\d\+_FuncTwo, line 1', v:throwpoint)
    didCatch = true
  endtry
  assert_true(didCatch)

  lines =<< trim END
    vim9script
    def FuncOne(nr: number)
      echo nr
    enddef
    def FuncTwo()
      FuncOne(1, 2)
    enddef
    defcompile
  END
  writefile(lines, 'Xscript')
  didCatch = false
  try
    source Xscript
  catch
    assert_match('E118: Too many arguments for function: <SNR>\d\+_FuncOne', v:exception)
    assert_match('Xscript\[8\]..function <SNR>\d\+_FuncTwo, line 1', v:throwpoint)
    didCatch = true
  endtry
  assert_true(didCatch)

  delete('Xscript')
enddef

def Test_call_funcref_wrong_args()
  var head =<< trim END
      vim9script
      def Func3(a1: string, a2: number, a3: list<number>)
        echo a1 .. a2 .. a3[0]
      enddef
      def Testme()
        var funcMap: dict<func> = {func: Func3}
  END
  var tail =<< trim END
      enddef
      Testme()
  END
  CheckScriptSuccess(head + ["funcMap['func']('str', 123, [1, 2, 3])"] + tail)

  CheckScriptFailure(head + ["funcMap['func']('str', 123)"] + tail, 'E119:')
  CheckScriptFailure(head + ["funcMap['func']('str', 123, [1], 4)"] + tail, 'E118:')

  var lines =<< trim END
      vim9script
      var Ref: func(number): any
      Ref = (j) => !j
      echo Ref(false)
  END
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected number but got bool', 4)

  lines =<< trim END
      vim9script
      var Ref: func(number): any
      Ref = (j) => !j
      call Ref(false)
  END
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected number but got bool', 4)
enddef

def Test_call_lambda_args()
  var lines =<< trim END
    var Callback = (..._) => 'anything'
    assert_equal('anything', Callback())
    assert_equal('anything', Callback(1))
    assert_equal('anything', Callback('a', 2))

    assert_equal('xyz', ((a: string): string => a)('xyz'))
  END
  CheckDefAndScriptSuccess(lines)

  CheckDefFailure(['echo ((i) => 0)()'],
                  'E119: Not enough arguments for function: ((i) => 0)()')

  lines =<< trim END
      var Ref = (x: number, y: number) => x + y
      echo Ref(1, 'x')
  END
  CheckDefFailure(lines, 'E1013: Argument 2: type mismatch, expected number but got string')

  lines =<< trim END
    var Ref: func(job, string, number)
    Ref = (x, y) => 0
  END
  CheckDefAndScriptFailure(lines, 'E1012:')

  lines =<< trim END
    var Ref: func(job, string)
    Ref = (x, y, z) => 0
  END
  CheckDefAndScriptFailure(lines, 'E1012:')

  lines =<< trim END
      var one = 1
      var l = [1, 2, 3]
      echo map(l, (one) => one)
  END
  CheckDefFailure(lines, 'E1167:')
  CheckScriptFailure(['vim9script'] + lines, 'E1168:')

  lines =<< trim END
    var Ref: func(any, ?any): bool
    Ref = (_, y = 1) => false
  END
  CheckDefAndScriptFailure(lines, 'E1172:')

  lines =<< trim END
      var a = 0
      var b = (a == 0 ? 1 : 2)
      assert_equal(1, b)
      var txt = 'a'
      b = (txt =~ 'x' ? 1 : 2)
      assert_equal(2, b)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      def ShadowLocal()
        var one = 1
        var l = [1, 2, 3]
        echo map(l, (one) => one)
      enddef
  END
  CheckDefFailure(lines, 'E1167:')

  lines =<< trim END
      def Shadowarg(one: number)
        var l = [1, 2, 3]
        echo map(l, (one) => one)
      enddef
  END
  CheckDefFailure(lines, 'E1167:')

  lines =<< trim END
    echo ((a) => a)('aa', 'bb')
  END
  CheckDefAndScriptFailure(lines, 'E118:', 1)

  lines =<< trim END
    echo 'aa'->((a) => a)('bb')
  END
  CheckDefFailure(lines, 'E118: Too many arguments for function: ->((a) => a)(''bb'')', 1)
  CheckScriptFailure(['vim9script'] + lines, 'E118: Too many arguments for function: <lambda>', 2)
enddef

def Test_lambda_line_nr()
  var lines =<< trim END
      vim9script
      # comment
      # comment
      var id = timer_start(1'000, (_) => 0)
      var out = execute('verbose ' .. timer_info(id)[0].callback
          ->string()
          ->substitute("('\\|')", ' ', 'g'))
      assert_match('Last set from .* line 4', out)
  END
  CheckScriptSuccess(lines)
enddef

def FilterWithCond(x: string, Cond: func(string): bool): bool
  return Cond(x)
enddef

def Test_lambda_return_type()
  var lines =<< trim END
    var Ref = (): => 123
  END
  CheckDefAndScriptFailure(lines, 'E1157:', 1)

  # no space before the return type
  lines =<< trim END
    var Ref = (x):number => x + 1
  END
  CheckDefAndScriptFailure(lines, 'E1069:', 1)

  # this works
  for x in ['foo', 'boo']
    echo FilterWithCond(x, (v) => v =~ '^b')
  endfor

  # this fails
  lines =<< trim END
      echo FilterWithCond('foo', (v) => v .. '^b')
  END
  CheckDefAndScriptFailure(lines, 'E1013: Argument 2: type mismatch, expected func(string): bool but got func(any): string', 1)

  lines =<< trim END
      var Lambda1 = (x) => {
              return x
              }
      assert_equal('asdf', Lambda1('asdf'))
      var Lambda2 = (x): string => {
              return x
              }
      assert_equal('foo', Lambda2('foo'))
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      var Lambda = (x): string => {
              return x
              }
      echo Lambda(['foo'])
  END
  CheckDefExecAndScriptFailure(lines, 'E1012:')
enddef

def Test_lambda_uses_assigned_var()
  CheckDefSuccess([
        'var x: any = "aaa"'
        'x = filter(["bbb"], (_, v) => v =~ x)'])
enddef

def Test_pass_legacy_lambda_to_def_func()
  var lines =<< trim END
      vim9script
      func Foo()
        eval s:Bar({x -> 0})
      endfunc
      def Bar(y: any)
      enddef
      Foo()
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def g:TestFunc(f: func)
      enddef
      legacy call g:TestFunc({-> 0})
      delfunc g:TestFunc

      def g:TestFunc(f: func(number))
      enddef
      legacy call g:TestFunc({nr -> 0})
      delfunc g:TestFunc
  END
  CheckScriptSuccess(lines)
enddef

def Test_lambda_in_reduce_line_break()
  # this was using freed memory
  var lines =<< trim END
      vim9script
      const result: dict<number> =
          ['Bob', 'Sam', 'Cat', 'Bob', 'Cat', 'Cat']
          ->reduce((acc, val) => {
              if has_key(acc, val)
                  acc[val] += 1
                  return acc
              else
                  acc[val] = 1
                  return acc
              endif
          }, {})
      assert_equal({Bob: 2, Sam: 1, Cat: 3}, result)
  END
  CheckScriptSuccess(lines)
enddef

def Test_set_opfunc_to_lambda()
  var lines =<< trim END
    vim9script
    nnoremap <expr> <F4> <SID>CountSpaces() .. '_'
    def CountSpaces(type = ''): string
      if type == ''
        &operatorfunc = (t) => CountSpaces(t)
        return 'g@'
      endif
      normal! '[V']y
      g:result = getreg('"')->count(' ')
      return ''
    enddef
    new
    'a b c d e'->setline(1)
    feedkeys("\<F4>", 'x')
    assert_equal(4, g:result)
    bwipe!
  END
  CheckScriptSuccess(lines)
enddef

def Test_set_opfunc_to_global_function()
  var lines =<< trim END
    vim9script
    def g:CountSpaces(type = ''): string
      normal! '[V']y
      g:result = getreg('"')->count(' ')
      return ''
    enddef
    # global function works at script level
    &operatorfunc = g:CountSpaces
    new
    'a b c d e'->setline(1)
    feedkeys("g@_", 'x')
    assert_equal(4, g:result)

    &operatorfunc = ''
    g:result = 0
    # global function works in :def function
    def Func()
      &operatorfunc = g:CountSpaces
    enddef
    Func()
    feedkeys("g@_", 'x')
    assert_equal(4, g:result)

    bwipe!
  END
  CheckScriptSuccess(lines)
  &operatorfunc = ''
enddef

def Test_use_script_func_name_with_prefix()
  var lines =<< trim END
      vim9script
      func s:Getit()
        return 'it'
      endfunc
      var Fn = s:Getit
      assert_equal('it', Fn())
  END
  CheckScriptSuccess(lines)
enddef

def Test_lambda_type_allocated()
  # Check that unreferencing a partial using a lambda can use the variable type
  # after the lambda has been freed and does not leak memory.
  var lines =<< trim END
    vim9script

    func MyomniFunc1(val, findstart, base)
      return a:findstart ? 0 : []
    endfunc

    var Lambda = (a, b) => MyomniFunc1(19, a, b)
    &omnifunc = Lambda
    Lambda = (a, b) => MyomniFunc1(20, a, b)
    &omnifunc = string(Lambda)
    Lambda = (a, b) => strlen(a)
  END
  CheckScriptSuccess(lines)
enddef

" Default arg and varargs
def MyDefVarargs(one: string, two = 'foo', ...rest: list<string>): string
  var res = one .. ',' .. two
  for s in rest
    res ..= ',' .. s
  endfor
  return res
enddef

def Test_call_def_varargs()
  assert_fails('MyDefVarargs()', 'E119:', '', 1, 'Test_call_def_varargs')
  MyDefVarargs('one')->assert_equal('one,foo')
  MyDefVarargs('one', 'two')->assert_equal('one,two')
  MyDefVarargs('one', 'two', 'three')->assert_equal('one,two,three')
  CheckDefFailure(['MyDefVarargs("one", 22)'],
      'E1013: Argument 2: type mismatch, expected string but got number')
  CheckDefFailure(['MyDefVarargs("one", "two", 123)'],
      'E1013: Argument 3: type mismatch, expected string but got number')

  var lines =<< trim END
      vim9script
      def Func(...l: list<string>)
        echo l
      enddef
      Func('a', 'b', 'c')
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Func(...l: list<string>)
        echo l
      enddef
      Func()
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Func(...l: list<any>)
        echo l
      enddef
      Func(0)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Func(...l: any)
        echo l
      enddef
      Func(0)
  END
  CheckScriptFailure(lines, 'E1180:', 2)

  lines =<< trim END
      vim9script
      def Func(..._l: list<string>)
        echo _l
      enddef
      Func('a', 'b', 'c')
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Func(...l: list<string>)
        echo l
      enddef
      Func(1, 2, 3)
  END
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch')

  lines =<< trim END
      vim9script
      def Func(...l: list<string>)
        echo l
      enddef
      Func('a', 9)
  END
  CheckScriptFailure(lines, 'E1013: Argument 2: type mismatch')

  lines =<< trim END
      vim9script
      def Func(...l: list<string>)
        echo l
      enddef
      Func(1, 'a')
  END
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch')

  lines =<< trim END
      vim9script
      def Func(  # some comment
                ...l = []
                )
        echo l
      enddef
  END
  CheckScriptFailure(lines, 'E1160:')

  lines =<< trim END
      vim9script
      def DoIt()
        g:Later('')
      enddef
      defcompile
      def g:Later(...l:  list<number>)
      enddef
      DoIt()
  END
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected number but got string')
enddef

let s:value = ''

def FuncOneDefArg(opt = 'text')
  s:value = opt
enddef

def FuncTwoDefArg(nr = 123, opt = 'text'): string
  return nr .. opt
enddef

def FuncVarargs(...arg: list<string>): string
  return join(arg, ',')
enddef

def Test_func_type_varargs()
  var RefDefArg: func(?string)
  RefDefArg = FuncOneDefArg
  RefDefArg()
  s:value->assert_equal('text')
  RefDefArg('some')
  s:value->assert_equal('some')

  var RefDef2Arg: func(?number, ?string): string
  RefDef2Arg = FuncTwoDefArg
  RefDef2Arg()->assert_equal('123text')
  RefDef2Arg(99)->assert_equal('99text')
  RefDef2Arg(77, 'some')->assert_equal('77some')

  CheckDefFailure(['var RefWrong: func(string?)'], 'E1010:')
  CheckDefFailure(['var RefWrong: func(?string, string)'], 'E1007:')

  var RefVarargs: func(...list<string>): string
  RefVarargs = FuncVarargs
  RefVarargs()->assert_equal('')
  RefVarargs('one')->assert_equal('one')
  RefVarargs('one', 'two')->assert_equal('one,two')

  CheckDefFailure(['var RefWrong: func(...list<string>, string)'], 'E110:')
  CheckDefFailure(['var RefWrong: func(...list<string>, ?string)'], 'E110:')
enddef

" Only varargs
def MyVarargsOnly(...args: list<string>): string
  return join(args, ',')
enddef

def Test_call_varargs_only()
  MyVarargsOnly()->assert_equal('')
  MyVarargsOnly('one')->assert_equal('one')
  MyVarargsOnly('one', 'two')->assert_equal('one,two')
  CheckDefFailure(['MyVarargsOnly(1)'], 'E1013: Argument 1: type mismatch, expected string but got number')
  CheckDefFailure(['MyVarargsOnly("one", 2)'], 'E1013: Argument 2: type mismatch, expected string but got number')
enddef

def Test_using_var_as_arg()
  var lines =<< trim END
      def Func(x: number)
        var x = 234
      enddef
  END
  CheckDefFailure(lines, 'E1006:')

  lines =<< trim END
      def Func(Ref: number)
        def Ref()
        enddef
      enddef
  END
  CheckDefFailure(lines, 'E1073:')
enddef

def DictArg(arg: dict<string>)
  arg['key'] = 'value'
enddef

def ListArg(arg: list<string>)
  arg[0] = 'value'
enddef

def Test_assign_to_argument()
  # works for dict and list
  var d: dict<string> = {}
  DictArg(d)
  d['key']->assert_equal('value')
  var l: list<string> = []
  ListArg(l)
  l[0]->assert_equal('value')

  CheckScriptFailure(['def Func(arg: number)', 'arg = 3', 'enddef', 'defcompile'], 'E1090:')
  delfunc! g:Func
enddef

" These argument names are reserved in legacy functions.
def WithReservedNames(firstline: string, lastline: string): string
  return firstline .. lastline
enddef

def Test_argument_names()
  assert_equal('OK', WithReservedNames('O', 'K'))
enddef

def Test_call_func_defined_later()
  g:DefinedLater('one')->assert_equal('one')
  assert_fails('NotDefined("one")', 'E117:', '', 2, 'Test_call_func_defined_later')
enddef

func DefinedLater(arg)
  return a:arg
endfunc

def Test_call_funcref()
  g:SomeFunc('abc')->assert_equal(3)
  assert_fails('NotAFunc()', 'E117:', '', 2, 'Test_call_funcref') # comment after call
  assert_fails('g:NotAFunc()', 'E1085:', '', 3, 'Test_call_funcref')

  var lines =<< trim END
    vim9script
    def RetNumber(): number
      return 123
    enddef
    var Funcref: func: number = function('RetNumber')
    Funcref()->assert_equal(123)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    def RetNumber(): number
      return 123
    enddef
    def Bar(F: func: number): number
      return F()
    enddef
    var Funcref = function('RetNumber')
    Bar(Funcref)->assert_equal(123)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    def UseNumber(nr: number)
      echo nr
    enddef
    var Funcref: func(number) = function('UseNumber')
    Funcref(123)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    def UseNumber(nr: number)
      echo nr
    enddef
    var Funcref: func(string) = function('UseNumber')
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected func(string) but got func(number)')

  lines =<< trim END
    vim9script
    def EchoNr(nr = 34)
      g:echo = nr
    enddef
    var Funcref: func(?number) = function('EchoNr')
    Funcref()
    g:echo->assert_equal(34)
    Funcref(123)
    g:echo->assert_equal(123)
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    def EchoList(...l: list<number>)
      g:echo = l
    enddef
    var Funcref: func(...list<number>) = function('EchoList')
    Funcref()
    g:echo->assert_equal([])
    Funcref(1, 2, 3)
    g:echo->assert_equal([1, 2, 3])
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    def OptAndVar(nr: number, opt = 12, ...l: list<number>): number
      g:optarg = opt
      g:listarg = l
      return nr
    enddef
    var Funcref: func(number, ?number, ...list<number>): number = function('OptAndVar')
    Funcref(10)->assert_equal(10)
    g:optarg->assert_equal(12)
    g:listarg->assert_equal([])

    Funcref(11, 22)->assert_equal(11)
    g:optarg->assert_equal(22)
    g:listarg->assert_equal([])

    Funcref(17, 18, 1, 2, 3)->assert_equal(17)
    g:optarg->assert_equal(18)
    g:listarg->assert_equal([1, 2, 3])
  END
  CheckScriptSuccess(lines)
enddef

let SomeFunc = function('len')
let NotAFunc = 'text'

def CombineFuncrefTypes()
  # same arguments, different return type
  var Ref1: func(bool): string
  var Ref2: func(bool): number
  var Ref3: func(bool): any
  Ref3 = g:cond ? Ref1 : Ref2

  # different number of arguments
  var Refa1: func(bool): number
  var Refa2: func(bool, number): number
  var Refa3: func: number
  Refa3 = g:cond ? Refa1 : Refa2

  # different argument types
  var Refb1: func(bool, string): number
  var Refb2: func(string, number): number
  var Refb3: func(any, any): number
  Refb3 = g:cond ? Refb1 : Refb2
enddef

def FuncWithForwardCall()
  return g:DefinedEvenLater("yes")
enddef

def DefinedEvenLater(arg: string): string
  return arg
enddef

def Test_error_in_nested_function()
  # Error in called function requires unwinding the call stack.
  assert_fails('FuncWithForwardCall()', 'E1096:', '', 1, 'FuncWithForwardCall')
enddef

def Test_nested_function_with_nextcmd()
  var lines =<< trim END
      vim9script
      # Define an outer function
      def FirstFunction()
        # Define an inner function
        def SecondFunction()
          # the function has a body, a double free is detected.
          AAAAA

         # enddef followed by | or } followed by # one or more characters
         enddef|BBBB
      enddef

      # Compile all functions
      defcompile
  END
  CheckScriptFailure(lines, 'E1173: Text found after enddef: BBBB')
enddef

def Test_nested_function_with_args_split()
  var lines =<< trim END
      vim9script
      def FirstFunction()
        def SecondFunction(
        )
        # had a double free if the right parenthesis of the nested function is
        # on the next line
         
        enddef|BBBB
      enddef
      # Compile all functions
      defcompile
  END
  CheckScriptFailure(lines, 'E1173: Text found after enddef: BBBB')

  lines =<< trim END
      vim9script
      def FirstFunction()
        func SecondFunction()
        endfunc|BBBB
      enddef
      defcompile
  END
  CheckScriptFailure(lines, 'E1173: Text found after endfunction: BBBB')
enddef

def Test_return_type_wrong()
  CheckScriptFailure([
        'def Func(): number',
        'return "a"',
        'enddef',
        'defcompile'], 'expected number but got string')
  delfunc! g:Func
  CheckScriptFailure([
        'def Func(): string',
        'return 1',
        'enddef',
        'defcompile'], 'expected string but got number')
  delfunc! g:Func
  CheckScriptFailure([
        'def Func(): void',
        'return "a"',
        'enddef',
        'defcompile'],
        'E1096: Returning a value in a function without a return type')
  delfunc! g:Func
  CheckScriptFailure([
        'def Func()',
        'return "a"',
        'enddef',
        'defcompile'],
        'E1096: Returning a value in a function without a return type')
  delfunc! g:Func

  CheckScriptFailure([
        'def Func(): number',
        'return',
        'enddef',
        'defcompile'], 'E1003:')
  delfunc! g:Func

  CheckScriptFailure([
        'def Func():number',
        'return 123',
        'enddef',
        'defcompile'], 'E1069:')
  delfunc! g:Func

  CheckScriptFailure([
        'def Func() :number',
        'return 123',
        'enddef',
        'defcompile'], 'E1059:')
  delfunc! g:Func

  CheckScriptFailure([
        'def Func() : number',
        'return 123',
        'enddef',
        'defcompile'], 'E1059:')
  delfunc! g:Func

  CheckScriptFailure(['def Func(): list', 'return []', 'enddef'], 'E1008:')
  delfunc! g:Func
  CheckScriptFailure(['def Func(): dict', 'return {}', 'enddef'], 'E1008:')
  delfunc! g:Func
  CheckScriptFailure(['def Func()', 'return 1'], 'E1057:')
  delfunc! g:Func

  CheckScriptFailure([
        'vim9script',
        'def FuncB()',
        '  return 123',
        'enddef',
        'def FuncA()',
        '   FuncB()',
        'enddef',
        'defcompile'], 'E1096:')
enddef

def Test_arg_type_wrong()
  CheckScriptFailure(['def Func3(items: list)', 'echo "a"', 'enddef'], 'E1008: Missing <type>')
  CheckScriptFailure(['def Func4(...)', 'echo "a"', 'enddef'], 'E1055: Missing name after ...')
  CheckScriptFailure(['def Func5(items:string)', 'echo "a"'], 'E1069:')
  CheckScriptFailure(['def Func5(items)', 'echo "a"'], 'E1077:')
  CheckScriptFailure(['def Func6(...x:list<number>)', 'echo "a"', 'enddef'], 'E1069:')
  CheckScriptFailure(['def Func7(...x: int)', 'echo "a"', 'enddef'], 'E1010:')
enddef

def Test_white_space_before_comma()
  var lines =<< trim END
    vim9script
    def Func(a: number , b: number)
    enddef
  END
  CheckScriptFailure(lines, 'E1068:')
  call assert_fails('vim9cmd echo stridx("a" .. "b" , "a")', 'E1068:')
enddef

def Test_white_space_after_comma()
  var lines =<< trim END
    vim9script
    def Func(a: number,b: number)
    enddef
  END
  CheckScriptFailure(lines, 'E1069:')

  # OK in legacy function
  lines =<< trim END
    vim9script
    func Func(a,b)
    endfunc
  END
  CheckScriptSuccess(lines)
enddef

def Test_vim9script_call()
  var lines =<< trim END
    vim9script
    var name = ''
    def MyFunc(arg: string)
       name = arg
    enddef
    MyFunc('foobar')
    name->assert_equal('foobar')

    var str = 'barfoo'
    str->MyFunc()
    name->assert_equal('barfoo')

    g:value = 'value'
    g:value->MyFunc()
    name->assert_equal('value')

    var listvar = []
    def ListFunc(arg: list<number>)
       listvar = arg
    enddef
    [1, 2, 3]->ListFunc()
    listvar->assert_equal([1, 2, 3])

    var dictvar = {}
    def DictFunc(arg: dict<number>)
       dictvar = arg
    enddef
    {a: 1, b: 2}->DictFunc()
    dictvar->assert_equal({a: 1, b: 2})
    def CompiledDict()
      {a: 3, b: 4}->DictFunc()
    enddef
    CompiledDict()
    dictvar->assert_equal({a: 3, b: 4})

    {a: 3, b: 4}->DictFunc()
    dictvar->assert_equal({a: 3, b: 4})

    ('text')->MyFunc()
    name->assert_equal('text')
    ("some")->MyFunc()
    name->assert_equal('some')

    # line starting with single quote is not a mark
    # line starting with double quote can be a method call
    'asdfasdf'->MyFunc()
    name->assert_equal('asdfasdf')
    "xyz"->MyFunc()
    name->assert_equal('xyz')

    def UseString()
      'xyork'->MyFunc()
    enddef
    UseString()
    name->assert_equal('xyork')

    def UseString2()
      "knife"->MyFunc()
    enddef
    UseString2()
    name->assert_equal('knife')

    # prepending a colon makes it a mark
    new
    setline(1, ['aaa', 'bbb', 'ccc'])
    normal! 3Gmt1G
    :'t
    getcurpos()[1]->assert_equal(3)
    bwipe!

    MyFunc(
        'continued'
        )
    assert_equal('continued',
            name
            )

    call MyFunc(
        'more'
          ..
          'lines'
        )
    assert_equal(
        'morelines',
        name)
  END
  writefile(lines, 'Xcall.vim')
  source Xcall.vim
  delete('Xcall.vim')
enddef

def Test_vim9script_call_fail_decl()
  var lines =<< trim END
    vim9script
    var name = ''
    def MyFunc(arg: string)
       var name = 123
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E1054:')
enddef

def Test_vim9script_call_fail_type()
  var lines =<< trim END
    vim9script
    def MyFunc(arg: string)
      echo arg
    enddef
    MyFunc(1234)
  END
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got number')
enddef

def Test_vim9script_call_fail_const()
  var lines =<< trim END
    vim9script
    const var = ''
    def MyFunc(arg: string)
       var = 'asdf'
    enddef
    defcompile
  END
  writefile(lines, 'Xcall_const.vim')
  assert_fails('source Xcall_const.vim', 'E46:', '', 1, 'MyFunc')
  delete('Xcall_const.vim')

  lines =<< trim END
      const g:Aconst = 77
      def Change()
        # comment
        g:Aconst = 99
      enddef
      call Change()
      unlet g:Aconst
  END
  CheckScriptFailure(lines, 'E741: Value is locked: Aconst', 2)
enddef

" Test that inside :function a Python function can be defined, :def is not
" recognized.
func Test_function_python()
  CheckFeature python3
  let py = 'python3'
  execute py "<< EOF"
def do_something():
  return 1
EOF
endfunc

def Test_delfunc()
  var lines =<< trim END
    vim9script
    def g:GoneSoon()
      echo 'hello'
    enddef

    def CallGoneSoon()
      GoneSoon()
    enddef
    defcompile

    delfunc g:GoneSoon
    CallGoneSoon()
  END
  writefile(lines, 'XToDelFunc')
  assert_fails('so XToDelFunc', 'E933:', '', 1, 'CallGoneSoon')
  assert_fails('so XToDelFunc', 'E933:', '', 1, 'CallGoneSoon')

  delete('XToDelFunc')
enddef

func Test_free_dict_while_in_funcstack()
  " relies on the sleep command
  CheckUnix
  call Run_Test_free_dict_while_in_funcstack()
endfunc

def Run_Test_free_dict_while_in_funcstack()

  # this was freeing the TermRun() default argument dictionary while it was
  # still referenced in a funcstack_T
  var lines =<< trim END
      vim9script

      &updatetime = 400
      def TermRun(_ = {})
          def Post()
          enddef
          def Exec()
              term_start('sleep 1', {
                  term_finish: 'close',
                  exit_cb: (_, _) => Post(),
              })
          enddef
          Exec()
      enddef
      nnoremap <F4> <Cmd>call <SID>TermRun()<CR>
      timer_start(100, (_) => feedkeys("\<F4>"))
      timer_start(1000, (_) => feedkeys("\<F4>"))
      sleep 1500m
  END
  CheckScriptSuccess(lines)
  nunmap <F4>
  set updatetime&
enddef

def Test_redef_failure()
  writefile(['def Func0(): string',  'return "Func0"', 'enddef'], 'Xdef')
  so Xdef
  writefile(['def Func1(): string',  'return "Func1"', 'enddef'], 'Xdef')
  so Xdef
  writefile(['def! Func0(): string', 'enddef', 'defcompile'], 'Xdef')
  assert_fails('so Xdef', 'E1027:', '', 1, 'Func0')
  writefile(['def Func2(): string',  'return "Func2"', 'enddef'], 'Xdef')
  so Xdef
  delete('Xdef')

  assert_fails('g:Func0()', 'E1091:')
  g:Func1()->assert_equal('Func1')
  g:Func2()->assert_equal('Func2')

  delfunc! Func0
  delfunc! Func1
  delfunc! Func2
enddef

def Test_vim9script_func()
  var lines =<< trim END
    vim9script
    func Func(arg)
      echo a:arg
    endfunc
    Func('text')
  END
  writefile(lines, 'XVim9Func')
  so XVim9Func

  delete('XVim9Func')
enddef

let s:funcResult = 0

def FuncNoArgNoRet()
  s:funcResult = 11
enddef

def FuncNoArgRetNumber(): number
  s:funcResult = 22
  return 1234
enddef

def FuncNoArgRetString(): string
  s:funcResult = 45
  return 'text'
enddef

def FuncOneArgNoRet(arg: number)
  s:funcResult = arg
enddef

def FuncOneArgRetNumber(arg: number): number
  s:funcResult = arg
  return arg
enddef

def FuncTwoArgNoRet(one: bool, two: number)
  s:funcResult = two
enddef

def FuncOneArgRetString(arg: string): string
  return arg
enddef

def FuncOneArgRetAny(arg: any): any
  return arg
enddef

def Test_func_type()
  var Ref1: func()
  s:funcResult = 0
  Ref1 = FuncNoArgNoRet
  Ref1()
  s:funcResult->assert_equal(11)

  var Ref2: func
  s:funcResult = 0
  Ref2 = FuncNoArgNoRet
  Ref2()
  s:funcResult->assert_equal(11)

  s:funcResult = 0
  Ref2 = FuncOneArgNoRet
  Ref2(12)
  s:funcResult->assert_equal(12)

  s:funcResult = 0
  Ref2 = FuncNoArgRetNumber
  Ref2()->assert_equal(1234)
  s:funcResult->assert_equal(22)

  s:funcResult = 0
  Ref2 = FuncOneArgRetNumber
  Ref2(13)->assert_equal(13)
  s:funcResult->assert_equal(13)
enddef

def Test_repeat_return_type()
  var res = 0
  for n in repeat([1], 3)
    res += n
  endfor
  res->assert_equal(3)

  res = 0
  for n in add([1, 2], 3)
    res += n
  endfor
  res->assert_equal(6)
enddef

def Test_argv_return_type()
  next fileone filetwo
  var res = ''
  for name in argv()
    res ..= name
  endfor
  res->assert_equal('fileonefiletwo')
enddef

def Test_func_type_part()
  var RefVoid: func: void
  RefVoid = FuncNoArgNoRet
  RefVoid = FuncOneArgNoRet
  CheckDefFailure(['var RefVoid: func: void', 'RefVoid = FuncNoArgRetNumber'], 'E1012: Type mismatch; expected func(...) but got func(): number')
  CheckDefFailure(['var RefVoid: func: void', 'RefVoid = FuncNoArgRetString'], 'E1012: Type mismatch; expected func(...) but got func(): string')

  var RefAny: func(): any
  RefAny = FuncNoArgRetNumber
  RefAny = FuncNoArgRetString
  CheckDefFailure(['var RefAny: func(): any', 'RefAny = FuncNoArgNoRet'], 'E1012: Type mismatch; expected func(): any but got func()')
  CheckDefFailure(['var RefAny: func(): any', 'RefAny = FuncOneArgNoRet'], 'E1012: Type mismatch; expected func(): any but got func(number)')

  var RefAnyNoArgs: func: any = RefAny

  var RefNr: func: number
  RefNr = FuncNoArgRetNumber
  RefNr = FuncOneArgRetNumber
  CheckDefFailure(['var RefNr: func: number', 'RefNr = FuncNoArgNoRet'], 'E1012: Type mismatch; expected func(...): number but got func()')
  CheckDefFailure(['var RefNr: func: number', 'RefNr = FuncNoArgRetString'], 'E1012: Type mismatch; expected func(...): number but got func(): string')

  var RefStr: func: string
  RefStr = FuncNoArgRetString
  RefStr = FuncOneArgRetString
  CheckDefFailure(['var RefStr: func: string', 'RefStr = FuncNoArgNoRet'], 'E1012: Type mismatch; expected func(...): string but got func()')
  CheckDefFailure(['var RefStr: func: string', 'RefStr = FuncNoArgRetNumber'], 'E1012: Type mismatch; expected func(...): string but got func(): number')
enddef

def Test_func_type_fails()
  CheckDefFailure(['var ref1: func()'], 'E704:')

  CheckDefFailure(['var Ref1: func()', 'Ref1 = FuncNoArgRetNumber'], 'E1012: Type mismatch; expected func() but got func(): number')
  CheckDefFailure(['var Ref1: func()', 'Ref1 = FuncOneArgNoRet'], 'E1012: Type mismatch; expected func() but got func(number)')
  CheckDefFailure(['var Ref1: func()', 'Ref1 = FuncOneArgRetNumber'], 'E1012: Type mismatch; expected func() but got func(number): number')
  CheckDefFailure(['var Ref1: func(bool)', 'Ref1 = FuncTwoArgNoRet'], 'E1012: Type mismatch; expected func(bool) but got func(bool, number)')
  CheckDefFailure(['var Ref1: func(?bool)', 'Ref1 = FuncTwoArgNoRet'], 'E1012: Type mismatch; expected func(?bool) but got func(bool, number)')
  CheckDefFailure(['var Ref1: func(...bool)', 'Ref1 = FuncTwoArgNoRet'], 'E1012: Type mismatch; expected func(...bool) but got func(bool, number)')

  CheckDefFailure(['var RefWrong: func(string ,number)'], 'E1068:')
  CheckDefFailure(['var RefWrong: func(string,number)'], 'E1069:')
  CheckDefFailure(['var RefWrong: func(bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool)'], 'E1005:')
  CheckDefFailure(['var RefWrong: func(bool):string'], 'E1069:')
enddef

def Test_func_return_type()
  var nr: number
  nr = FuncNoArgRetNumber()
  nr->assert_equal(1234)

  nr = FuncOneArgRetAny(122)
  nr->assert_equal(122)

  var str: string
  str = FuncOneArgRetAny('yes')
  str->assert_equal('yes')

  CheckDefFailure(['var str: string', 'str = FuncNoArgRetNumber()'], 'E1012: Type mismatch; expected string but got number')
enddef

def Test_func_common_type()
  def FuncOne(n: number): number
    return n
  enddef
  def FuncTwo(s: string): number
    return len(s)
  enddef
  def FuncThree(n: number, s: string): number
    return n + len(s)
  enddef
  var list = [FuncOne, FuncTwo, FuncThree]
  assert_equal(8, list[0](8))
  assert_equal(4, list[1]('word'))
  assert_equal(7, list[2](3, 'word'))
enddef

def MultiLine(
    arg1: string,
    arg2 = 1234,
    ...rest: list<string>
      ): string
  return arg1 .. arg2 .. join(rest, '-')
enddef

def MultiLineComment(
    arg1: string, # comment
    arg2 = 1234, # comment
    ...rest: list<string> # comment
      ): string # comment
  return arg1 .. arg2 .. join(rest, '-')
enddef

def Test_multiline()
  MultiLine('text')->assert_equal('text1234')
  MultiLine('text', 777)->assert_equal('text777')
  MultiLine('text', 777, 'one')->assert_equal('text777one')
  MultiLine('text', 777, 'one', 'two')->assert_equal('text777one-two')
enddef

func Test_multiline_not_vim9()
  call MultiLine('text')->assert_equal('text1234')
  call MultiLine('text', 777)->assert_equal('text777')
  call MultiLine('text', 777, 'one')->assert_equal('text777one')
  call MultiLine('text', 777, 'one', 'two')->assert_equal('text777one-two')
endfunc


" When using CheckScriptFailure() for the below test, E1010 is generated instead
" of E1056.
func Test_E1056_1059()
  let caught_1056 = 0
  try
    def F():
      return 1
    enddef
  catch /E1056:/
    let caught_1056 = 1
  endtry
  eval caught_1056->assert_equal(1)

  let caught_1059 = 0
  try
    def F5(items : list)
      echo 'a'
    enddef
  catch /E1059:/
    let caught_1059 = 1
  endtry
  eval caught_1059->assert_equal(1)
endfunc

func DelMe()
  echo 'DelMe'
endfunc

def Test_error_reporting()
  # comment lines at the start of the function
  var lines =<< trim END
    " comment
    def Func()
      # comment
      # comment
      invalid
    enddef
    defcompile
  END
  writefile(lines, 'Xdef')
  try
    source Xdef
    assert_report('should have failed')
  catch /E476:/
    v:exception->assert_match('Invalid command: invalid')
    v:throwpoint->assert_match(', line 3$')
  endtry
  delfunc! g:Func

  # comment lines after the start of the function
  lines =<< trim END
    " comment
    def Func()
      var x = 1234
      # comment
      # comment
      invalid
    enddef
    defcompile
  END
  writefile(lines, 'Xdef')
  try
    source Xdef
    assert_report('should have failed')
  catch /E476:/
    v:exception->assert_match('Invalid command: invalid')
    v:throwpoint->assert_match(', line 4$')
  endtry
  delfunc! g:Func

  lines =<< trim END
    vim9script
    def Func()
      var db = {foo: 1, bar: 2}
      # comment
      var x = db.asdf
    enddef
    defcompile
    Func()
  END
  writefile(lines, 'Xdef')
  try
    source Xdef
    assert_report('should have failed')
  catch /E716:/
    v:throwpoint->assert_match('_Func, line 3$')
  endtry
  delfunc! g:Func

  delete('Xdef')
enddef

def Test_deleted_function()
  CheckDefExecFailure([
      'var RefMe: func = function("g:DelMe")',
      'delfunc g:DelMe',
      'echo RefMe()'], 'E117:')
enddef

def Test_unknown_function()
  CheckDefExecFailure([
      'var Ref: func = function("NotExist")',
      'delfunc g:NotExist'], 'E700:')
enddef

def RefFunc(Ref: func(any): any): string
  return Ref('more')
enddef

def Test_closure_simple()
  var local = 'some '
  RefFunc((s) => local .. s)->assert_equal('some more')
enddef

def MakeRef()
  var local = 'some '
  g:Ref = (s) => local .. s
enddef

def Test_closure_ref_after_return()
  MakeRef()
  g:Ref('thing')->assert_equal('some thing')
  unlet g:Ref
enddef

def MakeTwoRefs()
  var local = ['some']
  g:Extend = (s) => local->add(s)
  g:Read = () => local
enddef

def Test_closure_two_refs()
  MakeTwoRefs()
  join(g:Read(), ' ')->assert_equal('some')
  g:Extend('more')
  join(g:Read(), ' ')->assert_equal('some more')
  g:Extend('even')
  join(g:Read(), ' ')->assert_equal('some more even')

  unlet g:Extend
  unlet g:Read
enddef

def ReadRef(Ref: func(): list<string>): string
  return join(Ref(), ' ')
enddef

def ExtendRef(Ref: func(string): list<string>, add: string)
  Ref(add)
enddef

def Test_closure_two_indirect_refs()
  MakeTwoRefs()
  ReadRef(g:Read)->assert_equal('some')
  ExtendRef(g:Extend, 'more')
  ReadRef(g:Read)->assert_equal('some more')
  ExtendRef(g:Extend, 'even')
  ReadRef(g:Read)->assert_equal('some more even')

  unlet g:Extend
  unlet g:Read
enddef

def MakeArgRefs(theArg: string)
  var local = 'loc_val'
  g:UseArg = (s) => theArg .. '/' .. local .. '/' .. s
enddef

def MakeArgRefsVarargs(theArg: string, ...rest: list<string>)
  var local = 'the_loc'
  g:UseVararg = (s) => theArg .. '/' .. local .. '/' .. s .. '/' .. join(rest)
enddef

def Test_closure_using_argument()
  MakeArgRefs('arg_val')
  g:UseArg('call_val')->assert_equal('arg_val/loc_val/call_val')

  MakeArgRefsVarargs('arg_val', 'one', 'two')
  g:UseVararg('call_val')->assert_equal('arg_val/the_loc/call_val/one two')

  unlet g:UseArg
  unlet g:UseVararg

  var lines =<< trim END
      vim9script
      def Test(Fun: func(number): number): list<number>
        return map([1, 2, 3], (_, i) => Fun(i))
      enddef
      def Inc(nr: number): number
        return nr + 2
      enddef
      assert_equal([3, 4, 5], Test(Inc))
  END
  CheckScriptSuccess(lines)
enddef

def MakeGetAndAppendRefs()
  var local = 'a'

  def Append(arg: string)
    local ..= arg
  enddef
  g:Append = Append

  def Get(): string
    return local
  enddef
  g:Get = Get
enddef

def Test_closure_append_get()
  MakeGetAndAppendRefs()
  g:Get()->assert_equal('a')
  g:Append('-b')
  g:Get()->assert_equal('a-b')
  g:Append('-c')
  g:Get()->assert_equal('a-b-c')

  unlet g:Append
  unlet g:Get
enddef

def Test_nested_closure()
  var local = 'text'
  def Closure(arg: string): string
    return local .. arg
  enddef
  Closure('!!!')->assert_equal('text!!!')
enddef

func GetResult(Ref)
  return a:Ref('some')
endfunc

def Test_call_closure_not_compiled()
  var text = 'text'
  g:Ref = (s) =>  s .. text
  GetResult(g:Ref)->assert_equal('sometext')
enddef

def Test_double_closure_fails()
  var lines =<< trim END
    vim9script
    def Func()
      var name = 0
      for i in range(2)
          timer_start(0, () => name)
      endfor
    enddef
    Func()
  END
  CheckScriptSuccess(lines)
enddef

def Test_nested_closure_used()
  var lines =<< trim END
      vim9script
      def Func()
        var x = 'hello'
        var Closure = () => x
        g:Myclosure = () => Closure()
      enddef
      Func()
      assert_equal('hello', g:Myclosure())
  END
  CheckScriptSuccess(lines)
enddef

def Test_nested_closure_fails()
  var lines =<< trim END
    vim9script
    def FuncA()
      FuncB(0)
    enddef
    def FuncB(n: number): list<string>
      return map([0], (_, v) => n)
    enddef
    FuncA()
  END
  CheckScriptFailure(lines, 'E1012:')
enddef

def Test_global_closure()
  var lines =<< trim END
      vim9script
      def ReverseEveryNLines(n: number, line1: number, line2: number)
        var mods = 'sil keepj keepp lockm '
        var range = ':' .. line1 .. ',' .. line2
        def g:Offset(): number
            var offset = (line('.') - line1 + 1) % n
            return offset != 0 ? offset : n
        enddef
        exe mods .. range .. 'g/^/exe "m .-" .. g:Offset()'
      enddef

      new
      repeat(['aaa', 'bbb', 'ccc'], 3)->setline(1)
      ReverseEveryNLines(3, 1, 9)
  END
  CheckScriptSuccess(lines)
  var expected = repeat(['ccc', 'bbb', 'aaa'], 3)
  assert_equal(expected, getline(1, 9))
  bwipe!
enddef

def Test_global_closure_called_directly()
  var lines =<< trim END
      vim9script
      def Outer()
        var x = 1
        def g:Inner()
          var y = x
          x += 1
          assert_equal(1, y)
        enddef
        g:Inner()
        assert_equal(2, x)
      enddef
      Outer()
  END
  CheckScriptSuccess(lines)
  delfunc g:Inner
enddef

def Test_closure_called_from_legacy()
  var lines =<< trim END
      vim9script
      def Func()
        var outer = 'foo'
        var F = () => {
              outer = 'bar'
            }
        execute printf('call %s()', string(F))
      enddef
      Func()
  END
  CheckScriptFailure(lines, 'E1248')
enddef

def Test_failure_in_called_function()
  # this was using the frame index as the return value
  var lines =<< trim END
      vim9script
      au TerminalWinOpen * eval [][0]
      def PopupTerm(a: any)
        # make sure typvals on stack are string
        ['a', 'b', 'c', 'd', 'e', 'f', 'g']->join()
        FireEvent()
      enddef
      def FireEvent()
          do TerminalWinOpen
      enddef
      # use try/catch to make eval fail
      try
          call PopupTerm(0)
      catch
      endtry
      au! TerminalWinOpen
  END
  CheckScriptSuccess(lines)
enddef

def Test_nested_lambda()
  var lines =<< trim END
    vim9script
    def Func()
      var x = 4
      var Lambda1 = () => 7
      var Lambda2 = () => [Lambda1(), x]
      var res = Lambda2()
      assert_equal([7, 4], res)
    enddef
    Func()
  END
  CheckScriptSuccess(lines)
enddef

def Test_double_nested_lambda()
  var lines =<< trim END
      vim9script
      def F(head: string): func(string): func(string): string
        return (sep: string): func(string): string => ((tail: string): string => {
            return head .. sep .. tail
          })
      enddef
      assert_equal('hello-there', F('hello')('-')('there'))
  END
  CheckScriptSuccess(lines)
enddef

def Test_nested_inline_lambda()
  var lines =<< trim END
      vim9script
      def F(text: string): func(string): func(string): string
        return (arg: string): func(string): string => ((sep: string): string => {
            return sep .. arg .. text
          })
      enddef
      assert_equal('--there++', F('++')('there')('--'))
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      echo range(4)->mapnew((_, v) => {
        return range(v) ->mapnew((_, s) => {
          return string(s)
          })
        })
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      def s:func()
        range(10)
          ->mapnew((_, _) => ({
            key: range(10)->mapnew((_, _) => {
              return ' '
            }),
          }))
      enddef

      defcomp
  END
  CheckScriptSuccess(lines)
enddef

def Shadowed(): list<number>
  var FuncList: list<func: number> = [() => 42]
  return FuncList->mapnew((_, Shadowed) => Shadowed())
enddef

def Test_lambda_arg_shadows_func()
  assert_equal([42], Shadowed())
enddef

def Line_continuation_in_def(dir: string = ''): string
  var path: string = empty(dir)
          \ ? 'empty'
          \ : 'full'
  return path
enddef

def Test_line_continuation_in_def()
  Line_continuation_in_def('.')->assert_equal('full')
enddef

def Test_script_var_in_lambda()
  var lines =<< trim END
      vim9script
      var script = 'test'
      assert_equal(['test'], map(['one'], (_, _) => script))
  END
  CheckScriptSuccess(lines)
enddef

def Line_continuation_in_lambda(): list<string>
  var x = range(97, 100)
      ->mapnew((_, v) => nr2char(v)
          ->toupper())
      ->reverse()
  return x
enddef

def Test_line_continuation_in_lambda()
  Line_continuation_in_lambda()->assert_equal(['D', 'C', 'B', 'A'])

  var lines =<< trim END
      vim9script
      var res = [{n: 1, m: 2, s: 'xxx'}]
                ->mapnew((_, v: dict<any>): string => printf('%d:%d:%s',
                    v.n,
                    v.m,
                    substitute(v.s, '.*', 'yyy', '')
                    ))
      assert_equal(['1:2:yyy'], res)
  END
  CheckScriptSuccess(lines)
enddef

def Test_list_lambda()
  timer_start(1000, (_) => 0)
  var body = execute(timer_info()[0].callback
         ->string()
         ->substitute("('", ' ', '')
         ->substitute("')", '', '')
         ->substitute('function\zs', ' ', ''))
  assert_match('def <lambda>\d\+(_: any): number\n1  return 0\n   enddef', body)
enddef

def Test_lambda_block_variable()
  var lines =<< trim END
      vim9script
      var flist: list<func>
      for i in range(10)
          var inloop = i
          flist[i] = () => inloop
      endfor
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      if true
        var outloop = 5
        var flist: list<func>
        for i in range(10)
          flist[i] = () => outloop
        endfor
      endif
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      if true
        var outloop = 5
      endif
      var flist: list<func>
      for i in range(10)
        flist[i] = () => outloop
      endfor
  END
  CheckScriptFailure(lines, 'E1001: Variable not found: outloop', 1)

  lines =<< trim END
      vim9script
      for i in range(10)
        var Ref = () => 0
      endfor
      assert_equal(0, ((i) => 0)(0))
  END
  CheckScriptSuccess(lines)
enddef

def Test_legacy_lambda()
  legacy echo {x -> 'hello ' .. x}('foo')

  var lines =<< trim END
      echo {x -> 'hello ' .. x}('foo')
  END
  CheckDefAndScriptFailure(lines, 'E720:')

  lines =<< trim END
      vim9script
      def Func()
        echo (() => 'no error')()
      enddef
      legacy call s:Func()
  END
  CheckScriptSuccess(lines)
enddef

def Test_legacy()
  var lines =<< trim END
      vim9script
      func g:LegacyFunction()
        let g:legacyvar = 1
      endfunc
      def Testit()
        legacy call g:LegacyFunction()
      enddef
      Testit()
      assert_equal(1, g:legacyvar)
      unlet g:legacyvar
      delfunc g:LegacyFunction
  END
  CheckScriptSuccess(lines)
enddef

def Test_legacy_errors()
  for cmd in ['if', 'elseif', 'else', 'endif',
              'for', 'endfor', 'continue', 'break',
              'while', 'endwhile',
              'try', 'catch', 'finally', 'endtry']
    CheckDefFailure(['legacy ' .. cmd .. ' expr'], 'E1189:')
  endfor
enddef

def Test_call_legacy_with_dict()
  var lines =<< trim END
      vim9script
      func Legacy() dict
        let g:result = self.value
      endfunc
      def TestDirect()
        var d = {value: 'yes', func: Legacy}
        d.func()
      enddef
      TestDirect()
      assert_equal('yes', g:result)
      unlet g:result

      def TestIndirect()
        var d = {value: 'foo', func: Legacy}
        var Fi = d.func
        Fi()
      enddef
      TestIndirect()
      assert_equal('foo', g:result)
      unlet g:result

      var d = {value: 'bar', func: Legacy}
      d.func()
      assert_equal('bar', g:result)
      unlet g:result
  END
  CheckScriptSuccess(lines)
enddef

def DoFilterThis(a: string): list<string>
  # closure nested inside another closure using argument
  var Filter = (l) => filter(l, (_, v) => stridx(v, a) == 0)
  return ['x', 'y', 'a', 'x2', 'c']->Filter()
enddef

def Test_nested_closure_using_argument()
  assert_equal(['x', 'x2'], DoFilterThis('x'))
enddef

def Test_triple_nested_closure()
  var what = 'x'
  var Match = (val: string, cmp: string): bool => stridx(val, cmp) == 0
  var Filter = (l) => filter(l, (_, v) => Match(v, what))
  assert_equal(['x', 'x2'], ['x', 'y', 'a', 'x2', 'c']->Filter())
enddef

func Test_silent_echo()
  CheckScreendump
  call Run_Test_silent_echo()
endfunc

def Run_Test_silent_echo()
  var lines =<< trim END
    vim9script
    def EchoNothing()
      silent echo ''
    enddef
    defcompile
  END
  writefile(lines, 'XTest_silent_echo')

  # Check that the balloon shows up after a mouse move
  var buf = RunVimInTerminal('-S XTest_silent_echo', {'rows': 6})
  term_sendkeys(buf, ":abc")
  VerifyScreenDump(buf, 'Test_vim9_silent_echo', {})

  # clean up
  StopVimInTerminal(buf)
  delete('XTest_silent_echo')
enddef

def SilentlyError()
  execute('silent! invalid')
  g:did_it = 'yes'
enddef

func UserError()
  silent! invalid
endfunc

def SilentlyUserError()
  UserError()
  g:did_it = 'yes'
enddef

" This can't be a :def function, because the assert would not be reached.
func Test_ignore_silent_error()
  let g:did_it = 'no'
  call SilentlyError()
  call assert_equal('yes', g:did_it)

  let g:did_it = 'no'
  call SilentlyUserError()
  call assert_equal('yes', g:did_it)

  unlet g:did_it
endfunc

def Test_ignore_silent_error_in_filter()
  var lines =<< trim END
      vim9script
      def Filter(winid: number, key: string): bool
          if key == 'o'
              silent! eval [][0]
              return true
          endif
          return popup_filter_menu(winid, key)
      enddef

      popup_create('popup', {filter: Filter})
      feedkeys("o\r", 'xnt')
  END
  CheckScriptSuccess(lines)
enddef

def Fibonacci(n: number): number
  if n < 2
    return n
  else
    return Fibonacci(n - 1) + Fibonacci(n - 2)
  endif
enddef

def Test_recursive_call()
  Fibonacci(20)->assert_equal(6765)
enddef

def TreeWalk(dir: string): list<any>
  return readdir(dir)->mapnew((_, val) =>
            fnamemodify(dir .. '/' .. val, ':p')->isdirectory()
               ? {[val]: TreeWalk(dir .. '/' .. val)}
               : val
             )
enddef

def Test_closure_in_map()
  mkdir('XclosureDir/tdir', 'p')
  writefile(['111'], 'XclosureDir/file1')
  writefile(['222'], 'XclosureDir/file2')
  writefile(['333'], 'XclosureDir/tdir/file3')

  TreeWalk('XclosureDir')->assert_equal(['file1', 'file2', {tdir: ['file3']}])

  delete('XclosureDir', 'rf')
enddef

def Test_invalid_function_name()
  var lines =<< trim END
      vim9script
      def s: list<string>
  END
  CheckScriptFailure(lines, 'E129:')

  lines =<< trim END
      vim9script
      def g: list<string>
  END
  CheckScriptFailure(lines, 'E129:')

  lines =<< trim END
      vim9script
      def <SID>: list<string>
  END
  CheckScriptFailure(lines, 'E884:')

  lines =<< trim END
      vim9script
      def F list<string>
  END
  CheckScriptFailure(lines, 'E488:')
enddef

def Test_partial_call()
  var lines =<< trim END
      var Xsetlist: func
      Xsetlist = function('setloclist', [0])
      Xsetlist([], ' ', {title: 'test'})
      getloclist(0, {title: 1})->assert_equal({title: 'test'})

      Xsetlist = function('setloclist', [0, [], ' '])
      Xsetlist({title: 'test'})
      getloclist(0, {title: 1})->assert_equal({title: 'test'})

      Xsetlist = function('setqflist')
      Xsetlist([], ' ', {title: 'test'})
      getqflist({title: 1})->assert_equal({title: 'test'})

      Xsetlist = function('setqflist', [[], ' '])
      Xsetlist({title: 'test'})
      getqflist({title: 1})->assert_equal({title: 'test'})

      var Len: func: number = function('len', ['word'])
      assert_equal(4, Len())

      var RepeatFunc = function('repeat', ['o'])
      assert_equal('ooooo', RepeatFunc(5))
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      vim9script
      def Foo(Parser: any)
      enddef
      var Expr: func(dict<any>): dict<any>
      const Call = Foo(Expr)
  END
  CheckScriptFailure(lines, 'E1235:')
enddef

def Test_cmd_modifier()
  tab echo '0'
  CheckDefFailure(['5tab echo 3'], 'E16:')
enddef

def Test_restore_modifiers()
  # check that when compiling a :def function command modifiers are not messed
  # up.
  var lines =<< trim END
      vim9script
      set eventignore=
      autocmd QuickFixCmdPost * copen
      def AutocmdsDisabled()
        eval 1 + 2
      enddef
      func Func()
        noautocmd call s:AutocmdsDisabled()
        let g:ei_after = &eventignore
      endfunc
      Func()
  END
  CheckScriptSuccess(lines)
  g:ei_after->assert_equal('')
enddef

def StackTop()
  eval 1 + 2
  eval 2 + 3
  # call not on fourth line
  StackBot()
enddef

def StackBot()
  # throw an error
  eval [][0]
enddef

def Test_callstack_def()
  try
    StackTop()
  catch
    v:throwpoint->assert_match('Test_callstack_def\[2\]..StackTop\[4\]..StackBot, line 2')
  endtry
enddef

" Re-using spot for variable used in block
def Test_block_scoped_var()
  var lines =<< trim END
      vim9script
      def Func()
        var x = ['a', 'b', 'c']
        if 1
          var y = 'x'
          map(x, (_, _) => y)
        endif
        var z = x
        assert_equal(['x', 'x', 'x'], z)
      enddef
      Func()
  END
  CheckScriptSuccess(lines)
enddef

def Test_reset_did_emsg()
  var lines =<< trim END
      @s = 'blah'
      au BufWinLeave * #
      def Func()
        var winid = popup_create('popup', {})
        exe '*s'
        popup_close(winid)
      enddef
      Func()
  END
  CheckScriptFailure(lines, 'E492:', 8)
  delfunc! g:Func
enddef

def Test_did_emsg_reset()
  # executing an autocommand resets did_emsg, this should not result in a
  # builtin function considered failing
  var lines =<< trim END
      vim9script
      au BufWinLeave * #
      def Func()
          popup_menu('', {callback: (a, b) => popup_create('', {})->popup_close()})
          eval [][0]
      enddef
      nno <F3> <cmd>call <sid>Func()<cr>
      feedkeys("\<F3>\e", 'xt')
  END
  writefile(lines, 'XemsgReset')
  assert_fails('so XemsgReset', ['E684:', 'E684:'], lines, 2)
  delete('XemsgReset')
  nunmap <F3>
  au! BufWinLeave
enddef

def Test_abort_with_silent_call()
  var lines =<< trim END
      vim9script
      g:result = 'none'
      def Func()
        g:result += 3
        g:result = 'yes'
      enddef
      # error is silenced, but function aborts on error
      silent! Func()
      assert_equal('none', g:result)
      unlet g:result
  END
  CheckScriptSuccess(lines)
enddef

def Test_continues_with_silent_error()
  var lines =<< trim END
      vim9script
      g:result = 'none'
      def Func()
        silent!  g:result += 3
        g:result = 'yes'
      enddef
      # error is silenced, function does not abort
      Func()
      assert_equal('yes', g:result)
      unlet g:result
  END
  CheckScriptSuccess(lines)
enddef

def Test_abort_even_with_silent()
  var lines =<< trim END
      vim9script
      g:result = 'none'
      def Func()
        eval {-> ''}() .. '' .. {}['X']
        g:result = 'yes'
      enddef
      silent! Func()
      assert_equal('none', g:result)
      unlet g:result
  END
  CheckScriptSuccess(lines)
enddef

def Test_cmdmod_silent_restored()
  var lines =<< trim END
      vim9script
      def Func()
        g:result = 'none'
        silent! g:result += 3
        g:result = 'none'
        g:result += 3
      enddef
      Func()
  END
  # can't use CheckScriptFailure, it ignores the :silent!
  var fname = 'Xdefsilent'
  writefile(lines, fname)
  var caught = 'no'
  try
    exe 'source ' .. fname
  catch /E1030:/
    caught = 'yes'
    assert_match('Func, line 4', v:throwpoint)
  endtry
  assert_equal('yes', caught)
  delete(fname)
enddef

def Test_cmdmod_silent_nested()
  var lines =<< trim END
      vim9script
      var result = ''

      def Error()
          result ..= 'Eb'
          eval [][0]
          result ..= 'Ea'
      enddef

      def Crash()
          result ..= 'Cb'
          sil! Error()
          result ..= 'Ca'
      enddef

      Crash()
      assert_equal('CbEbEaCa', result)
  END
  CheckScriptSuccess(lines)
enddef

def Test_dict_member_with_silent()
  var lines =<< trim END
      vim9script
      g:result = 'none'
      var d: dict<any>
      def Func()
        try
          g:result = map([], (_, v) => ({}[v]))->join() .. d['']
        catch
        endtry
      enddef
      silent! Func()
      assert_equal('0', g:result)
      unlet g:result
  END
  CheckScriptSuccess(lines)
enddef

def Test_skip_cmds_with_silent()
  var lines =<< trim END
      vim9script

      def Func(b: bool)
        Crash()
      enddef

      def Crash()
        sil! :/not found/d _
        sil! :/not found/put _
      enddef

      Func(true)
  END
  CheckScriptSuccess(lines)
enddef

def Test_opfunc()
  nnoremap <F3> <cmd>set opfunc=Opfunc<cr>g@
  def g:Opfunc(_: any): string
    setline(1, 'ASDF')
    return ''
  enddef
  new
  setline(1, 'asdf')
  feedkeys("\<F3>$", 'x')
  assert_equal('ASDF', getline(1))

  bwipe!
  nunmap <F3>
enddef

func Test_opfunc_error()
  CheckScreendump
  call Run_Test_opfunc_error()
endfunc

def Run_Test_opfunc_error()
  # test that the error from Opfunc() is displayed right away
  var lines =<< trim END
      vim9script

      def Opfunc(type: string)
        try
          eval [][0]
        catch /nothing/  # error not caught
        endtry
      enddef
      &operatorfunc = Opfunc
      nnoremap <expr> l <SID>L()
      def L(): string
        return 'l'
      enddef
      'x'->repeat(10)->setline(1)
      feedkeys('g@l', 'n')
      feedkeys('llll')
  END
  call writefile(lines, 'XTest_opfunc_error')

  var buf = RunVimInTerminal('-S XTest_opfunc_error', {rows: 6, wait_for_ruler: 0})
  WaitForAssert(() => assert_match('Press ENTER', term_getline(buf, 6)))
  WaitForAssert(() => assert_match('E684: list index out of range: 0', term_getline(buf, 5)))

  # clean up
  StopVimInTerminal(buf)
  delete('XTest_opfunc_error')
enddef

" this was crashing on exit
def Test_nested_lambda_in_closure()
  var lines =<< trim END
      vim9script
      command WriteDone writefile(['Done'], 'XnestedDone')
      def Outer()
          def g:Inner()
              echo map([1, 2, 3], {_, v -> v + 1})
          enddef
          g:Inner()
      enddef
      defcompile
      # not reached
  END
  if !RunVim([], lines, '--clean -c WriteDone -c quit')
    return
  endif
  assert_equal(['Done'], readfile('XnestedDone'))
  delete('XnestedDone')
enddef

def Test_check_func_arg_types()
  var lines =<< trim END
      vim9script
      def F1(x: string): string
        return x
      enddef

      def F2(x: number): number
        return x + 1
      enddef

      def G(g: func): dict<func>
        return {f: g}
      enddef

      def H(d: dict<func>): string
        return d.f('a')
      enddef
  END

  CheckScriptSuccess(lines + ['echo H(G(F1))'])
  CheckScriptFailure(lines + ['echo H(G(F2))'], 'E1013:')
enddef

def Test_list_any_type_checked()
  var lines =<< trim END
      vim9script
      def Foo()
        --decl--
        Bar(l)
      enddef
      def Bar(ll: list<dict<any>>)
      enddef
      Foo()
  END
  lines[2] = 'var l: list<any>'
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected list<dict<any>> but got list<any>', 2)

  lines[2] = 'var l: list<any> = []'
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected list<dict<any>> but got list<any>', 2)

  lines[2] = 'var l: list<any> = [11]'
  CheckScriptFailure(lines, 'E1013: Argument 1: type mismatch, expected list<dict<any>> but got list<number>', 2)
enddef

def Test_compile_error()
  var lines =<< trim END
    def g:Broken()
      echo 'a' + {}
    enddef
    call g:Broken()
  END
  # First call: compilation error
  CheckScriptFailure(lines, 'E1051: Wrong argument type for +')

  # Second call won't try compiling again
  assert_fails('call g:Broken()', 'E1091: Function is not compiled: Broken')
  delfunc g:Broken

  # No error when compiling with :silent!
  lines =<< trim END
    def g:Broken()
      echo 'a' + []
    enddef
    silent! defcompile
  END
  CheckScriptSuccess(lines)

  # Calling the function won't try compiling again
  assert_fails('call g:Broken()', 'E1091: Function is not compiled: Broken')
  delfunc g:Broken
enddef

def Test_ignored_argument()
  var lines =<< trim END
      vim9script
      def Ignore(_, _): string
        return 'yes'
      enddef
      assert_equal('yes', Ignore(1, 2))

      func Ok(_)
        return a:_
      endfunc
      assert_equal('ok', Ok('ok'))

      func Oktoo()
        let _ = 'too'
        return _
      endfunc
      assert_equal('too', Oktoo())

      assert_equal([[1], [2], [3]], range(3)->mapnew((_, v) => [v]->map((_, w) => w + 1)))
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      def Ignore(_: string): string
        return _
      enddef
      defcompile
  END
  CheckScriptFailure(lines, 'E1181:', 1)

  lines =<< trim END
      var _ = 1
  END
  CheckDefAndScriptFailure(lines, 'E1181:', 1)

  lines =<< trim END
      var x = _
  END
  CheckDefAndScriptFailure(lines, 'E1181:', 1)
enddef

def Test_too_many_arguments()
  var lines =<< trim END
    echo [0, 1, 2]->map(() => 123)
  END
  CheckDefExecAndScriptFailure(lines, 'E1106: 2 arguments too many', 1)

  lines =<< trim END
    echo [0, 1, 2]->map((_) => 123)
  END
  CheckDefExecAndScriptFailure(lines, 'E1106: One argument too many', 1)
enddef

def Test_closing_brace_at_start_of_line()
  var lines =<< trim END
      def Func()
      enddef
      Func(
      )
  END
  call CheckDefAndScriptSuccess(lines)
enddef

func CreateMydict()
  let g:mydict = {}
  func g:mydict.afunc()
    let g:result = self.key
  endfunc
endfunc

def Test_numbered_function_reference()
  CreateMydict()
  var output = execute('legacy func g:mydict.afunc')
  var funcName = 'g:' .. substitute(output, '.*function \(\d\+\).*', '\1', '')
  execute 'function(' .. funcName .. ', [], {key: 42})()'
  # check that the function still exists
  assert_equal(output, execute('legacy func g:mydict.afunc'))
  unlet g:mydict
enddef

def Test_go_beyond_end_of_cmd()
  # this was reading the byte after the end of the line
  var lines =<< trim END
    def F()
      cal
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E476:')
enddef

if has('python3')
  def Test_python3_heredoc()
    py3 << trim EOF
      import vim
      vim.vars['didit'] = 'yes'
    EOF
    assert_equal('yes', g:didit)

    python3 << trim EOF
      import vim
      vim.vars['didit'] = 'again'
    EOF
    assert_equal('again', g:didit)
  enddef
endif

" This messes up syntax highlight, keep near the end.
if has('lua')
  def Test_lua_heredoc()
    g:d = {}
    lua << trim EOF
        x = vim.eval('g:d')
        x['key'] = 'val'
    EOF
    assert_equal('val', g:d.key)
  enddef
endif


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
