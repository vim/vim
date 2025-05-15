" Test Vim9 generic function

source check.vim
import './vim9.vim' as v9

" Test for definint a generic function
def Test_generic_func_definition()
  var lines =<< trim END
    vim9script
    def Fn<A, B>(x: A, y: B): A
      return x
    enddef
    defcompile
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    def Fn<Tn>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1548: Generic type name is not a single character: Tn>()', 2)

  lines =<< trim END
    vim9script
    def Fn<t>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1549: Generic type name must be a single uppercase letter: t>()', 2)

  lines =<< trim END
    vim9script
    def Fn<>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 2)

  lines =<< trim END
    vim9script
    def Fn<T, >()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after  >()', 2)

  lines =<< trim END
    vim9script
    def Fn<,>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <,>(', 2)

  lines =<< trim END
    vim9script
    def Fn<T()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1550: Missing comma after type in generic function: T()', 2)

  lines =<< trim END
    vim9script
    def Fn<T, ()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after  ()', 2)
enddef

" Test for white space error when defining a generic function
def Test_generic_func_definition_whitespace_error()
  var lines =<< trim END
    vim9script
    def Fn <A>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '<': <A>()", 2)

  lines =<< trim END
    vim9script
    def Fn<A> ()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '(':  (", 2)

  lines =<< trim END
    vim9script
    def Fn< A>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after '<': < A>()", 2)

  lines =<< trim END
    vim9script
    def Fn<A >()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'A': A >()", 2)

  lines =<< trim END
    vim9script
    def Fn<A,>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,>()", 2)

  lines =<< trim END
    vim9script
    def Fn<A, >()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after  >()", 2)

  lines =<< trim END
    vim9script
    def Fn<, A>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <, A>()", 2)

  lines =<< trim END
    vim9script
    def Fn<,A>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <,A>()", 2)

  lines =<< trim END
    vim9script
    def Fn< , A>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after '<': < , A>()", 2)

  lines =<< trim END
    vim9script
    def Fn<A,B>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,B>(", 2)

  lines =<< trim END
    vim9script
    def Fn<A , B>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'A': A , B>()", 2)

  lines =<< trim END
    vim9script
    def Fn<A, B >()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'B': B >()", 2)
enddef

" Test for invoking a generic function
def Test_generic_func_invoke()
  var lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    Fn<number, number>()
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 4)

  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    Fn<number>()
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<>()
  END
  v9.CheckSourceFailure(lines, "E1552: Empty type list specified for generic function 'Fn'", 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<>
  END
  v9.CheckSourceFailure(lines, 'E492: Not an editor command: Fn<>', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<
  END
  v9.CheckSourceFailure(lines, "E1551: Missing '>' in generic function: Fn<", 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<()
  END
  v9.CheckSourceFailure(lines, "E1551: Missing '>' in generic function: Fn<()", 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<number>
  END
  v9.CheckSourceFailure(lines, 'E492: Not an editor command: Fn<number>', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<number>(
  END
  v9.CheckSourceFailure(lines, 'E116: Invalid arguments for function Fn<number>(', 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn <number>()
  END
  v9.CheckSourceFailure(lines, 'E492: Not an editor command: Fn <number>()', 4)

  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    Fn<number, >()
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, >()', 4)

  lines =<< trim END
    vim9script
    def Fn<T, X>()
    enddef
    Fn<number, abc>()
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 4)

  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    Fn<number string>()
  END
  v9.CheckSourceFailure(lines, 'E1550: Missing comma after type in generic function: <number string>()', 4)

  # Error when compiling a generic function
  lines =<< trim END
    vim9script
    def Fn<A, B>()
      xxx
    enddef
    Fn<number, string>()
  END
  v9.CheckSourceFailure(lines, 'E476: Invalid command: xxx', 1)
enddef

" Test for whitespace error when invoking a generic function
def Test_generic_func_invoke_whitespace_error()
  var lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn< number>()
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after '<': < number>()", 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<number >()
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '>': >()", 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<number,>()
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,>(", 4)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    Fn<number, >()
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <number, >()", 4)

  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    Fn<number,string>()
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,string>()", 4)
enddef

def Test_generic_func_typename()
  var lines =<< trim END
    vim9script

    def Foo(a: list<string>, b: dict<number>): list<blob>
      return []
    enddef

    def Fn<T>(x: T, s: string)
      assert_equal(s, typename(x))
    enddef

    Fn<bool>(true, 'bool')
    Fn<number>(10, 'number')
    Fn<float>(3.4, 'float')
    Fn<string>('abc', 'string')
    Fn<blob>(0z1020, 'blob')
    Fn<list<list<blob>>>([[0z10, 0z20], [0z30]], 'list<list<blob>>')
    Fn<tuple<number, string>>((1, 'abc'), 'tuple<number, string>')
    Fn<dict<string>>({a: 'a', b: 'b'}, 'dict<string>')
    Fn<job>(test_null_job(), 'job')
    Fn<channel>(test_null_channel(), 'channel')
    Fn<func>(function('Foo'), 'func(list<string>, dict<number>): list<blob>')
  END
  v9.CheckSourceSuccess(lines)
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
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic type as the type of a function argument
def Test_generic_func_arg_type()
  var lines =<< trim END
    vim9script

    def F1<A>(x: list<A>): list<A>
      return x
    enddef

    def F2<B>(y: dict<B>): dict<B>
      return y
    enddef

    assert_equal(['a', 'b'], F1<string>(['a', 'b']))
    assert_equal({a: 0z10, b: 0z20}, F2<blob>({a: 0z10, b: 0z20}))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a tuple type for a generic function argument
def Test_generic_func_tuple_arg_type()
  var lines =<< trim END
    vim9script

    def Fn<T>(x: tuple<T, T>): tuple<T, T>
      return x
    enddef
    assert_equal((1, 2), Fn<number>((1, 2)))
    assert_equal(('a', 'b'), Fn<string>(('a', 'b')))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    def Fn<A, B>(x: tuple<A, ...list<B>>): tuple<A, ...list<B>>
      return x
    enddef
    assert_equal(('a', 1, 2), Fn<string, number>(('a', 1, 2)))
    assert_equal((3, 'a', 'b'), Fn<number, string>((3, 'a', 'b')))
  END
  v9.CheckSourceSuccess(lines)
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
  v9.CheckSourceSuccess(lines)

  # Using the generic type as the member of the List return value
  lines =<< trim END
    vim9script

    def Fn<A>(x: A): list<A>
      return [x]
    enddef

    assert_equal([1], Fn<number>(1))
    assert_equal(['abc'], Fn<string>('abc'))
  END
  v9.CheckSourceSuccess(lines)

  # Using the generic type as the member of the Dict return value
  lines =<< trim END
    vim9script

    def Fn<A>(x: A): dict<A>
      return {v: x}
    enddef

    assert_equal({v: 1}, Fn<number>(1))
    assert_equal({v: 'abc'}, Fn<string>('abc'))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic type as the type of the vararg variable
def Test_generic_func_varargs()
  var lines =<< trim END
    vim9script

    def Fn<A>(...x: list<list<A>>): list<list<A>>
      return x
    enddef

    assert_equal([[1], [2], [3]], Fn<number>([1], [2], [3]))
    assert_equal([['a'], ['b'], ['c']], Fn<string>(['a'], ['b'], ['c']))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using func type as a generic function argument type
def Test_generic_func_type_as_argument()
  var lines =<< trim END
    vim9script

    def Fn<A, B, C>(Foo: func(A, B): C): string
        return typename(Foo)
    enddef

    def F1(a: number, b: string): blob
        return 0z10
    enddef

    def F2(a: float, b: blob): string
        return 'abc'
    enddef

    assert_equal('func(number, string): blob', Fn<number, string, blob>(F1))
    assert_equal('func(float, blob): string', Fn<float, blob, string>(F2))
  END
  v9.CheckSourceSuccess(lines)
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
  v9.CheckSourceSuccess(lines)
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
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    def Foo()
      Fn<abc>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<number, string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 1)

  lines =<< trim END
    vim9script

    def Fn<A, B>()
    enddef

    def Foo()
      Fn<number>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1552: Empty type list specified for generic function 'Fn'", 1)

  lines =<< trim END
    vim9script

    def Fn<A, B>(x: A, y: B)
    enddef

    def Foo()
      Fn(10, 'abc')
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 1)

  lines =<< trim END
    vim9script

    def Fn(x: number)
    enddef

    def Foo()
      Fn<number>(10)
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function: Fn', 1)

  lines =<< trim END
    vim9script

    def Fn<A, B>(x: A, y: B)
    enddef

    def Foo()
      Fn<number, string>(10)
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E119: Not enough arguments for function', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<number, >()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, >()', 1)

  lines =<< trim END
    vim9script

    def Fn<T, X>()
    enddef

    def Foo()
      Fn<number, abc>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    def Fn<T>()
    enddef

    def Foo()
      Fn<number string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1550: Missing comma after type in generic function: <number string>()', 1)
enddef

" Test for using function() to get a generic funcref
def Test_get_generic_funcref_using_function()
  var lines =<< trim END
    vim9script
    def Fn<A>(x: A): A
      return x
    enddef
    var Fx = function(Fn<list<number>>)
    assert_equal([1], Fx([1]))
  END
  v9.CheckSourceSuccess(lines)

  # Get a generic funcref without specifying any type arguments
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function(Fn)
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  # Get a generic funcref specifying additional type arguments
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function(Fn<number, string>)
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 4)

  # Get a generic funcref specifying less type arguments
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    var Fx = function(Fn<string>)
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 4)

  # Get a generic funcref specifying non-existing type
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function(Fn<foobar>)
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: foobar', 4)

  # Get a generic funcref specifying an empty type argument list
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function(Fn<>)
  END
  v9.CheckSourceFailure(lines, "E1552: Empty type list specified for generic function 'Fn'", 4)

  # Get a generic funcref specifying only the opening bracket after name
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function(Fn<)
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <)', 4)

  # Get a generic funcref specifying only the opening bracket and type
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function(Fn<number)
  END
  v9.CheckSourceFailure(lines, 'E1550: Missing comma after type in generic function:', 4)

  # Get a generic funcref without specifying a type after comma
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function(Fn<number,)
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,)", 4)

  # Get a funcref to a regular function as a generic function
  lines =<< trim END
    vim9script
    def Fn()
    enddef
    var Fx = function(Fn<number>)
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function: Fn', 4)

  # Call a generic funcref using a different argument type
  lines =<< trim END
    vim9script
    def Fn<T>(t: T)
    enddef
    var Fx = function(Fn<string>)
    Fx(10)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got number', 5)

  # Assign a generic funcref return value to a variable of different type
  lines =<< trim END
    vim9script
    def Fn<T>(t: T): T
      return t
    enddef
    var Fx = function(Fn<string>)
    var x: number = Fx('abc')
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 6)

  # Call a generic funcref specifying types
  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    var Fx = function(Fn<string>)
    Fx<string>()
  END
  v9.CheckSourceFailure(lines, 'E15: Invalid expression: "Fx<string>()"', 5)
enddef

def Test_generic_funcref_string()
  var lines =<< trim END
    vim9script

    def Fn<A>(x: A): A
      return x
    enddef

    var Fx = function('Fn<list<number>>')
    assert_equal([1], Fx([1]))
  END
  v9.CheckSourceSuccess(lines)

  # Get a generic funcref without specifying any type arguments
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn')
    Fx()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 5)

  # Get a generic funcref specifying additional type arguments
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn<number, string>')
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 4)

  # Get a generic funcref specifying less type arguments
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    var Fx = function('Fn<string>')
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 4)

  # Get a generic funcref specifying non-existing type
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn<foobar>')
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: foobar', 4)

  # Get a generic funcref specifying an empty type argument list
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn<>')
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 4)

  # Get a generic funcref specifying only the opening bracket after name
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn<')
    Fx()
  END
  v9.CheckSourceFailure(lines, 'E15: Invalid expression: "Fx()"', 5)

  # Get a generic funcref specifying only the opening bracket and type
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn<number')
  END
  v9.CheckSourceFailure(lines, 'E475: Invalid argument: Fn<number', 4)

  # Get a generic funcref without specifying a type after comma
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn<number,')
  END
  v9.CheckSourceFailure(lines, 'E475: Invalid argument: Fn<number,', 4)

  # Get a funcref to a regular function as a generic function
  lines =<< trim END
    vim9script
    def Fn()
    enddef
    var Fx = function('Fn<number>')
    Fx()
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function:', 5)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn')
    Fx<string>()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 5)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn')
    Fx<>()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 5)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn')
    Fx<number, string>()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 5)

  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    var Fx = function('Fn')
    Fx<string>()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 5)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    var Fx = function('Fn')
    Fx()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 5)
