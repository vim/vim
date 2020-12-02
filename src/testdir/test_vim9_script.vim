" Test various aspects of the Vim9 script language.

source check.vim
source term_util.vim
source view_util.vim
source vim9.vim
source shared.vim

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

let g:alist = [7]
let g:astring = 'text'
let g:anumber = 123

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
  var lines =<< trim END
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

def Test_wrong_type()
  CheckDefFailure(['var name: list<nothing>'], 'E1010:')
  CheckDefFailure(['var name: list<list<nothing>>'], 'E1010:')
  CheckDefFailure(['var name: dict<nothing>'], 'E1010:')
  CheckDefFailure(['var name: dict<dict<nothing>>'], 'E1010:')

  CheckDefFailure(['var name: dict<number'], 'E1009:')
  CheckDefFailure(['var name: dict<list<number>'], 'E1009:')

  CheckDefFailure(['var name: ally'], 'E1010:')
  CheckDefFailure(['var name: bram'], 'E1010:')
  CheckDefFailure(['var name: cathy'], 'E1010:')
  CheckDefFailure(['var name: dom'], 'E1010:')
  CheckDefFailure(['var name: freddy'], 'E1010:')
  CheckDefFailure(['var name: john'], 'E1010:')
  CheckDefFailure(['var name: larry'], 'E1010:')
  CheckDefFailure(['var name: ned'], 'E1010:')
  CheckDefFailure(['var name: pam'], 'E1010:')
  CheckDefFailure(['var name: sam'], 'E1010:')
  CheckDefFailure(['var name: vim'], 'E1010:')

  CheckDefFailure(['var Ref: number', 'Ref()'], 'E1085:')
  CheckDefFailure(['var Ref: string', 'var res = Ref()'], 'E1085:')
enddef

def Test_script_wrong_type()
  var lines =<< trim END
      vim9script
      var s:dict: dict<string>
      s:dict['a'] = ['x']
  END
  CheckScriptFailure(lines, 'E1012: Type mismatch; expected string but got list<string>', 3)
enddef

