" Test Vim9 Generics

source check.vim
import './vim9.vim' as v9

" Test for declaring a generic function
def Test_generic_function_declaration()
  var lines =<< trim END
    vim9script
    def Fn<Tn>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1548: Generic type name is not a single character: Tn>()')

  lines =<< trim END
    vim9script
    def Fn<t>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1549: Generic type name must a single uppercase letter: t>()')

  lines =<< trim END
    vim9script
    def Fn<>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1552: Empty type list in generic function')

  lines =<< trim END
    vim9script
    def Fn<T()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1550: Missing comma after type in generics function: T()')

  lines =<< trim END
    vim9script
    def Fn<T, ()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1549: Generic type name must a single uppercase letter: ()')
enddef

" Test for invoking a generic function
def Test_generic_function_invoke()
  var lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    Fn<number, number>()
  END
  v9.CheckScriptFailure(lines, 'E1553: Too many generic types for function: Fn', 4)

  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    Fn<number>()
  END
  v9.CheckScriptFailure(lines, 'E1554: Not enough generic types for function: Fn', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<>()
  END
  v9.CheckScriptFailure(lines, 'E1552: Empty type list in generic function', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn()
  END
  v9.CheckScriptFailure(lines, 'E1554: Not enough generic types for function: Fn', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<()
  END
  v9.CheckScriptFailure(lines, 'E492: Not an editor command: Fn<()', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<number>
  END
  v9.CheckScriptFailure(lines, 'E492: Not an editor command: Fn<number>', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<number>(
  END
  v9.CheckScriptFailure(lines, 'E116: Invalid arguments for function Fn<number>(', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn <number>()
  END
  v9.CheckScriptFailure(lines, 'E492: Not an editor command: Fn <number>()', 4)

  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    Fn<number, >()
  END
  v9.CheckScriptFailure(lines, 'E1008: Missing <type> after <number, >()', 4)

  lines =<< trim END
    vim9script
    def Fn<T, X>()
    enddef
    Fn<number, abc>()
  END
  v9.CheckScriptFailure(lines, 'E1010: Type not recognized: abc', 4)

  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    Fn<number string>()
  END
  v9.CheckScriptFailure(lines, 'E1550: Missing comma after type in generics function: <number string>()', 4)
enddef

def Test_generic_func_single_arg()
  var lines =<< trim END
    vim9script

    def Fn<A>(x: A): number
      return len(x)
    enddef

    assert_equal(3, Fn<list<number>>([1, 2, 3]))
    assert_equal(2, Fn<dict<number>>({a: 1, b: 2}))
    assert_equal(1, Fn<blob>(0z10))
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_generic_func_ret_type()
  var lines =<< trim END
    vim9script

    def Fn<A>(x: A): A
      return x
    enddef

    assert_equal([1], Fn<list<number>>([1]))
    assert_equal({a: 1}, Fn<dict<number>>({a: 1}))
    assert_equal((1,), Fn<tuple<number>>((1,)))
    assert_equal(0z10, Fn<blob>(0z10))
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_generic_nested_call()
  var lines =<< trim END
    vim9script

    def Fn<A>(n: number, x: A): A
      if n
        return x
      endif

      assert_equal('abc', Fn<string>(1, 'abc'))

      return x
    enddef

    assert_equal(10, Fn<number>(0, 10))
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_generic_failure_in_def_function()
  var lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<abc>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    def Foo()
      Fn<abc>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<number, string>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1553: Too many generic types for function: Fn', 1)

  lines =<< trim END
    vim9script

    def Fn<A, B>()
    enddef

    def Foo()
      Fn<number>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1554: Not enough generic types for function: Fn', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1552: Empty type list in generic function', 1)

  lines =<< trim END
    vim9script

    def Fn<A, B>(x: A, y: B)
    enddef

    def Foo()
      Fn(10, 'abc')
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1554: Not enough generic types for function: Fn', 1)

  lines =<< trim END
    vim9script

    def Fn(x: number)
    enddef

    def Foo()
      Fn<number>(10)
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1556: Unknown generic function: Fn', 1)

  lines =<< trim END
    vim9script

    def Fn<A, B>(x: A, y: B)
    enddef

    def Foo()
      Fn<number, string>(10)
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E119: Not enough arguments for function', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<number, >()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1008: Missing <type> after <number, >()', 1)

  lines =<< trim END
    vim9script

    def Fn<T, X>()
    enddef

    def Foo()
      Fn<number, abc>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<number string>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1550: Missing comma after type in generics function: <number string>()', 1)
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
