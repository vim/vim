" Tests for the Tuple types

import './vim9.vim' as v9

func TearDown()
  " Run garbage collection after every test
  call test_garbagecollect_now()
endfunc

" Tuple declaration
func Test_tuple_declaration()
  let lines =<< trim END
    var Fn = function('min')
    var t = (1, 'a', true, 3.1, 0z10, ['x'], {'a': []}, Fn)
    assert_equal((1, 'a', true, 3.1, 0z10, ['x'], {'a': []}, Fn), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  " Multiline tuple declaration
  let lines =<< trim END
    var t = (
        'a',
        'b',
      )
    assert_equal(('a', 'b'), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  " Tuple declaration with comments
  let lines =<< trim END
    var t = (   # xxx
        # xxx
        'a',  # xxx
        # xxx
        'b',  # xxx
      )  # xxx
    assert_equal(('a', 'b'), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  " Tuple declaration separated by '|'
  let lines =<< trim END
    VAR t1 = ('a', 'b') | VAR t2 = ('c', 'd')
    call assert_equal(('a', 'b'), t1)
    call assert_equal(('c', 'd'), t2)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  " Space after and before parens
  let lines =<< trim END
    var t = ( 1, 2 )
    assert_equal((1, 2), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)
endfunc

" Tuple declaration error
func Test_tuple_declaration_error()
  let lines =<< trim END
    var t: tuple<> = ('a', 'b')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1008: Missing <type> after > = ('a', 'b')")

  let lines =<< trim END
    var t: tuple = ('a', 'b')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1008: Missing <type> after tuple")

  let lines =<< trim END
    var t: tuple<number> = ('a','b')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1069: White space required after ',': ,'b')")

  let lines =<< trim END
    var t: tuple<number> = ('a', 'b','c')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1069: White space required after ',': ,'c')")

  let lines =<< trim END
    var t: tuple <number> = ()
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1068: No white space allowed before '<'")

  let lines =<< trim END
    var t: tuple<number,string>
  END
  call v9.CheckSourceDefAndScriptFailure(lines, "E1069: White space required after ','")

  let lines =<< trim END
    var t: tuple<number , string>
  END
  call v9.CheckSourceDefFailure(lines, "E1068: No white space allowed before ','")

  let lines =<< trim END
    var t = ('a', 'b' , 'c')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, [
        \ "E1068: No white space allowed before ','",
        \ "E1068: No white space allowed before ','"])

  let lines =<< trim END
    VAR t = ('a', 'b' 'c')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ "E1527: Missing comma in Tuple: 'c')",
        \ "E1527: Missing comma in Tuple: 'c')",
        \ "E1527: Missing comma in Tuple: 'c')"])

  let lines =<< trim END
    VAR t = ('a', 'b',
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ "E1526: Missing end of Tuple ')'",
        \ "E1526: Missing end of Tuple ')'",
        \ "E1526: Missing end of Tuple ')'"])

  let lines =<< trim END
    var t: tuple<number, ...> = (1, 2, 3)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1010: Type not recognized: ',
        \ 'E1010: Type not recognized: '])

  let lines =<< trim END
    var t: tuple<number, ...number> = (1, 2, 3)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1539: Variadic tuple must end with a list type: number',
        \ 'E1539: Variadic tuple must end with a list type: number'])

  " Invalid expression in the tuple
  let lines =<< trim END
    def Foo()
      var t = (1, 1*2, 2)
    enddef
    defcompile
  END
  call v9.CheckSourceDefFailure(lines, 'E1004: White space required before and after ''*'' at "*2, 2)"')

  let lines =<< trim END
    VAR t = ('a', , 'b',)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E15: Invalid expression: ", ''b'',)"',
        \ "E1068: No white space allowed before ',': , 'b',)",
        \ 'E15: Invalid expression: ", ''b'',)"'])

  let lines =<< trim END
    VAR t = ('a', 'b', ,)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E15: Invalid expression: ",)"',
        \ "E1068: No white space allowed before ',': ,)",
        \ 'E15: Invalid expression: ",)"'])

  let lines =<< trim END
    VAR t = (, 'a', 'b')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E15: Invalid expression: ", ''a'', ''b'')"',
        \ "E1015: Name expected: , 'a', 'b')",
        \ 'E15: Invalid expression: ", ''a'', ''b'')"'])

  let lines =<< trim END
    var t: tupel<number> = (1,)
  END
  call v9.CheckSourceDefAndScriptFailure(lines, 'E1010: Type not recognized: tupel<number>')

  let lines =<< trim END
    var t: tuple<number> = [1, 2]
  END
  call v9.CheckSourceDefAndScriptFailure(lines, 'E1012: Type mismatch; expected tuple<number> but got list<number>')
endfunc

" Test for indexing a tuple
func Test_tuple_indexing()
  let lines =<< trim END
    VAR t = ('a', 'b', 'c')
    call assert_equal(['a', 'b', 'c'], [t[0], t[1], t[2]])
    call assert_equal(['c', 'b', 'a'], [t[-1], t[-2], t[-3]])
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  " Indexing a tuple passed as a function argument
  let lines =<< trim END
    vim9script
    def Fn(t: any)
      call assert_equal(['a', 'b', 'c'], [t[0], t[1], t[2]])
      call assert_equal(['c', 'b', 'a'], [t[-1], t[-2], t[-3]])
    enddef
    Fn(('a', 'b', 'c'))
  END
  call v9.CheckSourceSuccess(lines)

  let lines =<< trim END
    var t: tuple<...list<number>> = (10, 20)
    var x: number = t[0]
    assert_equal(10, x)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    var t: tuple<...list<list<number>>> = ([1, 2], [3, 4])
    t[0][1] = 5
    assert_equal(([1, 5], [3, 4]), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    var t: tuple<list<number>> = ([2, 4],)
    t[0][1] = 6
    assert_equal(([2, 6],), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)
endfunc

" Indexing a tuple in a Dict
func Test_tuple_in_a_dict_index()
  let lines =<< trim END
    vim9script
    def Fn()
      var d = {a: (1, 2)}
      var x = d.a[0]
      assert_equal('number', typename(x))
    enddef
    Fn()
  END
  call v9.CheckSourceSuccess(lines)
endfunc

func Test_tuple_index_error()
  let lines =<< trim END
    echo ('a', 'b', 'c')[3]
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1519: Tuple index out of range: 3',
        \ 'E1519: Tuple index out of range: 3',
        \ 'E1519: Tuple index out of range: 3'])

  let lines =<< trim END
    echo ('a', 'b', 'c')[-4]
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1519: Tuple index out of range: -4',
        \ 'E1519: Tuple index out of range: -4',
        \ 'E1519: Tuple index out of range: -4'])

  let lines =<< trim END
    vim9script
    def Fn(t: any)
      echo t[3]
    enddef
    Fn(('a', 'b', 'c'))
  END
  call v9.CheckSourceFailure(lines, 'E1519: Tuple index out of range: 3')

  let lines =<< trim END
    vim9script
    def Fn(t: any)
      echo t[-4]
    enddef
    Fn(('a', 'b', 'c'))
  END
  call v9.CheckSourceFailure(lines, 'E1519: Tuple index out of range: -4')

  let lines =<< trim END
    vim9script
    def Fn(t: any)
      var x = t[0]
    enddef
    Fn(())
  END
  call v9.CheckSourceFailure(lines, 'E1519: Tuple index out of range: 0')

  " Index a null tuple
  let lines =<< trim END
    VAR t = test_null_tuple()
    LET t[0][0] = 10
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1519: Tuple index out of range: 0',
        \ 'E1519: Tuple index out of range: 0',
        \ 'E1519: Tuple index out of range: 0'])

  let lines =<< trim END
    var x = null_tuple
    x[0][0] = 10
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1519: Tuple index out of range: 0',
        \ 'E1519: Tuple index out of range: 0'])

  " Use a float as the index
  let lines =<< trim END
    VAR t = (1, 2)
    VAR x = t[0.1]
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E805: Using a Float as a Number',
        \ 'E1012: Type mismatch; expected number but got float',
        \ 'E805: Using a Float as a Number'])
endfunc