enddef

" Test for calling a generic funcref from another function
def Test_generic_funcref_string_from_another_function()
  var lines =<< trim END
    vim9script

    def Fn<A>(x: A): A
      return x
    enddef

    def Foo()
      var Fx = function('Fn<list<number>>')
      assert_equal([1], Fx([1]))
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)

  # Get a generic funcref without specifying any type arguments
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 2)

  # Get a generic funcref specifying additional type arguments
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn<number, string>')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 2)

  # Get a generic funcref specifying less type arguments
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    def Foo()
      var Fx = function('Fn<string>')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 2)

  # Get a generic funcref specifying an empty type argument list
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn<>')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 2)

  # Get a generic funcref specifying only the opening bracket after name
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn<')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E117: Unknown function', 2)

  # Get a generic funcref specifying only the opening bracket and type
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn<number')
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E475: Invalid argument: Fn<number', 1)

  # Get a generic funcref without specifying a type after comma
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn<number,')
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E475: Invalid argument: Fn<number,', 1)

  # Get a funcref to a regular function as a generic function
  lines =<< trim END
    vim9script
    def Fn()
    enddef
    def Foo()
      var Fx = function('Fn<number>')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function:', 2)

  lines =<< trim END
    vim9script
    def Fn<A>(x: A): A
      return x
    enddef
    def Foo()
      var Fx = function('Fn')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 2)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn')
      Fx<>()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 2)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn')
      Fx<number>()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 2)

  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      var Fx = function('Fn')
      Fx()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 2)
