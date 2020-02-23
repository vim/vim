" Test various aspects of the Vim9 script language.

source check.vim

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

def Test_syntax()
  let var = 234
  let other: list<string> = ['asdf']
enddef

func Test_def_basic()
  def SomeFunc(): string
    return 'yes'
  enddef
  call assert_equal('yes', SomeFunc())
endfunc

let s:appendToMe = 'xxx'
let s:addToMe = 111
let g:existing = 'yes'

def Test_assignment()
  let bool1: bool = true
  assert_equal(v:true, bool1)
  let bool2: bool = false
  assert_equal(v:false, bool2)

  let list1: list<string> = ['sdf', 'asdf']
  let list2: list<number> = [1, 2, 3]

  let listS: list<string> = []
  let listN: list<number> = []

  let dict1: dict<string> = #{key: 'value'}
  let dict2: dict<number> = #{one: 1, two: 2}

  g:newvar = 'new'
  assert_equal('new', g:newvar)

  assert_equal('yes', g:existing)
  g:existing = 'no'
  assert_equal('no', g:existing)

  v:char = 'abc'
  assert_equal('abc', v:char)

  $ENVVAR = 'foobar'
  assert_equal('foobar', $ENVVAR)
  $ENVVAR = ''

  s:appendToMe ..= 'yyy'
  assert_equal('xxxyyy', s:appendToMe)
  s:addToMe += 222
  assert_equal(333, s:addToMe)
  s:newVar = 'new'
  assert_equal('new', s:newVar)
enddef

func Test_assignment_failure()
  call CheckDefFailure(['let var=234'], 'E1004:')
  call CheckDefFailure(['let var =234'], 'E1004:')
  call CheckDefFailure(['let var= 234'], 'E1004:')

  call CheckDefFailure(['let true = 1'], 'E1034:')
  call CheckDefFailure(['let false = 1'], 'E1034:')

  call CheckDefFailure(['let var: list<string> = [123]'], 'expected list<string> but got list<number>')
  call CheckDefFailure(['let var: list<number> = ["xx"]'], 'expected list<number> but got list<string>')

  call CheckDefFailure(['let var: dict<string> = #{key: 123}'], 'expected dict<string> but got dict<number>')
  call CheckDefFailure(['let var: dict<number> = #{key: "xx"}'], 'expected dict<number> but got dict<string>')

  call CheckDefFailure(['let var = feedkeys("0")'], 'E1031:')
  call CheckDefFailure(['let var: number = feedkeys("0")'], 'expected number but got void')
endfunc

func Test_const()
  call CheckDefFailure(['const var = 234', 'var = 99'], 'E1018:')
  call CheckDefFailure(['const one = 234', 'let one = 99'], 'E1017:')
  call CheckDefFailure(['const two'], 'E1021:')
endfunc

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

def Test_return_string()
  assert_equal('string', ReturnString())
  assert_equal(123, ReturnNumber())
  assert_fails('call ReturnGlobal()', 'E1029: Expected number but got string')
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
enddef

func Test_call_default_args_from_func()
  call assert_equal('string', MyDefaultArgs())
  call assert_equal('one', MyDefaultArgs('one'))
  call assert_fails('call MyDefaultArgs("one", "two")', 'E118:')
endfunc

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
enddef


"def Test_call_func_defined_later()
"  call assert_equal('one', DefineLater('one'))
"  call assert_fails('call NotDefined("one")', 'E99:')
"enddef

func DefineLater(arg)
  return a:arg
endfunc

def Test_return_type_wrong()
  CheckScriptFailure(['def Func(): number', 'return "a"', 'enddef'], 'expected number but got string')
  CheckScriptFailure(['def Func(): string', 'return 1', 'enddef'], 'expected string but got number')
  CheckScriptFailure(['def Func(): void', 'return "a"', 'enddef'], 'expected void but got string')
  CheckScriptFailure(['def Func()', 'return "a"', 'enddef'], 'expected void but got string')
enddef

def Test_arg_type_wrong()
  CheckScriptFailure(['def Func3(items: list)', 'echo "a"', 'enddef'], 'E1008: Missing <type>')
enddef

def Test_try_catch()
  let l = []
  try
    add(l, '1')
    throw 'wrong'
    add(l, '2')
  catch
    add(l, v:exception)
  finally
    add(l, '3')
  endtry
  assert_equal(['1', 'wrong', '3'], l)
enddef

def ThrowFromDef()
  throw 'getout'
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
  finally
    seq ..= 'c'
  endtry
  assert_equal('abc', seq)
enddef

let s:export_script_lines =<< trim END
  vim9script
  let name: string = 'bob'
  def Concat(arg: string): string
    return name .. arg
  enddef
  let g:result = Concat('bie')
  let g:localname = name

  export const CONST = 1234
  export let exported = 9876
  export let exp_name = 'John'
  export def Exported(): string
    return 'Exported'
  enddef
END