" Test for slicing a tuple
func Test_tuple_slice()
  let lines =<< trim END
    VAR t = (1, 3, 5, 7, 9)
    call assert_equal((3, 5), t[1 : 2])
    call assert_equal((9,), t[4 : 4])
    call assert_equal((7, 9), t[3 : 6])
    call assert_equal((1, 3, 5), t[: 2])
    call assert_equal((5, 7, 9), t[2 :])
    call assert_equal((1, 3, 5, 7, 9), t[:])
    call assert_equal((), test_null_tuple()[:])
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    call assert_equal(('b', 'c'), ('a', 'b', 'c')[1 : 5])
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for concatenating tuples
func Test_tuple_concatenate()
  let lines =<< trim END
    VAR t1 = ('a', 'b') + ('c', 'd')
    call assert_equal(('a', 'b', 'c', 'd'), t1)

    VAR t2 = ('a',) + ('b',)
    call assert_equal(('a', 'b'), t2)

    VAR t3 = ('a',) + ()
    call assert_equal(('a',), t3)

    VAR t4 = () + ('b',)
    call assert_equal(('b',), t4)

    VAR t5 = ('a', 'b') + test_null_tuple()
    call assert_equal(('a', 'b'), t5)
    call assert_equal('tuple<string, string>', typename(t5))

    VAR t6 = test_null_tuple() + ('c', 'd')
    call assert_equal(('c', 'd'), t6)
    call assert_equal('tuple<string, string>', typename(t6))

    VAR t7 = ('a', 'b') + (8, 9)
    call assert_equal(('a', 'b', 8, 9), t7)
    call assert_equal('tuple<string, string, number, number>', typename(t7))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    var t1: tuple<...list<tuple<number, number>>> = ()
    var t2: tuple<...list<tuple<number, number>>> = ()
    var t: tuple<...list<tuple<number, number>>> = t1 + t2
    assert_equal((), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    var t: tuple<...list<number>> = (1, 2) + ('a', 'b')
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1012: Type mismatch; expected tuple<...list<number>> but got tuple<number, number, string, string>',
        \ 'E1012: Type mismatch; expected tuple<...list<number>> but got tuple<number, number, string, string>'])

  let lines =<< trim END
    var a: tuple<...list<number>> = (1, 2)
    var b: tuple<...list<string>> = ('a', 'b')
    var t = a + b
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1540: Cannot use a variadic tuple in concatenation',
        \ 'E1540: Cannot use a variadic tuple in concatenation'])

  let lines =<< trim END
    var a: tuple<...list<number>> = (1, 2)
    var b: tuple<string, string> = ('a', 'b')
    var t = a + b
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1540: Cannot use a variadic tuple in concatenation',
        \ 'E1540: Cannot use a variadic tuple in concatenation'])

  let lines =<< trim END
    var a: tuple<number, ...list<string>> = (1, 'a', 'b')
    var b: tuple<number, ...list<string>> = (2, 'c', 'd')
    var t = a + b
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1540: Cannot use a variadic tuple in concatenation',
        \ 'E1540: Cannot use a variadic tuple in concatenation'])

  let lines =<< trim END
    var a: tuple<number, ...list<string>> = (1, 'a', 'b')
    var b: tuple<...list<string>> = ('c', 'd')
    var t = a + b
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1540: Cannot use a variadic tuple in concatenation',
        \ 'E1540: Cannot use a variadic tuple in concatenation'])

  let lines =<< trim END
    var a: tuple<...list<string>> = ('a', 'b')
    var b: tuple<number, ...list<string>> = (2, 'c', 'd')
    var t = a + b
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1540: Cannot use a variadic tuple in concatenation',
        \ 'E1540: Cannot use a variadic tuple in concatenation'])

  let lines =<< trim END
    var t1: tuple<...list<tuple<number, number>>> = ()
    var t2: tuple<...list<tuple<number, string>>> = ()
    var t = t1 + t2
  END
  call v9.CheckSourceDefExecAndScriptFailure(lines, [
        \ 'E1540: Cannot use a variadic tuple in concatenation',
        \ 'E1540: Cannot use a variadic tuple in concatenation'])

  " Make sure the correct line number is used in the error message
  let lines =<< trim END
    vim9script
    var t1: tuple<...list<tuple<number, number>>> = ()
    var t2: tuple<...list<tuple<number, string>>> = ()
    var t = t1 + t2

  END
  call v9.CheckSourceFailure(lines, 'E1540: Cannot use a variadic tuple in concatenation', 4)

  let lines =<< trim END
    vim9script

    def Fn()
      var t1: tuple<...list<tuple<number, number>>> = ()
      var t2: tuple<...list<tuple<number, string>>> = ()
      var t = t1 + t2

    enddef
    Fn()
  END
  call v9.CheckSourceFailure(lines, 'E1540: Cannot use a variadic tuple in concatenation', 3)

  " One or both the operands are variadic tuples
  let lines =<< trim END
    var a1: tuple<number, number> = (1, 2)
    var b1: tuple<...list<string>> = ('a', 'b')
    var t1 = a1 + b1
    assert_equal((1, 2, 'a', 'b'), t1)

    var a2: tuple<string, string> = ('a', 'b')
    var b2: tuple<number, ...list<string>> = (1, 'c', 'd')
    var t2 = a2 + b2
    assert_equal(('a', 'b', 1, 'c', 'd'), t2)

    var a3: tuple<...list<string>> = ('a', 'b')
    var b3: tuple<...list<string>> = ('c', 'd')
    var t3 = a3 + b3
    assert_equal(('a', 'b', 'c', 'd'), t3)

    var a4: tuple<...list<number>> = (1, 2)
    var t4 = a4 + ()
    assert_equal((1, 2), t4)

    var b5: tuple<...list<number>> = (1, 2)
    var t5 = () + b5
    assert_equal((1, 2), t5)

    var a6: tuple<...list<number>> = (1, 2)
    var t6 = a6 + null_tuple
    assert_equal((1, 2), t6)

    var b7: tuple<...list<string>> = ('a', 'b')
    var t7 = null_tuple + b7
    assert_equal(('a', 'b'), t7)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    VAR t = test_null_tuple() + test_null_tuple()
    call assert_equal(test_null_tuple(), t)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    vim9script
    def Fn(x: any, y: any): any
      return x + y
    enddef
    assert_equal((1, 2), Fn((1,), (2,)))
    assert_equal((1, 'a'), Fn((1,), ('a',)))
    assert_equal((1,), Fn((1,), null_tuple))
    assert_equal(('a',), Fn(null_tuple, ('a',)))
    assert_equal((), Fn(null_tuple, null_tuple))
  END
  call v9.CheckSourceScriptSuccess(lines)

  " Test for concatenating to lists containing tuples
  let lines =<< trim END
    var x = [test_null_tuple()] + [test_null_tuple()]
    assert_equal([(), ()], x)
    var y = [()] + [()]
    assert_equal([(), ()], y)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)
endfunc

" Test for comparing tuples
func Test_tuple_compare()
  let lines =<< trim END
    call assert_false((1, 2) == (1, 3))
    call assert_true((1, 2) == (1, 2))
    call assert_true((1,) == (1,))
    call assert_true(() == ())
    call assert_false((1, 2) == (1, 2, 3))
    call assert_false((1, 2) == test_null_tuple())
    VAR t1 = (1, 2)
    VAR t2 = t1
    call assert_true(t1 == t2)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    echo (1.0, ) == 1.0
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1517: Can only compare Tuple with Tuple',
        \ 'E1072: Cannot compare tuple with float',
        \ 'E1072: Cannot compare tuple with float'])

  let lines =<< trim END
    echo 1.0 == (1.0,)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1517: Can only compare Tuple with Tuple',
        \ 'E1072: Cannot compare float with tuple',
        \ 'E1072: Cannot compare float with tuple'])

  let lines =<< trim END
    echo (1, 2) =~ []
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E691: Can only compare List with List',
        \ 'E1072: Cannot compare tuple with list',
        \ 'E1072: Cannot compare tuple with list'])

  let lines =<< trim END
    echo (1, 2) =~ (1, 2)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1518: Invalid operation for Tuple',
        \ 'E1518: Invalid operation for Tuple',
        \ 'E1518: Invalid operation for Tuple'])
endfunc

