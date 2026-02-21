" Test Vim9 generic class

import './util/vim9.vim' as v9

" Test for defining a generic class
def Test_generic_class_definition()
  var lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var f1 = Foo<number>.new()
    var f2 = Foo<string>.new()
    assert_equal('object<Foo<number>>', typename(f1))
    assert_equal('object<Foo<string>>', typename(f2))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Foo<t>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1552: Type variable name must start with an uppercase letter: t>', 2)

  lines =<< trim END
    vim9script
    class Foo<>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1555: Empty type list specified for generic', 2)

  lines =<< trim END
    vim9script
    class Foo<T, >
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after  >', 2)

  lines =<< trim END
    vim9script
    class Foo<,>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <,>', 2)

  lines =<< trim END
    vim9script
    class Foo<T
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1553: Missing comma after type in generic: T', 2)

  # Use a multi-character generic type name
  lines =<< trim END
    vim9script
    class Foo<MyType1, MyType2>
    endclass
    var f = Foo<number, string>.new()
    assert_equal('object<Foo<number, string>>', typename(f))
  END
  v9.CheckSourceSuccess(lines)

  # Use a generic type name starting with a lower case letter
  lines =<< trim END
    vim9script
    class Foo<mytype>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1552: Type variable name must start with an uppercase letter: mytype>', 2)

  # Use a non-alphanumeric character in the generic type name
  lines =<< trim END
    vim9script
    class Foo<My-type>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1553: Missing comma after type in generic: My-type>', 2)

  # Use an existing type name as the generic type name
  lines =<< trim END
    vim9script
    type FooBar = number
    class Foo<FooBar>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "FooBar"', 3)

  # Use a very long type name
  lines =<< trim END
    vim9script
    class Foo<XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX>
    endclass
    var f = Foo<number>.new()
    assert_equal('object<Foo<number>>', typename(f))
  END
  v9.CheckSourceSuccess(lines)

  # Use a function name as the generic type name
  lines =<< trim END
    vim9script
    def MyFunc()
    enddef
    class Foo<MyFunc>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "MyFunc"', 4)
enddef

" Test for white space error when defining a generic class
def Test_generic_class_definition_whitespace_error()
  var lines =<< trim END
    vim9script
    class Foo <A>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '<': <A>", 2)

  lines =<< trim END
    vim9script
    class Foo< A>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after '<': < A>", 2)

  lines =<< trim END
    vim9script
    class Foo<A >
    endclass
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'A': A >", 2)

  lines =<< trim END
    vim9script
    class Foo<A,>()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,>()", 2)

  lines =<< trim END
    vim9script
    class Foo<A, >()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after  >()", 2)

  lines =<< trim END
    vim9script
    class Foo<, A>()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <, A>()", 2)

  lines =<< trim END
    vim9script
    class Foo<,A>()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <,A>()", 2)

  lines =<< trim END
    vim9script
    class Foo< , A>()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after '<': < , A>()", 2)

  lines =<< trim END
    vim9script
    class Foo<A,B>()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': ,B>(", 2)

  lines =<< trim END
    vim9script
    class Foo<A , B>()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'A': A , B>()", 2)

  lines =<< trim END
    vim9script
    class Foo<A, B >()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'B': B >()", 2)

  lines =<< trim END
    vim9script
    class Foo<MyType , FooBar>()
    endclass
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'MyType': MyType , FooBar>()", 2)
enddef

