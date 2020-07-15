" Test various aspects of the Vim9 script language.

source check.vim
source view_util.vim
source vim9.vim
source screendump.vim

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

  CheckScriptFailure(['def Func(arg: number = asdf)', 'enddef', 'defcompile'], 'E1001:')
  CheckScriptFailure(['def Func(arg: number = "text")', 'enddef', 'defcompile'], 'E1013: argument 1: type mismatch, expected number but got string')
enddef

def Test_nested_function()
  def Nested(arg: string): string
    return 'nested ' .. arg
  enddef
  assert_equal('nested function', Nested('function'))

  CheckDefFailure(['def Nested()', 'enddef', 'Nested(66)'], 'E118:')
  CheckDefFailure(['def Nested(arg: string)', 'enddef', 'Nested()'], 'E119:')

  CheckDefFailure(['func Nested()', 'endfunc'], 'E1086:')
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
  call CheckScriptFailure(['def Func(Ref: func(s: string))'], 'E475:')
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
  call writefile(['def Func(x: number)',  'let x = 234', 'enddef', 'defcompile'], 'Xdef')
  call assert_fails('so Xdef', 'E1006:')
  call delete('Xdef')
enddef

def DictArg(arg: dict<string>)
  arg['key'] = 'value'
enddef

def ListArg(arg: list<string>)
  arg[0] = 'value'
enddef

def Test_assign_to_argument()
  " works for dict and list
  let d: dict<string> = {}
  DictArg(d)
  assert_equal('value', d['key'])
  let l: list<string> = []
  ListArg(l)
  assert_equal('value', l[0])

  call CheckScriptFailure(['def Func(arg: number)', 'arg = 3', 'enddef', 'defcompile'], 'E1090:')
enddef

def Test_call_func_defined_later()
  call assert_equal('one', g:DefinedLater('one'))
  call assert_fails('call NotDefined("one")', 'E117:')
enddef

func DefinedLater(arg)
  return a:arg
endfunc

def Test_call_funcref()
  assert_equal(3, g:SomeFunc('abc'))
  assert_fails('NotAFunc()', 'E117:')
  assert_fails('g:NotAFunc()', 'E117:')
enddef

let SomeFunc = function('len')
let NotAFunc = 'text'

def CombineFuncrefTypes()
  " same arguments, different return type
  let Ref1: func(bool): string
  let Ref2: func(bool): number
  let Ref3: func(bool): any
  Ref3 = g:cond ? Ref1 : Ref2

  " different number of arguments
  let Refa1: func(bool): number
  let Refa2: func(bool, number): number
  let Refa3: func: number
  Refa3 = g:cond ? Refa1 : Refa2

  " different argument types
  let Refb1: func(bool, string): number
  let Refb2: func(string, number): number
  let Refb3: func(any, any): number
  Refb3 = g:cond ? Refb1 : Refb2
enddef

def FuncWithForwardCall()
  return g:DefinedEvenLater("yes")
enddef

def DefinedEvenLater(arg: string): string
  return arg
enddef

def Test_error_in_nested_function()
  " Error in called function requires unwinding the call stack.
  assert_fails('call FuncWithForwardCall()', 'E1096')
enddef

def Test_return_type_wrong()
  CheckScriptFailure(['def Func(): number', 'return "a"', 'enddef', 'defcompile'], 'expected number but got string')
  CheckScriptFailure(['def Func(): string', 'return 1', 'enddef', 'defcompile'], 'expected string but got number')
  CheckScriptFailure(['def Func(): void', 'return "a"', 'enddef', 'defcompile'], 'E1096: Returning a value in a function without a return type')
  CheckScriptFailure(['def Func()', 'return "a"', 'enddef', 'defcompile'], 'E1096: Returning a value in a function without a return type')

  CheckScriptFailure(['def Func(): number', 'return', 'enddef', 'defcompile'], 'E1003:')

  CheckScriptFailure(['def Func(): list', 'return []', 'enddef'], 'E1008:')
  CheckScriptFailure(['def Func(): dict', 'return {}', 'enddef'], 'E1008:')
  CheckScriptFailure(['def Func()', 'return 1'], 'E1057:')
enddef

def Test_arg_type_wrong()
  CheckScriptFailure(['def Func3(items: list)', 'echo "a"', 'enddef'], 'E1008: Missing <type>')
  CheckScriptFailure(['def Func4(...)', 'echo "a"', 'enddef'], 'E1055: Missing name after ...')
  CheckScriptFailure(['def Func5(items:string)', 'echo "a"'], 'E1069:')
  CheckScriptFailure(['def Func5(items)', 'echo "a"'], 'E1077:')
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

    g:value = 'value'
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

    'asdfasdf'->MyFunc()
    assert_equal('asdfasdf', var)

    def UseString()
      'xyork'->MyFunc()
    enddef
    UseString()
    assert_equal('xyork', var)

    MyFunc(
        'continued'
        )
    assert_equal('continued',
            var
            )

    call MyFunc(
        'more'
          ..
          'lines'
        )
    assert_equal(
        'morelines',
        var)
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
    defcompile
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
    defcompile
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
  assert_fails('so XToDelFunc', 'E933')
  assert_fails('so XToDelFunc', 'E933')

  delete('XToDelFunc')