" Test for assigning multiple items from a tuple
func Test_multi_assign_from_tuple()
  let lines =<< trim END
    VAR [v1, v2] = ('a', 'b')
    call assert_equal(['a', 'b'], [v1, v2])

    VAR [v3] = ('c',)
    call assert_equal('c', v3)

    VAR [v4; v5] = ('a', 'b', 'c')
    call assert_equal('a', v4)
    call assert_equal(('b', 'c'), v5)

    VAR [v6; v7] = ('a',)
    call assert_equal('a', v6)
    call assert_equal((), v7)

    VAR sum = 0
    for [v8, v9] in ((2, 2), (2, 3))
      LET sum += v8 * v9
    endfor
    call assert_equal(10, sum)

    #" for: rest of the items in a List
    LET sum = 0
    for [v10; v11] in ((2, 1, 2, 5), (2, 1, 2, 10))
      LET sum += v10 * max(v11)
    endfor
    call assert_equal(30, sum)

    #" for: one item in the list
    LET sum = 0
    for [v12; v13] in ((2, 6), (2, 7))
      LET sum += v12 * max(v13)
    endfor
    call assert_equal(26, sum)

    #" for: zero items in the list
    LET sum = 0
    for [v14; v15] in ((4,), (5,))
      LET sum += v14 + max(v15)
    endfor
    call assert_equal(9, sum)

    #" A null tuple should be treated like an empty tuple
    for [v16, v17] in test_null_tuple()
    endfor
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    var t: tuple<...list<number>> = (4, 8)
    var [x: number, y: number] = t
    assert_equal([4, 8], [x, y])
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  " Test a mix lists and tuples with "any" type
  let lines =<< trim END
    vim9script
    def Fn(x: any): string
      var str = ''
      for [a, b] in x
        str ..= a .. b
      endfor
      return str
    enddef
    # List of lists
    assert_equal('abcd', Fn([['a', 'b'], ['c', 'd']]))
    # List of tuples
    assert_equal('abcd', Fn([('a', 'b'), ('c', 'd')]))
    # Tuple of lists
    assert_equal('abcd', Fn((['a', 'b'], ['c', 'd'])))
    # Tuple of tuples
    assert_equal('abcd', Fn((('a', 'b'), ('c', 'd'))))
  END
  call v9.CheckSourceSuccess(lines)

  let lines =<< trim END
    VAR [v1, v2] = ('a', 'b', 'c')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1537: Less targets than Tuple items',
        \ 'E1093: Expected 2 items but got 3',
        \ 'E1537: Less targets than Tuple items'])

  let lines =<< trim END
    VAR [v1, v2, v3] = ('a', 'b')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1538: More targets than Tuple items',
        \ 'E1093: Expected 3 items but got 2',
        \ 'E1538: More targets than Tuple items'])

  let lines =<< trim END
    VAR [v1; v2] = test_null_tuple()
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1536: Tuple required',
        \ 'E1093: Expected 1 items but got 0',
        \ 'E1536: Tuple required'])

  let lines =<< trim END
    for [v1, v2] in (('a', 'b', 'c'),)
    endfor
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1537: Less targets than Tuple items',
        \ 'E1537: Less targets than Tuple items',
        \ 'E1537: Less targets than Tuple items'])

  let lines =<< trim END
    for [v1, v2] in (('a',),)
    endfor
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1538: More targets than Tuple items',
        \ 'E1538: More targets than Tuple items',
        \ 'E1538: More targets than Tuple items'])

  let lines =<< trim END
    for [v1, v2] in (test_null_tuple(),)
    endfor
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1536: Tuple required',
        \ 'E1538: More targets than Tuple items',
        \ 'E1536: Tuple required'])

  let lines =<< trim END
    for [v1; v2] in (test_null_tuple(),)
    endfor
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1536: Tuple required',
        \ 'E1538: More targets than Tuple items',
        \ 'E1536: Tuple required'])

  " List assignment errors using a function tuple argument
  let lines =<< trim END
    vim9script
    def Fn(x: tuple<...list<number>>)
      var [a, b] = x
    enddef
    Fn((1, 2, 3))
  END
  call v9.CheckSourceFailure(lines, 'E1093: Expected 2 items but got 3')

  let lines =<< trim END
    vim9script
    def Fn(x: tuple<number>)
      var [a, b] = x
    enddef
    Fn((1,))
  END
  call v9.CheckSourceFailure(lines, 'E1093: Expected 2 items but got 1')

  let lines =<< trim END
    vim9script
    def Fn(x: tuple<number>)
      var [a, b] = x
    enddef
    Fn(null_tuple)
  END
  call v9.CheckSourceFailure(lines, 'E1093: Expected 2 items but got 0')
endfunc

" Test for performing an arithmetic operation on multiple variables using
" items from a tuple
func Test_multi_arithmetic_op_from_tuple()
  let lines =<< trim END
    VAR x = 10
    VAR y = 10
    LET [x, y] += (2, 4)
    call assert_equal([12, 14], [x, y])
    LET [x, y] -= (4, 2)
    call assert_equal([8, 12], [x, y])
    LET [x, y] *= (2, 3)
    call assert_equal([16, 36], [x, y])
    LET [x, y] /= (4, 2)
    call assert_equal([4, 18], [x, y])
    LET [x, y] %= (3, 5)
    call assert_equal([1, 3], [x, y])
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  " The "." operator is supported only in Vim script
  let lines =<< trim END
    let x = 'a'
    let y = 'b'
    let [x, y] .= ('a', 'b')
    call assert_equal(['aa', 'bb'], [x, y])
  END
  call v9.CheckSourceSuccess(lines)

  let lines =<< trim END
    VAR x = 'a'
    VAR y = 'b'
    LET [x, y] ..= ('a', 'b')
    call assert_equal(('aa', 'bb'), (x, y))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for using a tuple in a for statement
func Test_tuple_for()
  let lines =<< trim END
    VAR sum = 0
    for v1 in (1, 3, 5)
      LET sum += v1
    endfor
    call assert_equal(9, sum)

    LET sum = 0
    for v2 in ()
      LET sum += v2
    endfor
    call assert_equal(0, sum)

    LET sum = 0
    for v2 in test_null_tuple()
      LET sum += v2
    endfor
    call assert_equal(0, sum)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  " ignoring the for loop assignment using '_'
  let lines =<< trim END
    vim9script
    var count = 0
    for _ in (1, 2, 3)
      count += 1
    endfor
    assert_equal(3, count)
  END
  call v9.CheckSourceSuccess(lines)

  let lines =<< trim END
    var sum = 0
    for v in null_tuple
      sum += v
    endfor
    assert_equal(0, sum)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    vim9script
    def Foo()
      for x in ((1, 2), (3, 4))
      endfor
    enddef
    Foo()
  END
  call v9.CheckSourceSuccess(lines)

  " Test for assigning multiple items from a tuple in a for loop
  let lines =<< trim END
    vim9script
    def Fn()
      for [x, y] in ([1, 2],)
        assert_equal([1, 2], [x, y])
      endfor
    enddef
    defcompile
    Fn()
  END
  call v9.CheckSourceSuccess(lines)

  " iterate over tuple<...list<number>
  let lines =<< trim END
    vim9script
    def Fn()
      var t: tuple<...list<number>> = (1, 2)
      var sum = 0
      for i: number in t
        sum += i
      endfor
      assert_equal(3, sum)
    enddef
    Fn()
  END
  call v9.CheckSourceSuccess(lines)

  " iterate over tuple<...list<list<number>>>
  let lines =<< trim END
    vim9script
    def Fn()
      var t: tuple<...list<list<number>>> = ([1, 2], [3, 4])
      var sum = 0
      for [x: number, y: number] in t
        sum += x + y
      endfor
      assert_equal(10, sum)
    enddef
    Fn()
  END
  call v9.CheckSourceSuccess(lines)

  " iterate over tuple<...list<tuple<...list<number>>>>
  let lines =<< trim END
    vim9script
    def Fn()
      var t: tuple<...list<tuple<...list<number>>>> = ((1, 2), (3, 4))
      var sum = 0
      for [x: number, y: number] in t
        sum += x + y
      endfor
      assert_equal(10, sum)
    enddef
    Fn()
  END
  call v9.CheckSourceSuccess(lines)

  " iterate over tuple<...list<list<number>>>
  let lines =<< trim END
    vim9script
    def Fn()
      var t: tuple<...list<list<number>>> = ([1, 2], [3, 4])
      var sum = 0
      for [x: number, y: number] in t
        sum += x + y
      endfor
      assert_equal(10, sum)
    enddef
    Fn()
  END
  call v9.CheckSourceSuccess(lines)

  " iterate over a tuple<...list<any>>
  let lines =<< trim END
    vim9script
    def Fn()
      var t: tuple<...list<any>> = (1, 'x', true, [], {}, ())
      var str = ''
      for v in t
        str ..= string(v)
      endfor
      assert_equal("1'x'true[]{}()", str)
    enddef
    Fn()
  END
  call v9.CheckSourceSuccess(lines)

  " use multiple variable assignment syntax with a tuple<...list<number>>
  let lines =<< trim END
    vim9script
    def Fn()
      var t: tuple<...list<number>> = (1, 2, 3)
      for [i] in t
      endfor
    enddef
    Fn()
  END
  call v9.CheckSourceFailure(lines, 'E1140: :for argument must be a sequence of lists or tuples', 2)
endfunc

