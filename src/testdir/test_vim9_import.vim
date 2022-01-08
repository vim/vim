" Test import/export of the Vim9 script language.

source check.vim
source term_util.vim
source vim9.vim

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
  export def ExportedValue(): number
    return exported
  enddef
  export def ExportedInc()
    exported += 5
  enddef
  export final theList = [1]
END

def Undo_export_script_lines()
  unlet g:result
  unlet g:localname
enddef

def Test_vim9_import_export()
  writefile(s:export_script_lines, 'Xexport.vim')
  var import_script_lines =<< trim END
    vim9script
    var dir = './'
    var ext = ".vim"
    import dir .. 'Xexport' .. ext as expo

    g:exported1 = expo.exported
    expo.exported += 3
    g:exported2 = expo.exported
    g:exported3 = expo.ExportedValue()

    expo.ExportedInc()
    g:exported_i1 = expo.exported
    g:exported_i2 = expo.ExportedValue()

    expo.exported = 11
    g:exported_s1 = expo.exported
    g:exported_s2 = expo.ExportedValue()

    g:imported_func = expo.Exported()

    def GetExported(): string
      var local_dict = {ref: expo.Exported}
      return local_dict.ref()
    enddef
    g:funcref_result = GetExported()

    g:imported_name = expo.exp_name
    expo.exp_name ..= ' Doe'
    g:imported_name_appended = expo.exp_name
    g:exported_later = expo.exported

    expo.theList->add(2)
    assert_equal([1, 2], expo.theList)
  END
  writefile(import_script_lines, 'Ximport.vim')
  source Ximport.vim

  assert_equal('bobbie', g:result)
  assert_equal('bob', g:localname)
  assert_equal(9876, g:exported1)
  assert_equal(9879, g:exported2)
  assert_equal(9879, g:exported3)

  assert_equal(9884, g:exported_i1)
  assert_equal(9884, g:exported_i2)

  assert_equal(11, g:exported_s1)
  assert_equal(11, g:exported_s2)
  assert_equal(11, g:exported_later)

  assert_equal('Exported', g:imported_func)
  assert_equal('Exported', g:funcref_result)
  assert_equal('John', g:imported_name)
  assert_equal('John Doe', g:imported_name_appended)
  assert_false(exists('g:name'))

  Undo_export_script_lines()
  unlet g:exported1
  unlet g:exported2
  unlet g:exported3
  unlet g:exported_i1
  unlet g:exported_i2
  unlet g:exported_later
  unlet g:imported_func
  unlet g:imported_name g:imported_name_appended
  delete('Ximport.vim')

  # similar, with line breaks
  var import_line_break_script_lines =<< trim END
    vim9script
    import './Xexport.vim'
        as expo
    g:exported = expo.exported
    expo.exported += 7
    g:exported_added = expo.exported
    g:imported_func = expo.Exported()
  END
  writefile(import_line_break_script_lines, 'Ximport_lbr.vim')
  source Ximport_lbr.vim

  assert_equal(11, g:exported)
  assert_equal(18, g:exported_added)
  assert_equal('Exported', g:imported_func)

  # exported script not sourced again
  assert_false(exists('g:result'))
  unlet g:exported
  unlet g:exported_added
  unlet g:imported_func
  delete('Ximport_lbr.vim')

  var line_break_before_dot =<< trim END
    vim9script
    import './Xexport.vim' as expo
    g:exported = expo
                  .exported
  END
  writefile(line_break_before_dot, 'Ximport_lbr_before_dot.vim')
  assert_fails('source Ximport_lbr_before_dot.vim', 'E1060:', '', 3)
  delete('Ximport_lbr_before_dot.vim')

  var line_break_after_dot =<< trim END
    vim9script
    import './Xexport.vim' as expo
    g:exported = expo.
                  exported
  END
  writefile(line_break_after_dot, 'Ximport_lbr_after_dot.vim')
  assert_fails('source Ximport_lbr_after_dot.vim', 'E1074:', '', 3)
  delete('Ximport_lbr_after_dot.vim')

  var import_star_as_lines =<< trim END
    vim9script
    import './Xexport.vim' as Export
    def UseExport()
      g:exported_def = Export.exported
    enddef
    g:exported_script = Export.exported
    assert_equal(1, exists('Export.exported'))
    assert_equal(0, exists('Export.notexported'))
    UseExport()
  END
  writefile(import_star_as_lines, 'Ximport.vim')
  source Ximport.vim

  assert_equal(18, g:exported_def)
  assert_equal(18, g:exported_script)
  unlet g:exported_def
  unlet g:exported_script

  var import_star_as_lines_no_dot =<< trim END
    vim9script
    import './Xexport.vim' as Export
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
    import './Xexport.vim' as Export
    def Func()
      var imported = Export . exported
    enddef
    defcompile
  END
  writefile(import_star_as_lines_dot_space, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1074:', '', 1, 'Func')

  writefile(s:export_script_lines, 'Xexport2.vim')
  var import_as_duplicated =<< trim END
    vim9script
    import './Xexport.vim' as expo
    import './Xexport2.vim' as expo
  END
  writefile(import_as_duplicated, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1073:', '', 3, 'Ximport.vim')
  delete('Xexport2.vim')

  var import_star_as_lines_script_no_dot =<< trim END
    vim9script
    import './Xexport.vim' as Export
    g:imported_script = Export exported
  END
  writefile(import_star_as_lines_script_no_dot, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1060: Expected dot after name: Export exported')

  var import_star_as_lines_script_space_after_dot =<< trim END
    vim9script
    import './Xexport.vim' as Export
    g:imported_script = Export. exported
  END
  writefile(import_star_as_lines_script_space_after_dot, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1074:')

  var import_star_as_lines_missing_name =<< trim END
    vim9script
    import './Xexport.vim' as Export
    def Func()
      var imported = Export.
    enddef
    defcompile
  END
  writefile(import_star_as_lines_missing_name, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1048:', '', 1, 'Func')

  var import_star_as_lbr_lines =<< trim END
    vim9script
    import './Xexport.vim'
        as Export
    def UseExport()
      g:exported = Export.exported
    enddef
    UseExport()
  END
  writefile(import_star_as_lbr_lines, 'Ximport.vim')
  source Ximport.vim
  assert_equal(18, g:exported)
  unlet g:exported

  # try to use something that exists but is not exported
  var import_not_exported_lines =<< trim END
    vim9script
    import './Xexport.vim' as expo
    echo expo.name
  END
  writefile(import_not_exported_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1049:', '', 3, 'Ximport.vim')

  # try to import something that is already defined
  var import_already_defined =<< trim END
    vim9script
    var exported = 'something'
    import './Xexport.vim' as exported
  END
  writefile(import_already_defined, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1054:', '', 3, 'Ximport.vim')

  # try changing an imported const
  var import_assign_to_const =<< trim END
    vim9script
    import './Xexport.vim' as expo
    def Assign()
      expo.CONST = 987
    enddef
    defcompile
  END
  writefile(import_assign_to_const, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E46:', '', 1, '_Assign')

  # try changing an imported final
  var import_assign_to_final =<< trim END
    vim9script
    import './Xexport.vim' as expo
    def Assign()
      expo.theList = [2]
    enddef
    defcompile
  END
  writefile(import_assign_to_final, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E46:', '', 1, '_Assign')

  var import_no_as_lines =<< trim END
    vim9script
    import './Xexport.vim' name
  END
  writefile(import_no_as_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E488:', '', 2, 'Ximport.vim')

  var import_invalid_string_lines =<< trim END
    vim9script
    import Xexport.vim
  END
  writefile(import_invalid_string_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E121:', '', 2, 'Ximport.vim')

  var import_wrong_name_lines =<< trim END
    vim9script
    import './XnoExport.vim'
  END
  writefile(import_wrong_name_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1053:', '', 2, 'Ximport.vim')

  var import_redefining_lines =<< trim END
    vim9script
    import './Xexport.vim' as exported
    var exported = 5
  END
  writefile(import_redefining_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1213: Redefining imported item "exported"', '', 3)

  var import_assign_wrong_type_lines =<< trim END
    vim9script
    import './Xexport.vim' as expo
    expo.exported = 'xxx'
  END
  writefile(import_assign_wrong_type_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E1012: Type mismatch; expected number but got string', '', 3)

  var import_assign_const_lines =<< trim END
    vim9script
    import './Xexport.vim' as expo
    expo.CONST = 4321
  END
  writefile(import_assign_const_lines, 'Ximport.vim')
  assert_fails('source Ximport.vim', 'E46: Cannot change read-only variable "CONST"', '', 3)

  delete('Ximport.vim')
  delete('Ximport3.vim')
  delete('Xexport.vim')

  # Check that in a Vim9 script 'cpo' is set to the Vim default.
  # Flags added or removed are also applied to the restored value.
  set cpo=abcd
  var lines =<< trim END
    vim9script
    g:cpo_in_vim9script = &cpo
    set cpo+=f
    set cpo-=c
    g:cpo_after_vim9script = &cpo
  END
  writefile(lines, 'Xvim9_script')
  source Xvim9_script
  assert_equal('fabd', &cpo)
  set cpo&vim
  assert_equal(&cpo, g:cpo_in_vim9script)
  var newcpo = substitute(&cpo, 'c', '', '') .. 'f'
  assert_equal(newcpo, g:cpo_after_vim9script)

  delete('Xvim9_script')
enddef

def Test_import_funcref()
  var lines =<< trim END
      vim9script
      export def F(): number
        return 42
      enddef
      export const G = F
  END
  writefile(lines, 'Xlib.vim')

  lines =<< trim END
      vim9script
      import './Xlib.vim' as lib
      const Foo = lib.G()
      assert_equal(42, Foo)

      def DoTest()
        const Goo = lib.G()
        assert_equal(42, Goo)
      enddef
      DoTest()
  END
  CheckScriptSuccess(lines)

  delete('Xlib.vim')
enddef

def Test_import_fails()
  writefile([], 'Xfoo.vim')
  var lines =<< trim END
      import './Xfoo.vim' as foo
      foo = 'bar'
  END
  CheckDefAndScriptFailure(lines, ['E1094:', 'E1236: Cannot use foo itself'])
  lines =<< trim END
      vim9script
      import './Xfoo.vim' as foo
      var that = foo
  END
  CheckScriptFailure(lines, 'E1060: Expected dot after name: foo')

  lines =<< trim END
      vim9script
      import './Xfoo.vim' as 9foo
  END
  CheckScriptFailure(lines, 'E1047:')
  lines =<< trim END
      vim9script
      import './Xfoo.vim' as the#foo
  END
  CheckScriptFailure(lines, 'E1047:')
  lines =<< trim END
      vim9script
      import './Xfoo.vim' as g:foo
  END
  CheckScriptFailure(lines, 'E1047:')

  delete('Xfoo.vim')

  lines =<< trim END
      vim9script
      def TheFunc()
        echo 'the func'
      enddef
      export var Ref = TheFunc
  END
  writefile([], 'Xthat.vim')

  lines =<< trim END
      import './Xthat.vim' as That
      That()
  END
  CheckDefAndScriptFailure(lines, ['E1094:', 'E1236: Cannot use That itself'])

  lines =<< trim END
      import './Xthat.vim' as one
      import './Xthat.vim' as two
  END
  CheckScriptFailure(lines, 'E1262:')

  delete('Xthat.vim')
 
  mkdir('Ximport')

  writefile(['vim9script'], 'Ximport/.vim')
  lines =<< trim END
      vim9script
      import './Ximport/.vim'
  END
  CheckScriptFailure(lines, 'E1261: Cannot import .vim without using "as"')
  lines =<< trim END
      vim9script
      import './Ximport/.vim' as vim
  END
  CheckScriptSuccess(lines)

  writefile(['vim9script'], 'Ximport/.vimrc')
  lines =<< trim END
      vim9script
      import './Ximport/.vimrc'
  END
  CheckScriptFailure(lines, 'E1257: Imported script must use "as" or end in .vim')
  lines =<< trim END
      vim9script
      import './Ximport/.vimrc' as vimrc
  END
  CheckScriptSuccess(lines)

  delete('Ximport', 'rf')
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
    import './Xexport_that.vim' as that
    assert_equal('yes', that.That())
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
    import './Xexport_ft.vim' as ft
    assert_equal('yes', ft.That)
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
      import './XsomeExport.vim' as some
      var Funcy = some.Funcx
      nnoremap <F3> :call <sid>Funcy()<cr>
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

def Test_export_fails()
  CheckScriptFailure(['export var some = 123'], 'E1042:')
  CheckScriptFailure(['vim9script', 'export var g:some'], 'E1022:')
  CheckScriptFailure(['vim9script', 'export echo 134'], 'E1043:')

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

  var buf = RunVimInTerminal('-c "import Foo from ''./XexportCmd.vim''"', {
                rows: 6, wait_for_ruler: 0})
  WaitForAssert(() => assert_match('^E1094:', term_getline(buf, 5)))

  delete('XexportCmd.vim')
  StopVimInTerminal(buf)
enddef

def Test_vim9_reload_noclear()
  var lines =<< trim END
    vim9script
    export var exported = 'thexport'

    export def TheFunc(x = 0)
    enddef
  END
  writefile(lines, 'XExportReload')
  lines =<< trim END
    vim9script noclear
    g:loadCount += 1
    var s:reloaded = 'init'
    import './XExportReload' as exp

    def Again(): string
      return 'again'
    enddef

    exp.TheFunc()

    if exists('s:loaded') | finish | endif
    var s:loaded = true

    var s:notReloaded = 'yes'
    s:reloaded = 'first'
    def g:Values(): list<string>
      return [s:reloaded, s:notReloaded, Again(), Once(), exp.exported]
    enddef

    def Once(): string
      return 'once'
    enddef
  END
  writefile(lines, 'XReloaded')
  g:loadCount = 0
  source XReloaded
  assert_equal(1, g:loadCount)
  assert_equal(['first', 'yes', 'again', 'once', 'thexport'], g:Values())
  source XReloaded
  assert_equal(2, g:loadCount)
  assert_equal(['init', 'yes', 'again', 'once', 'thexport'], g:Values())
  source XReloaded
  assert_equal(3, g:loadCount)
  assert_equal(['init', 'yes', 'again', 'once', 'thexport'], g:Values())

  delete('XReloaded')
  delete('XExportReload')
  delfunc g:Values
  unlet g:loadCount

  lines =<< trim END
      vim9script
      def Inner()
      enddef
  END
  lines->writefile('XreloadScript.vim')
  source XreloadScript.vim

  lines =<< trim END
      vim9script
      def Outer()
        def Inner()
        enddef
      enddef
      defcompile
  END
  lines->writefile('XreloadScript.vim')
  source XreloadScript.vim

  delete('XreloadScript.vim')
enddef

def Test_vim9_reload_import()
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

" if a script is reloaded with a script-local variable that changed its type, a
" compiled function using that variable must fail.
def Test_script_reload_change_type()
  var lines =<< trim END
    vim9script noclear
    var str = 'string'
    def g:GetStr(): string
      return str .. 'xxx'
    enddef
  END
  writefile(lines, 'Xreload.vim')
  source Xreload.vim
  echo g:GetStr()

  lines =<< trim END
    vim9script noclear
    var str = 1234
  END
  writefile(lines, 'Xreload.vim')
  source Xreload.vim
  assert_fails('echo g:GetStr()', 'E1150:')

  delfunc g:GetStr
  delete('Xreload.vim')
enddef

" Define CallFunc so that the test can be compiled
command CallFunc echo 'nop'

def Test_script_reload_from_function()
  var lines =<< trim END
      vim9script

      if exists('g:loaded')
        finish
      endif
      g:loaded = 1
      delcommand CallFunc
      command CallFunc Func()
      def Func()
        so XreloadFunc.vim
        g:didTheFunc = 1
      enddef
  END
  writefile(lines, 'XreloadFunc.vim')
  source XreloadFunc.vim
  CallFunc
  assert_equal(1, g:didTheFunc)

  delete('XreloadFunc.vim')
  delcommand CallFunc
  unlet g:loaded
  unlet g:didTheFunc
enddef

def s:RetSome(): string
  return 'some'
enddef

" Not exported function that is referenced needs to be accessed by the
" script-local name.
def Test_vim9_funcref()
  var sortlines =<< trim END
      vim9script
      def Compare(i1: number, i2: number): number
        return i2 - i1
      enddef

      export def FastSort(): list<number>
        return range(5)->sort(Compare)
      enddef

      export def GetString(arg: string): string
        return arg
      enddef
  END
  writefile(sortlines, 'Xsort.vim')

  var lines =<< trim END
    vim9script
    import './Xsort.vim'
    def Test()
      g:result = Xsort.FastSort()
    enddef
    Test()
  END
  writefile(lines, 'Xscript.vim')
  source Xscript.vim
  assert_equal([4, 3, 2, 1, 0], g:result)
  unlet g:result

  lines =<< trim END
    vim9script
    # using a function imported with "as"
    import './Xsort.vim' as anAlias
    assert_equal('yes', anAlias.GetString('yes'))

    # using the function from a compiled function
    def TestMore(): string
      var s = s:anAlias.GetString('foo')
      return s .. anAlias.GetString('bar')
    enddef
    assert_equal('foobar', TestMore())

    # error when using a function that isn't exported
    assert_fails('anAlias.Compare(1, 2)', 'E1049:')
  END
  writefile(lines, 'Xscript.vim')

  delete('Xsort.vim')
  delete('Xscript.vim')

  var Funcref = function('s:RetSome')
  assert_equal('some', Funcref())
enddef

" Check that when searching for "FilterFunc" it finds the import in the
" script where FastFilter() is called from, both as a string and as a direct
" function reference.
def Test_vim9_funcref_other_script()
  var filterLines =<< trim END
    vim9script
    export def FilterFunc(idx: number, val: number): bool
      return idx % 2 == 1
    enddef
    export def FastFilter(): list<number>
      return range(10)->filter('FilterFunc(v:key, v:val)')
    enddef
    export def FastFilterDirect(): list<number>
      return range(10)->filter(FilterFunc)
    enddef
  END
  writefile(filterLines, 'Xfilter.vim')

  var lines =<< trim END
    vim9script
    import './Xfilter.vim' as filter
    def Test()
      var x: list<number> = filter.FastFilter()
    enddef
    Test()
    def TestDirect()
      var x: list<number> = filter.FastFilterDirect()
    enddef
    TestDirect()
  END
  CheckScriptSuccess(lines)
  delete('Xfilter.vim')
enddef

def Test_import_absolute()
  var import_lines = [
        'vim9script',
        'import "' .. escape(getcwd(), '\') .. '/Xexport_abs.vim" as abs',
        'def UseExported()',
        '  g:imported_abs = abs.exported',
        '  abs.exported = 8888',
        '  g:imported_after = abs.exported',
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
          'g:imported_abs = abs.exported\_s*' ..
          '0 LOADSCRIPT exported-2 from .*Xexport_abs.vim\_s*' ..
          '1 STOREG g:imported_abs\_s*' ..
          'abs.exported = 8888\_s*' ..
          '2 PUSHNR 8888\_s*' ..
          '3 STORESCRIPT exported-2 in .*Xexport_abs.vim\_s*' ..
          'g:imported_after = abs.exported\_s*' ..
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
        'import "Xexport_rtp.vim" as rtp',
        'g:imported_rtp = rtp.exported',
        ]
  writefile(import_lines, 'Ximport_rtp.vim')
  mkdir('import', 'p')
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
        'import "./Xexported.vim" as expo',
        'def ImpFunc()',
        '  echo expo.ExpFunc()',
        'enddef',
        'defcompile',
        ]
  writefile(import_lines, 'Ximport.vim')

  try
    source Ximport.vim
  catch /E1001/
    # Error should be before the Xexported.vim file.
    assert_match('E1001: Variable not found: notDefined', v:exception)
    assert_match('function <SNR>\d\+_ImpFunc\[1\]..<SNR>\d\+_ExpFunc, line 1', v:throwpoint)
  endtry

  delete('Xexported.vim')
  delete('Ximport.vim')
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
    import './XexportedFunc.vim' as Func
    def Func()
      echo 'local to function'
    enddef
  END
  CheckScriptFailure(lines, 'E1236:')

  lines =<< trim END
    vim9script
    import './XexportedFunc.vim' as Func
    def Outer()
      def Func()
        echo 'local to function'
      enddef
    enddef
    defcompile
  END
  CheckScriptFailure(lines, 'E1236:')

  delete('XexportedFunc.vim')
enddef

def Test_source_vim9_from_legacy()
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

  var legacy_lines =<< trim END
    source Xvim9_script.vim

    call assert_false(exists('local'))
    call assert_false(exists('exported'))
    call assert_false(exists('s:exported'))
    call assert_equal('global', global)
    call assert_equal('global', g:global)

    "" imported variable becomes script-local
    "import exported from './Xvim9_script.vim'
    "call assert_equal('exported', s:exported)
    "call assert_false(exists('exported'))

    "" imported function becomes script-local
    "import GetText from './Xvim9_script.vim'
    "call assert_equal('text', s:GetText())
    "call assert_false(exists('*GetText'))
  END
  writefile(legacy_lines, 'Xlegacy_script.vim')

  source Xlegacy_script.vim
  assert_equal('global', g:global)
  unlet g:global

  delete('Xlegacy_script.vim')
  delete('Xvim9_script.vim')
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
    import './Xexport.vim' as exp
    echo exp.That
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

def Test_import_gone_when_sourced_twice()
  var exportlines =<< trim END
      vim9script
      if exists('g:guard')
        finish
      endif
      g:guard = 1
      export var name = 'someName'
  END
  writefile(exportlines, 'XexportScript.vim')

  var lines =<< trim END
      vim9script
      import './XexportScript.vim' as expo
      def g:GetName(): string
        return expo.name
      enddef
  END
  writefile(lines, 'XscriptImport.vim')
  so XscriptImport.vim
  assert_equal('someName', g:GetName())

  so XexportScript.vim
  assert_fails('call g:GetName()', 'E1149:')

  delfunc g:GetName
  delete('XexportScript.vim')
  delete('XscriptImport.vim')
  unlet g:guard
enddef


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
