" Tests the Opaque data type

import './util/vim9.vim' as v9

func TearDown()
  " Run garbage collection after every test
  call test_garbagecollect_now()
endfunc

" Opaque declaration
func Test_opaque_declaration()
  let lines =<< trim END
    var op = test_opaque(2)

    assert_equal(test_opaque(2), op)
    assert_equal("opaque<TestOpaque>", typename(op))
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    var op: opaque<TestOpaque> = test_opaque(100)

    assert_equal(test_opaque(100), op)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    var op: any = test_opaque(30)

    assert_equal(test_opaque(30), op)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  " Opaque declaration separated by '|'
  let lines =<< trim END
    VAR o1 = test_opaque(1) | VAR o2 = test_opaque(2)
    call assert_equal(test_opaque(1), o1)
    call assert_equal(test_opaque(2), o2)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Opaque declaration error
func Test_opaque_declaration_error()
  let lines =<< trim END
    var r: opaque<> = test_opaque(1)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1008: Missing <type> after > = test_opaque(1)")

  let lines =<< trim END
    var r: opaque<Unknown> = test_opaque(1)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1577: Opaque type Unknown does not exist")

  let lines =<< trim END
    var r: opaque <TestOpaque> = test_opaque(1)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1068: No white space allowed before '<'")

  let lines =<< trim END
    var t: opque<TestOpaque> = test_opaque(1)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, 'E1010: Type not recognized: opque<TestOpaque>')

  let lines =<< trim END
    var t: opaque<TestOpaque> = [1, 2]
  END
  call v9.CheckSourceDefAndScriptFailure(lines, 'E1012: Type mismatch; expected opaque<TestOpaque> but got list<number>')

  let lines =<< trim END
    var t: opaque = test_opaque(100)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, 'E1008: Missing <type> after opaque')
endfunc

" Test opaque properties
func Test_opaque_properties()
  let lines =<< trim END
    var r: opaque<TestOpaque> = test_opaque(1)

    assert_equal(1, r.val)
    assert_equal("TestOpaque", r.type)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    let r = test_opaque(1)

    call assert_equal(1, r.val)
    call assert_equal("TestOpaque", r.type)
  END
  call v9.CheckSourceLegacySuccess(lines)

  " Test access through lists and dictionaries
  let lines =<< trim END
    VAR d = {}
    VAR a = 0
    VAR b = ""

    LET d['A'] = [test_opaque(2)]
    LET a = d['A'][0].val
    LET b = d['A'][0].type
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Opaque properties error
func Test_opaque_properties_error()
  let lines =<< trim END
    var r: opaque<TestOpaque> = test_null_opaque()

    r.val
  END
  call v9.CheckSourceDefExecFailure(lines, 'E1575: Using a null opaque')

  let lines =<< trim END
     VAR r: any = test_opaque(1)

    call assert_equal(1, r.unknown)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, "E1574: Property 'unknown' of Opaque of type TestOpaque does not exist")
endfunc

" vim: shiftwidth=2 sts=2 expandtab