enddef

def Test_generic_obj_method()
  var lines =<< trim END
    vim9script

    class A
      def Fn<X>(t: X): X
        var n: X = t
        return n
      enddef
    endclass

    var a = A.new()
    assert_equal(['a', 'b'], a.Fn<list<string>>(['a', 'b']))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    class A
      def Fn<>()
      enddef
    endclass
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1552: Empty type list specified for generic function 'Fn'", 4)

  lines =<< trim END
    vim9script

    class A
      def Fn<,>()
      enddef
    endclass
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <,>()", 4)

  lines =<< trim END
    vim9script

    class A
      def Fn<T>()
      enddef
    endclass
    var a = A.new()
    a.Fn<>()
  END
  v9.CheckSourceFailureList(lines, ['E15: Invalid expression: ">()"', "E1552: Empty type list specified for generic function 'Fn'"])

  lines =<< trim END
    vim9script

    class A
      def Fn<T>()
      enddef
    endclass
    var a = A.new()
    a.Fn<number, string>()
  END
  v9.CheckSourceFailure(lines, "E1553: Too many types specified for generic function 'Fn'", 8)

  lines =<< trim END
    vim9script

    class A
      def Fn<X, Y>()
      enddef
    endclass
    var a = A.new()
    a.Fn<string>()
  END
  v9.CheckSourceFailure(lines, "E1554: Not enough types specified for generic function 'Fn'", 8)

  lines =<< trim END
    vim9script

    class A
      def Fn<T>()
      enddef
    endclass
    var a = A.new()
    a.Fn()
  END
  v9.CheckSourceFailure(lines, "E1556: Type arguments missing for generic function", 8)

  lines =<< trim END
    vim9script

    class A
      def Fn()
      enddef
    endclass
    var a = A.new()
    a.Fn<number>()
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function: Fn', 8)
enddef