def Test_const()
  CheckDefFailure(['final name = 234', 'name = 99'], 'E1018:')
  CheckDefFailure(['final one = 234', 'var one = 99'], 'E1017:')
  CheckDefFailure(['final list = [1, 2]', 'var list = [3, 4]'], 'E1017:')
  CheckDefFailure(['final two'], 'E1125:')
  CheckDefFailure(['final &option'], 'E996:')

  var lines =<< trim END
    final list = [1, 2, 3]
    list[0] = 4
    list->assert_equal([4, 2, 3])
    const other = [5, 6, 7]
    other->assert_equal([5, 6, 7])

    var varlist = [7, 8]
    const constlist = [1, varlist, 3]
    varlist[0] = 77
    # TODO: does not work yet
    # constlist[1][1] = 88
    var cl = constlist[1]
    cl[1] = 88
    constlist->assert_equal([1, [77, 88], 3])

    var vardict = #{five: 5, six: 6}
    const constdict = #{one: 1, two: vardict, three: 3}
    vardict['five'] = 55
    # TODO: does not work yet
    # constdict['two']['six'] = 66
    var cd = constdict['two']
    cd['six'] = 66
    constdict->assert_equal(#{one: 1, two: #{five: 55, six: 66}, three: 3})
  END
  CheckDefAndScriptSuccess(lines)
enddef

def Test_const_bang()
  var lines =<< trim END
      const var = 234
      var = 99
  END
  CheckDefExecFailure(lines, 'E1018:', 2)
  CheckScriptFailure(['vim9script'] + lines, 'E46:', 3)

  lines =<< trim END
      const ll = [2, 3, 4]
      ll[0] = 99
  END
  CheckDefExecFailure(lines, 'E1119:', 2)
  CheckScriptFailure(['vim9script'] + lines, 'E741:', 3)

  lines =<< trim END
      const ll = [2, 3, 4]
      ll[3] = 99
  END
  CheckDefExecFailure(lines, 'E1118:', 2)
  CheckScriptFailure(['vim9script'] + lines, 'E684:', 3)

  lines =<< trim END
      const dd = #{one: 1, two: 2}
      dd["one"] = 99
  END
  CheckDefExecFailure(lines, 'E1121:', 2)
  CheckScriptFailure(['vim9script'] + lines, 'E741:', 3)

  lines =<< trim END
      const dd = #{one: 1, two: 2}
      dd["three"] = 99
  END
  CheckDefExecFailure(lines, 'E1120:')
  CheckScriptFailure(['vim9script'] + lines, 'E741:', 3)
enddef

def Test_range_no_colon()
  CheckDefFailure(['%s/a/b/'], 'E1050:')
  CheckDefFailure(['+ s/a/b/'], 'E1050:')
  CheckDefFailure(['- s/a/b/'], 'E1050:')
  CheckDefFailure(['. s/a/b/'], 'E1050:')
enddef


def Test_block()
  var outer = 1
  {
    var inner = 2
    assert_equal(1, outer)
    assert_equal(2, inner)
  }
  assert_equal(1, outer)
enddef

def Test_block_failure()
  CheckDefFailure(['{', 'var inner = 1', '}', 'echo inner'], 'E1001:')
  CheckDefFailure(['}'], 'E1025:')
  CheckDefFailure(['{', 'echo 1'], 'E1026:')
enddef

def Test_block_local_vars()
  var lines =<< trim END
      vim9script
      v:testing = 1
      if true
        var text = ['hello']
        def SayHello(): list<string>
          return text
        enddef
        def SetText(v: string)
          text = [v]
        enddef
      endif

      if true
        var text = ['again']
        def SayAgain(): list<string>
          return text
        enddef
      endif

      # test that the "text" variables are not cleaned up
      test_garbagecollect_now()

      defcompile

      assert_equal(['hello'], SayHello())
      assert_equal(['again'], SayAgain())

      SetText('foobar')
      assert_equal(['foobar'], SayHello())

      call writefile(['ok'], 'Xdidit')
      qall!
  END

  # need to execute this with a separate Vim instance to avoid the current
  # context gets garbage collected.
  writefile(lines, 'Xscript')
  RunVim([], [], '-S Xscript')
  assert_equal(['ok'], readfile('Xdidit'))

  delete('Xscript')
  delete('Xdidit')
enddef

def Test_block_local_vars_with_func()
  var lines =<< trim END
      vim9script
      if true
        var foo = 'foo'
        if true
          var bar = 'bar'
          def Func(): list<string>
            return [foo, bar]
          enddef
        endif
      endif
      # function is compiled here, after blocks have finished, can still access
      # "foo" and "bar"
      assert_equal(['foo', 'bar'], Func())
  END
  CheckScriptSuccess(lines)
enddef

func g:NoSuchFunc()
  echo 'none'
endfunc

def Test_try_catch()
  var l = []
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
  fina
    add(l, 'finally')
  endtry
  assert_equal(['1', 'caught', 'finally'], l)

  var n: number
  try
    n = l[3]
  catch /E684:/
    n = 99
  endtry
  assert_equal(99, n)

  try
    # string slice returns a string, not a number
    n = g:astring[3]
  catch /E1012:/
    n = 77
  endtry
  assert_equal(77, n)

  try
    n = l[g:astring]
  catch /E1012:/
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

  var d = #{one: 1}
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

  var nd: dict<any>
  try
    nd = {[g:anumber]: 1}
  catch /E1012:/
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
  catch /E1012:/
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
    d = {text: 1, [g:astring]: 2}
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

  var P = function('g:NoSuchFunc')
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
  finall
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

def TryOne(): number
  try
    return 0
  catch
  endtry
  return 0
enddef

def TryTwo(n: number): string
  try
    var x = {}
  catch
  endtry
  return 'text'
enddef

def Test_try_catch_twice()
  assert_equal('text', TryOne()->TryTwo())
enddef

def Test_try_catch_match()
  var seq = 'a'
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
  CheckDefFailure(['if 1', 'endtry'], 'E171:')
  CheckDefFailure(['try', 'echo 1', 'endtry'], 'E1032:')

  CheckDefFailure(['throw'], 'E1015:')
  CheckDefFailure(['throw xxx'], 'E1001:')
enddef

def Test_throw_vimscript()
  # only checks line continuation
  var lines =<< trim END
      vim9script
      try
        throw 'one'
              .. 'two'
      catch
        assert_equal('onetwo', v:exception)
      endtry
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    @r = ''
    def Func()
      throw @r
    enddef
    var result = ''
    try
      Func()
    catch /E1129:/
      result = 'caught'
    endtry
    assert_equal('caught', result)
  END
  CheckScriptSuccess(lines)
enddef

def Test_error_in_nested_function()
  # an error in a nested :function aborts executin in the calling :def function
  var lines =<< trim END
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
  var lines =<< trim END
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
  var lines =<< trim END
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
  var lines =<< trim END
      vim9script
      var mylist = [
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
  var name: string = 'bob'
  def Concat(arg: string): string
    return name .. arg
  enddef
  g:result = Concat('bie')
  g:localname = name

  export const CONST = 1234
  export var exported = 9876
  export var exp_name = 'John'
  export def Exported(): string
    return 'Exported'
  enddef
END

def Undo_export_script_lines()
  unlet g:result
  unlet g:localname
enddef

def Test_vim9_import_export()
  var import_script_lines =<< trim END
    vim9script
    import {exported, Exported} from './Xexport.vim'
    g:imported = exported
    exported += 3
    g:imported_added = exported
    g:imported_func = Exported()

    def GetExported(): string
      var local_dict = #{ref: Exported}
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
  var import_line_break_script_lines =<< trim END
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
  var import_in_def_lines =<< trim END
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

  var import_star_as_lines =<< trim END
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

  var import_star_as_lines_no_dot =<< trim END
    vim9script
    import * as Export from './Xexport.vim'
    def Func()
      var dummy = 1
      var imported = Export + dummy
    enddef
    defcompile
  END
  writefile(import_star_as_lines_no_dot, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1060:', '', 2, 'Func')

  var import_star_as_lines_dot_space =<< trim END
    vim9script
    import * as Export from './Xexport.vim'
    def Func()
      var imported = Export . exported
    enddef
    defcompile
  END
  writefile(import_star_as_lines_dot_space, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1074:', '', 1, 'Func')

  var import_star_as_lines_missing_name =<< trim END
    vim9script
    import * as Export from './Xexport.vim'
    def Func()
      var imported = Export.
    enddef
    defcompile
  END
  writefile(import_star_as_lines_missing_name, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1048:', '', 1, 'Func')

  var import_star_as_lbr_lines =<< trim END
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

  var import_star_lines =<< trim END
    vim9script
    import * from './Xexport.vim'
  END
  writefile(import_star_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1045:', '', 2, 'Ximport.vim')

  # try to import something that exists but is not exported
  var import_not_exported_lines =<< trim END
    vim9script
    import name from './Xexport.vim'
  END
  writefile(import_not_exported_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1049:', '', 2, 'Ximport.vim')

  # try to import something that is already defined
  var import_already_defined =<< trim END
    vim9script
    var exported = 'something'
    import exported from './Xexport.vim'
  END
  writefile(import_already_defined, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1073:', '', 3, 'Ximport.vim')

  # try to import something that is already defined
  import_already_defined =<< trim END
    vim9script
    var exported = 'something'
    import * as exported from './Xexport.vim'
  END
  writefile(import_already_defined, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1073:', '', 3, 'Ximport.vim')

  # try to import something that is already defined
  import_already_defined =<< trim END
    vim9script
    var exported = 'something'
    import {exported} from './Xexport.vim'
  END
  writefile(import_already_defined, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1073:', '', 3, 'Ximport.vim')

  # import a very long name, requires making a copy
  var import_long_name_lines =<< trim END
    vim9script
    import name012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 from './Xexport.vim'
  END
  writefile(import_long_name_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1048:', '', 2, 'Ximport.vim')

  var import_no_from_lines =<< trim END
    vim9script
    import name './Xexport.vim'
  END
  writefile(import_no_from_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1070:', '', 2, 'Ximport.vim')

  var import_invalid_string_lines =<< trim END
    vim9script
    import name from Xexport.vim
  END
  writefile(import_invalid_string_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1071:', '', 2, 'Ximport.vim')

  var import_wrong_name_lines =<< trim END
    vim9script
    import name from './XnoExport.vim'
  END
  writefile(import_wrong_name_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1053:', '', 2, 'Ximport.vim')

  var import_missing_comma_lines =<< trim END
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
  var cpo_before = &cpo
  var lines =<< trim END
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
  var export_lines =<< trim END
    vim9script
    export def That(): string
      return 'yes'
    enddef
  END
  writefile(export_lines, 'Xexport_that.vim')

  var import_lines =<< trim END
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
  var export_lines =<< trim END
    vim9script
    export var That = 'yes'
  END
  writefile(export_lines, 'ftplugin/Xexport_ft.vim')

  var import_lines =<< trim END
    vim9script
    import That from './Xexport_ft.vim'
    assert_equal('yes', That)
    g:did_load_mytpe = 1
  END
  writefile(import_lines, 'ftplugin/qf.vim')

  var save_rtp = &rtp
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
  var lines =<< trim END
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
  CheckScriptFailure(['export var some = 123'], 'E1042:')
  CheckScriptFailure(['import some from "./Xexport.vim"'], 'E1048:')
  CheckScriptFailure(['vim9script', 'export var g:some'], 'E1022:')
  CheckScriptFailure(['vim9script', 'export echo 134'], 'E1043:')

  CheckScriptFailure(['vim9script', 'var str: string', 'str = 1234'], 'E1012:')
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
  var export =<< trim END
    vim9script
    export def Foo(): number
        return 0
    enddef
  END
  writefile(export, 'XexportCmd.vim')

  var buf = RunVimInTerminal('-c "import Foo from ''./XexportCmd.vim''"', #{
                rows: 6, wait_for_ruler: 0})
  WaitForAssert({-> assert_match('^E1094:', term_getline(buf, 5))})

  delete('XexportCmd.vim')
  StopVimInTerminal(buf)
enddef

def Test_vim9script_reload_import()
  var lines =<< trim END
    vim9script
    const var = ''
    var valone = 1234
    def MyFunc(arg: string)
       valone = 5678
    enddef
  END
  var morelines =<< trim END
    var valtwo = 222
    export def GetValtwo(): number
      return valtwo
    enddef
  END
  writefile(lines + morelines, 'Xreload.vim')
  source Xreload.vim
  source Xreload.vim
  source Xreload.vim

  var testlines =<< trim END
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
    var valone = 1234
    var valone = 5678
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
  var sortlines =<< trim END
      vim9script
      def Compare(i1: number, i2: number): number
        return i2 - i1
      enddef

      export def FastSort(): list<number>
        return range(5)->sort(Compare)
      enddef
  END
  writefile(sortlines, 'Xsort.vim')

  var lines =<< trim END
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

  var Funcref = function('s:RetSome')
  assert_equal('some', Funcref())
enddef

" Check that when searching for "FilterFunc" it finds the import in the
" script where FastFilter() is called from, both as a string and as a direct
" function reference.
def Test_vim9script_funcref_other_script()
  var filterLines =<< trim END
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

  var lines =<< trim END
    vim9script
    import {FilterFunc, FastFilter, FastFilterDirect} from './Xfilter.vim'
    def Test()
      var x: list<number> = FastFilter()
    enddef
    Test()
    def TestDirect()
      var x: list<number> = FastFilterDirect()
    enddef
    TestDirect()
  END
  CheckScriptSuccess(lines)
  delete('Xfilter.vim')
enddef

def Test_vim9script_reload_delfunc()
  var first_lines =<< trim END
    vim9script
    def FuncYes(): string
      return 'yes'
    enddef
  END
  var withno_lines =<< trim END
    def FuncNo(): string
      return 'no'
    enddef
    def g:DoCheck(no_exists: bool)
      assert_equal('yes', FuncYes())
      assert_equal('no', FuncNo())
    enddef
  END
  var nono_lines =<< trim END
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
  var lines =<< trim END
    vim9script
    var name = 'string'
  END
  writefile(lines, 'XreloadVar.vim')
  source XreloadVar.vim

  # now write the script using the same variable locally - works
  lines =<< trim END
    vim9script
    def Func()
      var name = 'string'
    enddef
  END
  writefile(lines, 'XreloadVar.vim')
  source XreloadVar.vim

  delete('XreloadVar.vim')
enddef

def Test_import_absolute()
  var import_lines = [
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
  assert_match('<SNR>\d\+_UseExported\_s*' ..
          'g:imported_abs = exported\_s*' ..
          '0 LOADSCRIPT exported-2 from .*Xexport_abs.vim\_s*' ..
          '1 STOREG g:imported_abs\_s*' ..
          'exported = 8888\_s*' ..
          '2 PUSHNR 8888\_s*' ..
          '3 STORESCRIPT exported-2 in .*Xexport_abs.vim\_s*' ..
          'g:imported_after = exported\_s*' ..
          '4 LOADSCRIPT exported-2 from .*Xexport_abs.vim\_s*' ..
          '5 STOREG g:imported_after',
        g:import_disassembled)

  Undo_export_script_lines()
  unlet g:imported_abs
  unlet g:import_disassembled

  delete('Ximport_abs.vim')
  delete('Xexport_abs.vim')
enddef

def Test_import_rtp()
  var import_lines = [
        'vim9script',
        'import exported from "Xexport_rtp.vim"',
        'g:imported_rtp = exported',
        ]
  writefile(import_lines, 'Ximport_rtp.vim')
  mkdir('import')
  writefile(s:export_script_lines, 'import/Xexport_rtp.vim')

  var save_rtp = &rtp
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
  var export_lines = [
        'vim9script',
        'export def ExpFunc(): string',
        '  return notDefined',
        'enddef',
        ]
  writefile(export_lines, 'Xexported.vim')

  var import_lines = [
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
    assert_match('E1001: Variable not found: notDefined', v:exception)
    assert_match('function <SNR>\d\+_ImpFunc\[1\]..<SNR>\d\+_ExpFunc, line 1', v:throwpoint)
  endtry

  delete('Xexported.vim')
  delete('Ximport.vim')
enddef

def Test_func_redefine_error()
  var lines = [
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
  var export_lines =<< trim END
      vim9script
      export def Func()
        echo 'imported'
      enddef
  END
  writefile(export_lines, 'XexportedFunc.vim')

  var lines =<< trim END
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
  var lines =<< trim END
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
      var  Foo = {-> 'lambda'}
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E1073:')
enddef

def Test_fixed_size_list()
  # will be allocated as one piece of memory, check that changes work
  var l = [1, 2, 3, 4]
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
  var res = ''
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
  var res = false
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
  var x: number = 0
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

  var cmd1 = 'setline(1,'
  var cmd2 = '"execute-var")'
  execute cmd1 cmd2 # comment
  assert_equal('execute-var', getline(1))

  execute cmd1 cmd2 '|setline(1, "execute-var-string")'
  assert_equal('execute-var-string', getline(1))

  var cmd_first = 'call '
  var cmd_last = 'setline(1, "execute-var-var")'
  execute cmd_first .. cmd_last
  assert_equal('execute-var-var', getline(1))
  bwipe!

  var n = true
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
  var lines =<< trim END
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

  var str1 = 'some'
  var str2 = 'more'
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
  var lines =<< trim END
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
  var lines =<< trim END
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
  var lines =<< trim END
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
  var result = ''
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

  var concat = ''
  for str in eval('["one", "two"]')
    concat ..= str
  endfor
  assert_equal('onetwo', concat)

  var total = 0
  for nr in
      [1, 2, 3]
    total += nr
  endfor
  assert_equal(6, total)

  total = 0
  for nr
    in [1, 2, 3]
    total += nr
  endfor
  assert_equal(6, total)

  total = 0
  for nr
    in
    [1, 2, 3]
    total += nr
  endfor
  assert_equal(6, total)
enddef

def Test_for_loop_fails()
  CheckDefFailure(['for # in range(5)'], 'E690:')
  CheckDefFailure(['for i In range(5)'], 'E690:')
  CheckDefFailure(['var x = 5', 'for x in range(5)'], 'E1017:')
  CheckScriptFailure(['def Func(arg: any)', 'for arg in range(5)', 'enddef', 'defcompile'], 'E1006:')
  CheckDefFailure(['for i in "text"'], 'E1012:')
  CheckDefFailure(['for i in xxx'], 'E1001:')
  CheckDefFailure(['endfor'], 'E588:')
  CheckDefFailure(['for i in range(3)', 'echo 3'], 'E170:')
enddef

def Test_for_loop_script_var()
  # cannot use s:var in a :def function
  CheckDefFailure(['for s:var in range(3)', 'echo 3'], 'E1101:')

  # can use s:var in Vim9 script, with or without s:
  var lines =<< trim END
    vim9script
    var total = 0
    for s:var in [1, 2, 3]
      total += s:var
    endfor
    assert_equal(6, total)

    total = 0
    for var in [1, 2, 3]
      total += var
    endfor
    assert_equal(6, total)
  END
enddef

def Test_for_loop_unpack()
  var lines =<< trim END
      var result = []
      for [v1, v2] in [[1, 2], [3, 4]]
        result->add(v1)
        result->add(v2)
      endfor
      assert_equal([1, 2, 3, 4], result)

      result = []
      for [v1, v2; v3] in [[1, 2], [3, 4, 5, 6]]
        result->add(v1)
        result->add(v2)
        result->add(v3)
      endfor
      assert_equal([1, 2, [], 3, 4, [5, 6]], result)

      result = []
      for [&ts, &sw] in [[1, 2], [3, 4]]
        result->add(&ts)
        result->add(&sw)
      endfor
      assert_equal([1, 2, 3, 4], result)

      var slist: list<string>
      for [$LOOPVAR, @r, v:errmsg] in [['a', 'b', 'c'], ['d', 'e', 'f']]
        slist->add($LOOPVAR)
        slist->add(@r)
        slist->add(v:errmsg)
      endfor
      assert_equal(['a', 'b', 'c', 'd', 'e', 'f'], slist)

      slist = []
      for [g:globalvar, b:bufvar, w:winvar, t:tabvar] in [['global', 'buf', 'win', 'tab'], ['1', '2', '3', '4']]
        slist->add(g:globalvar)
        slist->add(b:bufvar)
        slist->add(w:winvar)
        slist->add(t:tabvar)
      endfor
      assert_equal(['global', 'buf', 'win', 'tab', '1', '2', '3', '4'], slist)
  END
  CheckDefAndScriptSuccess(lines)

  lines =<< trim END
      for [v1, v2] in [[1, 2, 3], [3, 4]]
        echo v1 v2
      endfor
  END
  CheckDefExecFailure(lines, 'E710:', 1)

  lines =<< trim END
      for [v1, v2] in [[1], [3, 4]]
        echo v1 v2
      endfor
  END
  CheckDefExecFailure(lines, 'E711:', 1)

  lines =<< trim END
      for [v1, v1] in [[1, 2], [3, 4]]
        echo v1
      endfor
  END
  CheckDefExecFailure(lines, 'E1017:', 1)
enddef

def Test_while_loop()
  var result = ''
  var cnt = 0
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
  var caught = false
  var x = 0
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
  var mylist = [
      'one',
      'two',
      'three',
      ] # comment
  assert_equal(['one', 'two', 'three'], mylist)

  var mydict = {
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
      'let var = 123',
      ], 'E1126: Cannot use :let in Vim9 script')

  CheckScriptFailure([
      'vim9script',
      'var g:var = 123',
      ], 'E1016: Cannot declare a global variable:')

  CheckScriptFailure([
      'vim9script',
      'var b:var = 123',
      ], 'E1016: Cannot declare a buffer variable:')

  CheckScriptFailure([
      'vim9script',
      'var w:var = 123',
      ], 'E1016: Cannot declare a window variable:')

  CheckScriptFailure([
      'vim9script',
      'var t:var = 123',
      ], 'E1016: Cannot declare a tab variable:')

  CheckScriptFailure([
      'vim9script',
      'var v:version = 123',
      ], 'E1016: Cannot declare a v: variable:')

  CheckScriptFailure([
      'vim9script',
      'var $VARIABLE = "text"',
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
      'var v = 1 # comment5',
      ])

  CheckScriptFailure([
      'vim9script',
      'var v = 1# comment6',
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
  var lines =<< trim END
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

def Test_forward_declaration()
  var lines =<< trim END
    vim9script
    def GetValue(): string
      return theVal
    enddef
    var theVal = 'something'
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
  var legacy_lines =<< trim END
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

  var vim9_lines =<< trim END
    vim9script
    var local = 'local'
    g:global = 'global'
    export var exported = 'exported'
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
    var name = 'local'
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
  var lines =<< trim END
     vim9script
     def foo#test()
         echomsg getreg('"')
     enddef
  END

  mkdir('Xdir/autoload', 'p')
  writefile(lines, 'Xdir/autoload/foo.vim')
  var save_rtp = &rtp
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

" This was causing a crash because suppress_errthrow wasn't reset.
def Test_vim9_autoload_error()
  var lines =<< trim END
      vim9script
      def crash#func()
          try
              for x in List()
              endfor
          catch
          endtry
          g:ok = true
      enddef
      fu List()
          invalid
      endfu
      try
          invalid
      catch /wontmatch/
      endtry
  END
  call mkdir('Xruntime/autoload', 'p')
  call writefile(lines, 'Xruntime/autoload/crash.vim')

  # run in a separate Vim to avoid the side effects of assert_fails()
  lines =<< trim END
    exe 'set rtp^=' .. getcwd() .. '/Xruntime'
    call crash#func()
    call writefile(['ok'], 'Xdidit')
    qall!
  END
  writefile(lines, 'Xscript')
  RunVim([], [], '-S Xscript')
  assert_equal(['ok'], readfile('Xdidit'))

  delete('Xdidit')
  delete('Xscript')
  delete('Xruntime', 'rf')
enddef

def Test_script_var_in_autocmd()
  # using a script variable from an autocommand, defined in a :def function in a
  # legacy Vim script, cannot check the variable type.
  var lines =<< trim END
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
  var export_lines =<< trim END
    vim9script
    export var That = 'yes'
  END
  writefile(export_lines, 'rtp/syntax/Xexport.vim')
  var import_lines =<< trim END
    vim9script
    import That from './Xexport.vim'
  END
  writefile(import_lines, 'rtp/syntax/vim.vim')
  var save_rtp = &rtp
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

  if RunVim([], ['wq! Xdidit'], '+"func <SNR>1_func"')
    assert_equal([], readfile('Xdidit'))
  endif
  delete('Xdidit')
enddef

def Test_unset_any_variable()
  var lines =<< trim END
    var name: any
    assert_equal(0, name)
  END
  CheckDefAndScriptSuccess(lines)
enddef

func Test_define_func_at_command_line()
  CheckRunVimInTerminal

  " call indirectly to avoid compilation error for missing functions
  call Run_Test_define_func_at_command_line()
endfunc

def Run_Test_define_func_at_command_line()
  # run in a separate Vim instance to avoid the script context
  var lines =<< trim END
    func CheckAndQuit()
      call assert_fails('call Afunc()', 'E117: Unknown function: Bfunc')
      call writefile(['errors: ' .. string(v:errors)], 'Xdidcmd')
    endfunc
  END
  writefile([''], 'Xdidcmd')
  writefile(lines, 'XcallFunc')
  var buf = RunVimInTerminal('-S XcallFunc', #{rows: 6})
  # define Afunc() on the command line
  term_sendkeys(buf, ":def Afunc()\<CR>Bfunc()\<CR>enddef\<CR>")
  term_sendkeys(buf, ":call CheckAndQuit()\<CR>")
  WaitForAssert({-> assert_equal(['errors: []'], readfile('Xdidcmd'))})

  call StopVimInTerminal(buf)
  delete('XcallFunc')
  delete('Xdidcmd')
enddef

def Test_script_var_scope()
  var lines =<< trim END
      vim9script
      if true
        if true
          var one = 'one'
          echo one
        endif
        echo one
      endif
  END
  CheckScriptFailure(lines, 'E121:', 7)

  lines =<< trim END
      vim9script
      if true
        if false
          var one = 'one'
          echo one
        else
          var one = 'one'
          echo one
        endif
        echo one
      endif
  END
  CheckScriptFailure(lines, 'E121:', 10)

  lines =<< trim END
      vim9script
      while true
        var one = 'one'
        echo one
        break
      endwhile
      echo one
  END
  CheckScriptFailure(lines, 'E121:', 7)

  lines =<< trim END
      vim9script
      for i in range(1)
        var one = 'one'
        echo one
      endfor
      echo one
  END
  CheckScriptFailure(lines, 'E121:', 6)

  lines =<< trim END
      vim9script
      {
        var one = 'one'
        assert_equal('one', one)
      }
      assert_false(exists('one'))
      assert_false(exists('s:one'))
  END
  CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      {
        var one = 'one'
        echo one
      }
      echo one
  END
  CheckScriptFailure(lines, 'E121:', 6)
enddef

def Test_catch_exception_in_callback()
  var lines =<< trim END
    vim9script
    def Callback(...l: any)
      try
        var x: string
        var y: string
        # this error should be caught with CHECKLEN
        [x, y] = ['']
      catch
        g:caught = 'yes'
      endtry
    enddef
    popup_menu('popup', #{callback: Callback})
    feedkeys("\r", 'xt')
  END
  CheckScriptSuccess(lines)

  unlet g:caught
enddef

def Test_no_unknown_error_after_error()
  if !has('unix') || !has('job')
    throw 'Skipped: not unix of missing +job feature'
  endif
  var lines =<< trim END
      vim9script
      var source: list<number>
      def Out_cb(...l: any)
          eval [][0]
      enddef
      def Exit_cb(...l: any)
          sleep 1m
          source += l
      enddef
      var myjob = job_start('echo burp', #{out_cb: Out_cb, exit_cb: Exit_cb, mode: 'raw'})
      sleep 100m
  END
  writefile(lines, 'Xdef')
  assert_fails('so Xdef', ['E684:', 'E1012:'])
  delete('Xdef')
enddef

def Test_put_with_linebreak()
  new
  var lines =<< trim END
    vim9script
    pu=split('abc', '\zs')
            ->join()
  END
  CheckScriptSuccess(lines)
  getline(2)->assert_equal('a b c')
  bwipe!
enddef

def InvokeNormal()
  exe "norm! :m+1\r"
enddef

def Test_invoke_normal_in_visual_mode()
  xnoremap <F3> <Cmd>call <SID>InvokeNormal()<CR>
  new
  setline(1, ['aaa', 'bbb'])
  feedkeys("V\<F3>", 'xt')
  assert_equal(['bbb', 'aaa'], getline(1, 2))
  xunmap <F3>
enddef

" Keep this last, it messes up highlighting.
def Test_substitute_cmd()
  new
  setline(1, 'something')
  :substitute(some(other(
  assert_equal('otherthing', getline(1))
  bwipe!

  # also when the context is Vim9 script
  var lines =<< trim END
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
