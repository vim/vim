" Test various aspects of the Vim9 script language.

source check.vim
source view_util.vim

" Check that "lines" inside ":def" results in an "error" message.
func CheckDefFailure(lines, error)
  call writefile(['def Func()'] + a:lines + ['enddef'], 'Xdef')
  call assert_fails('so Xdef', a:error, a:lines)
  call delete('Xdef')
endfunc

func CheckScriptFailure(lines, error)
  call writefile(a:lines, 'Xdef')
  call assert_fails('so Xdef', a:error, a:lines)
  call delete('Xdef')
endfunc

func Test_def_basic()
  def SomeFunc(): string
    return 'yes'
  enddef
  call assert_equal('yes', SomeFunc())
endfunc

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
  assert_equal('string', ReturnString())
  assert_equal(123, ReturnNumber())
  assert_fails('call ReturnGlobal()', 'E1029: Expected number but got string')
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
  assert_equal(1, s:nothing)
enddef

func Increment()
  let g:counter += 1
endfunc

def Test_call_ufunc_count()
  g:counter = 1
  Increment()
  Increment()
  Increment()
  " works with and without :call
  assert_equal(4, g:counter)
  call assert_equal(4, g:counter)
  unlet g:counter
enddef

def MyVarargs(arg: string, ...rest: list<string>): string
  let res = arg
  for s in rest
    res ..= ',' .. s
  endfor
  return res
enddef

def Test_call_varargs()
  assert_equal('one', MyVarargs('one'))
  assert_equal('one,two', MyVarargs('one', 'two'))
  assert_equal('one,two,three', MyVarargs('one', 'two', 'three'))
enddef

def MyDefaultArgs(name = 'string'): string
  return name
enddef

def Test_call_default_args()
  assert_equal('string', MyDefaultArgs())
  assert_equal('one', MyDefaultArgs('one'))
  assert_fails('call MyDefaultArgs("one", "two")', 'E118:')

  call CheckScriptFailure(['def Func(arg: number = asdf)', 'enddef'], 'E1001:')
enddef

func Test_call_default_args_from_func()
  call assert_equal('string', MyDefaultArgs())
  call assert_equal('one', MyDefaultArgs('one'))
  call assert_fails('call MyDefaultArgs("one", "two")', 'E118:')
endfunc

func TakesOneArg(arg)
  echo a:arg
endfunc

def Test_call_wrong_args()
  call CheckDefFailure(['TakesOneArg()'], 'E119:')
  call CheckDefFailure(['TakesOneArg(11, 22)'], 'E118:')
  call CheckDefFailure(['bufnr(xxx)'], 'E1001:')
enddef

" Default arg and varargs
def MyDefVarargs(one: string, two = 'foo', ...rest: list<string>): string
  let res = one .. ',' .. two
  for s in rest
    res ..= ',' .. s
  endfor
  return res
enddef

def Test_call_def_varargs()
  call assert_fails('call MyDefVarargs()', 'E119:')
  assert_equal('one,foo', MyDefVarargs('one'))
  assert_equal('one,two', MyDefVarargs('one', 'two'))
  assert_equal('one,two,three', MyDefVarargs('one', 'two', 'three'))
  call CheckDefFailure(['MyDefVarargs("one", 22)'], 'E1013: argument 2: type mismatch, expected string but got number')
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
  let RefDefArg: func(?string)
  RefDefArg = FuncOneDefArg
  RefDefArg()
  assert_equal('text', s:value)
  RefDefArg('some')
  assert_equal('some', s:value)

  let RefDef2Arg: func(?number, ?string): string
  RefDef2Arg = FuncTwoDefArg
  assert_equal('123text', RefDef2Arg())
  assert_equal('99text', RefDef2Arg(99))
  assert_equal('77some', RefDef2Arg(77, 'some'))

  call CheckDefFailure(['let RefWrong: func(string?)'], 'E1010:')
  call CheckDefFailure(['let RefWrong: func(?string, string)'], 'E1007:')

  let RefVarargs: func(...list<string>): string
  RefVarargs = FuncVarargs
  assert_equal('', RefVarargs())
  assert_equal('one', RefVarargs('one'))
  assert_equal('one,two', RefVarargs('one', 'two'))

  call CheckDefFailure(['let RefWrong: func(...list<string>, string)'], 'E110:')
  call CheckDefFailure(['let RefWrong: func(...list<string>, ?string)'], 'E110:')