def Test_generic_obj_method_call_from_another_method()
  var lines =<< trim END
    vim9script

    class A
      def Fn<X>(t: X): X
        var n: X = t
        return n
      enddef
    endclass

    def Foo()
      var a = A.new()
      assert_equal(['a', 'b'], a.Fn<list<string>>(['a', 'b']))
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    class A
      def Fn<T>()
      enddef
    endclass

    def Foo()
      var a = A.new()
      a.Fn<>()
    enddef
    defcompile
  END
  v9.CheckSourceFailureList(lines, ['E15: Invalid expression: ">()"', "E1552: Empty type list specified for generic function 'Fn'"])

  lines =<< trim END
    vim9script

    class A
      def Fn<T>()
      enddef
    endclass

    def Foo()
      var a = A.new()
      a.Fn<number, string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1553: Too many types specified for generic function 'Fn'", 2)

  lines =<< trim END
    vim9script

    class A
      def Fn<X, Y>()
      enddef
    endclass

    def Foo()
      var a = A.new()
      a.Fn<string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1554: Not enough types specified for generic function 'Fn'", 2)

  lines =<< trim END
    vim9script

    class A
      def Fn<T>()
      enddef
    endclass

    def Foo()
      var a = A.new()
      a.Fn()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1556: Type arguments missing for generic function", 2)

  lines =<< trim END
    vim9script

    class A
      def Fn()
      enddef
    endclass

    def Foo()
      var a = A.new()
      a.Fn<number>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function: Fn<number>()', 2)

  # Try calling a non-existing generic object method
  lines =<< trim END
    vim9script

    class A
    endclass

    def Foo()
      var a = A.new()
      a.Bar<number, string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1325: Method "Bar" not found in class "A"', 2)

  # Error in compiling generic object method arguments
  lines =<< trim END
    vim9script

    class A
      def Fn<T, X>(x: number, y: number)
      enddef
    endclass

    def Foo()
      var a = A.new()
      a.Fn<number, string>(10,)
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,)", 2)

  # Try calling a super abstract method from a child class
  lines =<< trim END
    vim9script

    abstract class A
      abstract def F1()
    endclass

    class B extends A
      def F1()
      enddef
      def Foo()
        super.F1<number, string>()
      enddef
    endclass
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1431: Abstract method "F1" in class "A" cannot be accessed directly', 1)

  # Try calling a protected method in a class
  lines =<< trim END
    vim9script

    class A
      def _Foo()
      enddef
    endclass

    def Bar()
      var a = A.new()
      a._Foo<number, string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1366: Cannot access protected method: _Foo<number, string>()', 2)
enddef

def Test_generic_class_method()
  var lines =<< trim END
    vim9script

    class A
      static def Fn<X>(t: X): X
        var n: X = t
        return n
      enddef
    endclass

    assert_equal(['a', 'b'], A.Fn<list<string>>(['a', 'b']))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    class A
      static def Fn<>()
      enddef
    endclass
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1552: Empty type list specified for generic function 'Fn'", 4)

  lines =<< trim END
    vim9script

    class A
      static def Fn<,>()
      enddef
    endclass
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <,>()", 4)

  lines =<< trim END
    vim9script

    class A
      static def Fn<T>()
      enddef
    endclass
    A.Fn<>()
  END
  v9.CheckSourceFailureList(lines, ['E15: Invalid expression: ">()"', "E1552: Empty type list specified for generic function 'Fn'"])

  lines =<< trim END
    vim9script

    class A
      static def Fn<T>()
      enddef
    endclass
    A.Fn<number, string>()
  END
  v9.CheckSourceFailure(lines, "E1553: Too many types specified for generic function 'Fn'", 7)

  lines =<< trim END
    vim9script

    class A
      static def Fn<X, Y>()
      enddef
    endclass
    A.Fn<string>()
  END
  v9.CheckSourceFailure(lines, "E1554: Not enough types specified for generic function 'Fn'", 7)

  lines =<< trim END
    vim9script

    class A
      static def Fn<T>()
      enddef
    endclass
    A.Fn()
  END
  v9.CheckSourceFailure(lines, "E1556: Type arguments missing for generic function 'Fn'", 7)

  lines =<< trim END
    vim9script

    class A
      static def Fn()
      enddef
    endclass
    A.Fn<number>()
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function: Fn', 7)
enddef

def Test_generic_class_method_call_from_another_method()
  var lines =<< trim END
    vim9script

    class A
      static def Fn<X>(t: X): X
        var n: X = t
        return n
      enddef
    endclass

    def Foo()
      assert_equal(['a', 'b'], A.Fn<list<string>>(['a', 'b']))
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    class A
      static def Fn<T>()
      enddef
    endclass

    def Foo()
      A.Fn<>()
    enddef
    defcompile
  END
  v9.CheckSourceFailureList(lines, ['E15: Invalid expression: ">()"', "E1552: Empty type list specified for generic function 'Fn'"])

  lines =<< trim END
    vim9script

    class A
      static def Fn<T>()
      enddef
    endclass

    def Foo()
      A.Fn<number, string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1553: Too many types specified for generic function 'Fn'", 1)

  lines =<< trim END
    vim9script

    class A
      static def Fn<X, Y>()
      enddef
    endclass

    def Foo()
      A.Fn<string>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1554: Not enough types specified for generic function 'Fn'", 1)

  lines =<< trim END
    vim9script

    class A
      static def Fn<T>()
      enddef
    endclass

    def Foo()
      A.Fn()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1556: Type arguments missing for generic function 'Fn'", 1)

  lines =<< trim END
    vim9script

    class A
      static def Fn()
      enddef
    endclass

    def Foo()
      A.Fn<number>()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function: Fn<number>()', 1)
enddef