" Test for checking the tuple type in assignment and return value
func Test_tuple_type_check()
  let lines =<< trim END
    var t: tuple<...list<number>> = ('a', 'b')
  END
  call v9.CheckSourceDefFailure(lines, 'E1012: Type mismatch; expected tuple<...list<number>> but got tuple<string, string>', 1)

  let lines =<< trim END
    var t1: tuple<...list<string>> = ('a', 'b')
    assert_equal(('a', 'b'), t1)
    var t2 = (1, 2)
    assert_equal((1, 2), t2)
    var t = null_tuple
    assert_equal(null_tuple, t)
    t = test_null_tuple()
    assert_equal(test_null_tuple(), t)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    var t = ('a', 'b')
    t = (1, 2)
  END
  call v9.CheckSourceDefFailure(lines, 'E1012: Type mismatch; expected tuple<string, string> but got tuple<number, number>', 2)

  let lines =<< trim END
    var t: tuple<number> = []
  END
  call v9.CheckSourceDefFailure(lines, 'E1012: Type mismatch; expected tuple<number> but got list<any>', 1)

  let lines =<< trim END
    var t: tuple<number> = {}
  END
  call v9.CheckSourceDefFailure(lines, 'E1012: Type mismatch; expected tuple<number> but got dict<any>', 1)

  let lines =<< trim END
    var l: list<number> = (1, 2)
  END
  call v9.CheckSourceDefFailure(lines, 'E1012: Type mismatch; expected list<number> but got tuple<number, number>', 1)

  let lines =<< trim END
    vim9script
    def Fn(): tuple<...list<tuple<...list<string>>>>
      return ((1, 2), (3, 4))
    enddef
    defcompile
  END
  call v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected tuple<...list<tuple<...list<string>>>> but got tuple<tuple<number, number>, tuple<number, number>>', 1)

  let lines =<< trim END
    var t: tuple<number> = ()
  END
  call v9.CheckSourceDefSuccess(lines)

  let lines =<< trim END
    vim9script
    def Fn(): tuple<tuple<string>>
      return ()
    enddef
    defcompile
  END
  call v9.CheckSourceSuccess(lines)

  let lines =<< trim END
    vim9script
    def Fn(t: tuple<...list<number>>)
    enddef
    Fn(('a', 'b'))
  END
  call v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected tuple<...list<number>> but got tuple<string, string>')

  let lines =<< trim END
    var t: any = (1, 2)
    t = ('a', 'b')
  END
  call v9.CheckSourceDefSuccess(lines)

  let lines =<< trim END
    var t: tuple<...list<any>> = (1, 2)
    t = ('a', 'b')
  END
  call v9.CheckSourceDefSuccess(lines)

  let lines =<< trim END
    var nll: tuple<list<number>> = ([1, 2],)
    nll->copy()[0]->extend(['x'])
  END
  call v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1013: Argument 2: type mismatch, expected list<number> but got list<string>',
        \ 'E1013: Argument 2: type mismatch, expected list<number> but got list<string> in extend()'])

  let lines =<< trim END
    vim9script
    def Fn(y: tuple<number, ...list<bool>>)
      var x: tuple<number, ...list<string>>
      x = y
    enddef

    var t: tuple<number, ...list<bool>> = (1, true, false)
    Fn(t)
  END
  call v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected tuple<number, ...list<string>> but got tuple<number, ...list<bool>>')
endfunc

" Test for setting the type of a script variable to tuple
func Test_tuple_scriptvar_type()
  " Uninitialized script variable should retain the type
  let lines =<< trim END
    vim9script
    var foobar: tuple<list<string>>
    def Foo()
      var x = foobar
      assert_equal('tuple<list<string>>', typename(x))
    enddef
    Foo()
  END
  call v9.CheckSourceScriptSuccess(lines)

  " Initialized script variable should retain the type
  let lines =<< trim END
    vim9script
    var foobar: tuple<...list<string>> = ('a', 'b')
    def Foo()
      var x = foobar
      assert_equal('tuple<...list<string>>', typename(x))
    enddef
    Foo()
  END
  call v9.CheckSourceScriptSuccess(lines)
endfunc

" Test for modifying a tuple
func Test_tuple_modify()
  let lines =<< trim END
    var t = (1, 2)
    t[0] = 3
  END
  call v9.CheckSourceDefAndScriptFailure(lines, ['E1532: Cannot modify a tuple', 'E1532: Cannot modify a tuple'])
endfunc

def Test_using_null_tuple()
  var lines =<< trim END
    var x = null_tuple
    assert_true(x is null_tuple)
    var y = copy(x)
    assert_true(y is null_tuple)
    call assert_true((1, 2) != null_tuple)
    call assert_true(null_tuple != (1, 2))
    assert_equal(0, count(null_tuple, 'xx'))
    var z = deepcopy(x)
    assert_true(z is null_tuple)
    assert_equal(1, empty(x))
    assert_equal('xx', get(x, 0, 'xx'))
    assert_equal(-1, index(null_tuple, 10))
    assert_equal(-1, indexof(null_tuple, 'v:val == 2'))
    assert_equal('', join(null_tuple))
    assert_equal(0, len(x))
    assert_equal(0, min(null_tuple))
    assert_equal(0, max(null_tuple))
    assert_equal((), repeat(null_tuple, 3))
    assert_equal((), reverse(null_tuple))
    assert_equal((), slice(null_tuple, 0, 0))
    assert_equal('()', string(x))
    assert_equal('tuple<any>', typename(x))
    assert_equal(17, type(x))
  END
  v9.CheckSourceDefAndScriptSuccess(lines)

  lines =<< trim END
    # An uninitialized tuple is not equal to null
    var t1: tuple<any>
    assert_true(t1 != null)

    # An empty tuple is equal to null_tuple but not equal to null
    var t2: tuple<any> = ()
    assert_true(t2 == null_tuple)
    assert_true(t2 != null)

    # null_tuple is equal to null
    assert_true(null_tuple == null)
  END
  v9.CheckSourceDefAndScriptSuccess(lines)

  lines =<< trim END
    var x = null_tupel
  END
  v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1001: Variable not found: null_tupel',
        \ 'E121: Undefined variable: null_tupel'])
enddef

" Test for modifying a mutable item in a tuple
func Test_tuple_modify_mutable_item()
  let lines =<< trim END
    VAR t = ('a', ['b', 'c'], {'a': 10, 'b': 20})
    LET t[1][1] = 'x'
    LET t[2].a = 30
    call assert_equal(('a', ['b', 'x'], {'a': 30, 'b': 20}), t)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    VAR t = ('a', (['b'], 'c'))
    LET t[1][0][0] = 'x'
    call assert_equal(('a', (['x'], 'c')), t)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  " Use a negative index
  let lines =<< trim END
    VAR t = ([1, 2], [3])
    LET t[-2][-2] = 5
    call assert_equal(([5, 2], [3]), t)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    VAR t = ('a', ('b', 'c'))
    LET t[1][0] = 'x'
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple'])

  let lines =<< trim END
    VAR t = ['a', ('b', 'c')]
    LET t[1][0] = 'x'
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple'])

  let lines =<< trim END
    VAR t = {'a': ('b', 'c')}
    LET t['a'][0] = 'x'
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple'])

  let lines =<< trim END
    VAR t = {'a': ['b', ('c',)]}
    LET t['a'][1][0] = 'x'
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple',
        \ 'E1532: Cannot modify a tuple'])

  let lines =<< trim END
    let t = ('a', 'b', 'c', 'd')
    let t[1 : 2] = ('x', 'y')
  END
  call v9.CheckSourceFailure(lines, 'E1533: Cannot slice a tuple')

  let lines =<< trim END
    var t: tuple<...list<string>> = ('a', 'b', 'c', 'd')
    t[1 : 2] = ('x', 'y')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1533: Cannot slice a tuple',
        \ 'E1533: Cannot slice a tuple'])

  let lines =<< trim END
    var t: tuple<...list<string>> = ('a', 'b', 'c', 'd')
    t[ : 2] = ('x', 'y')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1533: Cannot slice a tuple',
        \ 'E1533: Cannot slice a tuple'])

  let lines =<< trim END
    let t = ('a', 'b', 'c', 'd')
    let t[ : ] = ('x', 'y')
  END
  call v9.CheckSourceFailure(lines, 'E1533: Cannot slice a tuple')

  let lines =<< trim END
    var t: tuple<...list<string>> = ('a', 'b', 'c', 'd')
    t[ : ] = ('x', 'y')
  END
  call v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1533: Cannot slice a tuple',
        \ 'E1533: Cannot slice a tuple'])

  let lines =<< trim END
    VAR t = ('abc',)
    LET t[0][1] = 'x'
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ "E689: Index not allowed after a string: t[0][1] = 'x'",
        \ 'E1148: Cannot index a string',
        \ "E689: Index not allowed after a string: t[0][1] = 'x'"])

  " Out of range indexing
  let lines =<< trim END
    VAR t = ([1, 2], [3])
    LET t[2][0] = 5
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1519: Tuple index out of range: 2',
        \ 'E1519: Tuple index out of range: 2',
        \ 'E1519: Tuple index out of range: 2'])

  let lines =<< trim END
    VAR t = ([1, 2], [3])
    LET t[-3][0] = 5
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1519: Tuple index out of range: -3',
        \ 'E1519: Tuple index out of range: -3',
        \ 'E1519: Tuple index out of range: -3'])

  " Use a null tuple
  let lines =<< trim END
    VAR t = test_null_tuple()
    LET t[0][0] = 5
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1519: Tuple index out of range: 0',
        \ 'E1519: Tuple index out of range: 0',
        \ 'E1519: Tuple index out of range: 0'])