enddef

def Test_redef_failure()
  call writefile(['def Func0(): string',  'return "Func0"', 'enddef'], 'Xdef')
  so Xdef
  call writefile(['def Func1(): string',  'return "Func1"', 'enddef'], 'Xdef')
  so Xdef
  call writefile(['def! Func0(): string', 'enddef', 'defcompile'], 'Xdef')
  call assert_fails('so Xdef', 'E1027:')
  call writefile(['def Func2(): string',  'return "Func2"', 'enddef'], 'Xdef')
  so Xdef
  call delete('Xdef')

  call assert_equal(0, g:Func0())
  call assert_equal('Func1', g:Func1())
  call assert_equal('Func2', g:Func2())

  delfunc! Func0
  delfunc! Func1
  delfunc! Func2
enddef

def Test_vim9script_func()
  let lines =<< trim END
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
    defcompile
  END
  call writefile(l, 'Xinvalidarg')
  call assert_fails('so Xinvalidarg', 'E118:')
  let l =<< trim END
    def! FArgErr(): float
      return ceil()
    enddef
    defcompile
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

def Test_repeat_return_type()
  let res = 0
  for n in repeat([1], 3)
    res += n
  endfor
  assert_equal(3, res)

  res = 0
  for n in add([1, 2], 3)
    res += n
  endfor
  assert_equal(6, res)
enddef

def Test_argv_return_type()
  next fileone filetwo
  let res = ''
  for name in argv()
    res ..= name
  endfor
  assert_equal('fileonefiletwo', res)
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
  assert_equal('text1234', MultiLine('text'))
  assert_equal('text777', MultiLine('text', 777))
  assert_equal('text777one', MultiLine('text', 777, 'one'))
  assert_equal('text777one-two', MultiLine('text', 777, 'one', 'two'))
enddef

func Test_multiline_not_vim9()
  call assert_equal('text1234', MultiLine('text'))
  call assert_equal('text777', MultiLine('text', 777))
  call assert_equal('text777one', MultiLine('text', 777, 'one'))
  call assert_equal('text777one-two', MultiLine('text', 777, 'one', 'two'))
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

func DelMe()
  echo 'DelMe'
endfunc

def Test_deleted_function()
  CheckDefExecFailure([
      'let RefMe: func = function("g:DelMe")',
      'delfunc g:DelMe',
      'echo RefMe()'], 'E117:')
enddef

def Test_unknown_function()
  CheckDefExecFailure([
      'let Ref: func = function("NotExist")',
      'delfunc g:NotExist'], 'E700:')
enddef

def RefFunc(Ref: func(string): string): string
  return Ref('more')
enddef

def Test_closure_simple()
  let local = 'some '
  assert_equal('some more', RefFunc({s -> local .. s}))
enddef

def MakeRef()
  let local = 'some '
  g:Ref = {s -> local .. s}
enddef

def Test_closure_ref_after_return()
  MakeRef()
  assert_equal('some thing', g:Ref('thing'))
  unlet g:Ref
enddef

def MakeTwoRefs()
  let local = ['some']
  g:Extend = {s -> local->add(s)}
  g:Read = {-> local}
enddef

def Test_closure_two_refs()
  MakeTwoRefs()
  assert_equal('some', join(g:Read(), ' '))
  g:Extend('more')
  assert_equal('some more', join(g:Read(), ' '))
  g:Extend('even')
  assert_equal('some more even', join(g:Read(), ' '))

  unlet g:Extend
  unlet g:Read
enddef

def ReadRef(Ref: func(): list<string>): string
  return join(Ref(), ' ')
enddef

def ExtendRef(Ref: func(string), add: string)
  Ref(add)
enddef

def Test_closure_two_indirect_refs()
  MakeTwoRefs()
  assert_equal('some', ReadRef(g:Read))
  ExtendRef(g:Extend, 'more')
  assert_equal('some more', ReadRef(g:Read))
  ExtendRef(g:Extend, 'even')
  assert_equal('some more even', ReadRef(g:Read))

  unlet g:Extend
  unlet g:Read
enddef

def MakeArgRefs(theArg: string)
  let local = 'loc_val'
  g:UseArg = {s -> theArg .. '/' .. local .. '/' .. s}
enddef

def MakeArgRefsVarargs(theArg: string, ...rest: list<string>)
  let local = 'the_loc'
  g:UseVararg = {s -> theArg .. '/' .. local .. '/' .. s .. '/' .. join(rest)}
enddef

def Test_closure_using_argument()
  MakeArgRefs('arg_val')
  assert_equal('arg_val/loc_val/call_val', g:UseArg('call_val'))

  MakeArgRefsVarargs('arg_val', 'one', 'two')
  assert_equal('arg_val/the_loc/call_val/one two', g:UseVararg('call_val'))

  unlet g:UseArg
  unlet g:UseVararg