" Test for using a generic funcref from another method
def Test_generic_funcref_use_from_def_method()
  var lines =<< trim END
    vim9script

    def Foo<T>(t: T): T
      return t
    enddef

    def Fx()
      var Fn = Foo<list<string>>
      var x: list<string> = Fn(['abc', 'b'])
      assert_equal(['abc', 'b'], x)
    enddef
    Fx()
  END
  v9.CheckSourceSuccess(lines)

  # Assigning a generic function without specifying any type arguments
  lines =<< trim END
    vim9script

    def Foo<T>()
    enddef

    def Fx()
      var Fn = Foo
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 1)

  # Assigning a generic function specifying additional type arguments
  lines =<< trim END
    vim9script

    def Foo<T>()
    enddef

    def Fx()
      var Fn = Foo<number, string>
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 1)

  # Assigning a generic function specifying less type arguments
  lines =<< trim END
    vim9script

    def Foo<X, Y>()
    enddef

    def Fx()
      var Fn = Foo<string>
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 1)

  # Assigning a generic function specifying an empty type argument list
  lines =<< trim END
    vim9script

    def Foo<T>()
    enddef

    def Fx()
      var Fn = Foo<>
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 1)

  # Assigning a generic function specifying only the opening bracket
  lines =<< trim END
    vim9script

    def Foo<T>()
    enddef

    def Fx()
      var Fn = Foo<
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1551: Missing '>' in generic function: Foo<", 1)

  # Assigning a generic function without specifying the closing bracket
  lines =<< trim END
    vim9script

    def Foo<T>()
    enddef

    def Fx()
      var Fn = Foo<number
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1551: Missing '>' in generic function: Foo<number", 1)

  # Assigning a generic function without specifying a type after comma
  lines =<< trim END
    vim9script

    def Foo<X, Y>()
    enddef

    def Fx()
      var Fn = Foo<number,
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1551: Missing '>' in generic function: Foo<number", 1)

  # Create a funcref to a regular function as a generic function
  lines =<< trim END
    vim9script

    def Foo()
    enddef

    def Fx()
      var Fn = Foo<number>
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function: Foo', 1)

  # Call a generic funcref using a different argument type
  lines =<< trim END
    vim9script

    def Foo<T>(t: T)
    enddef

    def Fx()
      var Fn = Foo<string>
      Fn(10)
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got number', 2)

  # Assign a generic funcref return value to a variable of different type
  lines =<< trim END
    vim9script

    def Foo<T>(t: T): T
      return t
    enddef

    def Fx()
      var Fn = Foo<string>
      var x: number = Fn('abc')
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 2)
enddef

def Test_generic_vim9_lambda()
  var lines =<< trim END
    vim9script

    def Fn<A, B, C>()
      var Lambda = (x: A, y: B): C => x + y
      assert_equal(30, Lambda(10, 20))
    enddef

    Fn<number, number, number>()
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic type in a nested def function
def Test_generic_nested_def()
  var lines =<< trim END
    vim9script
    def F1<A, B, C>()
      def F2(x: A, y: B): C
        return x + y
      enddef
      assert_equal(30, F2(10, 20))
    enddef
    F1<number, number, number>()
  END
  v9.CheckSourceSuccess(lines)

  # Lambda function in a nested def function
  lines =<< trim END
    vim9script
    def F1<A, B, C>()
      def F2(): func
        var Lambda = (x: A, y: B): C => x + y
        return Lambda
      enddef
      var Fx = F2()
      assert_equal(60, Fx(20, 40))
    enddef
    F1<number, number, number>()
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for type substitution in a generic function call.  Only the generic types
" should be substituted.  Other "any" types should be ignored.
def Test_generic_type_substitution()
  var lines =<< trim END
    vim9script
    def Fn<T>(a: any, b: T): any
      return a
    enddef
    assert_equal('abc', Fn<number>('abc', 20))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a global generic function g:MyFunc<T>
def Test_generic_global_function()
  var lines =<< trim END
    vim9script
    def g:Fn1<T>(a: T): T
      return a
    enddef
    assert_equal('abc', g:Fn1<string>('abc'))

    def Foo()
      assert_equal(['a', 'b'], g:Fn1<list<string>>(['a', 'b']))
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for an imported generic function foo#MyFunc<T>
def Test_generic_import()
  var lines =<< trim END
    vim9script
    export def Fn<A, B>(a: A, b: B): B
      return b
    enddef
    export def Foobar()
    enddef
  END
  writefile(lines, 'Ximport_generic.vim', 'D')

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo
    assert_equal(20, Foo.Fn<string, number>('abc', 20))
    def MyFunc()
      assert_equal('xyz', Foo.Fn<number, string>(30, 'xyz'))
    enddef
    MyFunc()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    Foo.Fn<string>('abc', 20)
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 4)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    def MyFunc()
      Foo.Fn<string>('abc', 20)
    enddef
    MyFunc()
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 1)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    Foo.Fn<string, number>(10, 20)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got number', 4)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    def MyFunc()
      Foo.Fn<string, number>(10, 20)
    enddef
    MyFunc()
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got number', 1)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    Foo.Fn(10, 20)
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    def MyFunc()
      Foo.Fn(10, 20)
    enddef
    MyFunc()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 1)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    Foo.Fn<string, ('abc', 20)
  END
  v9.CheckSourceFailure(lines, "E1551: Missing '>' in generic function: <string, ('abc', 20)", 4)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    def MyFunc()
      Foo.Fn<string, ('abc', 20)
    enddef
    MyFunc()
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <string, ('abc', 20)", 1)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    Foo.Foobar<string>()
  END
  v9.CheckSourceFailure(lines, "E1557: Not a generic function", 4)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    def MyFunc()
      Foo.Foobar<string>()
    enddef
    MyFunc()
  END
  v9.CheckSourceFailure(lines, "E1557: Not a generic function: Foo", 1)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    Foo.Fx<string, number>('abc', 20)
  END
  v9.CheckSourceFailure(lines, 'E1048: Item not found in script: Fx', 4)

  lines =<< trim END
    vim9script
    import "./Ximport_generic.vim" as Foo

    def MyFunc()
      Foo.Fx<string, number>('abc', 20)
    enddef
    MyFunc()
  END
  v9.CheckSourceFailure(lines, 'E1048: Item not found in script: Fx', 1)