endfunc

" Test for locking and unlocking a tuple variable
func Test_tuple_lock()
  " lockvar 0
  let g:t = ([0, 1],)
  let lines =<< trim END
    lockvar 0 g:t
    LET g:t = ()
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1122: Variable is locked: g:t',
        \ 'E1122: Variable is locked: t',
        \ 'E1122: Variable is locked: g:t'])
  unlet g:t

  " Tuple is immutable.  So "lockvar 1" is not applicable to a tuple.

  " lockvar 2
  let g:t = ([0, 1],)
  let lines =<< trim END
    lockvar 2 g:t
    call add(g:t[0], 2)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E741: Value is locked: add() argument',
        \ 'E741: Value is locked: add() argument',
        \ 'E741: Value is locked: add() argument'])
  unlet g:t

  " lockvar 3
  let g:t = ([0, 1],)
  let lines =<< trim END
    lockvar 3 g:t
    LET g:t[0][0] = 10
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E741: Value is locked: g:t[0][0] = 10',
        \ 'E1119: Cannot change locked list item',
        \ 'E741: Value is locked: g:t[0][0] = 10'])
  unlet g:t

  let lines =<< trim END
    VAR t = ([0, 1],)
    lockvar 2 t
    call add(t[0], 2)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E741: Value is locked: add() argument',
        \ 'E1178: Cannot lock or unlock a local variable',
        \ 'E741: Value is locked: add() argument'])

  let lines =<< trim END
    LET g:t = ([0, 1],)
    lockvar 2 g:t
    unlockvar 2 g:t
    call add(g:t[0], 3)
    call assert_equal(([0, 1, 3], ), g:t)
    unlet g:t
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    VAR t1 = (1, 2)
    const t2 = t1
    LET t2 = ()
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E741: Value is locked: t2',
        \ 'E1018: Cannot assign to a constant: t2',
        \ 'E46: Cannot change read-only variable "t2"'])
endfunc

" Test for using a class as a tuple item
func Test_tuple_use_class_item()
  let lines =<< trim END
    vim9script
    class A
    endclass
    var t = (A,)
  END
  call v9.CheckSourceScriptFailure(lines, 'E1405: Class "A" cannot be used as a value', 4)

  let lines =<< trim END
    vim9script
    class A
    endclass
    var t = ('a', A)
  END
  call v9.CheckSourceScriptFailure(lines, 'E1405: Class "A" cannot be used as a value', 4)

  let lines =<< trim END
    vim9script
    class A
    endclass
    def Fn()
      var t = (A,)
    enddef
    defcompile
  END
  call v9.CheckSourceScriptFailure(lines, 'E1405: Class "A" cannot be used as a value', 1)

  let lines =<< trim END
    vim9script
    class A
    endclass
    def Fn()
      var t = ('a', A)
    enddef
    defcompile
  END
  call v9.CheckSourceScriptFailure(lines, 'E1405: Class "A" cannot be used as a value', 1)
endfunc

" Test for using a user-defined type as a tuple item
func Test_tuple_user_defined_type_as_item()
  let lines =<< trim END
    vim9script
    type N = number
    var t = (N,)
  END
  call v9.CheckSourceScriptFailure(lines, 'E1403: Type alias "N" cannot be used as a value', 3)

  let lines =<< trim END
    vim9script
    type N = number
    var t = ('a', N)
  END
  call v9.CheckSourceScriptFailure(lines, 'E1403: Type alias "N" cannot be used as a value', 3)

  let lines =<< trim END
    vim9script
    type N = number
    def Fn()
      var t = (N,)
    enddef
    defcompile
  END
  call v9.CheckSourceScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  let lines =<< trim END
    vim9script
    type N = number
    def Fn()
      var t = ('a', N)
    enddef
    defcompile
  END
  call v9.CheckSourceScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)
endfunc

" Test for using a tuple as a function argument
func Test_tuple_func_arg()
  let lines =<< trim END
    vim9script
    def Fn(t: tuple<...list<string>>): tuple<...list<string>>
      return t[:]
    enddef
    var r1 = Fn(('a', 'b'))
    assert_equal(('a', 'b'), r1)
    var r2 = Fn(('a',))
    assert_equal(('a',), r2)
    var r3 = Fn(())
    assert_equal((), r3)
    var r4 = Fn(null_tuple)
    assert_equal((), r4)
  END
  call v9.CheckSourceScriptSuccess(lines)

  func TupleArgFunc(t)
    return a:t[:]
  endfunc
  let r = TupleArgFunc(('a', 'b'))
  call assert_equal(('a', 'b'), r)
  let r = TupleArgFunc(('a',))
  call assert_equal(('a',), r)
  let r = TupleArgFunc(())
  call assert_equal((), r)
  let r = TupleArgFunc(test_null_tuple())
  call assert_equal((), r)
  delfunc TupleArgFunc
endfunc

" Test for tuple identity
func Test_tuple_identity()
  let lines =<< trim END
    call assert_false((1, 2) is (1, 2))
    call assert_true((1, 2) isnot (1, 2))
    call assert_true((1, 2) isnot test_null_tuple())
    VAR t1 = ('abc', 'def')
    VAR t2 = t1
    call assert_true(t2 is t1)
    VAR t3 = (1, 2)
    call assert_false(t3 is t1)
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for using a compound op with a tuple
func Test_tuple_compound_op()
  let lines =<< trim END
    VAR t = (1, 2)
    LET t += (3,)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E734: Wrong variable type for +=',
        \ 'E734: Wrong variable type for +=',
        \ 'E734: Wrong variable type for +='])

  for op in ['-', '*', '/', '%']
    let lines =<< trim eval END
      VAR t = (1, 2)
      LET t {op}= (3,)
    END
    call v9.CheckSourceLegacyAndVim9Failure(lines, [
          \ $'E734: Wrong variable type for {op}=',
          \ $'E734: Wrong variable type for {op}=',
          \ $'E734: Wrong variable type for {op}='])
  endfor

  let lines =<< trim END
    VAR t = (1, 2)
    LET t ..= (3,)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E734: Wrong variable type for .=',
        \ 'E1019: Can only concatenate to string',
        \ 'E734: Wrong variable type for .='])
endfunc

" Test for using the falsy operator with tuple
func Test_tuple_falsy_op()
  let lines =<< trim END
    VAR t = test_null_tuple()
    call assert_equal('null tuple', t ?? 'null tuple')
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for tuple typecasting
def Test_tuple_typecast()
  var lines =<< trim END
    var x = <tuple<number>>('a', 'b')
  END
  v9.CheckSourceDefAndScriptFailure(lines, [
        \ 'E1012: Type mismatch; expected tuple<number> but got tuple<string, string>',
        \ 'E1012: Type mismatch; expected tuple<number> but got tuple<string, string>'])
enddef

" Test for using a tuple in string interpolation
def Test_tuple_string_interop()
  var lines =<< trim END
    VAR emptytuple = ()
    call assert_equal("a()b", $'a{emptytuple}b')
    VAR nulltuple = test_null_tuple()
    call assert_equal("a()b", $'a{nulltuple}b')

    #" Tuple interpolation
    VAR t = ('a', 'b', 'c')
    call assert_equal("x('a', 'b', 'c')x", $'x{t}x')
  END
  v9.CheckSourceLegacyAndVim9Success(lines)

  lines =<< trim END
    call assert_equal("a()b", $'a{null_tuple}b')
  END
  v9.CheckSourceDefAndScriptSuccess(lines)

  #" Tuple evaluation in heredoc
  lines =<< trim END
    VAR t1 = ('a', 'b', 'c')
    VAR data =<< eval trim DATA
      let x = {t1}
    DATA
    call assert_equal(["let x = ('a', 'b', 'c')"], data)
  END
  v9.CheckSourceLegacyAndVim9Success(lines)

  #" Empty tuple evaluation in heredoc
  lines =<< trim END
    VAR t1 = ()
    VAR data =<< eval trim DATA
      let x = {t1}
    DATA
    call assert_equal(["let x = ()"], data)
  END
  v9.CheckSourceLegacyAndVim9Success(lines)

  #" Null tuple evaluation in heredoc
  lines =<< trim END
    VAR t1 = test_null_tuple()
    VAR data =<< eval trim DATA
      let x = {t1}
    DATA
    call assert_equal(["let x = ()"], data)
  END
  v9.CheckSourceLegacyAndVim9Success(lines)

  lines =<< trim END
    var t1 = null_tuple
    var data =<< eval trim DATA
      let x = {t1}
    DATA
    call assert_equal(["let x = ()"], data)
  END
  v9.CheckSourceDefAndScriptSuccess(lines)