enddef

def MakeGetAndAppendRefs()
  let local = 'a'

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
  assert_equal('a', g:Get())
  g:Append('-b')
  assert_equal('a-b', g:Get())
  g:Append('-c')
  assert_equal('a-b-c', g:Get())

  unlet g:Append
  unlet g:Get
enddef

def Test_nested_closure()
  let local = 'text'
  def Closure(arg: string): string
    return local .. arg
  enddef
  assert_equal('text!!!', Closure('!!!'))
enddef

func GetResult(Ref)
  return a:Ref('some')
endfunc

def Test_call_closure_not_compiled()
  let text = 'text'
  g:Ref = {s ->  s .. text}
  assert_equal('sometext', GetResult(g:Ref))
enddef

def Test_sort_return_type()
  let res: list<number>
  res = [1, 2, 3]->sort()
enddef

def Test_getqflist_return_type()
  let l = getqflist()
  assert_equal([], l)

  let d = getqflist(#{items: 0})
  assert_equal(#{items: []}, d)
enddef

def Test_getloclist_return_type()
  let l = getloclist(1)
  assert_equal([], l)

  let d = getloclist(1, #{items: 0})
  assert_equal(#{items: []}, d)
enddef

def Test_copy_return_type()
  let l = copy([1, 2, 3])
  let res = 0
  for n in l
    res += n
  endfor
  assert_equal(6, res)

  let dl = deepcopy([1, 2, 3])
  res = 0
  for n in dl
    res += n
  endfor
  assert_equal(6, res)
enddef

def Test_extend_return_type()
  let l = extend([1, 2], [3])
  let res = 0
  for n in l
    res += n
  endfor
  assert_equal(6, res)
enddef

def Test_insert_return_type()
  let l = insert([2, 1], 3)
  let res = 0
  for n in l
    res += n
  endfor
  assert_equal(6, res)
enddef

def Test_reverse_return_type()
  let l = reverse([1, 2, 3])
  let res = 0
  for n in l
    res += n
  endfor
  assert_equal(6, res)
enddef

def Test_remove_return_type()
  let l = remove(#{one: [1, 2], two: [3, 4]}, 'one')
  let res = 0
  for n in l
    res += n
  endfor
  assert_equal(3, res)
enddef

def Test_filter_return_type()
  let l = filter([1, 2, 3], {-> 1})
  let res = 0
  for n in l
    res += n
  endfor
  assert_equal(6, res)
enddef

def Wrong_dict_key_type(items: list<number>): list<number>
  return filter(items, {_, val -> get({val: 1}, 'x')})
enddef

def Test_wrong_dict_key_type()
  assert_fails('Wrong_dict_key_type([1, 2, 3])', 'E1029:')
enddef

def Line_continuation_in_def(dir: string = ''): string
    let path: string = empty(dir)
            \ ? 'empty'
            \ : 'full'
    return path
enddef

def Test_line_continuation_in_def()
  assert_equal('full', Line_continuation_in_def('.'))
enddef

def Line_continuation_in_lambda(): list<number>
  let x = range(97, 100)
      ->map({_, v -> nr2char(v)
          ->toupper()})
      ->reverse()
  return x
enddef

def Test_line_continuation_in_lambda()
  assert_equal(['D', 'C', 'B', 'A'], Line_continuation_in_lambda())
enddef

func Test_silent_echo()
  CheckScreendump

  let lines =<< trim END
    vim9script
    def EchoNothing()
      silent echo ''
    enddef
    defcompile
  END
  call writefile(lines, 'XTest_silent_echo')

  " Check that the balloon shows up after a mouse move
  let buf = RunVimInTerminal('-S XTest_silent_echo', {'rows': 6})
  call term_sendkeys(buf, ":abc")
  call VerifyScreenDump(buf, 'Test_vim9_silent_echo', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XTest_silent_echo')
endfunc

def Fibonacci(n: number): number
  if n < 2
    return n
  else
    return Fibonacci(n - 1) + Fibonacci(n - 2)
  endif
enddef

def Test_recursive_call()
  assert_equal(6765, Fibonacci(20))
enddef

def TreeWalk(dir: string): list<any>
  return readdir(dir)->map({_, val ->
            fnamemodify(dir .. '/' .. val, ':p')->isdirectory()
               ? {val : TreeWalk(dir .. '/' .. val)}
               : val
             })
enddef

def Test_closure_in_map()
  mkdir('XclosureDir/tdir', 'p')
  writefile(['111'], 'XclosureDir/file1')
  writefile(['222'], 'XclosureDir/file2')
  writefile(['333'], 'XclosureDir/tdir/file3')

  assert_equal(['file1', 'file2', {'tdir': ['file3']}], TreeWalk('XclosureDir'))

  delete('XclosureDir', 'rf')
enddef


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