enddef

" Only varargs
def MyVarargsOnly(...args: list<string>): string
  return join(args, ',')
enddef

def Test_call_varargs_only()
  assert_equal('', MyVarargsOnly())
  assert_equal('one', MyVarargsOnly('one'))
  assert_equal('one,two', MyVarargsOnly('one', 'two'))
  call CheckDefFailure(['MyVarargsOnly(1)'], 'E1013: argument 1: type mismatch, expected string but got number')
  call CheckDefFailure(['MyVarargsOnly("one", 2)'], 'E1013: argument 2: type mismatch, expected string but got number')
enddef

def Test_using_var_as_arg()
  call writefile(['def Func(x: number)',  'let x = 234', 'enddef'], 'Xdef')
  call assert_fails('so Xdef', 'E1006:')
  call delete('Xdef')
enddef

def Test_call_func_defined_later()
  call assert_equal('one', DefinedLater('one'))
  call assert_fails('call NotDefined("one")', 'E117:')
enddef

func DefinedLater(arg)
  return a:arg
endfunc

def FuncWithForwardCall()
  return DefinedEvenLater("yes")
enddef

def DefinedEvenLater(arg: string): string
  return arg
enddef

def Test_error_in_nested_function()
  " Error in called function requires unwinding the call stack.
  assert_fails('call FuncWithForwardCall()', 'E1029')
enddef

def Test_return_type_wrong()
  CheckScriptFailure(['def Func(): number', 'return "a"', 'enddef'], 'expected number but got string')
  CheckScriptFailure(['def Func(): string', 'return 1', 'enddef'], 'expected string but got number')
  CheckScriptFailure(['def Func(): void', 'return "a"', 'enddef'], 'expected void but got string')
  CheckScriptFailure(['def Func()', 'return "a"', 'enddef'], 'expected void but got string')

  CheckScriptFailure(['def Func(): number', 'return', 'enddef'], 'E1003:')

  CheckScriptFailure(['def Func(): list', 'return []', 'enddef'], 'E1008:')
  CheckScriptFailure(['def Func(): dict', 'return {}', 'enddef'], 'E1008:')
  CheckScriptFailure(['def Func()', 'return 1'], 'E1057:')
enddef

def Test_arg_type_wrong()
  CheckScriptFailure(['def Func3(items: list)', 'echo "a"', 'enddef'], 'E1008: Missing <type>')
  CheckScriptFailure(['def Func4(...)', 'echo "a"', 'enddef'], 'E1055: Missing name after ...')
enddef