enddef

" Test for a return in "finally" block overriding the tuple return value in a
" try block.
func Test_try_finally_with_tuple_return()
  let lines =<< trim END
    func s:Fn()
      try
        return (1, 2)
      finally
        return (3, 4)
      endtry
    endfunc
    call assert_equal((3, 4), s:Fn())
    delfunc s:Fn
  END
  call v9.CheckSourceSuccess(lines)

  let lines =<< trim END
    vim9script
    def Fn(): tuple<...list<number>>
      try
        return (1, 2)
      finally
        return (3, 4)
      endtry
    enddef
    assert_equal((3, 4), Fn())
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" Test for add() with a tuple
func Test_tuple_add()
  let lines =<< trim END
    VAR t = (1, 2)
    call add(t, 3)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E897: List or Blob required',
        \ 'E1013: Argument 1: type mismatch, expected list<any> but got tuple<number, number>',
        \ 'E1226: List or Blob required for argument 1'])
endfunc

" Test for copy()
func Test_tuple_copy()
  let lines =<< trim END
    VAR t1 = (['a', 'b'], ['c', 'd'], ['e', 'f'])
    VAR t2 = copy(t1)
    VAR t3 = t1
    call assert_false(t2 is t1)
    call assert_true(t3 is t1)
    call assert_true(t2[1] is t1[1])
    call assert_equal((), copy(()))
    call assert_equal((), copy(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for count()
func Test_tuple_count()
  let lines =<< trim END
    VAR t = ('ab', 'cd', 'ab')
    call assert_equal(2, count(t, 'ab'))
    call assert_equal(0, count(t, 'xx'))
    call assert_equal(0, count((), 'xx'))
    call assert_equal(0, count(test_null_tuple(), 'xx'))
    call assert_fails("call count((1, 2), 1, v:true, 2)", 'E1519: Tuple index out of range: 2')
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for deepcopy()
func Test_tuple_deepcopy()
  let lines =<< trim END
    VAR t1 = (['a', 'b'], ['c', 'd'], ['e', 'f'])
    VAR t2 = deepcopy(t1)
    VAR t3 = t1
    call assert_false(t2 is t1)
    call assert_true(t3 is t1)
    call assert_false(t2[1] is t1[1])
    call assert_equal((), deepcopy(()))
    call assert_equal((), deepcopy(test_null_tuple()))

    #" copy a recursive tuple
    VAR l = []
    VAR tuple = (l,)
    call add(l, tuple)
    call assert_equal('([(...)], )', string(deepcopy(tuple)))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for empty()
func Test_tuple_empty()
  let lines =<< trim END
    call assert_true(empty(()))
    call assert_true(empty(test_null_tuple()))
    call assert_false(empty((1, 2)))
    VAR t = ('abc', 'def')
    call assert_false(empty(t))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for eval()
func Test_tuple_eval()
  let lines =<< trim END
    call assert_equal((), eval('()'))
    call assert_equal(([],), eval('([],)'))
    call assert_equal((1, 2, 3), eval('(1, 2, 3)'))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for extend() with a tuple
func Test_tuple_extend()
  let lines =<< trim END
    VAR t = (1, 2, 3)
    call extend(t, (4, 5))
    call extendnew(t, (4, 5))
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E712: Argument of extend() must be a List or Dictionary',
        \ 'E1013: Argument 1: type mismatch, expected list<any> but got tuple<number, number, number>',
        \ 'E712: Argument of extend() must be a List or Dictionary'])
endfunc

" Test for filter() with a tuple
func Test_tuple_filter()
  let lines =<< trim END
    VAR t = (1, 2, 3)
    call filter(t, 'v:val == 2')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1524: Cannot use a tuple with function filter()',
        \ 'E1013: Argument 1: type mismatch, expected list<any> but got tuple<number, number, number>',
        \ 'E1524: Cannot use a tuple with function filter()'])
endfunc

" Test for flatten() with a tuple
func Test_tuple_flatten()
  let t = ([1, 2], [3, 4], [5, 6])
  call assert_fails("call flatten(t, 2)", 'E686: Argument of flatten() must be a List')
endfunc

" Test for flattennew() with a tuple
func Test_tuple_flattennew()
  let lines =<< trim END
    var t = ([1, 2], [3, 4], [5, 6])
    flattennew(t, 2)
  END
  call v9.CheckSourceDefFailure(lines, 'E1013: Argument 1: type mismatch, expected list<any> but got tuple<list<number>, list<number>, list<number>>')
endfunc

" Test for foreach() with a tuple
func Test_tuple_foreach()
  let t = ('a', 'b', 'c')
  let str = ''
  call foreach(t, 'let str ..= v:val')
  call assert_equal('abc', str)

  let sum = 0
  call foreach(test_null_tuple(), 'let sum += v:val')
  call assert_equal(0, sum)

  let lines =<< trim END
    def Concatenate(k: number, v: string)
      g:str ..= v
    enddef
    var t = ('a', 'b', 'c')
    var str = 0
    g:str = ''
    call foreach(t, Concatenate)
    call assert_equal('abc', g:str)

    g:str = ''
    call foreach(test_null_tuple(), Concatenate)
    call assert_equal('', g:str)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  let lines =<< trim END
    LET g:sum = 0
    call foreach((1, 2, 3), 'LET g:sum += x')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E121: Undefined variable: x',
        \ 'E121: Undefined variable: x',
        \ 'E121: Undefined variable: x'])
endfunc

" Test for get()
func Test_tuple_get()
  let lines =<< trim END
    VAR t = (10, 20, 30)
    for [i, v] in [[0, 10], [1, 20], [2, 30], [3, 0]]
      call assert_equal(v, get(t, i))
    endfor

    for [i, v] in [[-1, 30], [-2, 20], [-3, 10], [-4, 0]]
      call assert_equal(v, get(t, i))
    endfor
    call assert_equal(0, get((), 5))
    call assert_equal('c', get(('a', 'b'), 2, 'c'))
    call assert_equal('x', get(test_null_tuple(), 0, 'x'))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for id()
func Test_tuple_id()
  let lines =<< trim END
    VAR t1 = (['a'], ['b'], ['c'])
    VAR t2 = (['a'], ['b'], ['c'])
    VAR t3 = t1
    call assert_true(id(t1) != id(t2))
    call assert_true(id(t1) == id(t3))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for index() function
func Test_tuple_index_func()
  let lines =<< trim END
    VAR t = (88, 33, 99, 77)
    call assert_equal(3, index(t, 77))
    call assert_equal(2, index(t, 99, 1))
    call assert_equal(2, index(t, 99, -4))
    call assert_equal(2, index(t, 99, -5))
    call assert_equal(-1, index(t, 66))
    call assert_equal(-1, index(t, 77, 4))
    call assert_equal(-1, index((), 8))
    call assert_equal(-1, index(test_null_tuple(), 9))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    VAR t = (88, 33, 99, 77)
    call assert_equal(-1, index(t, 77, []))
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E745: Using a List as a Number',
        \ 'E1013: Argument 3: type mismatch, expected number but got list<any>',
        \ 'E1210: Number required for argument 3'])

  let lines =<< trim END
    VAR t = (88,)
    call assert_equal(-1, index(t, 77, 1, ()))
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1520: Using a Tuple as a Number',
        \ 'E1013: Argument 4: type mismatch, expected bool but got tuple<any>',
        \ 'E1212: Bool required for argument 4'])
endfunc

" Test for indexof()
func Test_tuple_indexof()
  let lines =<< trim END
    VAR t = ('a', 'b', 'c', 'd')
    call assert_equal(2, indexof(t, 'v:val =~ "c"'))
    call assert_equal(2, indexof(t, 'v:val =~ "c"', {'startidx': 2}))
    call assert_equal(-1, indexof(t, 'v:val =~ "c"', {'startidx': 3}))
    call assert_equal(2, indexof(t, 'v:val =~ "c"', {'startidx': -3}))
    call assert_equal(2, indexof(t, 'v:val =~ "c"', {'startidx': -6}))
    call assert_equal(-1, indexof(t, 'v:val =~ "e"'))
    call assert_equal(-1, indexof((), 'v:val == 1'))
    call assert_equal(-1, indexof(test_null_tuple(), 'v:val == 2'))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  func g:MyIndexOf(k, v)
    echoerr 'MyIndexOf failed'
  endfunc
  let lines =<< trim END
    VAR t = (1, 2, 3)
    echo indexof(t, function('g:MyIndexOf'))
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'MyIndexOf failed',
        \ 'MyIndexOf failed',
        \ 'MyIndexOf failed'])
  delfunc g:MyIndexOf