enddef

" Test for disassembling a generic function
def Test_generic_disassemble()
  var lines =<< trim END
    vim9script
    def Fn<A, B>(): A
      var x: A
      var y: B
      [x, y] = g:values
      return x
    enddef
    g:instr = execute('disassemble Fn<list<string>, dict<number>>')
  END
  v9.CheckScriptSuccess(lines)
  assert_match('<SNR>\d\+_Fn<list<string>, dict<number>>\_s*' ..
    'var x: A\_s*' ..
    '0 NEWLIST size 0\_s*' ..
    '1 SETTYPE list<string>\_s*' ..
    '2 STORE $0\_s*' ..
    'var y: B\_s*' ..
    '3 NEWDICT size 0\_s*' ..
    '4 SETTYPE dict<number>\_s*' ..
    '5 STORE $1\_s*' ..
    '\[x, y\] = g:values\_s*' ..
    '6 LOADG g:values\_s*\_s*' ..
    '7 CHECKTYPE list<any> stack\[-1\]\_s*' ..
    '8 CHECKLEN 2\_s*' ..
    '9 ITEM 0\_s*' ..
    '10 CHECKTYPE list<string> stack\[-1\] var 1\_s*' ..
    '11 SETTYPE list<string>\_s*' ..
    '12 STORE $0\_s*' ..
    '13 ITEM 1\_s*' ..
    '14 CHECKTYPE dict<number> stack\[-1\] var 2\_s*' ..
    '15 SETTYPE dict<number>\_s*' ..
    '16 STORE $1\_s*' ..
    '17 DROP\_s*' ..
    'return x\_s*' ..
    '18 LOAD $0\_s*' ..
    '19 RETURN', g:instr)

  lines =<< trim END
    vim9script
    disassemble Fn<number, dict<number>
  END
  v9.CheckScriptFailure(lines, 'E488: Trailing characters: <number, dict<number>', 2)

  lines =<< trim END
    vim9script
    disassemble Fn<number, dict<number>>
  END
  v9.CheckScriptFailure(lines, 'E1061: Cannot find function Fn<number, dict<number>>', 2)

  lines =<< trim END
    vim9script
    disassemble Fn<number,
  END
  v9.CheckScriptFailure(lines, 'E488: Trailing characters: <number,', 2)

  lines =<< trim END
    vim9script
    disassemble Fn<
  END
  v9.CheckScriptFailure(lines, 'E488: Trailing characters: <', 2)

  lines =<< trim END
    vim9script
    def Fn()
    enddef
    disassemble Fn<number>
  END
  v9.CheckScriptFailure(lines, 'E1557: Not a generic function:', 4)

  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    disassemble Fn<number, string>
  END
  v9.CheckScriptFailure(lines, 'E1553: Too many types specified for generic function', 4)

  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    disassemble Fn
  END
  v9.CheckScriptFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    disassemble Fn<number>
  END
  v9.CheckScriptFailure(lines, 'E1554: Not enough types specified for generic function', 4)

  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    disassemble Fn<>
  END
  v9.CheckScriptFailure(lines, 'E1552: Empty type list specified for generic function', 4)
enddef

" Test for disassembling a generic object method
def Test_generic_disassemble_generic_obj_method()
  var lines =<< trim END
    vim9script
    class Foo
      def Fn<A, B>()
        var x: A
        var y: B
        [x, y] = g:values
      enddef
    endclass
    g:instr = execute('disassemble Foo.Fn<list<string>, dict<number>>')
  END
  v9.CheckScriptSuccess(lines)
  assert_match('Fn<list<string>, dict<number>>\_s*' ..
    'var x: A\_s*' ..
    '0 NEWLIST size 0\_s*' ..
    '1 SETTYPE list<string>\_s*' ..
    '2 STORE $1\_s*' ..
    'var y: B\_s*' ..
    '3 NEWDICT size 0\_s*' ..
    '4 SETTYPE dict<number>\_s*' ..
    '5 STORE $2\_s*' ..
    '\[x, y\] = g:values\_s*' ..
    '6 LOADG g:values\_s*' ..
    '7 CHECKTYPE list<any> stack\[-1\]\_s*' ..
    '8 CHECKLEN 2\_s*' ..
    '9 ITEM 0\_s*' ..
    '10 CHECKTYPE list<string> stack\[-1\] var 1\_s*' ..
    '11 SETTYPE list<string>\_s*' ..
    '12 STORE $1\_s*' ..
    '13 ITEM 1\_s*' ..
    '14 CHECKTYPE dict<number> stack\[-1\] var 2\_s*' ..
    '15 SETTYPE dict<number>\_s*' ..
    '16 STORE $2\_s*' ..
    '17 DROP\_s*' ..
    '18 RETURN void', g:instr)

  lines =<< trim END
    vim9script
    class Foo
      def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<number, dict<number>
  END
  v9.CheckScriptFailure(lines, 'E1550: Missing comma after type in generic function: <number, dict<number>', 6)

  lines =<< trim END
    vim9script
    class Foo
    endclass
    disassemble Foo.Fn<number, dict<number>>
  END
  v9.CheckScriptFailure(lines, 'E1337: Class variable "Fn" not found in class "Foo"', 4)

  lines =<< trim END
    vim9script
    class Foo
      def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<number,
  END
  v9.CheckScriptFailure(lines, "E1069: White space required after ','", 6)

  lines =<< trim END
    vim9script
    class Foo
      def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<
  END
  v9.CheckScriptFailure(lines, 'E475: Invalid argument: Foo.Fn<', 6)

  lines =<< trim END
    vim9script
    class Foo
      def Fn()
      enddef
    endclass
    disassemble Foo.Fn<number>
  END
  v9.CheckScriptFailure(lines, 'E1557: Not a generic function:', 6)

  lines =<< trim END
    vim9script
    class Foo
      def Fn<X>()
      enddef
    endclass
    disassemble Foo.Fn<number, string>
  END
  v9.CheckScriptFailure(lines, 'E1553: Too many types specified for generic function', 6)

  lines =<< trim END
    vim9script
    class Foo
      def Fn<X>()
      enddef
    endclass
    disassemble Foo.Fn
  END
  v9.CheckScriptFailure(lines, 'E1556: Type arguments missing for generic function', 6)

  lines =<< trim END
    vim9script
    class Foo
      def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<number>
  END
  v9.CheckScriptFailure(lines, 'E1554: Not enough types specified for generic function', 6)

  lines =<< trim END
    vim9script
    class Foo
      def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<>
  END
  v9.CheckScriptFailure(lines, "E1552: Empty type list specified for generic function 'Fn'", 6)
