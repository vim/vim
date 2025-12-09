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

  " Test opaque defined without declaring opaque type
  let lines =<< trim END
    var op: opaque = test_opaque(30)

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
endfunc

" vim: shiftwidth=2 sts=2 expandtab