endfunc

" Test for insert()
func Test_tuple_insert()
  let lines =<< trim END
    VAR t = (1, 2, 3)
    call insert(t, 4)
    call insert(t, 4, 2)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E899: Argument of insert() must be a List or Blob',
        \ 'E1013: Argument 1: type mismatch, expected list<any> but got tuple<number, number, number>',
        \ 'E1226: List or Blob required for argument 1'])
endfunc

" Test for islocked()
func Test_tuple_islocked()
  let lines =<< trim END
    let t = (1, [2], 3)
    call assert_equal(0, islocked('t'))
    call assert_equal(0, islocked('t[1]'))
    lockvar 1 t
    call assert_equal(1, islocked('t'))
    call assert_equal(0, islocked('t[1]'))
    unlockvar t
    call assert_equal(0, islocked('t'))
    lockvar 2 t
    call assert_equal(1, islocked('t[1]'))
    unlockvar t
    call assert_equal(0, islocked('t[1]'))
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" Test for items()
func Test_tuple_items()
  let lines =<< trim END
    VAR t = ([], {}, ())
    call assert_equal([[0, []], [1, {}], [2, ()]], items(t))
    call assert_equal([[0, 1]], items((1, )))
    call assert_equal([], items(()))
    call assert_equal([], items(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for join()
func Test_tuple_join()
  let lines =<< trim END
    VAR t = ('a', 'b', 'c')
    call assert_equal('a b c', join(t))
    call assert_equal('f o o', ('f', 'o', 'o')->join())
    call assert_equal('a-b-c', join(t, '-'))
    call assert_equal('', join(()))
    call assert_equal('', join(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for js_encode()
func Test_tuple_js_encode()
  let lines =<< trim END
    call assert_equal('["a","b","c"]', js_encode(('a', 'b', 'c')))
    call assert_equal('["a","b"]', js_encode(('a', 'b')))
    call assert_equal('["a"]', js_encode(('a',)))
    call assert_equal("[]", js_encode(()))
    call assert_equal("[]", js_encode(test_null_tuple()))
    call assert_equal('["a",,]', js_encode(('a', v:none)))

    #" encode a recursive tuple
    VAR l = []
    VAR tuple = (l,)
    call add(l, tuple)
    call assert_equal("[[[]]]", js_encode(tuple))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for json_encode()
func Test_tuple_json_encode()
  let lines =<< trim END
    call assert_equal('["a","b","c"]', json_encode(('a', 'b', 'c')))
    call assert_equal('["a","b"]', json_encode(('a', 'b')))
    call assert_equal('["a"]', json_encode(('a',)))
    call assert_equal("[]", json_encode(()))
    call assert_equal("[]", json_encode(test_null_tuple()))

    #" encode a recursive tuple
    VAR l = []
    VAR tuple = (l,)
    call add(l, tuple)
    call assert_equal("[[[]]]", json_encode(tuple))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    VAR t = (function('min'), function('max'))
    VAR s = json_encode(t)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1161: Cannot json encode a func',
        \ 'E1161: Cannot json encode a func',
        \ 'E1161: Cannot json encode a func'])
endfunc

" Test for len()
func Test_tuple_len()
  let lines =<< trim END
    call assert_equal(0, len(()))
    call assert_equal(0, len(test_null_tuple()))
    call assert_equal(1, len(("abc",)))
    call assert_equal(3, len(("abc", "def", "ghi")))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for map() with a tuple
func Test_tuple_map()
  let t = (1, 3, 5)
  call assert_fails("call map(t, 'v:val + 1')", 'E1524: Cannot use a tuple with function map()')
endfunc

" Test for max()
func Test_tuple_max()
  let lines =<< trim END
    VAR t1 = (1, 3, 5)
    call assert_equal(5, max(t1))
    VAR t2 = (6,)
    call assert_equal(6, max(t2))
    call assert_equal(0, max(()))
    call assert_equal(0, max(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    vim9script
    var x = max(('a', 2))
  END
  call v9.CheckSourceFailure(lines, 'E1030: Using a String as a Number: "a"')

  let lines =<< trim END
    vim9script
    var x = max((1, 'b'))
  END
  call v9.CheckSourceFailure(lines, 'E1030: Using a String as a Number: "b"')

  let lines =<< trim END
    vim9script
    def Fn()
      var x = max(('a', 'b'))
    enddef
    Fn()
  END
  call v9.CheckSourceFailure(lines, 'E1030: Using a String as a Number: "a"')

  let lines =<< trim END
    echo max([('a', 'b'), 20])
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1520: Using a Tuple as a Number',
        \ 'E1520: Using a Tuple as a Number',
        \ 'E1520: Using a Tuple as a Number'])
endfunc

" Test for min()
func Test_tuple_min()
  let lines =<< trim END
    VAR t1 = (5, 3, 1)
    call assert_equal(1, min(t1))
    VAR t2 = (6,)
    call assert_equal(6, min(t2))
    call assert_equal(0, min(()))
    call assert_equal(0, min(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    vim9script
    var x = min(('a', 2))
  END
  call v9.CheckSourceFailure(lines, 'E1030: Using a String as a Number: "a"')

  let lines =<< trim END
    vim9script
    var x = min((1, 'b'))
  END
  call v9.CheckSourceFailure(lines, 'E1030: Using a String as a Number: "b"')


  let lines =<< trim END
    vim9script
    def Fn()
      var x = min(('a', 'b'))
    enddef
    Fn()
  END
  call v9.CheckSourceFailure(lines, 'E1030: Using a String as a Number: "a"')
endfunc

" Test for reduce()
func Test_tuple_reduce()
  let lines =<< trim END
    call assert_equal(1, reduce((), LSTART acc, val LMIDDLE acc + val LEND, 1))
    call assert_equal(10, reduce((1, 3, 5), LSTART acc, val LMIDDLE acc + val LEND, 1))
    call assert_equal(2 * (2 * ((2 * 1) + 2) + 3) + 4, reduce((2, 3, 4), LSTART acc, val LMIDDLE 2 * acc + val LEND, 1))
    call assert_equal('a x y z', ('x', 'y', 'z')->reduce(LSTART acc, val LMIDDLE acc .. ' ' .. val LEND, 'a'))

    VAR t = ('x', 'y', 'z')
    call assert_equal(42, reduce(t, function('get'), {'x': {'y': {'z': 42 } } }))
    call assert_equal(('x', 'y', 'z'), t)
    call assert_equal(1, reduce((1,), LSTART acc, val LMIDDLE acc + val LEND))
    call assert_equal('x y z', reduce(('x', 'y', 'z'), LSTART acc, val LMIDDLE acc .. ' ' .. val LEND))
    call assert_equal(5, reduce(test_null_tuple(), LSTART acc, val LMIDDLE acc + val LEND, 5))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  call assert_equal({'x': 1, 'y': 1, 'z': 1 }, ('x', 'y', 'z')->reduce({ acc, val -> extend(acc, { val: 1 }) }, {}))

  call assert_fails("call reduce((), { acc, val -> acc + val })", 'E998: Reduce of an empty Tuple with no initial value')
  call assert_fails("call reduce(test_null_tuple(), { acc, val -> acc + val })", 'E998: Reduce of an empty Tuple with no initial value')

  let lines =<< trim END
    echo reduce((1, 2, 3), LSTART acc, val LMIDDLE acc + foo LEND)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E121: Undefined variable: foo',
        \ 'E1001: Variable not found: foo',
        \ 'E1001: Variable not found: foo'])
endfunc

" Test for remove()
func Test_tuple_remove()
  let lines =<< trim END
    VAR t = (1, 3, 5)
    call remove(t, 1)
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E896: Argument of remove() must be a List, Dictionary or Blob',
        \ 'E1013: Argument 1: type mismatch, expected list<any> but got tuple<number, number, number>',
        \ 'E1228: List, Dictionary or Blob required for argument 1'])
endfunc

" Test for test_refcount()
func Test_tuple_refcount()
  let lines =<< trim END
    VAR t = (1, 2, 3)
    call assert_equal(1, test_refcount(t))
    VAR x = t
    call assert_equal(2, test_refcount(t))
    LET x = (4, 5, 6)
    call assert_equal(1, test_refcount(t))
    for n in t
      call assert_equal(2, test_refcount(t))
    endfor
    call assert_equal(1, test_refcount(t))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for repeat()