enddef

" Test for disassembling a generic class method
def Test_generic_disassemble_generic_class_method()
  var lines =<< trim END
    vim9script
    class Foo
      static def Fn<A, B>()
        var x: A
        var y: B
        [x, y] = g:values
      enddef
    endclass
    g:instr = execute('disassemble Foo.Fn<list<string>, dict<number>>')
  END
  v9.CheckScriptSuccess(lines)
  assert_match('Fn<list<string>, dict<number>>\_s*' ..
    'var x: A\_s*' ..
    '0 NEWLIST size 0\_s*' ..
    '1 SETTYPE list<string>\_s*' ..
    '2 STORE $0\_s*' ..
    'var y: B\_s*' ..
    '3 NEWDICT size 0\_s*' ..
    '4 SETTYPE dict<number>\_s*' ..
    '5 STORE $1\_s*' ..
    '\[x, y\] = g:values\_s*' ..
    '6 LOADG g:values\_s*' ..
    '7 CHECKTYPE list<any> stack\[-1\]\_s*' ..
    '8 CHECKLEN 2\_s*' ..
    '9 ITEM 0\_s*' ..
    '10 CHECKTYPE list<string> stack\[-1\] var 1\_s*' ..
    '11 SETTYPE list<string>\_s*' ..
    '12 STORE $0\_s*' ..
    '13 ITEM 1\_s*' ..
    '14 CHECKTYPE dict<number> stack\[-1\] var 2\_s*' ..
    '15 SETTYPE dict<number>\_s*' ..
    '16 STORE $1\_s*' ..
    '17 DROP\_s*' ..
    '18 RETURN void', g:instr)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<number, dict<number>
  END
  v9.CheckScriptFailure(lines, 'E1550: Missing comma after type in generic function: <number, dict<number>', 6)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<number,
  END
  v9.CheckScriptFailure(lines, "E1069: White space required after ','", 6)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<
  END
  v9.CheckScriptFailure(lines, 'E475: Invalid argument: Foo.Fn<', 6)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn()
      enddef
    endclass
    disassemble Foo.Fn<number>
  END
  v9.CheckScriptFailure(lines, 'E1557: Not a generic function:', 6)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn<X>()
      enddef
    endclass
    disassemble Foo.Fn<number, string>
  END
  v9.CheckScriptFailure(lines, 'E1553: Too many types specified for generic function', 6)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn<X>()
      enddef
    endclass
    disassemble Foo.Fn
  END
  v9.CheckScriptFailure(lines, 'E1556: Type arguments missing for generic function', 6)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<number>
  END
  v9.CheckScriptFailure(lines, 'E1554: Not enough types specified for generic function', 6)

  lines =<< trim END
    vim9script
    class Foo
      static def Fn<X, Y>()
      enddef
    endclass
    disassemble Foo.Fn<>
  END
  v9.CheckScriptFailure(lines, "E1552: Empty type list specified for generic function 'Fn'", 6)
enddef

def Test_generic_duplicate_names()
  var lines =<< trim END
    vim9script
    def Fn<A, B, A>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1558: Duplicate generic type: A', 2)

  lines =<< trim END
    vim9script
    class A
    endclass
    def Fn<A, B>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1041: Redefining script item: "A"', 4)

  lines =<< trim END
    vim9script
    type A = number
    def Fn<A, B>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1041: Redefining script item: "A"', 3)

  lines =<< trim END
    vim9script
    var B = 'abc'
    def Fn<A, B>()
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1041: Redefining script item: "B"', 3)

  lines =<< trim END
    vim9script
    def Fn1<A, B>()
    enddef
    def Fn2<A, B>()
    enddef
    defcompile
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for nested generic functions
def Test_generic_nested_functions()
  var lines =<< trim END
    vim9script
    def Fn<T>(t: T): T
      def Fx<A>(a: A): A
        return a
      enddef
      return Fx<number>(t)
    enddef
    assert_equal(100, Fn<number>(100))
  END
  v9.CheckScriptSuccess(lines)

  # Use the generic type from the outer generic function
  lines =<< trim END
    vim9script
    def Fn<A, B>(b: B): B
      def Fx<T>(t: T): T
        return t
      enddef
      return Fx<B>(b)
    enddef
    assert_equal(100, Fn<number, number>(100))
    assert_equal('abc', Fn<number, string>('abc'))
  END
  v9.CheckScriptSuccess(lines)

  # duplicate definition
  lines =<< trim END
    vim9script
    def Fn<T>()
      def Fn<A>()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1073: Name already defined: Fn', 1)

  # overlaps with a script-local function
  lines =<< trim END
    vim9script
    def Fx()
    enddef
    def Fn<T>()
      def Fx<A>()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1073: Name already defined: Fx', 1)

  # overlaps with another nested function
  lines =<< trim END
    vim9script
    def Fn<T>()
      def Fx<A>()
      enddef
      def Fx<B>()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1073: Name already defined: Fx', 3)

  # Empty list of types
  lines =<< trim END
    vim9script
    def Fn<T>()
      def Fx<>()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1552: Empty type list specified for generic function', 1)

  lines =<< trim END
    vim9script
    def Fn<T>()
      def Fx<,>()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1008: Missing <type> after <,>()', 1)

  # missing closing bracket in the inner generic function
  lines =<< trim END
    vim9script
    def Fn<T>()
      def Fx<A()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, "E1551: Missing '>' in generic function: <A()", 3)

  lines =<< trim END
    vim9script
    def Fn<T>()
      def Fx<a>()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1549: Generic type name must be a single uppercase letter: a>()', 1)

  # duplicate generic type
  lines =<< trim END
    vim9script
    def Fn<T>()
      def Fx<T>()
      enddef
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1558: Duplicate generic type: T', 1)
enddef