def Test_vim9script_call()
  let lines =<< trim END
    vim9script
    let var = ''
    def MyFunc(arg: string)
       var = arg
    enddef
    MyFunc('foobar')
    assert_equal('foobar', var)

    let str = 'barfoo'
    str->MyFunc()
    assert_equal('barfoo', var)

    let g:value = 'value'
    g:value->MyFunc()
    assert_equal('value', var)

    let listvar = []
    def ListFunc(arg: list<number>)
       listvar = arg
    enddef
    [1, 2, 3]->ListFunc()
    assert_equal([1, 2, 3], listvar)

    let dictvar = {}
    def DictFunc(arg: dict<number>)
       dictvar = arg
    enddef
    {'a': 1, 'b': 2}->DictFunc()
    assert_equal(#{a: 1, b: 2}, dictvar)
    def CompiledDict()
      {'a': 3, 'b': 4}->DictFunc()
    enddef
    CompiledDict()
    assert_equal(#{a: 3, b: 4}, dictvar)

    #{a: 3, b: 4}->DictFunc()
    assert_equal(#{a: 3, b: 4}, dictvar)

    ('text')->MyFunc()
    assert_equal('text', var)
    ("some")->MyFunc()
    assert_equal('some', var)
  END
  writefile(lines, 'Xcall.vim')
  source Xcall.vim
  delete('Xcall.vim')
enddef

def Test_vim9script_call_fail_decl()
  let lines =<< trim END
    vim9script
    let var = ''
    def MyFunc(arg: string)
       let var = 123
    enddef
  END
  writefile(lines, 'Xcall_decl.vim')
  assert_fails('source Xcall_decl.vim', 'E1054:')
  delete('Xcall_decl.vim')
enddef

def Test_vim9script_call_fail_const()
  let lines =<< trim END
    vim9script
    const var = ''
    def MyFunc(arg: string)
       var = 'asdf'
    enddef
  END
  writefile(lines, 'Xcall_const.vim')
  assert_fails('source Xcall_const.vim', 'E46:')
  delete('Xcall_const.vim')
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
  let lines =<< trim END
    vim9script
    def GoneSoon()
      echo 'hello'
    enddef

    def CallGoneSoon()
      GoneSoon()
    enddef

    delfunc GoneSoon
    CallGoneSoon()
  END
  writefile(lines, 'XToDelFunc')
  assert_fails('so XToDelFunc', 'E933')
  assert_fails('so XToDelFunc', 'E933')

  delete('XToDelFunc')
enddef

def Test_redef_failure()
  call writefile(['def Func0(): string',  'return "Func0"', 'enddef'], 'Xdef')
  so Xdef
  call writefile(['def Func1(): string',  'return "Func1"', 'enddef'], 'Xdef')
  so Xdef
  call writefile(['def! Func0(): string', 'enddef'], 'Xdef')
  call assert_fails('so Xdef', 'E1027:')
  call writefile(['def Func2(): string',  'return "Func2"', 'enddef'], 'Xdef')
  so Xdef
  call delete('Xdef')

  call assert_equal(0, Func0())
  call assert_equal('Func1', Func1())
  call assert_equal('Func2', Func2())

  delfunc! Func0
  delfunc! Func1
  delfunc! Func2
enddef

" Test for internal functions returning different types
func Test_InternalFuncRetType()
  let lines =<< trim END
    def RetFloat(): float
      return ceil(1.456)
    enddef

    def RetListAny(): list<any>
      return items({'k' : 'v'})
    enddef

    def RetListString(): list<string>
      return split('a:b:c', ':')
    enddef

    def RetListDictAny(): list<dict<any>>
      return getbufinfo()
    enddef

    def RetDictNumber(): dict<number>
      return wordcount()
    enddef

    def RetDictString(): dict<string>
      return environ()
    enddef
  END
  call writefile(lines, 'Xscript')
  source Xscript

  call assert_equal(2.0, RetFloat())
  call assert_equal([['k', 'v']], RetListAny())
  call assert_equal(['a', 'b', 'c'], RetListString())
  call assert_notequal([], RetListDictAny())
  call assert_notequal({}, RetDictNumber())
  call assert_notequal({}, RetDictString())
  call delete('Xscript')
endfunc

" Test for passing too many or too few arguments to internal functions
func Test_internalfunc_arg_error()
  let l =<< trim END
    def! FArgErr(): float
      return ceil(1.1, 2)
    enddef
  END
  call writefile(l, 'Xinvalidarg')
  call assert_fails('so Xinvalidarg', 'E118:')
  let l =<< trim END
    def! FArgErr(): float
      return ceil()
    enddef
  END
  call writefile(l, 'Xinvalidarg')
  call assert_fails('so Xinvalidarg', 'E119:')
  call delete('Xinvalidarg')
endfunc

let s:funcResult = 0

def FuncNoArgNoRet()
  funcResult = 11
enddef

def FuncNoArgRetNumber(): number
  funcResult = 22
  return 1234
enddef

def FuncNoArgRetString(): string
  funcResult = 45
  return 'text'
enddef

def FuncOneArgNoRet(arg: number)
  funcResult = arg
enddef

def FuncOneArgRetNumber(arg: number): number
  funcResult = arg
  return arg
enddef

def FuncTwoArgNoRet(one: bool, two: number)
  funcResult = two
enddef

def FuncOneArgRetString(arg: string): string
  return arg
enddef

def FuncOneArgRetAny(arg: any): any
  return arg
enddef

def Test_func_type()
  let Ref1: func()
  funcResult = 0
  Ref1 = FuncNoArgNoRet
  Ref1()
  assert_equal(11, funcResult)

  let Ref2: func
  funcResult = 0
  Ref2 = FuncNoArgNoRet
  Ref2()
  assert_equal(11, funcResult)

  funcResult = 0
  Ref2 = FuncOneArgNoRet
  Ref2(12)
  assert_equal(12, funcResult)

  funcResult = 0
  Ref2 = FuncNoArgRetNumber
  assert_equal(1234, Ref2())
  assert_equal(22, funcResult)

  funcResult = 0
  Ref2 = FuncOneArgRetNumber
  assert_equal(13, Ref2(13))
  assert_equal(13, funcResult)
enddef

def Test_func_type_part()
  let RefVoid: func: void
  RefVoid = FuncNoArgNoRet
  RefVoid = FuncOneArgNoRet
  CheckDefFailure(['let RefVoid: func: void', 'RefVoid = FuncNoArgRetNumber'], 'E1013: type mismatch, expected func() but got func(): number')
  CheckDefFailure(['let RefVoid: func: void', 'RefVoid = FuncNoArgRetString'], 'E1013: type mismatch, expected func() but got func(): string')

  let RefAny: func(): any
  RefAny = FuncNoArgRetNumber
  RefAny = FuncNoArgRetString
  CheckDefFailure(['let RefAny: func(): any', 'RefAny = FuncNoArgNoRet'], 'E1013: type mismatch, expected func(): any but got func()')
  CheckDefFailure(['let RefAny: func(): any', 'RefAny = FuncOneArgNoRet'], 'E1013: type mismatch, expected func(): any but got func(number)')

  let RefNr: func: number
  RefNr = FuncNoArgRetNumber
  RefNr = FuncOneArgRetNumber
  CheckDefFailure(['let RefNr: func: number', 'RefNr = FuncNoArgNoRet'], 'E1013: type mismatch, expected func(): number but got func()')
  CheckDefFailure(['let RefNr: func: number', 'RefNr = FuncNoArgRetString'], 'E1013: type mismatch, expected func(): number but got func(): string')

  let RefStr: func: string
  RefStr = FuncNoArgRetString
  RefStr = FuncOneArgRetString
  CheckDefFailure(['let RefStr: func: string', 'RefStr = FuncNoArgNoRet'], 'E1013: type mismatch, expected func(): string but got func()')
  CheckDefFailure(['let RefStr: func: string', 'RefStr = FuncNoArgRetNumber'], 'E1013: type mismatch, expected func(): string but got func(): number')
enddef

def Test_func_type_fails()
  CheckDefFailure(['let ref1: func()'], 'E704:')

  CheckDefFailure(['let Ref1: func()', 'Ref1 = FuncNoArgRetNumber'], 'E1013: type mismatch, expected func() but got func(): number')
  CheckDefFailure(['let Ref1: func()', 'Ref1 = FuncOneArgNoRet'], 'E1013: type mismatch, expected func() but got func(number)')
  CheckDefFailure(['let Ref1: func()', 'Ref1 = FuncOneArgRetNumber'], 'E1013: type mismatch, expected func() but got func(number): number')
  CheckDefFailure(['let Ref1: func(bool)', 'Ref1 = FuncTwoArgNoRet'], 'E1013: type mismatch, expected func(bool) but got func(bool, number)')
  CheckDefFailure(['let Ref1: func(?bool)', 'Ref1 = FuncTwoArgNoRet'], 'E1013: type mismatch, expected func(?bool) but got func(bool, number)')
  CheckDefFailure(['let Ref1: func(...bool)', 'Ref1 = FuncTwoArgNoRet'], 'E1013: type mismatch, expected func(...bool) but got func(bool, number)')

  call CheckDefFailure(['let RefWrong: func(string ,number)'], 'E1068:')
  call CheckDefFailure(['let RefWrong: func(string,number)'], 'E1069:')
  call CheckDefFailure(['let RefWrong: func(bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool)'], 'E740:')
  call CheckDefFailure(['let RefWrong: func(bool):string'], 'E1069:')
enddef

def Test_func_return_type()
  let nr: number
  nr = FuncNoArgRetNumber()
  assert_equal(1234, nr)

  nr = FuncOneArgRetAny(122)
  assert_equal(122, nr)

  let str: string
  str = FuncOneArgRetAny('yes')
  assert_equal('yes', str)

  CheckDefFailure(['let str: string', 'str = FuncNoArgRetNumber()'], 'E1013: type mismatch, expected string but got number')
enddef

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
  call assert_equal(1, caught_1056)

  let caught_1059 = 0
  try
    def F5(items : list)
      echo 'a'
    enddef
  catch /E1059:/
    let caught_1059 = 1
  endtry
  call assert_equal(1, caught_1059)
endfunc

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