func Test_tuple_repeat()
  let lines =<< trim END
    VAR t = ('a', 'b')
    call assert_equal(('a', 'b', 'a', 'b', 'a', 'b'), repeat(('a', 'b'), 3))
    call assert_equal(('x', 'x', 'x'), repeat(('x',), 3))
    call assert_equal((), repeat((), 3))
    call assert_equal((), repeat((), 0))
    call assert_equal((), repeat((), -1))
    call assert_equal((), repeat(test_null_tuple(), 3))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for reverse()
func Test_tuple_reverse()
  let lines =<< trim END
    VAR t = (['a'], ['b'], ['c'])
    call assert_equal((['c'], ['b'], ['a']), reverse(t))
    call assert_equal(('a',), reverse(('a',)))
    call assert_equal((), reverse(()))
    call assert_equal((), reverse(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for slicing a tuple
func Test_tuple_slice_func()
  let lines =<< trim END
    VAR t = (1, 3, 5, 7, 9)
    call assert_equal((9,), slice(t, 4))
    call assert_equal((5, 7, 9), slice(t, 2))
    call assert_equal((), slice(t, 5))
    call assert_equal((), slice((), 1, 2))
    call assert_equal((), slice(test_null_tuple(), 1, 2))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  " return value of slice() should be the correct tuple type
  let lines =<< trim END
    var t: tuple<...list<number>> = (1, 3, 5)
    var x: tuple<...list<number>> = slice(t, 1, 2)
    assert_equal((3,), x)
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)
endfunc

" Test for sort()
func Test_tuple_sort()
  let lines =<< trim END
    call sort([1.1, (1.2,)], 'f')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1521: Using a Tuple as a Float',
        \ 'E1521: Using a Tuple as a Float',
        \ 'E1521: Using a Tuple as a Float'])
endfunc

" Test for stridx()
func Test_tuple_stridx()
  let lines =<< trim END
    call stridx(('abc', ), 'a')
  END
  call v9.CheckSourceLegacyAndVim9Failure(lines, [
        \ 'E1522: Using a Tuple as a String',
        \ 'E1013: Argument 1: type mismatch, expected string but got tuple<string>',
        \ 'E1174: String required for argument 1'])
endfunc

" Test for string()
func Test_tuple_string()
  let lines =<< trim END
    VAR t1 = (1, 'as''d', [1, 2, function("strlen")], {'a': 1}, )
    call assert_equal("(1, 'as''d', [1, 2, function('strlen')], {'a': 1})", string(t1))

    #" empty tuple
    VAR t2 = ()
    call assert_equal("()", string(t2))

    #" one item tuple
    VAR t3 = ("a", )
    call assert_equal("('a', )", string(t3))

    call assert_equal("()", string(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  " recursive tuple
  let lines =<< trim END
    VAR l = []
    VAR t = (l,)
    call add(l, t)
    call assert_equal('([(...)], )', string(t))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for type()
func Test_tuple_type()
  let lines =<< trim END
    VAR t = (1, 2)
    call assert_equal(17, type(t))
    call assert_equal(v:t_tuple, type(t))
    call assert_equal(v:t_tuple, type(()))
    call assert_equal(v:t_tuple, type(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)
endfunc

" Test for typename()
func Test_tuple_typename()
  let lines =<< trim END
    call assert_equal('tuple<number, number>', typename((1, 2)))
    call assert_equal('tuple<string, string>', typename(('a', 'b')))
    call assert_equal('tuple<bool, bool>', typename((v:true, v:true)))
    call assert_equal('tuple<number, string>', typename((1, 'b')))
    call assert_equal('tuple<any>', typename(()))
    call assert_equal('tuple<dict<any>>', typename(({}, )))
    call assert_equal('tuple<list<any>>', typename(([], )))
    call assert_equal('tuple<list<number>>', typename(([1, 2], )))
    call assert_equal('tuple<list<string>>', typename((['a', 'b'], )))
    call assert_equal('tuple<list<list<number>>>', typename(([[1], [2]], )))
    call assert_equal('tuple<tuple<number, number>>', typename(((1, 2), )))
    VAR t1 = (([1, 2],), (['a', 'b'],))
    call assert_equal('tuple<tuple<list<number>>, tuple<list<string>>>', typename(t1))
    call assert_equal('list<tuple<number>>', typename([(1,)]))
    call assert_equal('list<tuple<any>>', typename([()]))
    call assert_equal('tuple<any>', typename(test_null_tuple()))
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  let lines =<< trim END
    var d: dict<any> = {a: 0}
    var t2 = (d,)
    t2[0].e = {b: t2}
    call assert_equal('tuple<dict<any>>', typename(t2))
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  " check the type of a circular reference tuple
  let lines =<< trim END
    # circular reference tuple
    var l: list<tuple<any>> = []
    var t = (l,)
    add(l, t)
    assert_equal('tuple<list<tuple<any>>>', typename(t))
    assert_equal('list<tuple<any>>', typename(l))
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)

  " When a tuple item is used in a "for" loop, the type is tuple<any>
  let lines =<< trim END
    vim9script
    var l = [(1, 2)]
    for t in l
      assert_equal('tuple<any>', typename(t))
    endfor
  END
  call v9.CheckSourceScriptSuccess(lines)

  " type of a tuple copy should be the same
  let lines =<< trim END
    var t: tuple<...list<number>> =  (1, 2)
    var x: tuple<...list<number>> =  t
    assert_equal('tuple<...list<number>>', typename(x))
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)
endfunc

" Test for saving and restoring tuples from a viminfo file
func Test_tuple_viminfo()
  let viminfo_save = &viminfo
  set viminfo^=!

  let g:MYTUPLE = ([1, 2], [3, 4], 'a', 'b', 1, 2)

  " create a tuple with circular reference
  " This should not be saved in the viminfo file
  let l = []
  let g:CIRCTUPLE = (l,)
  call add(l, g:CIRCTUPLE)

  wviminfo! Xviminfo
  unlet g:MYTUPLE
  unlet g:CIRCTUPLE
  rviminfo! Xviminfo
  call assert_equal(([1, 2], [3, 4], 'a', 'b', 1, 2), g:MYTUPLE)
  call assert_false(exists('g:CIRCTUPLE'))
  let &viminfo = viminfo_save
  call delete('Xviminfo')
endfunc

" Test for list2tuple()
func Test_list2tuple()
  let lines =<< trim END
    call assert_equal((), list2tuple([]))
    call assert_equal((), list2tuple(test_null_list()))
    call assert_equal(('a', ['b'], {'n': 20}), list2tuple(['a', ['b'], {'n': 20}]))

    VAR l = ['a', 'b']
    VAR t = list2tuple(l)
    LET l[0] = 'x'
    call assert_equal(('a', 'b'), t)

    call assert_equal((0, 1, 2), list2tuple(range(3)))

    call assert_equal(((),), [()]->list2tuple())
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  call assert_fails('call list2tuple(())', 'E1211: List required for argument 1')

  " Check the returned type
  let lines =<< trim END
    var l1 = [1, 2]
    var t1: tuple<...list<number>> = list2tuple(l1)
    assert_equal('tuple<...list<number>>', typename(t1))
    var l2 = ['a', 'b']
    var t2: tuple<...list<string>> = list2tuple(l2)
    assert_equal('tuple<...list<string>>', typename(t2))
    var l3 = []
    var t3 = list2tuple(l3)
    assert_equal('tuple<any>', typename(t3))
    var l4 = [([{}])]
    var t4: tuple<list<dict<any>>> = list2tuple(l4)
    assert_equal('tuple<list<dict<any>>>', typename(t4))
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)
endfunc

" Test for tuple2list()
func Test_tuple2list()
  let lines =<< trim END
    call assert_equal([], tuple2list(()))
    call assert_equal([], tuple2list(test_null_tuple()))

    VAR t1 = ('a', ['b'], {'n': 20}, ('a',))
    call assert_equal(['a', ['b'], {'n': 20}, ('a',)], tuple2list(t1))

    VAR t = ('a', 'b')
    VAR l = tuple2list(t)
    LET l[0] = 'x'
    call assert_equal(('a', 'b'), t)

    call assert_equal([[]], ([],)->tuple2list())
  END
  call v9.CheckSourceLegacyAndVim9Success(lines)

  call assert_fails('call tuple2list([])', 'E1534: Tuple required for argument 1')

  " Check the returned type
  let lines =<< trim END
    var t1 = (1, 2)
    var l1 = tuple2list(t1)
    assert_equal('list<number>', typename(l1))
    var t2 = ('a', 'b')
    var l2 = tuple2list(t2)
    assert_equal('list<string>', typename(l2))
    var t3 = ()
    var l3 = tuple2list(t3)
    assert_equal('list<any>', typename(l3))
    var t4 = ([({},)],)
    var l4 = tuple2list(t4)
    assert_equal('list<list<tuple<dict<any>>>>', typename(l4))
  END
  call v9.CheckSourceDefAndScriptSuccess(lines)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