" Test for creating an instance of a generic class
def Test_generic_class_instance()
  var lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var f = Foo<number, number>.new()
  END
  v9.CheckSourceFailure(lines, "E1572: Too many types specified for generic class 'Foo'", 4)

  lines =<< trim END
    vim9script
    class Foo<A, B>
    endclass
    var f = Foo<number>.new()
  END
  v9.CheckSourceFailure(lines, "E1573: Not enough types specified for generic class 'Foo'", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<>.new()
  END
  v9.CheckSourceFailure(lines, "E1555: Empty type list specified for generic '<>.new()'", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<>.new()
  END
  v9.CheckSourceFailure(lines, "E1555: Empty type list specified for generic '<>.new()'", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo.new()
  END
  v9.CheckSourceFailure(lines, "E1570: Type arguments missing for generic class 'Foo'", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<
  END
  v9.CheckSourceFailure(lines, "E1554: Missing '>' in generic: <", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<.new()
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<number>
  END
  v9.CheckSourceFailure(lines, 'E1405: Class "Foo<number>" cannot be used as a value', 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<number>.
  END
  v9.CheckSourceFailure(lines, 'E15: Invalid expression: "Foo<number>."', 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo <number>.new()
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '<':  <number>.new(", 4)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var f = Foo<number, >.new()
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, >.new()', 4)

  lines =<< trim END
    vim9script
    class Foo<T, X>
    endclass
    var f = Foo<number, abc>.new()
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 4)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var f = Foo<number string>.new()
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'number': <number", 4)

  # Test for assigning an object of a generic class to the wrong type
  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var f: Foo<string> = Foo<number>.new()
  END
  v9.CheckSourceFailure(lines, "E1012: Type mismatch; expected object<Foo<string>> but got object<Foo<number>>", 4)

  # Error when compiling a generic class
  lines =<< trim END
    vim9script
    class Foo<A, B>
      xxx
    endclass
    var f = Foo<number, string>.new()
  END
  v9.CheckSourceFailure(lines, 'E1318: Not a valid command in a class: xxx', 3)
enddef

" Test for whitespace error when using a generic class
def Test_generic_class_instance_whitespace_error()
  var lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo< number>.new()
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after '<': < number>.new()", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<number >.new()
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'number': <number", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<number,>.new()
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': <number,>.new()", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<number, >.new()
  END
  v9.CheckSourceFailure(lines, "E1008: Missing <type> after <number, >.new()", 4)

  lines =<< trim END
    vim9script
    class Foo<A, B>
    endclass
    var f = Foo<number,string>.new()
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': <number,string>.new()", 4)

  lines =<< trim END
    vim9script
    class Foo<A>
    endclass
    var f = Foo<number> .new()
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after '>': <number> .new()", 4)
enddef

def Test_generic_class_typename()
  var lines =<< trim END
    vim9script

    def Tfunc(a: list<string>, b: dict<number>): list<blob>
      return []
    enddef

    class Foo<T>
      def Fn(x: T, s: string)
        assert_equal(s, typename(x))
      enddef
    endclass

    var f1 = Foo<bool>.new()
    f1.Fn(true, 'bool')

    var f2 = Foo<number>.new()
    f2.Fn(10, 'number')

    var f3 = Foo<float>.new()
    f3.Fn(3.4, 'float')

    var f4 = Foo<string>.new()
    f4.Fn('abc', 'string')

    var f5 = Foo<blob>.new()
    f5.Fn(0z1020, 'blob')

    var f6 = Foo<list<list<blob>>>.new()
    f6.Fn([[0z10, 0z20], [0z30]], 'list<list<blob>>')

    var f7 = Foo<tuple<number, string>>.new()
    f7.Fn((1, 'abc'), 'tuple<number, string>')

    var f8 = Foo<dict<string>>.new()
    f8.Fn({a: 'a', b: 'b'}, 'dict<string>')

    if has('job')
      var f9 = Foo<job>.new()
      f9.Fn(test_null_job(), 'job')
    endif

    if has('channel')
      var f10 = Foo<channel>.new()
      f10.Fn(test_null_channel(), 'channel')
    endif

    var f11 = Foo<func>.new()
    f11.Fn(function('Tfunc'), 'func(list<string>, dict<number>): list<blob>')
  END
  v9.CheckSourceSuccess(lines)
enddef

def Test_generic_class_single_type()
  var lines =<< trim END
    vim9script

    class Foo<A>
      var v: A

      def len(): number
        return len(this.v)
      enddef
    endclass

    var f1 = Foo<list<string>>.new(['a', 'b', 'c'])
    assert_equal(3, len(f1))
    var f2 = Foo<dict<number>>.new({a: 1, b: 2})
    assert_equal(2, len(f2))
    var f3 = Foo<blob>.new(0z10)
    assert_equal(1, len(f3))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic type as the type of a object method argument
def Test_generic_class_arg_type()
  var lines =<< trim END
    vim9script

    class Foo<A, B, C>
      def F1(x: list<A>): list<A>
        return x
      enddef

      def F2(y: tuple<...list<B>>): tuple<...list<B>>
        return y
      enddef

      def F3(z: dict<C>): dict<C>
        return z
      enddef
    endclass

    var f = Foo<string, number, blob>.new()
    assert_equal(['a', 'b'], f.F1(['a', 'b']))
    assert_equal((8, 9), f.F2((8, 9)))
    assert_equal({a: 0z10, b: 0z20}, f.F3({a: 0z10, b: 0z20}))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a tuple type for a generic object method argument
def Test_generic_class_tuple_arg_type()
  var lines =<< trim END
    vim9script

    class Foo<T>
      def Fn(x: tuple<T, T>): tuple<T, T>
        return x
      enddef
    endclass
    var f1 = Foo<number>.new()
    var f2 = Foo<string>.new()
    assert_equal((1, 2), f1.Fn((1, 2)))
    assert_equal(('a', 'b'), f2.Fn(('a', 'b')))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    class Foo<A, B>
      def Fn(x: tuple<A, ...list<B>>): tuple<A, ...list<B>>
        return x
      enddef
    endclass
    var f1 = Foo<string, number>.new()
    assert_equal(('a', 1, 2), f1.Fn(('a', 1, 2)))
    var f2 = Foo<number, string>.new()
    assert_equal((3, 'a', 'b'), f2.Fn((3, 'a', 'b')))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic type in an object method return value
def Test_generic_class_ret_type()
  var lines =<< trim END
    vim9script

    class Foo<A>
      def Fn(x: A): A
        return x
      enddef
    endclass

    var f1 = Foo<list<number>>.new()
    assert_equal([1], f1.Fn([1]))
    var f2 = Foo<dict<number>>.new()
    assert_equal({a: 1}, f2.Fn({a: 1}))
    var f3 = Foo<tuple<number>>.new()
    assert_equal((1,), f3.Fn((1,)))
    var f4 = Foo<blob>.new()
    assert_equal(0z10, f4.Fn(0z10))
  END
  v9.CheckSourceSuccess(lines)

  # Using the generic type as the member of the List return value
  lines =<< trim END
    vim9script

    class Foo<A>
      def Fn(x: A): list<A>
        return [x]
      enddef
    endclass

    var f1 = Foo<number>.new()
    assert_equal([1], f1.Fn(1))
    var f2 = Foo<string>.new()
    assert_equal(['abc'], f2.Fn('abc'))
  END
  v9.CheckSourceSuccess(lines)

  # Using the generic type as the member of the Dict return value
  lines =<< trim END
    vim9script

    class Foo<A>
      def Fn(x: A): dict<A>
        return {v: x}
      enddef
    endclass

    var f1 = Foo<number>.new()
    assert_equal({v: 1}, f1.Fn(1))
    var f2 = Foo<string>.new()
    assert_equal({v: 'abc'}, f2.Fn('abc'))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic type as the type of the vararg variable
def Test_generic_class_varargs()
  var lines =<< trim END
    vim9script

    class Foo<A>
      def Fn(...x: list<list<A>>): list<list<A>>
        return x
      enddef
    endclass

    var f1 = Foo<number>.new()
    assert_equal([[1], [2], [3]], f1.Fn([1], [2], [3]))
    var f2 = Foo<string>.new()
    assert_equal([['a'], ['b'], ['c']], f2.Fn(['a'], ['b'], ['c']))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using func type as a generic object method argument type
def Test_generic_class_func_type_as_argument()
  var lines =<< trim END
    vim9script

    class Foo<A, B, C>
      def Fn(Farg: func(A, B): C): string
        return typename(Farg)
      enddef
    endclass

    def F1(a: number, b: string): blob
      return 0z10
    enddef

    def F2(a: float, b: blob): string
      return 'abc'
    enddef

    var f1 = Foo<number, string, blob>.new()
    assert_equal('func(number, string): blob', f1.Fn(F1))
    var f2 = Foo<float, blob, string>.new()
    assert_equal('func(float, blob): string', f2.Fn(F2))
  END
  v9.CheckSourceSuccess(lines)
enddef

def Test_generic_class_nested_call()
  var lines =<< trim END
    vim9script

    class Foo<A>
      def Fn(n: number, x: A): A
        if n
          return x
        endif

        var f2 = Foo<string>.new()
        assert_equal('abc', f2.Fn(1, 'abc'))

        return x
      enddef
    endclass

    var f = Foo<number>.new()
    assert_equal(10, f.Fn(0, 10))
  END
  v9.CheckSourceSuccess(lines)
enddef

def Test_generic_class_failure_in_def_function()
  var lines =<< trim END
    vim9script

    class Foo<T>
    endclass

    def Fn()
      var f = Foo<abc>.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    def Fn()
      var f = Foo<abc>.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    class Foo<T>
    endclass

    def Fn()
      var f = Foo<number, string>.new()
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, "E1572: Too many types specified for generic class 'Foo'", 1)

  lines =<< trim END
    vim9script

    class Foo<A, B>
    endclass

    def Fn()
      var f = Foo<number>.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1573: Not enough types specified for generic class 'Foo'", 1)

  lines =<< trim END
    vim9script

    class Foo<T>
    endclass

    def Fn()
      var f = Foo<>.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1555: Empty type list specified for generic '<>.new()'", 1)

  lines =<< trim END
    vim9script

    class Foo<A, B>
    endclass

    def Fn()
      var f = Foo.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1570: Type arguments missing for generic class 'Foo'", 1)

  lines =<< trim END
    vim9script

    class Foo
    endclass

    def Fn()
      var f = Foo<number>.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1571: Not a generic class: Foo', 1)

  lines =<< trim END
    vim9script

    class Foo<T>
    endclass

    def Fn()
      var f = Foo<number, >.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1008: Missing <type> after <number, >.new()', 1)

  lines =<< trim END
    vim9script

    class Foo<T, X>
    endclass

    def Fn()
      var f = Foo<number, abc>.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: abc', 1)

  lines =<< trim END
    vim9script

    class Foo<T>
    endclass

    def Fn()
      var f = Foo<number string>.new()
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1202: No white space allowed after 'number'", 1)
enddef

" Test for extending a generic class
def Test_generic_class_extend()
  var lines =<< trim END
    vim9script

    class A<T>
      def Fn(a: T): T
        return a
      enddef
    endclass

    class B extends A<string>
    endclass

    var b = B.new()
    assert_equal('aaa', b.Fn('aaa'))
  END
  v9.CheckSourceSuccess(lines)

  # Try to extend a regular class specifying the types
  lines =<< trim END
    vim9script

    class A
    endclass

    class B extends A<number>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1571: Not a generic class: A', 7)

  # Try to extend a generic class specifying more types
  lines =<< trim END
    vim9script

    class A<T>
    endclass

    class B extends A<number, string>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1572: Too many types specified for generic class 'A'", 7)

  # Try to extend a generic class specifying less types
  lines =<< trim END
    vim9script

    class A<X, Y>
    endclass

    class B extends A<string>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1573: Not enough types specified for generic class 'A'", 7)

  # Try to extend a generic class specifying empty types
  lines =<< trim END
    vim9script

    class A<T>
    endclass

    class B extends A<>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1555: Empty type list specified for generic '<>'", 6)

  # Try to extend a generic class without specifying the types
  lines =<< trim END
    vim9script

    class A<T>
    endclass

    class B extends A
    endclass
  END
  v9.CheckSourceFailure(lines, "E1570: Type arguments missing for generic class 'A'", 7)
enddef

" Test for type checkes with methods in an extended generic class
def Test_generic_class_extend_type_error()
  var lines =<< trim END
    vim9script

    class A<T>
      def Fn(a: T): T
        return a
      enddef
    endclass

    class B extends A<string>
    endclass

    var b = B.new()
    b.Fn(10)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got number', 13)

  lines =<< trim END
    vim9script

    class A<T>
      def Fn(a: T): T
        return a
      enddef
    endclass

    class B extends A<string>
    endclass

    var b = B.new()
    var x: number = b.Fn('abc')
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 13)

  lines =<< trim END
    vim9script

    class A<T>
      def Fn(a: T): T
        return a
      enddef
    endclass

    class B extends A<list<string>>
    endclass

    var b = B.new()
    var x = b.Fn([10, 20])
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected list<string> but got list<number>', 13)
enddef

" Test for generic interface
def Test_generic_interface()
  var lines =<< trim END
    vim9script

    interface A<T>
      def Fn(a: T): T
    endinterface

    class B implements A<string>
      def Fn(a: string): string
        return a
      enddef
    endclass

    var b = B.new()
    assert_equal('aaa', b.Fn('aaa'))
  END
  v9.CheckSourceSuccess(lines)

  # Try to implement a regular interface specifying the types
  lines =<< trim END
    vim9script

    interface A
    endinterface

    class B implements A<number>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1571: Not a generic class: A', 7)

  # Try to extend a generic class specifying more types
  lines =<< trim END
    vim9script

    interface A<T>
    endinterface

    class B implements A<number, string>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1572: Too many types specified for generic class 'A'", 7)

  # Try to extend a generic class specifying less types
  lines =<< trim END
    vim9script

    interface A<X, Y>
    endinterface

    class B implements A<string>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1573: Not enough types specified for generic class 'A'", 7)

  # Try to extend a generic class specifying empty types
  lines =<< trim END
    vim9script

    interface A<T>
    endinterface

    class B implements A<>
    endclass
  END
  v9.CheckSourceFailure(lines, "E1555: Empty type list specified for generic '<>'", 6)

  # Try to extend a generic class without specifying the types
  lines =<< trim END
    vim9script

    interface A<T>
    endinterface

    class B implements A
    endclass
  END
  v9.CheckSourceFailure(lines, "E1570: Type arguments missing for generic class 'A'", 7)
enddef

" Test for passing a generic class object to a def function
def Test_generic_class_arg_to_def_function()
  var lines =<< trim END
    vim9script

    class A<T>
      def Fn(a: T): T
        return a
      enddef
    endclass

    def Foo(t: A<string>): string
      return t.Fn('aaa')
    enddef

    var b = A<string>.new()
    assert_equal('aaa', Foo(b))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    class A<T>
    endclass

    def Foo(t: A<string>)
    enddef

    var b = A<number>.new()
    Foo(b)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected object<A<string>> but got object<A<number>>', 10)
enddef

" Test for creating two objects with different types from a generic class
def Test_generic_class_two_objects()
  var lines =<< trim END
    vim9script

    class A<T>
      def Fn(a: T): T
        var b: T = a
        return b
      enddef
    endclass

    var a1 = A<number>.new()
    var a2 = A<string>.new()
    assert_equal(33, a1.Fn(33))
    assert_equal('abc', a2.Fn('abc'))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for extending a generic class with another generic class
def Test_extending_generic_class()
  var lines =<< trim END
    vim9script

    class A<T>
      def Fn(a: T): string
        return typename(a)
      enddef
    endclass

    class B<U> extends A<U>
    endclass

    var b1 = B<list<string>>.new()
    var b2 = B<dict<string>>.new()
    assert_equal('list<string>', b1.Fn(['a']))
    assert_equal('dict<string>', b2.Fn({a: 'a'}))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a generic class implementing a generic interface
def Test_generic_class_implements_generic_interface()
  var lines =<< trim END
    vim9script

    interface I1<T, U>
      def Fn1(t: T): string
      def Fn2(u: U): string
    endinterface

    class A<X, Y> implements I1<X, Y>
      def Fn1(x: X): string
        return typename(x)
      enddef
      def Fn2(y: Y): string
        return typename(y)
      enddef
    endclass

    var a = A<list<string>, list<blob>>.new()
    assert_equal('list<string>', a.Fn1(['abc']))
    assert_equal('list<blob>', a.Fn2([0z10]))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for checking the signature of builtin functions
def Test_generic_class_builtin_function()
  # string() function
  var lines =<< trim END
    vim9script
    class S
    endclass

    class A<T>
      final _value: T

      def new(this._value)
      enddef

      def string(): T
        return this._value
      enddef
    endclass
    var a = A<S>.new(S.new())
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "string": type mismatch, expected func(): string but got func(): object<S>', 15)

  # len() function
  lines =<< trim END
    vim9script
    class S
    endclass

    class A<T>
      final _value: T

      def new(this._value)
      enddef

      def len(): T
        return this._value
      enddef
    endclass
    var a = A<S>.new(S.new())
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "len": type mismatch, expected func(): number but got func(): object<S>', 15)

  # empty() function
  lines =<< trim END
    vim9script
    class S
    endclass

    class A<T>
      final _value: T

      def new(this._value)
      enddef

      def empty(): T
        return this._value
      enddef
    endclass
    var a = A<S>.new(S.new())
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "empty": type mismatch, expected func(): bool but got func(): object<S>', 15)
enddef

" Test for using a generic class as argument to a function in an if statement
" which is skipped (not " evaluated).
def Test_generic_class_parse_skip_func_args()
  var lines =<< trim END
    vim9script

    class Pair<T, U>
    endclass

    if 0
      echo Pair<number, number>.new(
        Pair<number, number>.new(1, 2),
        Pair<number, number>.new(1, 2))
    endif
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic class recursively as an argument to another generic
" class.
def Test_generic_class_recursive_multiline()
  var lines =<< trim END
    vim9script

    class Pair<T, U>
      var X: T
      var Y: U
    endclass

    assert_equal(2, Pair<Pair<number, number>, Pair<number, number>>.new(
          Pair<number, number>.new(1, 2),
          Pair<number, number>.new(1, 2)).X.Y)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using generic interface argument in a function
" FIXME: Enable this test after addressing the interface clone failure
def FIXME_Test_generic_interface_function_arg()
  var lines =<< trim END
    vim9script

    interface Intf<T>
      def Fn(t: T): T
    endinterface

    class A implements Intf<number>
      def Fn(n: number): number
        return n
      enddef
    endclass

    class B implements Intf<string>
      def Fn(s: string): string
        return s
      enddef
    endclass

    def CheckFn<T>(if: Intf<T>, argT: T)
      assert_equal(argT, if.Fn(argT))
    enddef

    var a = A.new()
    var b = B.new()
    CheckFn<number>(a, 35)
    CheckFn<number>(b, 'abc')
  END
  v9.CheckSourceSuccess(lines)
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