def Test_vim9script()
  let import_script_lines =<< trim END
    vim9script
    import {exported, Exported} from './Xexport.vim'
    g:imported = exported
    exported += 3
    g:imported_added = exported
    g:imported_func = Exported()

    import {exp_name} from './Xexport.vim'
    g:imported_name = exp_name
    exp_name ..= ' Doe'
    g:imported_name_appended = exp_name
  END

  writefile(import_script_lines, 'Ximport.vim')
  writefile(s:export_script_lines, 'Xexport.vim')

  source Ximport.vim

  assert_equal('bobbie', g:result)
  assert_equal('bob', g:localname)
  assert_equal(9876, g:imported)
  assert_equal(9879, g:imported_added)
  assert_equal('Exported', g:imported_func)
  assert_equal('John', g:imported_name)
  assert_equal('John Doe', g:imported_name_appended)
  assert_false(exists('g:name'))

  unlet g:result
  unlet g:localname
  unlet g:imported
  unlet g:imported_added
  unlet g:imported_func
  unlet g:imported_name g:imported_name_appended
  delete('Ximport.vim')

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
  assert_equal(9876, g:imported)

  let import_star_lines =<< trim END
    vim9script
    import * from './Xexport.vim'
    g:imported = exported
  END
  writefile(import_star_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1045:')

  " try to import something that exists but is not exported
  let import_not_exported_lines =<< trim END
    vim9script
    import name from './Xexport.vim'
  END
  writefile(import_not_exported_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1049:')

  " import a very long name, requires making a copy
  let import_long_name_lines =<< trim END
    vim9script
    import name012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 from './Xexport.vim'
  END
  writefile(import_long_name_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1048:')

  let import_no_from_lines =<< trim END
    vim9script
    import name './Xexport.vim'
  END
  writefile(import_no_from_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1070:')

  let import_invalid_string_lines =<< trim END
    vim9script
    import name from Xexport.vim
  END
  writefile(import_invalid_string_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1071:')

  let import_wrong_name_lines =<< trim END
    vim9script
    import name from './XnoExport.vim'
  END
  writefile(import_wrong_name_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1053:')

  let import_missing_comma_lines =<< trim END
    vim9script
    import {exported name} from './Xexport.vim'
  END
  writefile(import_missing_comma_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1046:')

  delete('Ximport.vim')
  delete('Xexport.vim')

  " Check that in a Vim9 script 'cpo' is set to the Vim default.
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

def Test_vim9script_fails()
  CheckScriptFailure(['scriptversion 2', 'vim9script'], 'E1039:')
  CheckScriptFailure(['vim9script', 'scriptversion 2'], 'E1040:')
  CheckScriptFailure(['export let some = 123'], 'E1042:')
  CheckScriptFailure(['import some from "./Xexport.vim"'], 'E1042:')
  CheckScriptFailure(['vim9script', 'export let g:some'], 'E1044:')
  CheckScriptFailure(['vim9script', 'export echo 134'], 'E1043:')

  assert_fails('vim9script', 'E1038')
  assert_fails('export something', 'E1042')
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

def Test_vim9script_reload()
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

  " test that when not using "morelines" valtwo is still defined
  " need to source Xreload.vim again, import doesn't reload a script
  writefile(lines, 'Xreload.vim')
  source Xreload.vim
  source Ximport.vim

  " cannot declare a var twice
  lines =<< trim END
    vim9script
    let valone = 1234
    let valone = 5678
  END
  writefile(lines, 'Xreload.vim')
  assert_fails('source Xreload.vim', 'E1041:')

  delete('Xreload.vim')
  delete('Ximport.vim')
enddef

def Test_import_absolute()
  let import_lines = [
        \ 'vim9script',
        \ 'import exported from "' .. escape(getcwd(), '\') .. '/Xexport_abs.vim"',
        \ 'def UseExported()',
        \ '  g:imported_abs = exported',
        \ '  exported = 8888',
        \ '  g:imported_after = exported',
        \ 'enddef',
        \ 'UseExported()',
        \ 'g:import_disassembled = execute("disass UseExported")',
        \ ]
  writefile(import_lines, 'Ximport_abs.vim')
  writefile(s:export_script_lines, 'Xexport_abs.vim')

  source Ximport_abs.vim

  assert_equal(9876, g:imported_abs)
  assert_equal(8888, g:imported_after)
  assert_match('<SNR>\d\+_UseExported.*'
        \ .. 'g:imported_abs = exported.*'
        \ .. '0 LOADSCRIPT exported from .*Xexport_abs.vim.*'
        \ .. '1 STOREG g:imported_abs.*'
        \ .. 'exported = 8888.*'
        \ .. '3 STORESCRIPT exported in .*Xexport_abs.vim.*'
        \ .. 'g:imported_after = exported.*'
        \ .. '4 LOADSCRIPT exported from .*Xexport_abs.vim.*'
        \ .. '5 STOREG g:imported_after.*'
        \, g:import_disassembled)
  unlet g:imported_abs
  unlet g:import_disassembled

  delete('Ximport_abs.vim')
  delete('Xexport_abs.vim')
enddef

def Test_import_rtp()
  let import_lines = [
        \ 'vim9script',
        \ 'import exported from "Xexport_rtp.vim"',
        \ 'g:imported_rtp = exported',
        \ ]
  writefile(import_lines, 'Ximport_rtp.vim')
  mkdir('import')
  writefile(s:export_script_lines, 'import/Xexport_rtp.vim')

  let save_rtp = &rtp
  &rtp = getcwd()
  source Ximport_rtp.vim
  &rtp = save_rtp

  assert_equal(9876, g:imported_rtp)
  unlet g:imported_rtp

  delete('Ximport_rtp.vim')
  delete('import/Xexport_rtp.vim')
  delete('import', 'd')
enddef

def Test_fixed_size_list()
  " will be allocated as one piece of memory, check that changes work
  let l = [1, 2, 3, 4]
  l->remove(0)
  l->add(5)
  l->insert(99, 1)
  assert_equal([2, 99, 3, 4, 5], l)
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

def Test_substitute_cmd()
  new
  setline(1, 'something')
  :substitute(some(other(
  assert_equal('otherthing', getline(1))
  bwipe!

  " also when the context is Vim9 script
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