" Test for using a generic function in call() as a string
def Test_generic_function_use_in_call_function_as_string()
  var lines =<< trim END
    vim9script
    def Fn<A>(a: A): A
      return a
    enddef
    assert_equal(['a', 'b'], call("Fn<list<string>>", [['a', 'b']]))
  END
  v9.CheckSourceSuccess(lines)

  # Test for passing more types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call("Fn<number, string>", [])
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 4)

  # Test for passing less types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call("Fn<string>", [])
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 4)

  # Test for passing empty types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call("Fn<>", [])
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 4)

  # Test for passing no types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call("Fn", [])
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  # Test for missing types and closing bracket
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call("Fn<", [])
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call("Fn<number", [])
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call("Fn<number,", [])
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call("Fn<number,>", [])
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ','", 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call("Fn<number, >", [])
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, >', 4)

  # Test for calling a non-existing generic function
  lines =<< trim END
    vim9script
    call("FooBar<number>", [])
  END
  v9.CheckSourceFailure(lines, 'E1555: Unknown generic function', 2)

  # Test for calling a regular function as a generic function
  lines =<< trim END
    vim9script
    def Fn()
    enddef
    call("Fn<number>", [])
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function', 4)
enddef

" Test for using a generic function in call() as a string in a method
def Test_generic_use_in_call_func_as_string_in_method()
  var lines =<< trim END
    vim9script
    def Fn<A>(a: A): A
      return a
    enddef
    def Foo()
      assert_equal(['a', 'b'], call("Fn<list<string>>", [['a', 'b']]))
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)

  # Test for passing more types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      call("Fn<number, string>", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 1)

  # Test for passing less types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    def Foo()
      call("Fn<string>", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 1)

  # Test for passing empty types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      call("Fn<>", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 1)

  # Test for passing no types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      call("Fn", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 1)

  # Test for missing types and closing bracket
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      call("Fn<", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 1)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    def Foo()
      call("Fn<number", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 1)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    def Foo()
      call("Fn<number,", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 1)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    def Foo()
      call("Fn<number,>", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ','", 1)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    def Foo()
      call("Fn<number, >", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, >', 1)

  # Test for calling a non-existing generic function
  lines =<< trim END
    vim9script
    def Foo()
      call("FooBar<number>", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1555: Unknown generic function', 1)

  # Test for calling a regular function as a generic function
  lines =<< trim END
    vim9script
    def Fn()
    enddef
    def Foo()
      call("Fn<number>", [])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function', 1)
enddef

" Test for using a generic function in call() as a funcref
def Test_generic_function_use_in_call_function_as_funcref()
  var lines =<< trim END
    vim9script
    def Fn<A>(a: A): A
      return a
    enddef
    assert_equal({a: 'xyz'}, call(Fn<dict<string>>, [{a: 'xyz'}]))
  END
  v9.CheckSourceSuccess(lines)

  # Test for passing more types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call(Fn<number, string>, [])
  END
  v9.CheckSourceFailure(lines, 'E1553: Too many types specified for generic function', 4)

  # Test for passing less types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call(Fn<string>, [])
  END
  v9.CheckSourceFailure(lines, 'E1554: Not enough types specified for generic function', 4)

  # Test for passing empty types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call(Fn<>, [])
  END
  v9.CheckSourceFailure(lines, 'E1552: Empty type list specified for generic function', 4)

  # Test for passing no types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call(Fn, [])
  END
  v9.CheckSourceFailure(lines, 'E1556: Type arguments missing for generic function', 4)

  # Test for missing types and closing bracket
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call(Fn<, [])
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <, [])', 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A>()
    enddef
    call(Fn<number, [])
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, [])', 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call(Fn<number,, [])
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,, [])", 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call(Fn<number,>, [])
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ','", 4)

  # Test for missing types
  lines =<< trim END
    vim9script
    def Fn<A, B>()
    enddef
    call(Fn<number, >, [])
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, >', 4)

  # Test for calling a non-existing generic function
  lines =<< trim END
    vim9script
    call(FooBar<number>, [])
  END
  v9.CheckSourceFailure(lines, 'E121: Undefined variable: FooBar', 2)

  # Test for calling a regular function as a generic function
  lines =<< trim END
    vim9script
    def Fn()
    enddef
    call(Fn<number>, [])
  END
  v9.CheckSourceFailure(lines, 'E1557: Not a generic function', 4)
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
