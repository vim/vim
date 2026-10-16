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

    # Use multi-character type variable names
    class Bar<MyType1, MyType2>
    endclass
    var b = Bar<number, string>.new()
    assert_equal('object<Bar<number, string>>', typename(b))

    # Use a very long type variable name
    class Baz<XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX>
    endclass
    assert_equal('object<Baz<number>>', typename(Baz<number>.new()))
  END
  v9.CheckSourceSuccess(lines)

  # Errors in the type variable list of a generic class
  var errors = [
        ['Foo<t>', 'E1552: Type variable name must start with an uppercase letter: t>'],
        ['Foo<mytype>', 'E1552: Type variable name must start with an uppercase letter: mytype>'],
        ['Foo<>', "E1555: Empty type list specified for generic 'Foo'"],
        ['Foo<T, >', 'E1008: Missing <type> after  >'],
        ['Foo<A, >()', 'E1008: Missing <type> after  >()'],
        ['Foo<,>', 'E1008: Missing <type> after <,>'],
        ['Foo<, A>()', 'E1008: Missing <type> after <, A>()'],
        ['Foo<,A>()', 'E1008: Missing <type> after <,A>()'],
        ['Foo<T', 'E1553: Missing comma after type in generic: T'],
        ['Foo<My-type>', 'E1553: Missing comma after type in generic: My-type>'],
        ['Foo <A>', "E1068: No white space allowed before '<': <A>"],
        ['Foo< A>', "E1202: No white space allowed after '<': < A>"],
        ['Foo< , A>()', "E1202: No white space allowed after '<': < , A>()"],
        ['Foo<A >', "E1202: No white space allowed after 'A': A >"],
        ['Foo<A , B>()', "E1202: No white space allowed after 'A': A , B>()"],
        ['Foo<A, B >()', "E1202: No white space allowed after 'B': B >()"],
        ['Foo<MyType , FooBar>()', "E1202: No white space allowed after 'MyType': MyType , FooBar>()"],
        ['Foo<A,>()', "E1069: White space required after ',': ,>()"],
        ['Foo<A,B>()', "E1069: White space required after ',': ,B>("],
      ]
  for [decl, err] in errors
    v9.CheckSourceFailure(['vim9script', 'class ' .. decl, 'endclass'], err, 2)
  endfor
enddef

" Test for creating an instance of a generic class
def Test_generic_class_instance()
  # Errors in the type arguments when creating an object.  Each one is checked
  # at the script level and in a :def function.  The error can be a list of
  # the script level error and the :def function error.
  var errors = [
        ['Foo<T>', 'Foo<number, number>.new()', "E1590: Too many types specified for generic class 'Foo'"],
        ['Foo<A, B>', 'Foo<number>.new()', "E1591: Not enough types specified for generic class 'Foo'"],
        ['Foo<A>', 'Foo<>.new()', "E1555: Empty type list specified for generic '<>.new()'"],
        ['Foo<A>', 'Foo.new()', "E1588: Type arguments missing for generic class 'Foo'"],
        ['Foo', 'Foo<number>.new()', 'E1589: Not a generic class: Foo'],
        ['Foo<A>', 'Foo<', "E1554: Missing '>' in generic: <"],
        ['Foo<A>', 'Foo<.new()', 'E1008: Missing <type> after <'],
        ['Foo<T>', 'Foo<number, >.new()', 'E1008: Missing <type> after <number, >.new()'],
        ['Foo<T>', 'Foo<abc>.new()', 'E1010: Type not recognized: abc'],
        ['Foo<T, X>', 'Foo<number, abc>.new()', 'E1010: Type not recognized: abc'],
        ['', 'Foo<abc>.new()', ['E121: Undefined variable: Foo', 'E1010: Type not recognized: abc']],
        ['Foo<T>', 'Foo<number:string>.new()', 'E1553: Missing comma after type in generic: <number:string>.new()'],
        ['Foo<A>', 'Foo<number>', 'E1405: Class "Foo<number>" cannot be used as a value'],
        ['Foo<A>', 'Foo<number>.', ['E15: Invalid expression: "Foo<number>."', 'E1127: Missing name after dot']],
        ['Foo<A>', 'Foo <number>.new()', "E1068: No white space allowed before '<':  <number>.new("],
        ['Foo<A>', 'Foo< number>.new()', "E1202: No white space allowed after '<': < number>.new()"],
        ['Foo<T>', 'Foo<number string>.new()', "E1202: No white space allowed after 'number': <number"],
        ['Foo<A>', 'Foo<number >.new()', "E1202: No white space allowed after 'number': <number"],
        ['Foo<A>', 'Foo<number> .new()', "E1202: No white space allowed after '>': <number> .new()"],
        ['Foo<A>', 'Foo<number,>.new()', "E1069: White space required after ',': <number,>.new()"],
        ['Foo<A, B>', 'Foo<number,string>.new()', "E1069: White space required after ',': <number,string>.new()"],
      ]
  for [decl, expr, err] in errors
    var [script_err, def_err] = type(err) == v:t_list ? err : [err, err]
    var head = ['vim9script'] + (decl == '' ? [] : ['class ' .. decl, 'endclass'])
    v9.CheckSourceFailure(head + ['var f = ' .. expr], script_err, len(head) + 1)
    v9.CheckSourceFailure(head + ['def Fn()', '  var f = ' .. expr, 'enddef',
          'defcompile'], def_err, 1)
  endfor

  # An error in the type arguments in an :echo command in a :def function
  var lines = ['vim9script', 'class Foo<A>', 'endclass', 'def Fn()',
        '  echo Foo <number>.new()', 'enddef', 'defcompile']
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '<':  <number>.new()", 1)

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
        var y: A = x
        return y
      enddef
      # Using the generic type as the member of the List return value
      def L(x: A): list<A>
        return [x]
      enddef
      # Using the generic type as the member of the Dict return value
      def D(x: A): dict<A>
        return {v: x}
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

    # Two objects with different types created from the same generic class
    var f5 = Foo<number>.new()
    var f6 = Foo<string>.new()
    assert_equal([33, [1], {v: 1}], [f5.Fn(33), f5.L(1), f5.D(1)])
    assert_equal(['abc', ['abc'], {v: 'abc'}],
          [f6.Fn('abc'), f6.L('abc'), f6.D('abc')])
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
    assert_equal('func(...list<list<string>>): list<list<string>>',
                 typename(f2.Fn))
  END
  v9.CheckSourceSuccess(lines)

  # The varargs type is used in the method type
  lines =<< trim END
    vim9script
    class Foo<A>
      def Fn(n: number, ...x: list<A>)
      enddef
    endclass
    var F: func(number, ...list<string>) = Foo<number>.new().Fn
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(number, ...list<string>) but got func(number, ...list<number>)', 6)
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

" Test for extending a generic class and implementing a generic interface
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

  # Type checks with the methods in an extended generic class
  var errors = [
        ['A<string>', 'b.Fn(10)', 'E1013: Argument 1: type mismatch, expected string but got number'],
        ['A<string>', "var x: number = b.Fn('abc')", 'E1012: Type mismatch; expected number but got string'],
        ['A<list<string>>', 'var x = b.Fn([10, 20])', 'E1013: Argument 1: type mismatch, expected list<string> but got list<number>'],
      ]
  for [parent, cmd, err] in errors
    v9.CheckSourceFailure(lines[: 5] + ['class B extends ' .. parent,
          'endclass', 'var b = B.new()', cmd], err, 10)
  endfor

  # Errors in the type arguments after "extends" and "implements": a regular
  # class/interface with types, more types, fewer types, empty types and no
  # types
  var hdr_errors = [
        ['A', 'A<number>', 'E1589: Not a generic class: A', 5],
        ['A<T>', 'A<number, string>', "E1590: Too many types specified for generic class 'A'", 5],
        ['A<X, Y>', 'A<string>', "E1591: Not enough types specified for generic class 'A'", 5],
        ['A<T>', 'A<>', "E1555: Empty type list specified for generic '<>'", 4],
        ['A<T>', 'A', "E1588: Type arguments missing for generic class 'A'", 5],
      ]
  for [type, keyword] in [['class', 'extends'], ['interface', 'implements']]
    for [decl, ref, err, lnum] in hdr_errors
      lines = ['vim9script', $'{type} {decl}', $'end{type}',
            $'class B {keyword} {ref}', 'endclass']
      v9.CheckSourceFailure(lines, err, lnum)
    endfor
  endfor
enddef

" Test for a generic interface implemented by a regular class and used as a
" function argument type
def Test_generic_interface()
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
    assert_equal('aaa', b.Fn('aaa'))
    CheckFn<number>(a, 35)
    CheckFn<string>(b, 'abc')
  END
  v9.CheckSourceSuccess(lines)

  # Generic class implementing multiple generic interfaces
  lines =<< trim END
    vim9script

    interface Readable<T>
      def Read(): T
    endinterface

    interface Writable<T>
      def Write(val: T)
    endinterface

    class Buffer<T> implements Readable<T>, Writable<T>
      var data: T
      def new(this.data)
      enddef
      def Read(): T
        return this.data
      enddef
      def Write(val: T)
        this.data = val
      enddef
    endclass

    var buf = Buffer<string>.new('initial')
    var reader: Readable<string> = buf
    var writer: Writable<string> = buf
    writer.Write('updated')
    assert_equal('updated', reader.Read())

    def Foo()
      var nbuf = Buffer<number>.new(1)
      var w: Writable<number> = nbuf
      var r: Readable<number> = nbuf
      w.Write(2)
      assert_equal(2, r.Read())
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)
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

" Test for a builtin method (string(), len(), empty()) in a generic class
" returning the type variable with a wrong type
def Test_generic_class_builtin_function()
  for [name, ret_type] in [['string', 'string'], ['len', 'number'], ['empty', 'bool']]
    var lines =<< trim eval END
      vim9script
      class S
      endclass

      class A<T>
        final _value: T

        def new(this._value)
        enddef

        def {name}(): T
          return this._value
        enddef
      endclass
      var a = A<S>.new(S.new())
    END
    v9.CheckSourceFailure(lines, $'E1383: Method "{name}": type mismatch, expected func(): {ret_type} but got func(): object<S>', 15)
  endfor
enddef

" Test for using a generic class as argument to a function in an if statement
" which is skipped (not evaluated).
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

" Test for using a generic class with type variables as the type of a
" function argument, a return value or a variable.
def Test_generic_class_type_with_type_variables()
  # Generic method in a generic class returning a generic class
  var lines =<< trim END
    vim9script

    class Pair<T, U>
      def new()
      enddef

      static def Make<A, B>(X: A, Y: B): Pair<A, B>
        return Pair<A, B>.new()
      enddef

      def Foo(): string
        return "Foo called"
      enddef
    endclass

    var x = Pair<bool, bool>.Make<bool, bool>(false, true)
    assert_equal('object<Pair<bool, bool>>', typename(x))
    assert_equal('Foo called', x.Foo())
  END
  v9.CheckSourceSuccess(lines)

  # Using the class and method type variables in the generic class type
  lines =<< trim END
    vim9script

    class Pair<T, U>
      var a: T
      var b: U
      def new(this.a, this.b)
      enddef

      static def MakeT(x: T, y: U): Pair<T, U>
        return Pair<T, U>.new(x, y)
      enddef

      def Swap(): Pair<U, T>
        return Pair<U, T>.new(this.b, this.a)
      enddef

      def Nest<A>(x: A): Pair<list<A>, Pair<T, A>>
        return Pair<list<A>, Pair<T, A>>.new([x], Pair<T, A>.new(this.a, x))
      enddef

      def First(o: Pair<T, U>): T
        return o.a
      enddef
    endclass

    def Dup<A>(x: A): Pair<A, A>
      return Pair<A, A>.new(x, x)
    enddef

    var p = Pair<number, string>.MakeT(1, 'x')
    assert_equal('object<Pair<number, string>>', typename(p))

    var q = p.Swap()
    assert_equal('object<Pair<string, number>>', typename(q))
    assert_equal(['x', 1], [q.a, q.b])

    var r = p.Nest<bool>(true)
    assert_equal('object<Pair<list<bool>, object<Pair<number, bool>>>>',
          typename(r))
    assert_equal([[true], 1, true], [r.a, r.b.a, r.b.b])

    assert_equal(5, p.First(Pair<number, string>.new(5, 'y')))

    var d = Dup<string>('s')
    assert_equal('object<Pair<string, string>>', typename(d))
  END
  v9.CheckSourceSuccess(lines)

  # Generic class using itself as the type of a variable
  lines =<< trim END
    vim9script

    class Node<T>
      var val: T
      public var next: Node<T>
      def new(this.val)
      enddef
    endclass

    var n = Node<number>.new(1)
    n.next = Node<number>.new(2)
    assert_equal(2, n.next.val)
  END
  v9.CheckSourceSuccess(lines)

  # The type variables are replaced with the correct types
  lines =<< trim END
    vim9script

    class Pair<T, U>
      var a: T
      var b: U
      def new(this.a, this.b)
      enddef

      def Swap(): Pair<U, T>
        return Pair<U, T>.new(this.b, this.a)
      enddef
    endclass

    def Fn(): Pair<number, string>
      return Pair<number, string>.new(1, 'a').Swap()
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<Pair<number, string>> but got object<Pair<string, number>>', 1)

  # Wrong number of type arguments with a type variable
  lines =<< trim END
    vim9script

    class Pair<T, U>
      static def Make<A>(): Pair<A>
        return null_object
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, "E1591: Not enough types specified for generic class 'Pair'", 4)
enddef

" Test for the types of a generic method in a generic class
def Test_generic_method_types_in_generic_class()
  var lines =<< trim END
    vim9script

    class Box<T>
      static def Wrap<A>(x: A): list<A>
        return [x]
      enddef
    endclass

    def Fn()
      var l: list<string> = Box<bool>.Wrap<number>(3)
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<string> but got list<number>', 1)
enddef

" Test for a generic class creating an infinite number of generic classes
def Test_generic_class_nested_too_deep()
  var lines =<< trim END
    vim9script

    class Box<T>
      def Wrap(): Box<list<T>>
        return Box<list<T>>.new()
      enddef
    endclass

    var b = Box<number>.new()
  END
  v9.CheckSourceFailure(lines, "E1592: Generic class 'Box' nested too deep", 9)
enddef

" Test for using a generic class after creating a generic class used in its
" variable types failed.  The class must not be used with unresolved types.
def Test_generic_class_nested_init_failed()
  var lines =<< trim END
    vim9script
    var calls = 0
    def Init(): number
      calls += 1
      if calls == 1
        throw 'init failed'
      endif
      return 1
    enddef
    class Pair<A, B>
      static var s: number = Init()
      var a: A
    endclass
    class Box<T>
      public var p: Pair<T, T>
      def Set(x: T)
        this.p = Pair<T, T>.new(x)
      enddef
    endclass
    var caught = false
    try
      var b = Box<number>.new()
    catch /init failed/
      caught = true
    endtry
    assert_true(caught)

    var b = Box<number>.new()
    b.p = Pair<number, number>.new(1)
    assert_equal(1, b.p.a)
    b.Set(2)
    assert_equal(2, b.p.a)
    assert_equal('object<Pair<number, number>>', typename(b.p))

    def Fn()
      var o = Box<number>.new()
      o.p = Pair<number, number>.new(3)
      assert_equal(3, o.p.a)
      o.Set(4)
      assert_equal(4, o.p.a)
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic class again after creating it failed because it is
" nested too deep.  The same error must be given again.
def Test_generic_class_nested_too_deep_again()
  var lines =<< trim END
    vim9script

    class Box<T>
      def Wrap(): Box<list<T>>
        return Box<list<T>>.new()
      enddef
    endclass

    var caught = 0
    for i in range(2)
      try
        var b = Box<number>.new()
      catch /E1592: Generic class 'Box' nested too deep/
        caught += 1
      endtry
      assert_false(exists('Box<number>'))
    endfor
    assert_equal(2, caught)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using "<" after a class or an object which is not generic
def Test_generic_class_lt_after_non_generic_class()
  var lines =<< trim END
    vim9script
    class Bar
    endclass
    g:o = Bar.new()
    legacy echo g:o<3
  END
  v9.CheckSourceFailure(lines, 'E1437: Can only compare Object with Object', 5)
  unlet g:o
enddef

" Test for calling a variable with a long list of type arguments
def Test_generic_func_call_long_type_args()
  var lines =<< trim END
    vim9script
    var Foo = 10
    def Fn()
      var x = Foo<tuple<TYPES>>()
    enddef
    defcompile Fn
  END
  lines[3] = substitute(lines[3], 'TYPES',
        \ repeat(['number'], 500)->join(', '), '')
  v9.CheckSourceFailure(lines, 'E1085: Not a callable type: Foo', 1)
enddef

" Test for an error after the type parameters of a generic class
def Test_generic_class_error_after_type_params()
  var lines =<< trim END
    vim9script
    class A<T>extends B
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: extends B', 2)

  lines =<< trim END
    vim9script
    class A<T>#comment
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: #comment', 2)

  lines =<< trim END
    vim9script
    class B
    endclass
    class A<T> extends B foo
    endclass
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters: foo', 4)
enddef

" Test for using an object of a generic class after the script defining the
" class is sourced again
def Test_generic_class_object_after_script_reload()
  var lines =<< trim END
    vim9script
    class Pair<A, B>
      var a: A
      var b: B
      def new(this.a, this.b)
      enddef
      def Swap(): Pair<B, A>
        return Pair<B, A>.new(this.b, this.a)
      enddef
      def Get<X>(x: X): Pair<X, B>
        return Pair<X, B>.new(x, this.b)
      enddef
    endclass
    if !exists('g:pair')
      g:pair = Pair<number, string>.new(1, 'x')
      # compile the methods before the script is sourced again
      g:pair.Swap()
      g:pair.Get<bool>(false)
    endif
  END
  writefile(lines, 'XgenericReload.vim', 'D')
  source XgenericReload.vim
  source XgenericReload.vim

  lines =<< trim END
    vim9script
    var o = g:pair
    def Fn()
      var s = o.Swap()
      assert_equal('object<Pair<string, number>>', typename(s))
      assert_equal(['x', 1], [s.a, s.b])
      var g = o.Get<bool>(true)
      assert_equal('object<Pair<bool, string>>', typename(g))
      assert_equal([true, 'x'], [g.a, g.b])
    enddef
    Fn()
    assert_equal('object<Pair<string, number>>', typename(o.Swap()))
  END
  v9.CheckSourceSuccess(lines)
  unlet g:pair

  # A parent class and an interface using the type variables of a generic
  # class that is defined again
  lines =<< trim END
    vim9script
    class A<T>
      var a: T
      def GetA(): T
        return this.a
      enddef
    endclass
    interface I<T>
      def Get(): T
    endinterface
    class B<T> extends A<T> implements I<T>
      def new(this.a)
      enddef
      def Get(): T
        return this.a
      enddef
    endclass
    g:objs->add(B<number>.new(len(g:objs)))
    var i: I<string> = B<string>.new('s')
    assert_equal('s', i.Get())
  END
  writefile(lines, 'XgenericReload2.vim', 'D')
  g:objs = []
  for _ in range(3)
    source XgenericReload2.vim
  endfor
  assert_equal([0, 1, 2], g:objs->mapnew((_, o) => o.GetA()))
  unlet g:objs
enddef

" Test for using a funcref to a generic function after the script defining
" the function is sourced again
def Test_generic_func_funcref_after_script_reload()
  var lines =<< trim END
    vim9script
    def Id<T>(x: T): T
      return x
    enddef
    if !exists('g:IdRef')
      g:IdRef = Id<number>
    endif
  END
  writefile(lines, 'XgenericFuncReload.vim', 'D')
  source XgenericFuncReload.vim
  source XgenericFuncReload.vim
  assert_equal(5, g:IdRef(5))
  assert_equal('func(number): number', typename(g:IdRef))
  unlet g:IdRef
enddef

" Test for using an interface or a parent class to access the variables and
" methods of an object of a generic class
def Test_generic_class_access_through_interface()
  var lines =<< trim END
    vim9script
    interface I
      var a: number
      var b: number
      def Foo(): string
      def Bar(): string
    endinterface
    class C<T> implements I
      var x: T
      var a: number = 1
      var b: number = 2
      def Bar(): string
        return 'Bar'
      enddef
      def Foo(): string
        return 'Foo'
      enddef
    endclass
    def Fn(i: I): list<any>
      return [i.Foo(), i.Bar(), i.a, i.b]
    enddef
    assert_equal(['Foo', 'Bar', 1, 2], Fn(C<string>.new()))
    assert_equal(['Foo', 'Bar', 1, 2], Fn(C<number>.new()))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class P
      var a: number = 1
      var b: number = 2
      def Foo(): string
        return 'P.Foo'
      enddef
      def Bar(): string
        return 'P.Bar'
      enddef
    endclass
    class G<T> extends P
      var x: T
      def Bar(): string
        return 'G.Bar'
      enddef
    endclass
    def Fn(p: P): list<any>
      return [p.Foo(), p.Bar(), p.a, p.b]
    enddef
    assert_equal(['P.Foo', 'G.Bar', 1, 2], Fn(G<string>.new()))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a generic class extending a generic class or implementing a generic
" interface using the type variables
def Test_generic_class_extends_with_type_variables()
  var lines =<< trim END
    vim9script
    class A<T>
      var v: T
      def Set(x: T)
        this.v = x
      enddef
      def Who(): string
        return 'A'
      enddef
      def Type(x: T): string
        return typename(x)
      enddef
    endclass
    class B<U> extends A<U>
      def Who(): string
        return 'B'
      enddef
    endclass
    class C<V> extends A<list<V>>
    endclass

    var b = B<number>.new()
    assert_true(instanceof(b, A<number>))
    assert_false(instanceof(b, A<string>))
    var a: A<number> = b
    assert_equal('B', a.Who())
    def Fn(x: A<number>): string
      return x.Who()
    enddef
    assert_equal('B', Fn(b))
    assert_equal('list<string>', B<list<string>>.new().Type(['a']))
    assert_equal('dict<string>', B<dict<string>>.new().Type({a: 'a'}))

    var c = C<number>.new()
    assert_true(instanceof(c, A<list<number>>))
    assert_equal('list<number>', typename(c.v))
    c.Set([1])
    assert_equal([1], c.v)
  END
  v9.CheckSourceSuccess(lines)

  # Each class extending a generic class uses its own type variables
  lines =<< trim END
    vim9script
    class A<T>
      var v: T
      def Set(x: T)
        this.v = x
      enddef
    endclass
    class B<U> extends A<U>
    endclass
    class C<V> extends A<V>
    endclass
    var c = C<string>.new()
    c.Set(5)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected string but got number', 13)

  # A method overriding a generic method with a different type
  lines =<< trim END
    vim9script
    class A<T>
      def Put(x: T): string
        return 'A'
      enddef
    endclass
    class B<U> extends A<U>
      def Put(x: string): string
        return 'B'
      enddef
    endclass
    var b = B<number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Put": type mismatch, expected func(number): string but got func(string): string', 12)

  lines =<< trim END
    vim9script
    interface I<T>
      def Get(): T
    endinterface
    class A<X> implements I<X>
      var v: X
      def Get(): X
        return this.v
      enddef
    endclass
    var a = A<number>.new(1)
    assert_true(instanceof(a, I<number>))
    assert_false(instanceof(a, I<string>))
    var i: I<number> = a
    assert_equal(1, i.Get())
    def Fn(x: I<number>): number
      return x.Get()
    enddef
    assert_equal(1, Fn(a))

    interface I2<T, U>
      def Fn1(t: T): string
      def Fn2(u: U): string
    endinterface
    class A2<X, Y> implements I2<X, Y>
      def Fn1(x: X): string
        return typename(x)
      enddef
      def Fn2(y: Y): string
        return typename(y)
      enddef
    endclass
    var a2 = A2<list<string>, list<blob>>.new()
    assert_equal(['list<string>', 'list<blob>'], [a2.Fn1(['abc']), a2.Fn2([0z10])])
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using dict<T> in a generic class
def Test_generic_class_dict_type()
  var lines =<< trim END
    vim9script
    class Foo<T>
      def D(d: dict<T>): dict<T>
        return d
      enddef
    endclass
    var a = Foo<number>.new()
    assert_equal('func(dict<number>): dict<number>', typename(a.D))
    a.D({a: 'x'})
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected dict<number> but got dict<string>', 9)
enddef

" Test for class variables in a generic class
def Test_generic_class_static_variables()
  var lines =<< trim END
    vim9script
    class Box<T>
      static var items: list<T> = []
      static var count: number = 0
      static var v: T
      static def Add(x: T)
        items->add(x)
        count += 1
      enddef
    endclass
    Box<number>.Add(1)
    Box<number>.Add(2)
    Box<string>.Add('a')
    assert_equal([[1, 2], 2], [Box<number>.items, Box<number>.count])
    assert_equal([['a'], 1], [Box<string>.items, Box<string>.count])
    assert_equal('list<string>', typename(Box<string>.items))
    assert_equal(0, Box<number>.v)
    assert_equal('', Box<string>.v)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Box<T>
      static var items: list<T> = []
    endclass
    Box<number>.items->add('str')
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 5)

  # The type of the initializer is checked for each concrete class
  lines =<< trim END
    vim9script
    class Box<T>
      static var x: T = 10
    endclass
    assert_equal(10, Box<number>.x)
    var s = Box<string>.x
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "Box<string>.x": type mismatch, expected string but got number', 6)

  lines =<< trim END
    vim9script
    class Box<T>
      static var l: list<T> = [1]
    endclass
    var s = Box<string>.l
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "Box<string>.l": type mismatch, expected list<string> but got list<number>', 5)

  # A failing initializer gives the error each time the class is used
  lines =<< trim END
    vim9script
    class Box<T>
      static var a: number = 1
      static var x: T = Nosuch()
      static var y: number = 2
    endclass
    assert_equal(0, exists('Box<string>'))
    var y = Box<string>.y
  END
  v9.CheckSourceFailure(lines, 'E117: Unknown function: Nosuch', 8)

  # The type variables are not visible in a function called by an initializer
  lines =<< trim END
    vim9script
    def Init(): number
      execute 'var y: T = 1'
      return 1
    enddef
    class Box<T>
      static var l: list<T> = <list<T>>[]
      static var n = Init()
    endclass
    echo Box<string>.n
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: T', 1)
enddef

" Test for a type variable name which is a prefix of another name
def Test_generic_type_variable_name_prefix()
  var lines =<< trim END
    vim9script
    class C<Type>
      var x: T
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: T', 3)

  lines =<< trim END
    vim9script
    class D<TT, T>
      var x: T
      var y: TT
    endclass
    var d = D<number, string>.new()
    assert_equal(['string', 'number'], [typename(d.x), typename(d.y)])
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    def Fn<Type>(a: T): T
      return a
    enddef
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: T', 2)
enddef

" Test for using generic classes with classes which have the same name
def Test_generic_class_type_arg_same_class_name()
  var lines =<< trim END
    vim9script
    export class Foo
      var a: string = 'imported'
    endclass
  END
  writefile(lines, 'XgenericFoo.vim', 'D')

  lines =<< trim END
    vim9script
    import './XgenericFoo.vim' as imp
    class Foo
      var b: number = 42
    endclass
    class Box<T>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    def Id<T>(t: T): T
      return t
    enddef
    var x = Box<Foo>.new(Foo.new())
    var y = Box<imp.Foo>.new(imp.Foo.new())
    assert_equal(42, x.Get().b)
    assert_equal('imported', y.Get().a)
    assert_equal(42, Id<Foo>(Foo.new()).b)
    assert_equal('imported', Id<imp.Foo>(imp.Foo.new()).a)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a generic class in the class definition with concrete types
def Test_generic_class_used_in_own_definition()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
      public var child: Box<number>
      static var inst: Box<number> = Box<number>.new(7)
      def new(this.v)
      enddef
      def Get(): T
        return this.v
      enddef
      def Child(): Box<number>
        return Box<number>.new(3)
      enddef
    endclass
    assert_equal(1, Box<number>.new(1).Get())
    var b = Box<string>.new('x')
    b.child = Box<number>.new(5)
    assert_equal([5, 3, 7], [b.child.Get(), b.Child().Get(),
          Box<string>.inst.Get()])
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for the type variables of a generic class and function when a type
" sources an autoload script
def Test_generic_type_variable_with_autoload()
  mkdir('Xgenericdir/autoload', 'pR')
  var lines =<< trim END
    vim9script
    export class Foo
    endclass
  END
  writefile(lines, 'Xgenericdir/autoload/genericauto.vim')
  var save_rtp = &rtp
  exe 'set rtp^=' .. getcwd() .. '/Xgenericdir'

  lines =<< trim END
    vim9script
    import autoload 'genericauto.vim'
    class C<T>
      def M(x: genericauto.Foo, y: T): T
        return y
      enddef
    endclass
    assert_equal('ok', C<string>.new().M(genericauto.Foo.new(), 'ok'))
  END
  v9.CheckSourceSuccess(lines)

  # The type variables of the function or class being defined are not visible
  # in an autoload script sourced for an argument type.
  lines =<< trim END
    vim9script
    export class Foo
      var x: T
    endclass
  END
  writefile(lines, 'Xgenericdir/autoload/genericauto2.vim')
  lines =<< trim END
    vim9script
    import autoload 'genericauto2.vim'
    def Fn<T>(a: genericauto2.Foo, b: T)
    enddef
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: T', 3)

  lines =<< trim END
    vim9script
    export def Fn(a: T)
    enddef
  END
  writefile(lines, 'Xgenericdir/autoload/genericauto3.vim')
  lines =<< trim END
    vim9script
    import autoload 'genericauto3.vim'
    class C<T>
      def M(F: genericauto3.Fn)
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: T', 2)

  lines =<< trim END
    vim9script
    export class Bar<T>
      var v: T
    endclass
    export class Baz
    endclass
  END
  writefile(lines, 'Xgenericdir/autoload/genericauto4.vim')
  lines =<< trim END
    vim9script
    import autoload 'genericauto4.vim'
    class C<T>
      var b: genericauto4.Baz
    endclass
    assert_equal('object<Bar<number>>',
                 typename(genericauto4.Bar<number>.new(1)))
  END
  v9.CheckSourceSuccess(lines)

  &rtp = save_rtp
enddef

" Test for using a generic class with types as a value in a def function
def Test_generic_class_value_in_def_function()
  var lines =<< trim END
    vim9script
    class Foo<T>
      var v: T
      static var s: number = 9
    endclass
    def Fn()
      assert_equal('class<Foo<number>>', typename(Foo<number>))
      var o = Foo<number>.new(1)
      assert_true(instanceof(o, Foo<number>))
      assert_false(instanceof(o, Foo<string>))
      assert_equal(9, Foo<number>.s)
    enddef
    Fn()
    Fn()
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using an imported generic class in a def function
def Test_generic_class_imported_in_def_function()
  var lines =<< trim END
    vim9script
    export class Foo<T>
      var v: T
      static var s: number = 5
      def new(this.v)
      enddef
    endclass
  END
  writefile(lines, 'XgenericImport.vim', 'D')

  lines =<< trim END
    vim9script
    import './XgenericImport.vim' as x
    def Fn()
      var f = x.Foo<number>.new(3)
      assert_equal('object<Foo<number>>', typename(f))
      assert_equal(3, f.v)
      assert_equal(5, x.Foo<string>.s)
      assert_equal('class<Foo<string>>', typename(x.Foo<string>))
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    import './XgenericImport.vim' as x
    def Fn()
      var f = x.Foo.new(3)
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, "E1588: Type arguments missing for generic class 'Foo'", 1)
enddef

" Test for assigning to a class variable of a generic class
def Test_generic_class_assign_class_variable()
  var lines =<< trim END
    vim9script
    class Foo<T>
      public static var s: number = 1
      public static var t: T
    endclass
    Foo<number>.s = 5
    Foo<string>.s = 6
    Foo<number>.s += 10
    Foo<string>.t = 'abc'
    assert_equal([15, 6, 'abc'], [Foo<number>.s, Foo<string>.s, Foo<string>.t])
    def Fn()
      Foo<number>.s = 3
      Foo<number>.s += 1
      Foo<string>.t = 'def'
    enddef
    Fn()
    assert_equal([4, 6, 'def'], [Foo<number>.s, Foo<string>.s, Foo<string>.t])
  END
  v9.CheckSourceSuccess(lines)

  # Assign to an item of a class variable
  lines =<< trim END
    vim9script
    class Foo<T>
      public static var l: list<T> = []
      public static var d: dict<list<T>> = {}
    endclass
    Foo<number>.l = [1, 2]
    Foo<number>.l[0] = 5
    Foo<number>.d.k = [3]
    assert_equal([[5, 2], {k: [3]}], [Foo<number>.l, Foo<number>.d])
    def Fn()
      Foo<number>.l[0] = 6
      Foo<number>.l[1] += 10
      var x: number
      [x, Foo<number>.l[0]] = [3, 4]
      Foo<number>.d.k = [7]
      Foo<number>.d["k"][0] *= 2
      Foo<string>.d.s = ['a']
    enddef
    Fn()
    assert_equal([[4, 12], {k: [14]}], [Foo<number>.l, Foo<number>.d])
    assert_equal({s: ['a']}, Foo<string>.d)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Foo<T>
      public static var l: list<T> = [1]
    endclass
    def Fn()
      Foo<number>.l[0] = 'a'
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 1)

  # Index on the class, same error at the script level and in a def function
  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    Foo<number>[0] = 1
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters: <number>[0] = 1', 4)
  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    def Fn()
      Foo<number>[0] = 1
    enddef
    defcompile Fn
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters: <number>[0] = 1', 1)

  # Assignment in a skipped block
  lines =<< trim END
    vim9script
    class Foo<T>
      public static var s: number = 1
      public static var l: list<T> = [1]
    endclass
    def Fn()
      if false
        Foo<number>.s = 2
        Foo<number>.s += 1
        Foo<number>.l[0] = 3
      endif
    enddef
    Fn()
    assert_equal([1, [1]], [Foo<number>.s, Foo<number>.l])
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for string() of an object of a generic class
def Test_generic_class_object_string()
  var lines =<< trim END
    vim9script
    class C<T>
      var x: number = 3
      var y: T
    endclass
    assert_equal("object of C<string> {x: 3, y: ''}", string(C<string>.new()))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for exists() with a generic class
def Test_generic_class_exists()
  var lines =<< trim END
    vim9script
    class C<T>
    endclass
    assert_equal(1, exists('C'))
    assert_equal(1, exists('C<number>'))
    assert_equal(1, exists('C<number>.new'))
    assert_equal(0, exists('C<nosuch>'))
    assert_equal(0, exists('C.new'))
  END
  v9.CheckSourceSuccess(lines)

  # exists() with an imported generic class does not give an error
  lines =<< trim END
    vim9script
    export class C<T>
      static var sv: number = 1
    endclass
  END
  writefile(lines, 'Xgenericexists.vim', 'D')
  lines =<< trim END
    vim9script
    import './Xgenericexists.vim' as m
    assert_equal(1, exists('m.C<number>'))
    assert_equal(1, exists('m.C<number>.sv'))
    assert_equal(0, exists('m.C<nosuch>'))
    assert_equal(0, exists('m.C<number, string>'))
    assert_equal(0, exists('m.C.sv'))
    assert_equal(1, exists('m.C'))
  END
  v9.CheckSourceSuccess(lines)

  # exists() for a method does not give an error
  lines =<< trim END
    vim9script
    class Box<T>
      static def SGet(): number
        return 1
      enddef
    endclass
    assert_equal(0, exists('*Box.SGet'))
    assert_equal(0, exists('*Box<xyz>.SGet'))
    def Fn()
      assert_equal(0, exists('*Box.SGet'))
      assert_equal(0, exists('*Box<xyz>.SGet'))
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for the errors for an imported generic class, these are the same as
" for a generic class in the script and in a def function
def Test_generic_class_imported_errors()
  var lines =<< trim END
    vim9script
    export class Box<T>
      public static var sv: number = 1
    endclass
    export class Plain
      public static var sv: number = 2
    endclass
  END
  writefile(lines, 'XgenericImpErrors.vim', 'D')

  var errors = [
        ['typename(PBox)', "E1588: Type arguments missing for generic class 'Box'"],
        ['PBox <number>.sv', "E1068: No white space allowed before '<': "],
        ['PPlain<number>.sv', 'E1589: Not a generic class: Plain'],
      ]
  for [expr, err] in errors
    for prefix in ['', 'm.']
      var head = ['vim9script', "import './XgenericImpErrors.vim' as m",
            'class Box<T>', '  public static var sv: number = 1', 'endclass',
            'class Plain', '  public static var sv: number = 2', 'endclass']
      var e = substitute(expr, 'P', prefix, '')
      v9.CheckSourceFailure(head + ['echo ' .. e], err, 9)
      v9.CheckSourceFailure(head + ['def Fn()', '  echo ' .. e, 'enddef',
            'defcompile Fn'], err, 1)
    endfor
  endfor

  lines =<< trim END
    vim9script
    import './XgenericImpErrors.vim' as m
    assert_equal(1, instanceof(m.Box<number>.new(), m.Box<number>))
    echo instanceof(m.Box<number>.new(), m.Box)
  END
  v9.CheckSourceFailure(lines, "E1588: Type arguments missing for generic class 'Box'", 4)
enddef

" Test for sourcing the script defining a generic class again while a class
" is created from it: the class variable initializer sources the script
def Test_generic_class_script_sourced_while_creating()
  var lines =<< trim END
    vim9script
    if !exists('g:n')
      g:n = 0
      def g:Side(): number
        g:n += 1
        if g:n == 2
          source XgenericResource.vim
        endif
        return 1
      enddef
      # compiling this function creates "Box<string>"
      def g:Use(): number
        return Box<string>.x
      enddef
    endif
    class Box<T>
      static var x: number = g:Side()
      public static var y: number
      var v: T
    endclass
    if g:n == 0
      echo Box<number>.x
      execute g:cmd
    endif
  END
  writefile(lines, 'XgenericResource.vim', 'D')

  var cmds = [
        'echo Box<string>.x',
        'Box<string>.y = 2',
        'defcompile Box<string>',
        'echo g:Use()',
        'var o = Box<string>.new("a")',
      ]
  for cmd in cmds
    g:cmd = cmd
    assert_fails('source XgenericResource.vim', "E1595: Generic class 'Box' was deleted while creating a class from it", cmd)
    unlet g:n
    delfunc g:Side
    delfunc g:Use
  endfor

  # exists() does not give an error
  g:cmd = 'g:result = exists("Box<string>")'
  source XgenericResource.vim
  assert_equal(0, g:result)
  unlet g:n g:cmd g:result
  delfunc g:Side
  delfunc g:Use
enddef

" Test for an error in the implemented interfaces with more interfaces than
" have been validated
def Test_generic_class_implements_error_many_intfs()
  for name in ['C', 'C<T>']
    var lines =<< trim eval END
      vim9script
      interface I1
      endinterface
      class {name} implements I1, Nope, I3, I4, I5, I6, I7, I8, I9
      endclass
    END
    v9.CheckSourceFailure(lines, 'E1346: Interface name not found: Nope', 5)
  endfor
enddef

" Test for a deeply nested generic class type
def Test_generic_class_type_nested_too_deep()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
    endclass
    var x: TYPE
    echo 'done'
  END
  lines[4] = 'var x: ' .. repeat('Box<', 900) .. 'number' .. repeat('>', 900)
  v9.CheckSourceSuccess(lines)

  lines[4] = 'var x: ' .. repeat('Box<', 1001) .. 'number' .. repeat('>', 1001)
  v9.CheckSourceFailure(lines, 'E1596: Type nested too deep', 5)

  lines[4] = 'var x = ' .. repeat('Box<', 1001) .. 'number' .. repeat('>', 1001) .. '.new()'
  v9.CheckSourceFailure(lines, 'E1596: Type nested too deep', 5)

  lines[4] = 'def Fn(): ' .. repeat('Box<', 1001) .. 'number' .. repeat('>', 1001)
  lines[5] = 'enddef'
  v9.CheckSourceFailure(lines, 'E1596: Type nested too deep', 5)
enddef

" Test for using a generic class from an autoload script without type
" arguments in a def function compiled before the script is loaded
def Test_generic_class_autoload_without_type_args()
  mkdir('Xgenautodir/autoload', 'pR')
  var lines =<< trim END
    vim9script
    export class Box<T>
      public static var x: list<T> = [1]
      var v: T
      def new(this.v)
      enddef
      def Get(): T
        return this.v
      enddef
    endclass
  END
  var save_rtp = &rtp
  exe 'set rtp^=' .. getcwd() .. '/Xgenautodir'

  # Use a new script for each command, it must not be loaded yet.
  var n = 0
  for imp in ['import autoload "genautolibN.vim" as L',
              'import autoload "./Xgenautodir/genautorelN.vim" as L']
    for cmd in ['var o = L.Box.new(1)', 'echo L.Box.x', "L.Box.x = ['str']"]
      n += 1
      writefile(lines, $'Xgenautodir/autoload/genautolib{n}.vim')
      writefile(lines, $'Xgenautodir/genautorel{n}.vim')
      var src = ['vim9script', substitute(imp, 'N', n, ''), 'def Fn()',
            '  ' .. cmd, 'enddef', 'Fn()']
      v9.CheckSourceFailure(src, "E1588: Type arguments missing for generic class 'Box'", 1)
    endfor

    # After the script is loaded the class with types can be used
    var src = ['vim9script', substitute(imp, 'N', n, ''), 'def Fn()',
          '  assert_equal(2, L.Box<number>.new(2).Get())',
          '  assert_equal([1], L.Box<number>.x)', 'enddef', 'Fn()']
    v9.CheckSourceSuccess(src)
  endfor

  &rtp = save_rtp
enddef

" Test for using a generic class while it is being defined, from an
" initializer of the parent class
def Test_generic_class_used_while_defined()
  var lines =<< trim END
    vim9script
    class P<T>
      var p: T
      static var s: number = g:Cb()
    endclass
    def g:Cb(): number
      try
        exe 'g:during = Foo<string>.new(1, "a")'
      catch
        g:during = v:exception
      endtry
      return 1
    enddef
    class Foo<T> extends P<number>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    assert_match("E1597: Generic class 'Foo' is not completely defined", g:during)
    # After the definition the class can be used
    assert_equal('a', Foo<string>.new(1, 'a').Get())
    assert_equal(2, Foo<number>.new(1, 2).Get())
  END
  v9.CheckSourceSuccess(lines)
  unlet g:during
  delfunc g:Cb
enddef

" Test for defining a class or a generic class again
def Test_generic_class_redefine()
  var lines =<< trim END
    vim9script
    class A<T>
    endclass
    class A<T>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "A"', 4)

  lines =<< trim END
    vim9script
    class N
    endclass
    class N
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "N"', 4)
enddef

" Test for an error in the type arguments given only once, and for skipping
" type arguments
def Test_generic_class_type_args_error_once()
  var lines =<< trim END
    vim9script
    class Box<T>
    endclass
    echo Box<>.new()
  END
  v9.CheckSourceFailureList(lines, ["E1555: Empty type list specified for generic '<>.new()'", "E1555: Empty type list specified for generic '<>.new()'"], 4)

  lines =<< trim END
    vim9script
    def Fn<T>()
    enddef
    var F = Fn<>
  END
  v9.CheckSourceFailureList(lines, ["E1555: Empty type list specified for generic '<>'", "E1555: Empty type list specified for generic '<>'"], 4)

  # Type arguments are only skipped after a class or function name
  lines =<< trim END
    vim9script
    class Box<T>
      static var sv: number = 3
    endclass
    def F<T>(x: T): T
      return x
    enddef
    assert_false(false && Box<number>.sv)
    assert_false(false && F<number>(1))
    var x = 1
    echo false && x<number>
  END
  v9.CheckSourceFailure(lines, 'E15: Invalid expression: ">"', 11)
enddef

" Test for a generic enum
def Test_generic_enum()
  var lines =<< trim END
    vim9script
    enum E<T>
      A
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1593: Enum cannot be generic: E', 2)
enddef

" Test for :defcompile with generic functions and classes
def Test_generic_class_defcompile()
  var lines =<< trim END
    vim9script
    class Base
    endclass
    class Pair<A, B> extends Base
      var a: A
      var b: B
    endclass
    def TakeBase(p: Base): string
      return 'base'
    enddef
    def Take<T>(p: Pair<T, number>): string
      return typename(p)
    enddef
    def G<T>(x: T): string
      return Take<T>(Pair<T, number>.new(x, 1))
    enddef
    def H<T>(p: Pair<T, string>): string
      return TakeBase(p)
    enddef
    def Same<T>(p: Pair<T, number>): Pair<T, number>
      var q: Pair<T, number> = p
      return q
    enddef
    class Box<T>
      var v: T
      def M(): string
        return Take<T>(Pair<T, number>.new(this.v, 1))
      enddef
    endclass
    defcompile
    assert_equal('object<Pair<string, number>>', G<string>('a'))
    assert_equal('base', H<number>(Pair<number, string>.new(1, 'a')))
    assert_equal('object<Pair<number, number>>', Box<number>.new(1).M())
  END
  v9.CheckSourceSuccess(lines)

  # Compile a method of a class created from a generic class
  lines =<< trim END
    vim9script
    class Foo<T>
      var v: T
      static def S(): number
        return 1
      enddef
      def M(): T
        return this.v
      enddef
    endclass
    defcompile Foo<number>.S
    defcompile Foo<number>.M
    assert_equal([1, 2], [Foo<number>.S(), Foo<number>.new(2).M()])
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Foo<T>
      static def S(): number
        return 1
      enddef
    endclass
    defcompile Foo<number>.Nosuch
  END
  v9.CheckSourceFailure(lines, 'E1337: Class variable "Nosuch" not found in class "Foo<number>"', 7)
enddef

" Test for using a generic function in the initializer of a class variable
def Test_generic_func_in_class_var_init()
  var lines =<< trim END
    vim9script
    def Id<T>(x: T): T
      var y: T = x
      return y
    enddef
    class A
      static var s = Id<number>(3)
    endclass
    class B<T>
      static var s = Id<string>('a')
    endclass
    assert_equal(3, A.s)
    assert_equal('a', B<number>.s)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for :defcompile with the name of a generic class
def Test_generic_class_name_reused()
  var lines =<< trim END
    vim9script
    class Box<X>
      def F(): number
        return 1
      enddef
    endclass
    defcompile Box
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for checking a generic class against the interfaces it implements
def Test_generic_class_implements_type_check()
  var lines =<< trim END
    vim9script
    interface I
      def Get(): number
    endinterface
    class A<T> implements I
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    var a: I = A<number>.new(1)
    assert_equal(1, a.Get())
    var b = A<string>.new('s')
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Get": type mismatch, expected func(): number but got func(): string', 13)

  lines =<< trim END
    vim9script
    interface I<X>
      var v: X
    endinterface
    class A<T> implements I<number>
      var v: T
    endclass
    var a = A<string>.new('x')
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "v": type mismatch, expected number but got string', 8)

  lines =<< trim END
    vim9script
    interface I<X>
      def Get(): X
    endinterface
    class B<T> implements I<list<T>>
      var v: list<T>
      def Get(): list<T>
        return this.v
      enddef
    endclass
    var i: I<list<number>> = B<number>.new([3])
    assert_equal([3], i.Get())
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a generic class implementing a generic interface method with a
" method that has a different number of type variables
def Test_generic_class_implements_method_type_vars()
  var lines =<< trim END
    vim9script
    interface J<T>
      def Get<U>(x: U): T
    endinterface
    class D<T> implements J<T>
      def Get(x: number): T
        return null_object
      enddef
    endclass
    var d = D<number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1598: Number of type variables of method "Get" differs from interface "J<T>"', 9)

  lines =<< trim END
    vim9script
    interface J<T>
      def Get<U>(x: U): T
    endinterface
    class D<T> implements J<T>
      var v: T
      def Get<X>(x: X): T
        return this.v
      enddef
    endclass
    var j: J<number> = D<number>.new(5)
    assert_equal(5, j.Get<string>('a'))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for calling a generic method of a generic interface with type arguments
" through the interface.  The method of the object class is called with the
" same type arguments.
def Test_generic_interface_generic_method_call()
  var lines =<< trim END
    vim9script
    class Pair<A, B>
      var a: A
      var b: B
    endclass
    interface I<T>
      def Map<U>(F: func(T): U): list<U>
    endinterface
    interface J<T> extends I<T>
      def With<U>(u: U): Pair<T, U>
    endinterface

    # generic class
    class Box<T> implements J<T>
      var v: T
      def Map<U>(F: func(T): U): list<U>
        return [F(this.v)]
      enddef
      def With<U>(u: U): Pair<T, U>
        return Pair<T, U>.new(this.v, u)
      enddef
    endclass
    # class that is not generic, with other type variable names
    class Num implements J<number>
      var n: number
      def Map<X>(F: func(number): X): list<X>
        return [F(this.n), F(this.n + 1)]
      enddef
      def With<Y>(u: Y): Pair<number, Y>
        return Pair<number, Y>.new(-this.n, u)
      enddef
    endclass
    # overriding method
    class Sub extends Num
      def Map<X>(F: func(number): X): list<X>
        return [F(100)]
      enddef
    endclass
    # method inherited from a generic class
    class Base<T>
      var v: T
      def Map<U>(F: func(T): U): list<U>
        return [F(this.v), F(this.v)]
      enddef
    endclass
    class Derived<T> extends Base<T> implements I<T>
    endclass

    def UseI(i: I<number>): list<any>
      var s: list<string> = i.Map<string>((n: number): string => 'x' .. n)
      var b = i.Map<bool>((n: number): bool => n > 1)
      return [s, b, typename(b)]
    enddef
    def UseJ(j: J<number>): list<any>
      var p = j.With<string>('s')
      var q: Pair<number, list<bool>> = j.With<list<bool>>([true])
      return [typename(p), p.a, p.b, q.b,
            j.Map<number>((n: number): number => n * 2)]
    enddef

    assert_equal([['x1'], [false], 'list<bool>'], UseI(Box<number>.new(1)))
    assert_equal([['x2', 'x3'], [true, true], 'list<bool>'],
          UseI(Num.new(2)))
    assert_equal([['x100'], [true], 'list<bool>'], UseI(Sub.new(3)))
    assert_equal([['x4', 'x4'], [true, true], 'list<bool>'],
          UseI(Derived<number>.new(4)))

    assert_equal(['object<Pair<number, string>>', 1, 's', [true], [2]],
          UseJ(Box<number>.new(1)))
    assert_equal(['object<Pair<number, string>>', -2, 's', [true], [4, 6]],
          UseJ(Num.new(2)))
    assert_equal(['object<Pair<number, string>>', -3, 's', [true], [200]],
          UseJ(Sub.new(3)))

    # at the script level
    var i: I<number> = Box<number>.new(7)
    assert_equal(['y7'], i.Map<string>((n: number): string => 'y' .. n))
    var js: J<string> = Box<string>.new('z')
    assert_equal('object<Pair<string, number>>', typename(js.With<number>(1)))
    assert_equal([1], js.Map<number>((s: string): number => len(s)))
  END
  v9.CheckSourceSuccess(lines)

  # The return type uses the type arguments of the call
  lines =<< trim END
    vim9script
    interface I<T>
      def Conv<U>(x: T): U
    endinterface
    class A<T> implements I<T>
      def Conv<U>(x: T): U
        return x
      enddef
    endclass
    def Fn(i: I<any>): string
      return i.Conv<string>(5)
    enddef
    assert_equal('a', A<any>.new().Conv<string>('a'))
    Fn(A<any>.new())
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected string but got number', 1)

  # Missing type arguments or too many
  lines =<< trim END
    vim9script
    interface I<T>
      def Map<U>(F: func(T): U): list<U>
    endinterface
    def Fn(i: I<number>): any
      return i.Map((n: number): number => n)
    enddef
    defcompile Fn
  END
  v9.CheckSourceFailure(lines, "E1559: Type arguments missing for generic function 'Map'", 1)

  lines =<< trim END
    vim9script
    interface I<T>
      def Map<U>(F: func(T): U): list<U>
    endinterface
    def Fn(i: I<number>): any
      return i.Map<number, string>((n: number): number => n)
    enddef
    defcompile Fn
  END
  v9.CheckSourceFailure(lines, "E1556: Too many types specified for generic function 'Map'", 1)
enddef

" Test for a method returning the implementing class where the interface or
" the parent class method returns the interface or parent class type
def Test_generic_class_method_returns_own_class()
  var lines =<< trim END
    vim9script

    interface Copiable<T>
      var value: T
      def Copy(): Copiable<T>
    endinterface

    class NumRef implements Copiable<number>
      const value: number
      def new(this.value)
      enddef
      def Copy(): NumRef
        return NumRef.new(this.value)
      enddef
    endclass

    class StrRef implements Copiable<string>
      const value: string
      def new(this.value)
      enddef
      def Copy(): Copiable<string>
        return StrRef.new(this.value)
      enddef
    endclass

    class Box<T> implements Copiable<T>
      var value: T
      def new(this.value)
      enddef
      def Copy(): Box<T>
        return Box<T>.new(this.value)
      enddef
    endclass

    assert_equal(5, NumRef.new(5).Copy().value)
    assert_equal('a', StrRef.new('a').Copy().value)
    var c: Copiable<number> = Box<number>.new(3).Copy()
    assert_equal(3, c.value)
    assert_equal('object<Box<number>>', typename(c))

    def Foo()
      var n: Copiable<number> = NumRef.new(7).Copy()
      assert_equal(7, n.value)
      var s: Copiable<string> = Box<string>.new('x').Copy()
      assert_equal('x', s.value)
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)

  # Overriding a method of a generic parent class
  lines =<< trim END
    vim9script
    class Base<T>
      def Get(): Base<T>
        return this
      enddef
    endclass
    class Derived<T> extends Base<T>
      def Get(): Derived<T>
        return this
      enddef
    endclass
    var b: Base<number> = Derived<number>.new().Get()
    assert_equal('object<Derived<number>>', typename(b))
  END
  v9.CheckSourceSuccess(lines)

  # Return type doesn't match the interface type
  lines =<< trim END
    vim9script
    interface Copiable<T>
      def Copy(): Copiable<T>
    endinterface
    class NumRef implements Copiable<number>
      def Copy(): Copiable<string>
        return null_object
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Copy": type mismatch, expected func(): object<Copiable<number>> but got func(): object<Copiable<string>>', 9)

  lines =<< trim END
    vim9script
    interface Copiable<T>
      def Copy(): Copiable<T>
    endinterface
    class Other<T>
    endclass
    class Box<T> implements Copiable<T>
      def Copy(): Other<T>
        return Other<T>.new()
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Copy": type mismatch, expected func(): object<Copiable<any>> but got func(): object<Other<any>>', 11)

  # The return type matches only for some of the type arguments.  The error
  # is given each time the class is used.
  lines =<< trim END
    vim9script
    interface Copiable<T>
      def Copy(): Copiable<T>
    endinterface
    class Box<T> implements Copiable<T>
      def Copy(): Box<number>
        return Box<number>.new()
      enddef
    endclass
    var a = Box<number>.new()
    var b = Box<string>.new()
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Copy": type mismatch, expected func(): object<Copiable<string>> but got func(): object<Box<number>>', 11)

  lines =<< trim END
    vim9script
    interface Copiable<T>
      def Copy(): Copiable<T>
    endinterface
    class Box<T> implements Copiable<T>
      def Copy(): Box<number>
        return Box<number>.new()
      enddef
    endclass
    assert_equal(1, exists('Box<number>'))
    assert_equal(0, exists('Box<string>'))
    def Foo()
      var b = Box<string>.new()
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Copy": type mismatch, expected func(): object<Copiable<string>> but got func(): object<Box<number>>', 1)

  lines =<< trim END
    vim9script
    class Base<T>
      def Get(): Base<T>
        return this
      enddef
    endclass
    class Derived<T> extends Base<T>
      def Get(): Derived<number>
        return Derived<number>.new()
      enddef
    endclass
    var a = Derived<number>.new()
    var b = Derived<string>.new()
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Get": type mismatch, expected func(): object<Base<string>> but got func(): object<Derived<number>>', 13)
enddef

" Test for the type variables of a generic parent class used in an inherited
" method or variable initializer
def Test_generic_class_inherited_type_vars()
  var lines =<< trim END
    vim9script
    class A<T>
      var v: T
      var l: list<T> = <list<T>>[]
      def M(): string
        var x: T = this.v
        return typename(x)
      enddef
    endclass
    class B extends A<number>
      def new(this.v)
      enddef
    endclass
    class C<T> extends A<number>
      def new(this.v)
      enddef
    endclass
    class D<U> extends A<list<U>>
      def new(this.v)
      enddef
    endclass
    assert_equal(['number', 'list<number>'], [B.new(3).M(), typename(B.new(3).l)])
    assert_equal(['number', 'list<number>'],
                 [C<string>.new(1).M(), typename(C<string>.new(1).l)])
    assert_equal(['list<string>', 'list<list<string>>'],
                 [D<string>.new(['a']).M(), typename(D<string>.new(['a']).l)])
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a generic class implementing a generic interface with the class
" itself as the type argument
def Test_generic_class_implements_itself_as_type_arg()
  var lines =<< trim END
    vim9script
    interface Cmp<T>
      def Compare(other: T): number
    endinterface
    class Foo<T> implements Cmp<Foo<T>>
      var v: T
      def new(this.v)
      enddef
      def Compare(other: Foo<T>): number
        return this.v == other.v ? 0 : this.v < other.v ? -1 : 1
      enddef
    endclass
    var a = Foo<number>.new(1)
    var b = Foo<string>.new('b')
    var c: Cmp<Foo<number>> = a
    assert_equal(-1, c.Compare(Foo<number>.new(2)))
    assert_equal(1, b.Compare(Foo<string>.new('a')))
    assert_equal(['object<Foo<number>>', 'object<Foo<string>>'],
                 [typename(a), typename(b)])
    def Fn()
      var d: Cmp<Foo<string>> = Foo<string>.new('x')
      assert_equal(0, d.Compare(Foo<string>.new('x')))
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    interface Cmp<T>
      def Compare(other: T): number
    endinterface
    class Foo<T> implements Cmp<Foo<T>>
      def Compare(other: Foo<T>): number
        return 0
      enddef
    endclass
    var c: Cmp<Foo<number>> = Foo<number>.new()
    c.Compare(Foo<string>.new())
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected object<Foo<number>> but got object<Foo<string>>', 11)
enddef

" Test for using a generic class in a nested generic function
def Test_generic_class_in_nested_generic_func()
  var lines =<< trim END
    vim9script
    class Foo<T>
      var v: T
    endclass
    def Outer(): list<any>
      var base = 10
      var res = []
      def Inner<T>(y: T): Foo<T>
        res->add(base)
        var l: list<T> = [y]
        res->add(typename(l))
        return Foo<T>.new(y)
      enddef
      res->add(typename(Inner<bool>(true)))
      base = 20
      res->add(Inner<string>('s').v)
      return res
    enddef
    assert_equal([10, 'list<bool>', 'object<Foo<bool>>', 20, 'list<string>',
          's'], Outer())
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for :lockvar of a class variable in a generic class
def Test_generic_class_lockvar_class_variable()
  var lines =<< trim END
    vim9script
    class Box<T>
      public static var count: number = 0
      static def Lock()
        lockvar count
      enddef
    endclass
    Box<number>.Lock()
  END
  v9.CheckSourceFailure(lines, 'E1392: Cannot (un)lock class variable "count" in class "Box<number>"', 1)

  # Same error at the script level and in a def function
  var cmds = [
        ['lockvar Box<number>.count', 'E1392: Cannot (un)lock class variable "Box<number>.count" in class "Box<number>"'],
        ['unlet Box<number>.count', 'E1260: Cannot unlet an imported item: Box<number>.count'],
      ]
  for [cmd, err] in cmds
    lines =<< trim eval END
      vim9script
      class Box<T>
        public static var count: number = 0
      endclass
      {cmd}
    END
    v9.CheckSourceFailure(lines, err, 5)

    lines =<< trim eval END
      vim9script
      class Box<T>
        public static var count: number = 0
      endclass
      def Fn()
        {cmd}
      enddef
      Fn()
    END
    v9.CheckSourceFailure(lines, err, 1)
  endfor

  lines =<< trim END
    vim9script
    class Box<T>
      public static var d: dict<number> = {a: 1, b: 2}
    endclass
    unlet Box<number>.d.a
    def Fn()
      unlet Box<number>.d.b
      if false
        lockvar Box<number>.d
        unlet Box<number>.d
      endif
    enddef
    Fn()
    if false
      lockvar Box<number>.d
    endif
    assert_equal({}, Box<number>.d)
  END
  v9.CheckSourceSuccess(lines)

  # The type variables of the class used in a method
  lines =<< trim END
    vim9script
    class Box<T>
      public static var d: dict<list<T>> = {x: []}
      static def Lock()
        lockvar Box<T>.d.x
      enddef
      static def Unlock()
        unlockvar Box<T>.d.x
      enddef
    endclass
    Box<number>.Lock()
    assert_equal([1, 0], [islocked('Box<number>.d.x'), islocked('Box<string>.d.x')])
    Box<number>.Unlock()
    assert_equal(0, islocked('Box<number>.d.x'))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using protected variables and methods of another class created
" from the same generic class
def Test_generic_class_protected_other_type()
  var lines =<< trim END
    vim9script
    class Box<T>
      var _v: T
      static var _s: number = 7
      def new(this._v)
      enddef
      def _P(): T
        return this._v
      enddef
      static def _SP(): number
        return 8
      enddef
      def Zip<U>(o: Box<U>): tuple<T, U>
        return (this._v, o._v)
      enddef
      def Other(): list<any>
        var o = Box<string>.new('x')
        return [o._P(), Box<string>._s, Box<string>._SP()]
      enddef
    endclass
    assert_equal((1, 's'), Box<number>.new(1).Zip<string>(Box<string>.new('s')))
    assert_equal(['x', 7, 8], Box<number>.new(1).Other())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Box<T>
      var _v: T
      def new(this._v)
      enddef
    endclass
    class Other
      def Peek(): string
        return Box<string>.new('z')._v
      enddef
    endclass
    Other.new().Peek()
  END
  v9.CheckSourceFailure(lines, 'E1333: Cannot access protected variable "_v" in class "Box<string>"', 1)
enddef

" Test for a generic class extending itself
def Test_generic_class_extends_itself()
  var lines =<< trim END
    vim9script
    class A<T> extends A<number>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend A<number>', 3)

  lines =<< trim END
    vim9script
    interface I<T> extends I<string>
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend I<string>', 3)
enddef

" Test for extending or implementing a generic class or interface of the wrong
" kind.  The kind is checked before a class is created with the type
" arguments, thus the type arguments are not checked.
def Test_generic_class_extends_implements_wrong_kind()
  var lines =<< trim END
    vim9script
    class A<X>
    endclass
    class B<T> implements A<T, T>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1347: Not a valid interface: A<T, T>', 5)

  lines =<< trim END
    vim9script
    interface I<X>
    endinterface
    class B<T> extends I<T, T>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend I<T, T>', 5)

  lines =<< trim END
    vim9script
    class A<X>
    endclass
    interface J<T> extends A<T, T>
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend A<T, T>', 5)

  # With the right kind the type arguments are checked
  lines =<< trim END
    vim9script
    interface I<X>
    endinterface
    class B<T> implements I<T, T>
    endclass
  END
  v9.CheckSourceFailure(lines,
        "E1590: Too many types specified for generic class 'I'", 5)

  lines =<< trim END
    vim9script
    class A<X>
    endclass
    class B<T> extends A<T, T>
    endclass
  END
  v9.CheckSourceFailure(lines,
        "E1590: Too many types specified for generic class 'A'", 5)
enddef

" Test for a type variable of a generic method with the same name as a type
" variable of the class
def Test_generic_method_type_var_same_as_class()
  var errors = [
        ['class A<T>', '  def F<T>(x: T): T', '    return x', '  enddef', 'endclass'],
        ['class A<T>', '  static def F<U, T>(x: T): T', '    return x', '  enddef', 'endclass'],
        ['interface I<T>', '  def F<T>(x: T): T', 'endinterface'],
      ]
  for cmds in errors
    v9.CheckSourceFailure(['vim9script'] + cmds, 'E1561: Duplicate type variable name: T', 3)
  endfor

  # Duplicate type variable in the class or interface definition
  v9.CheckSourceFailure(['vim9script', 'class A<T, T>', 'endclass'],
                        'E1561: Duplicate type variable name: T', 2)
  v9.CheckSourceFailure(['vim9script', 'interface I<K, V, K>', 'endinterface'],
                        'E1561: Duplicate type variable name: K', 2)

  var lines =<< trim END
    vim9script
    class A<T>
      def F<U>(x: U): U
        return x
      enddef
    endclass
    class B
      def F<T>(x: T): T
        return x
      enddef
    endclass
    assert_equal('s', A<number>.new().F<string>('s'))
    assert_equal(3, B.new().F<number>(3))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a type variable with the name of the class or function being
" defined or with the name of another script item
def Test_generic_type_var_same_as_definition_name()
  var errors = [
        [['class A<A>', 'endclass'], 'A', 2],
        [['interface I<T, I>', 'endinterface'], 'I', 2],
        [['def Fn<Fn>()', 'enddef'], 'Fn', 2],
        [['def g:GenFn<GenFn>()', 'enddef'], 'GenFn', 2],
        [['type FooBar = number', 'class Foo<FooBar>', 'endclass'], 'FooBar', 3],
        [['def MyFunc()', 'enddef', 'class Foo<MyFunc>', 'endclass'], 'MyFunc', 4],
        [['class Box<X>', 'endclass', 'def Fn<Box>()', 'enddef'], 'Box', 4],
        [['class Box<X>', 'endclass', 'type Box = number'], 'Box', 4],
      ]
  for [cmds, name, lnum] in errors
    v9.CheckSourceFailure(['vim9script'] + cmds,
          $'E1041: Redefining script item: "{name}"', lnum)
  endfor

  var lines =<< trim END
    vim9script
    class Ab<A>
      var v: A
    endclass
    def Fn<F>(x: F): F
      return x
    enddef
    assert_equal(1, Ab<number>.new(1).v)
    assert_equal('x', Fn<string>('x'))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for an abstract generic class
def Test_generic_abstract_class()
  var lines =<< trim END
    vim9script
    abstract class Shape<T>
      var v: T
      abstract def Area(): T
      def Twice(): list<T>
        return [this.Area(), this.Area()]
      enddef
    endclass
    class Sq<T> extends Shape<T>
      def new(this.v)
      enddef
      def Area(): T
        return this.v
      enddef
    endclass
    class NumSq extends Shape<number>
      def Area(): number
        return 9
      enddef
    endclass
    assert_equal([4, 4], Sq<number>.new(4).Twice())
    assert_equal(['a', 'a'], Sq<string>.new('a').Twice())
    assert_equal([9, 9], NumSq.new().Twice())
    var s: Shape<number> = Sq<number>.new(2)
    assert_equal(2, s.Area())
  END
  v9.CheckSourceSuccess(lines)

  # Cannot create an object of an abstract generic class
  lines =<< trim END
    vim9script
    abstract class Shape<T>
    endclass
    var s = Shape<number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1325: Method "new" not found in class "Shape<number>"', 4)

  # Abstract method not implemented
  lines =<< trim END
    vim9script
    abstract class Shape<T>
      abstract def Area(): T
    endclass
    class Sq<T> extends Shape<T>
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1373: Abstract method "Area" is not implemented', 6)
enddef

" Test for the different kinds of object variables in a generic class
def Test_generic_class_object_variable_kinds()
  var lines =<< trim END
    vim9script
    class A<T>
      var ro: T
      public var rw: T
      final fin: T
      const con: T
      var _prot: T
      def new(v: T)
        this.ro = v
        this.rw = v
        this.fin = v
        this.con = v
        this._prot = v
      enddef
      def GetProt(): T
        return this._prot
      enddef
    endclass
    var a = A<string>.new('x')
    assert_equal(['x', 'x', 'x', 'x', 'x'],
          [a.ro, a.rw, a.fin, a.con, a.GetProt()])
    a.rw = 'y'
    assert_equal('y', a.rw)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class A<T>
      var ro: T
    endclass
    var a = A<number>.new(1)
    a.ro = 2
  END
  v9.CheckSourceFailure(lines, 'E1335: Variable "ro" in class "A<number>" is not writable', 6)

  lines =<< trim END
    vim9script
    class A<T>
      public final fin: T
      def new(this.fin)
      enddef
    endclass
    var a = A<number>.new(1)
    a.fin = 2
  END
  v9.CheckSourceFailure(lines, 'E1409: Cannot change read-only variable "fin" in class "A<number>"', 8)

  lines =<< trim END
    vim9script
    class A<T>
      var _prot: T
    endclass
    var a = A<number>.new(1)
    echo a._prot
  END
  v9.CheckSourceFailure(lines, 'E1333: Cannot access protected variable "_prot" in class "A<number>"', 6)

  # The initializer of an object variable is checked for each type
  lines =<< trim END
    vim9script
    class A<T>
      var v: T = 10
    endclass
    assert_equal(10, A<number>.new().v)
    var a = A<string>.new()
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "v": type mismatch, expected string but got number', 1)
enddef

" Test for the default value of an object variable with a type variable
def Test_generic_class_object_variable_default_value()
  var lines =<< trim END
    vim9script
    class Foo
    endclass
    class A<T>
      var v: T
    endclass
    assert_equal(0, A<number>.new().v)
    assert_equal('', A<string>.new().v)
    assert_equal([], A<list<number>>.new().v)
    assert_equal({}, A<dict<string>>.new().v)
    assert_equal(false, A<bool>.new().v)
    assert_equal(0.0, A<float>.new().v)
    assert_equal(0z, A<blob>.new().v)
    assert_equal((), A<tuple<number>>.new().v)
    assert_equal(null_object, A<Foo>.new().v)
    assert_equal('list<number>', typename(A<list<number>>.new().v))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for object variables using the type variables in other types
def Test_generic_class_object_variable_types()
  var lines =<< trim END
    vim9script
    class Pair<A, B>
      var a: A
      var b: B
    endclass
    class C<T, U>
      var t: tuple<T, U>
      var F: func(T): U
      var pairs: list<Pair<T, T>>
      var d: dict<list<T>>
    endclass
    var c = C<number, string>.new((1, 'a'), (n: number): string => string(n),
          [Pair<number, number>.new(1, 2)], {k: [3]})
    assert_equal('tuple<number, string>', typename(c.t))
    assert_equal('func(number): string', typename(c.F))
    assert_equal('list<object<Pair<number, number>>>', typename(c.pairs))
    assert_equal('dict<list<number>>', typename(c.d))
    assert_equal('5', c.F(5))
    assert_equal(2, c.pairs[0].b)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class C<T, U>
      var t: tuple<T, U>
    endclass
    var c = C<number, string>.new((1, 2))
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected tuple<number, string> but got tuple<number, number>', 5)

  lines =<< trim END
    vim9script
    class C<T, U>
      var F: func(T): U
    endclass
    var c = C<number, string>.new((n: number): number => n)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected func(number): string but got func(number): number', 5)
enddef

" Test for the different kinds of class variables in a generic class
def Test_generic_class_class_variable_kinds()
  var lines =<< trim END
    vim9script
    var init_count = 0
    def Count(): number
      init_count += 1
      return init_count
    enddef
    class A<T>
      static const C: number = 5
      static final F: list<T> = []
      static var _p: number = 7
      static var n: number = Count()
      static def GetP(): number
        return _p
      enddef
    endclass
    assert_equal(5, A<number>.C)
    A<number>.F->add(1)
    A<string>.F->add('s')
    assert_equal([[1], ['s']], [A<number>.F, A<string>.F])
    assert_equal(7, A<number>.GetP())
    # The initializer is used once for each type
    assert_equal(1, A<number>.n)
    assert_equal(1, A<number>.n)
    assert_equal(2, A<string>.n)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class A<T>
      public static const C: number = 5
    endclass
    A<number>.C = 6
  END
  v9.CheckSourceFailure(lines, 'E1409: Cannot change read-only variable "C" in class "A<number>"', 5)

  lines =<< trim END
    vim9script
    class A<T>
      static var _p: number = 7
    endclass
    echo A<number>._p
  END
  v9.CheckSourceFailure(lines, 'E1333: Cannot access protected variable "_p" in class "A<number>"', 5)
enddef

" Test for the different kinds of methods in a generic class
def Test_generic_class_method_kinds()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
      var items: list<T>
      def new(this.v)
      enddef
      def Set(v: T): Box<T>
        this.v = v
        return this
      enddef
      def Add(...l: list<T>): Box<T>
        this.items += l
        return this
      enddef
      def AddFirst(x: T, ...rest: list<T>): list<T>
        return [x] + rest
      enddef
      def Default(x: T = this.v): T
        return x
      enddef
      def Map<U>(F: func(T): U): Box<U>
        return Box<U>.new(F(this.v))
      enddef
      def Id<X>(x: X): X
        return x
      enddef
      def CallId(): T
        return this.Id<T>(this.v)
      enddef
      def _Prot(): T
        return this.v
      enddef
      def CallProt(): T
        return this._Prot()
      enddef
      def Lambda(): func(): T
        return () => this.v
      enddef
      def TypedLambda(): T
        var F = (x: T): T => x
        return F(this.v)
      enddef
      def BlockLambda(): list<T>
        var F = (x: T): list<T> => {
          var l: list<T> = [x, x]
          return l
        }
        return F(this.v)
      enddef
      def Cast(a: any): T
        return <T>a
      enddef
      static def Make(v: T): Box<T>
        return Box<T>.new(v)
      enddef
    endclass

    var b = Box<number>.new(1)
    assert_equal(5, b.Set(4).Set(5).v)
    assert_equal([1, 2, 3], b.Add(1, 2).Add(3).items)
    assert_equal([1, 2, 3], b.AddFirst(1, 2, 3))
    assert_equal(5, b.Default())
    assert_equal(7, b.Default(7))
    assert_equal('object<Box<string>>', typename(b.Map<string>((n) => string(n))))
    assert_equal('5', b.Map<string>((n) => string(n)).v)
    assert_equal(5, b.CallId())
    assert_equal(5, b.CallProt())
    assert_equal(5, b.Lambda()())
    assert_equal(5, b.TypedLambda())
    assert_equal([5, 5], b.BlockLambda())
    assert_equal(3, b.Cast(3))
    assert_equal('x', Box<string>.Make('x').v)

    def Fn()
      var s = Box<string>.new('a')
      assert_equal('b', s.Set('b').v)
      assert_equal(['b', 'b'], s.BlockLambda())
      assert_equal(1, s.Map<number>((x) => len(x)).v)
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Box<T>
      def _Prot(): T
        return null
      enddef
    endclass
    Box<number>.new()._Prot()
  END
  v9.CheckSourceFailure(lines, 'E1366: Cannot access protected method: _Prot', 7)

  lines =<< trim END
    vim9script
    class Box<T>
      def Cast(a: any): T
        return <T>a
      enddef
    endclass
    Box<number>.new().Cast('x')
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 1)

  lines =<< trim END
    vim9script
    class Box<T>
      def Add(...l: list<T>)
      enddef
    endclass
    Box<number>.new().Add(1, 'a')
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 2: type mismatch, expected number but got string', 6)
enddef

" Test for funcrefs to the methods of a generic class
def Test_generic_class_method_funcref()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
      def Get(): T
        return this.v
      enddef
      def Add(x: T): T
        return this.v + x
      enddef
      static def Make(v: T): Box<T>
        return Box<T>.new(v)
      enddef
    endclass
    var b = Box<number>.new(3)
    var F = b.Get
    assert_equal(3, F())
    assert_equal('func(): number', typename(F))
    var M = Box<number>.Make
    assert_equal('func(number): object<Box<number>>', typename(M))
    assert_equal(8, M(8).v)
    var P = function(b.Add, [4])
    assert_equal(7, P())
    assert_equal([4, 5], [1, 2]->map((_, x) => b.Add(x)))
    var boxes = [Box<number>.new(3), Box<number>.new(1), Box<number>.new(2)]
    assert_equal([1, 2, 3], boxes->sort((x, y) => x.v - y.v)->mapnew((_, x) => x.v))
    assert_equal([3], boxes->filter((_, x) => x.v > 2)->mapnew((_, x) => x.Get()))

    def Fn()
      var G = Box<string>.Make
      assert_equal('s', G('s').Get())
      var obj = Box<string>.new('t')
      var H = obj.Get
      assert_equal('t', H())
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for the constructors of a generic class
def Test_generic_class_constructors()
  var lines =<< trim END
    vim9script
    class Base<T>
      var b: T
    endclass
    class A<T> extends Base<T>
      var a: T
      var c: T = this.a
      def newBoth(v: T)
        this.a = v
        this.b = v
      enddef
      def newFromList(...l: list<T>)
        this.a = l[0]
        this.b = l[-1]
      enddef
      def newDefault(this.a, this.b = v:none)
      enddef
    endclass
    class Child<T> extends Base<T>
    endclass
    var o = A<number>.new(1, 2)
    assert_equal([1, 2], [o.b, o.a])
    o = A<number>.newBoth(3)
    assert_equal([3, 3], [o.a, o.b])
    o = A<number>.newFromList(4, 5, 6)
    assert_equal([4, 6], [o.a, o.b])
    o = A<number>.newDefault(7)
    assert_equal([7, 0], [o.a, o.b])
    assert_equal(8, Child<number>.new(8).b)
    var N = A<string>.newBoth
    assert_equal('x', N('x').b)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class A<T>
      var a: T
      def newBoth(v: T)
        this.a = v
      enddef
    endclass
    var o = A<number>.newBoth('x')
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected number but got string', 8)
enddef

" Test for the builtin methods in a generic class
def Test_generic_class_builtin_methods_ok()
  var lines =<< trim END
    vim9script
    class A<T>
      var v: T
      def string(): string
        return $'A({this.v})'
      enddef
      def len(): number
        return 2
      enddef
      def empty(): bool
        return false
      enddef
    endclass
    class B<T> extends A<T>
    endclass
    class S<T>
      var v: T
      def string(): T
        return this.v
      enddef
    endclass
    var a = A<number>.new(3)
    assert_equal(['A(3)', 2, 0], [string(a), len(a), empty(a)])
    assert_equal(['A(x)', 2, 0], [string(B<string>.new('x')),
          len(B<string>.new('x')), empty(B<string>.new('x'))])
    # A method returning the type variable is fine when it is a string
    assert_equal('s', string(S<string>.new('s')))
  END
  v9.CheckSourceSuccess(lines)

  # Creating the class fails when the parent class has a wrong builtin method
  lines =<< trim END
    vim9script
    class S<T>
      var v: T
      def string(): T
        return this.v
      enddef
    endclass
    class C<T> extends S<T>
    endclass
    var c = C<number>.new(1)
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "string": type mismatch, expected func(): string but got func(): number', 10)
enddef

" Test for more inheritance with generic classes and interfaces
def Test_generic_class_inheritance_more()
  var lines =<< trim END
    vim9script
    class A<T>
      var a: T
      def Who(): string
        return 'A'
      enddef
      def GetA(): T
        return this.a
      enddef
    endclass
    class B<T> extends A<T>
      def Who(): string
        return 'B' .. super.Who()
      enddef
    endclass
    class C<T> extends B<list<T>>
      def Who(): string
        return 'C' .. super.Who()
      enddef
    endclass
    var c = C<number>.new([1])
    assert_equal('CBA', c.Who())
    assert_equal([1], c.GetA())
    assert_true(instanceof(c, B<list<number>>))
    assert_true(instanceof(c, A<list<number>>))
    assert_false(instanceof(c, A<number>))
    var a: A<list<number>> = c
    assert_equal('CBA', a.Who())
    def Fn(x: A<list<number>>): string
      return x.Who() .. string(x.GetA())
    enddef
    assert_equal('CBA[1]', Fn(c))
  END
  v9.CheckSourceSuccess(lines)

  # A generic class extending a non-generic class and implementing a generic
  # interface
  lines =<< trim END
    vim9script
    class Base
      def Name(): string
        return 'base'
      enddef
    endclass
    interface I<T>
      var v: T
      def Get(): T
      def Conv<U>(F: func(T): U): U
    endinterface
    interface J<T> extends I<T>
      def Twice(): list<T>
    endinterface
    class A<T> extends Base implements J<T>
      var v: T
      def Get(): T
        return this.v
      enddef
      def Conv<U>(F: func(T): U): U
        return F(this.v)
      enddef
      def Twice(): list<T>
        return [this.v, this.v]
      enddef
    endclass
    class K implements J<string>
      var v: string = 'k'
      def Get(): string
        return this.v
      enddef
      def Conv<U>(F: func(string): U): U
        return F(this.v)
      enddef
      def Twice(): list<string>
        return [this.v]
      enddef
    endclass
    var j: J<number> = A<number>.new(3)
    assert_equal([3, 3, '3'], [j.Get(), j.v, j.Conv<string>((n) => string(n))])
    var i: I<number> = j
    assert_equal(3, i.Get())
    assert_equal([3, 3], j.Twice())
    assert_equal('base', A<number>.new(1).Name())
    var k: J<string> = K.new()
    assert_equal(['k'], k.Twice())
  END
  v9.CheckSourceSuccess(lines)

  # An overridden method with the wrong type in a generic class extending a
  # non-generic class
  lines =<< trim END
    vim9script
    class Base
      def Get(): number
        return 1
      enddef
    endclass
    class A<T> extends Base
      def Get(): T
        return null
      enddef
    endclass
    var a = A<string>.new()
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Get": type mismatch, expected func(): number but got func(): string', 12)

  # An overridden method with a list of the type variable
  lines =<< trim END
    vim9script
    class A<T>
      def Put(x: T)
      enddef
    endclass
    class B<T> extends A<list<T>>
      def Put(x: T)
      enddef
    endclass
    var b = B<number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Put": type mismatch, expected func(list<number>): void but got func(number): void', 10)
enddef

" Test for using a type alias for a class created from a generic class
def Test_generic_class_type_alias()
  var lines =<< trim END
    vim9script
    class Pair<A, B>
      var a: A
      var b: B
      public static var count: number = 0
      static def Make(a: A, b: B): Pair<A, B>
        return Pair<A, B>.new(a, b)
      enddef
    endclass
    type NumStr = Pair<number, string>
    type PairList = list<NumStr>
    var p: NumStr = NumStr.new(1, 'a')
    assert_equal('object<Pair<number, string>>', typename(p))
    assert_true(instanceof(p, NumStr))
    assert_true(instanceof(p, Pair<number, string>))
    NumStr.count = 3
    assert_equal(3, Pair<number, string>.count)
    assert_equal('b', NumStr.Make(2, 'b').b)
    var l: PairList = [p]
    assert_equal(1, l[0].a)
    def Fn(x: NumStr): NumStr
      return x
    enddef
    assert_equal('a', Fn(p).b)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Pair<A, B>
    endclass
    type P = Pair
  END
  v9.CheckSourceFailure(lines, "E1588: Type arguments missing for generic class 'Pair'", 4)

  lines =<< trim END
    vim9script
    class Pair<A, B>
    endclass
    type NumStr = Pair<number, string>
    var p: NumStr = Pair<string, number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<Pair<number, string>> but got object<Pair<string, number>>', 5)
enddef

" Test for using generic classes in other types
def Test_generic_class_in_other_types()
  var lines =<< trim END
    vim9script
    interface I
    endinterface
    class Impl implements I
    endclass
    enum Color
      Red, Green
    endenum
    class Foo<T>
      var v: T
    endclass
    var nested: Foo<Foo<number>> = Foo<Foo<number>>.new(Foo<number>.new(1))
    assert_equal(1, nested.v.v)
    var l: list<Foo<number>> = [Foo<number>.new(2)]
    var d: dict<Foo<string>> = {a: Foo<string>.new('s')}
    var t: tuple<Foo<number>, Foo<string>> = (l[0], d.a)
    assert_equal([2, 's'], [t[0].v, t[1].v])
    var F: func(Foo<number>): Foo<string> = (x) => Foo<string>.new(string(x.v))
    assert_equal('2', F(l[0]).v)
    var o: object<Foo<number>> = l[0]
    assert_equal(2, o.v)
    assert_equal('object<Foo<object<I>>>', typename(Foo<I>.new(Impl.new())))
    assert_equal(Color.Green, Foo<Color>.new(Color.Green).v)
    assert_equal(3, Foo<func(): number>.new(() => 3).v())
    assert_equal('object<Foo<any>>', typename(Foo<any>.new(1)))
    for x: Foo<number> in l
      assert_equal(2, x.v)
    endfor
    def Fn(): list<number>
      var r: list<number> = []
      for x: Foo<number> in [Foo<number>.new(5)]
        r->add(x.v)
      endfor
      var n: dict<list<Foo<number>>> = {k: [Foo<number>.new(6)]}
      r->add(n.k[0].v)
      return r
    enddef
    assert_equal([5, 6], Fn())
  END
  v9.CheckSourceSuccess(lines)

  # Foo<any> does not accept Foo<number>
  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var x: Foo<any> = Foo<number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<Foo<any>> but got object<Foo<number>>', 4)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var x: Foo<void>
  END
  v9.CheckSourceFailure(lines, 'E1330: Invalid type used in variable declaration: void', 4)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var o: object<Foo<string>> = Foo<number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<Foo<string>> but got object<Foo<number>>', 4)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var F: func(Foo<number>) = (x: Foo<string>) => 0
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(object<Foo<number>>) but got func(object<Foo<string>>): number', 4)
enddef

" Test for casts, "any" and comparing objects of generic classes
def Test_generic_class_cast_and_compare()
  var lines =<< trim END
    vim9script
    class Foo<T>
      var v: T
    endclass
    var a: any = Foo<number>.new(2)
    var f = <Foo<number>>a
    assert_equal(2, f.v)
    var g: Foo<number> = a
    assert_true(f is g)
    assert_true(f == g)
    assert_false(f == Foo<number>.new(3))
    assert_false(Foo<number>.new(1) == Foo<string>.new('1'))
    assert_equal(v:t_object, type(f))
    assert_equal(v:t_class, type(Foo<number>))
    assert_true(copy(f) is f)
    var n: Foo<number> = null_object
    assert_true(n == null_object)
    def Fn()
      var b: any = Foo<string>.new('s')
      var h = <Foo<string>>b
      assert_equal('s', h.v)
      var l: list<any> = [Foo<number>.new(1)]
      var ln: list<Foo<number>> = l
      assert_equal(1, ln[0].v)
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    var a: any = Foo<number>.new()
    var f = <Foo<string>>a
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<Foo<string>> but got object<Foo<number>>', 5)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    def Fn()
      var a: any = Foo<number>.new()
      var f: Foo<string> = a
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<Foo<string>> but got object<Foo<number>>', 2)

  lines =<< trim END
    vim9script
    class Foo<T>
    endclass
    def Fn()
      var l: list<any> = [Foo<number>.new()]
      var ls: list<Foo<string>> = l
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<object<Foo<string>>> but got list<object<Foo<number>>>', 2)
enddef

" Test for using an imported generic class at the script level
def Test_generic_class_imported_script_level()
  var lines =<< trim END
    vim9script
    export class Box<T>
      var v: T
      public static var s: number = 1
      def Get(): T
        return this.v
      enddef
    endclass
    export interface I<T>
      def Get(): T
    endinterface
  END
  writefile(lines, 'XgenericImp.vim', 'D')

  lines =<< trim END
    vim9script
    import './XgenericImp.vim' as m
    var b = m.Box<number>.new(4)
    assert_equal('object<Box<number>>', typename(b))
    assert_equal(4, b.Get())
    assert_equal(1, m.Box<string>.s)
    var c: m.Box<string> = m.Box<string>.new('c')
    assert_true(instanceof(c, m.Box<string>))
    class Sub<T> extends m.Box<T>
    endclass
    class Impl implements m.I<number>
      def Get(): number
        return 5
      enddef
    endclass
    assert_equal(6, Sub<number>.new(6).Get())
    var i: m.I<number> = Impl.new()
    assert_equal(5, i.Get())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    import './XgenericImp.vim' as m
    var b = m.Box<nosuch>.new(4)
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: nosuch', 3)

  lines =<< trim END
    vim9script
    import './XgenericImp.vim' as m
    echo m.Box.new(1)
  END
  v9.CheckSourceFailure(lines, "E1588: Type arguments missing for generic class 'Box'", 3)

  lines =<< trim END
    vim9script
    import './XgenericImp.vim' as m
    def Fn()
      var b = m.Box<>.new(4)
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, "E1555: Empty type list specified for generic 'Box'", 1)
enddef

" Test for using an object of a generic class from a legacy function
def Test_generic_class_legacy_function()
  var lines =<< trim END
    vim9script
    class Box<T>
      public var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    g:box = Box<number>.new(3)
    func g:Legacy()
      let g:box.v = 4
      return [g:box.v, g:box.Get(), typename(g:box)]
    endfunc
    assert_equal([4, 4, 'object<Box<number>>'], g:Legacy())
    try
      legacy let g:box.v = 'x'
      assert_report('should have failed')
    catch /E1012:/
    endtry
    unlet g:box
    delfunc g:Legacy
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for garbage collection with objects of generic classes
func Test_generic_class_garbage_collect()
  let lines =<< trim END
    vim9script
    class Node<T>
      public var v: T
      public var next: Node<T>
      public var other: Node<string>
    endclass
    var a = Node<number>.new(1)
    var b = Node<number>.new(2)
    var s = Node<string>.new('s')
    a.next = b
    b.next = a
    a.other = s
    s.next = s
    test_garbagecollect_now()
    assert_equal([2, 1, 's'], [a.next.v, a.next.next.v, a.other.next.v])
  END
  call v9.CheckSourceSuccess(lines)

  " The class variables of each concrete class are kept
  let lines =<< trim END
    vim9script
    class Box<T>
      public static var items: list<T> = []
      public static var d: dict<list<T>> = {}
      public static var self: Box<T>
      public static var F: func(): list<T>
      var v: T
    endclass
    Box<number>.items = [1, 2]
    Box<string>.items = ['a']
    Box<list<number>>.items = [[3], [4]]
    Box<number>.d = {k: [5]}
    Box<number>.self = Box<number>.new(6)
    def MakeF(): func(): list<number>
      var l = [7]
      return () => l
    enddef
    Box<number>.F = MakeF()
    test_garbagecollect_now()
    assert_equal([[1, 2], ['a'], [[3], [4]]],
          \ [Box<number>.items, Box<string>.items, Box<list<number>>.items])
    assert_equal([{k: [5]}, 6, [7]],
          \ [Box<number>.d, Box<number>.self.v, Box<number>.F()])
  END
  call v9.CheckSourceSuccess(lines)

  " Objects referenced through a generic interface type
  let lines =<< trim END
    vim9script
    interface Getter<T>
      def Get(): T
    endinterface
    class Holder<T> implements Getter<T>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    var g1: Getter<list<number>> = Holder<list<number>>.new([1, 2])
    var g2: Getter<dict<string>> = Holder<dict<string>>.new({a: 'b'})
    var gl: list<Getter<list<number>>> = [g1, Holder<list<number>>.new([3])]
    test_garbagecollect_now()
    assert_equal([[1, 2], {a: 'b'}, [3]], [g1.Get(), g2.Get(), gl[1].Get()])
  END
  call v9.CheckSourceSuccess(lines)

  " Closures and partials of generic class methods
  let lines =<< trim END
    vim9script
    class Mapper<T>
      var Transform: func(T): list<T>
      def Apply(x: T): list<T>
        return this.Transform(x)
      enddef
    endclass
    def MakeMapper(): Mapper<string>
      var extra = ['y']
      return Mapper<string>.new((x: string): list<string> => [x] + extra)
    enddef
    var m = MakeMapper()
    var Apply = Mapper<string>.new((x: string): list<string> => [x, x]).Apply
    test_garbagecollect_now()
    assert_equal([['a', 'y'], ['b', 'b']], [m.Apply('a'), Apply('b')])
  END
  call v9.CheckSourceSuccess(lines)

  " A class implementing an interface with itself as the type argument
  let lines =<< trim END
    vim9script
    interface Cmp<T>
      def Compare(other: T): number
    endinterface
    class Foo<T> implements Cmp<Foo<T>>
      var v: T
      var peers: list<Cmp<Foo<T>>> = []
      def Compare(other: Foo<T>): number
        return this.v == other.v ? 0 : 1
      enddef
    endclass
    var a = Foo<number>.new(1)
    var b = Foo<number>.new(1)
    a.peers->add(b)
    b.peers->add(a)
    a = null_object
    test_garbagecollect_now()
    assert_equal(0, b.peers[0].Compare(b))
  END
  call v9.CheckSourceSuccess(lines)

  " A concrete class that failed to initialize is kept, its class variables
  " are partly set
  let lines =<< trim END
    vim9script
    class Box<T>
      static var l: list<T> = []
      static var x: T = Nosuch()
    endclass
    assert_equal(0, exists('Box<number>'))
    test_garbagecollect_now()
    assert_fails('echo Box<number>.l', 'E117: Unknown function: Nosuch')
    test_garbagecollect_now()
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" Test for garbage collection with objects of a generic class defined in a
" script that is sourced again: the concrete classes of the old generic class
" are still used by the objects
func Test_generic_class_garbage_collect_after_reload()
  let lines =<< trim END
    vim9script
    class Box<T>
      static var items: list<T> = []
      var v: T
      def Add(): list<T>
        items->add(this.v)
        return items
      enddef
    endclass
    interface I<T>
      def Get(): list<T>
    endinterface
    class Impl<T> extends Box<T> implements I<T>
      def Get(): list<T>
        return [this.v]
      enddef
    endclass
    if !exists('g:box')
      g:box = Box<list<number>>.new([1])
      g:impl = Impl<dict<number>>.new({a: 1})
    endif
  END
  call writefile(lines, 'XgenericGcReload.vim', 'D')
  source XgenericGcReload.vim
  source XgenericGcReload.vim
  call test_garbagecollect_now()
  call assert_equal([[1]], g:box.Add())
  call assert_equal([{'a': 1}], g:impl.Get())
  source XgenericGcReload.vim
  call test_garbagecollect_now()
  call assert_equal([[1], [1]], g:box.Add())
  unlet g:box g:impl
  call test_garbagecollect_now()
endfunc

" Test for freeing generic classes which extend or implement generic classes
" using their type variables (e.g. "class B<U> extends A<U>")
func Test_generic_class_free_parent_classes()
  let lines =<< trim END
    vim9script
    class A<T>
      var a: T
    endclass
    class B<U> extends A<U>
    endclass
    class C<V> extends B<V>
      def Get(): V
        return this.a
      enddef
    endclass
    interface Cmp<T>
      def Compare(other: T): number
    endinterface
    class Foo<T> implements Cmp<Foo<T>>
      def Compare(other: Foo<T>): number
        return 7
      enddef
    endclass
    class Bar<T> extends Foo<T>
    endclass
    class Baz<T> extends Bar<list<T>>
    endclass
    g:objs->add(C<number>.new(len(g:objs)))
    g:objs->add(Baz<string>.new())
  END
  call writefile(lines, 'XgenericFreeParents.vim', 'D')
  let g:objs = []
  source XgenericFreeParents.vim
  source XgenericFreeParents.vim
  call test_garbagecollect_now()
  call assert_equal([0, 2], [g:objs[0].Get(), g:objs[2].Get()])
  call assert_equal(7, g:objs[3].Compare(g:objs[3]))
  unlet g:objs
  call test_garbagecollect_now()

  " A concrete class failing before its parent class is created
  let lines =<< trim END
    vim9script
    class A<T>
      static var s: T = 'x'
    endclass
    class B<U> extends A<U>
    endclass
    var o = B<number>.new()
  END
  call v9.CheckSourceFailure(lines, 'E1382: Variable "A<number>.s": type mismatch, expected number but got string', 7)

  " A generic class definition failing after the parent class is created
  let lines =<< trim END
    vim9script
    export class A<T>
      var v: T
    endclass
  END
  call writefile(lines, 'XgenericFreeParentsA.vim', 'D')
  let lines =<< trim END
    vim9script
    import './XgenericFreeParentsA.vim' as libA
    class B<U> extends libA.A<U>
      var v: U
    endclass
  END
  for i in range(2)
    call v9.CheckSourceFailure(lines, 'E1369: Duplicate variable: v', 5)
  endfor
endfunc

" Test for errors in the type arguments of a generic class used as a type
def Test_generic_class_type_args_errors()
  var errors = [
        [['var x: Foo<number >'], "E1068: No white space allowed before '>': >"],
        [['var x: Foo<number,>'], "E1069: White space required after ',': ,>"],
        [['class C<T> extends Foo<T,>', 'endclass'], "E1069: White space required after ',': <T,>"],
        [['def Fn<T>(x: Foo<T, T>)', 'enddef'], "E1590: Too many types specified for generic class 'Foo'"],
        # an optional type is not a type argument
        [['var x: Foo<?number>'], 'E488: Trailing characters: ?number>'],
        [['var x = Foo<?number>.new()'], 'E1008: Missing <type> after <'],
        [['def Fn(x: Foo<?number>)', 'enddef', 'defcompile'], 'E475: Invalid argument: x: Foo<?number>)'],
      ]
  for [cmds, err] in errors
    v9.CheckSourceFailure(['vim9script', 'class Foo<T>', 'endclass'] + cmds, err, 4)
  endfor
enddef

" Test for errors when using a nested generic function
def Test_generic_nested_func_errors()
  var lines =<< trim END
    vim9script
    class Foo<T>
      var v: T
    endclass
    def Outer(): list<string>
      var res: list<string> = []
      if true
        var x = 1
        def Inner<T>(y: T): Foo<T>
          return Foo<T>.new(y)
        enddef
        res->add(typename(Inner<string>('a')))
      endif
      return res
    enddef
    assert_equal(['object<Foo<string>>'], Outer())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    def Outer()
      def Inner<T>()
      enddef
      Inner<number, string>()
    enddef
    Outer()
  END
  v9.CheckSourceFailure(lines, 'E1556: Too many types specified for generic function', 3)
enddef

" Test for assigning to a class variable of an imported generic class
def Test_generic_class_imported_assign_class_variable()
  var lines =<< trim END
    vim9script
    export class Foo<T>
      public static var s: number = 1
      public static var t: T
      static var ro: number = 5
      static var _p: number = 6
    endclass
    export class NG
      public static var s: number = 1
    endclass
  END
  writefile(lines, 'XgenericImpAssign.vim', 'D')
  # The same classes are also defined in the sourced script
  var head = ['vim9script', "import './XgenericImpAssign.vim' as m"] + lines[1 :]

  lines =<< trim END
    vim9script
    import './XgenericImpAssign.vim' as m
    m.Foo<number>.s = 5
    m.Foo<number>.s += 2
    m.Foo<string>.t = 'a'
    m.Foo<string>.t ..= 'b'
    assert_equal([7, 1, 'ab', 0], [m.Foo<number>.s, m.Foo<string>.s,
          m.Foo<string>.t, m.Foo<number>.t])
    def Fn()
      m.Foo<number>.s = 10
      m.Foo<number>.s += 1
      m.Foo<list<number>>.t = [1]
      m.Foo<list<number>>.t += [2]
    enddef
    Fn()
    assert_equal([11, [1, 2]], [m.Foo<number>.s, m.Foo<list<number>>.t])
    # a non-generic imported class still works
    m.NG.s = 3
    def Fn2()
      m.NG.s = 4
    enddef
    Fn2()
    assert_equal(4, m.NG.s)
  END
  v9.CheckSourceSuccess(lines)

  # Errors for a local and an imported class at the script level and in a
  # :def function
  var errors = [
        ['Foo<number>.s = "x"', 'E1012: Type mismatch; expected number but got string'],
        ['Foo<number>.t = "x"', 'E1012: Type mismatch; expected number but got string'],
        ['Foo<number>.nosuch = 1', 'E1337: Class variable "nosuch" not found in class "Foo<number>"'],
        ['Foo<number>.ro = 1', 'E1335: Variable "ro" in class "Foo<number>" is not writable'],
        ['Foo<number>._p = 1', 'E1333: Cannot access protected variable "_p" in class "Foo<number>"'],
        ['Foo<nosuch>.s = 1', 'E1010: Type not recognized: nosuch'],
        ['Foo<number, string>.s = 1', "E1590: Too many types specified for generic class 'Foo'"],
        ['Foo.s = 1', "E1588: Type arguments missing for generic class 'Foo'"],
        ['NG<number>.s = 1', 'E1589: Not a generic class: NG'],
      ]
  for prefix in ['', 'm.']
    for [cmd, err] in errors
      v9.CheckSourceFailure(head + [prefix .. cmd], err, len(head) + 1)
      v9.CheckSourceFailure(head + ['def Fn()', '  ' .. prefix .. cmd, 'enddef',
            'Fn()'], err, 1)
    endfor
  endfor
enddef

" Test for :defcompile with a class created from a generic class
def Test_generic_class_defcompile_with_types()
  var lines =<< trim END
    vim9script
    class Foo<T>
      var v: T
      def Num(): number
        return this.v
      enddef
    endclass
    defcompile Foo<number>
    assert_match('Num\_s*return this.v', execute('disassemble Foo<number>.Num'))
  END
  v9.CheckSourceSuccess(lines)

  # Compiling the methods fails for this type
  lines =<< trim END
    vim9script
    class Foo<T>
      var v: T
      def Num(): number
        return this.v
      enddef
    endclass
    defcompile Foo<string>
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 1)

  var errors = [
        ['defcompile Foo<nosuch>', 'E1010: Type not recognized: nosuch'],
        ['defcompile Foo<number, string>', "E1590: Too many types specified for generic class 'Foo'"],
        ['defcompile Plain<number>', 'E1589: Not a generic class: Plain'],
        ['defcompile Foo<number> x', 'E488: Trailing characters:  x'],
        ['defcompile Foo<number', "E1554: Missing '>' in generic: Foo"],
      ]
  for [cmd, err] in errors
    lines = ['vim9script', 'class Foo<T>', 'endclass', 'class Plain',
          'endclass', cmd]
    v9.CheckSourceFailure(lines, err, 6)
  endfor
enddef

" Test for :disassemble with the methods of a local and an imported generic class
def Test_generic_class_disassemble_names()
  var lines =<< trim END
    vim9script
    export class G<T>
      var v: T
      def Get(): T
        return this.v
      enddef
      def Map<U>(x: U): U
        return x
      enddef
    endclass
    export class NG
      def M(): number
        return 1
      enddef
    endclass
  END
  writefile(lines, 'XgenericDisasm.vim', 'D')

  lines =<< trim END
    vim9script
    import './XgenericDisasm.vim' as m
    class Foo<T>
      var v: T
      def Get(): T
        return this.v
      enddef
      def Map<U>(x: U): U
        return x
      enddef
    endclass
    assert_match('^\nGet\n    return this.v\n   0 LOAD $0\n   1 OBJ_MEMBER 0\n   2 RETURN',
          execute('disassemble Foo<number>.Get'))
    assert_match('^\nGet\n    return this.v\n   0 LOAD $0',
          execute('disassemble m.G<number>.Get'))
    assert_match('^\nMap<string>\n    return x\n   0 LOAD arg\[-1\]',
          execute('disassemble Foo<number>.Map<string>'))
    assert_match('^\nMap<number>\n    return x\n   0 LOAD arg\[-1\]',
          execute('disassemble m.G<string>.Map<number>'))
    assert_match('^\nM\n    return 1\n   0 PUSHNR 1',
          execute('disassemble m.NG.M'))
    assert_match('^\nGet\n    return this.v\n   0 DEBUG',
          execute('disassemble debug Foo<number>.Get'))
  END
  v9.CheckSourceSuccess(lines)

  # Only an error, nothing is listed
  var errors = [
        ['Foo.Get', "E1588: Type arguments missing for generic class 'Foo'"],
        ['m.G.Get', "E1588: Type arguments missing for generic class 'G'"],
        ['Plain<number>.M', 'E1589: Not a generic class: Plain'],
        ['Foo<nosuch>.Get', 'E1010: Type not recognized: nosuch'],
        ['Foo<number>.Nosuch', 'E1337: Class variable "Nosuch" not found in class "Foo<number>"'],
        ['m.G<number>.Nosuch', 'E1337: Class variable "Nosuch" not found in class "G<number>"'],
      ]
  var head = ['vim9script', "import './XgenericDisasm.vim' as m",
        'class Foo<T>', '  def Get(): number', '    return 1', '  enddef',
        'endclass', 'class Plain', '  def M(): number', '    return 1',
        '  enddef', 'endclass']
  for [name, err] in errors
    v9.CheckSourceFailure(head + ['disassemble ' .. name], err, 13)
    # nothing is listed
    v9.CheckSourceSuccess(head + [
          'var res = execute("silent! disassemble ' .. name .. '")',
          'assert_notmatch(''\n\s*\d\+ \u'', res)'])
  endfor
enddef


" Test for using the same type arguments spelled differently.  The same
" concrete class must be used, with the same class variables.
def Test_generic_class_same_type_spelled_differently()
  var lines =<< trim END
    vim9script
    export class Foo
    endclass
    export type NumList = list<number>
    export class Box<T>
      public static var n: number = 0
    endclass
  END
  writefile(lines, 'XgenericSpelling.vim', 'D')

  lines =<< trim END
    vim9script
    import './XgenericSpelling.vim' as imp
    class Foo
    endclass
    class Box<T>
      static var n: number = 0
      static def Inc(): number
        n += 1
        return n
      enddef
    endclass
    type NL = list<number>
    type FooAlias = Foo
    type IFoo = imp.Foo

    Box<list<number>>.Inc()
    Box<NL>.Inc()
    def F1()
      Box<list<number>>.Inc()
      Box<NL>.Inc()
      Box<imp.NumList>.Inc()
    enddef
    F1()
    assert_equal([5, 5], [Box<list<number>>.n, Box<NL>.n])

    Box<Foo>.Inc()
    Box<FooAlias>.Inc()
    Box<imp.Foo>.Inc()
    Box<IFoo>.Inc()
    Box<IFoo>.Inc()
    def F2(): list<number>
      return [Box<Foo>.n, Box<FooAlias>.n, Box<imp.Foo>.n, Box<IFoo>.n]
    enddef
    assert_equal([2, 2, 3, 3], F2())

    # The generic class with the same name in the imported script is different
    imp.Box<number>.n = 7
    assert_equal([7, 0], [imp.Box<number>.n, Box<number>.n])
    assert_equal([1, 1, 1], [exists('Box<list<number>>'), exists('Box<NL>'),
          exists('Box<imp.Foo>')])

    var o1: Box<NL> = Box<list<number>>.new()
    var o2: Box<FooAlias> = Box<Foo>.new()
    assert_equal('object<Box<list<number>>>', typename(o1))
    assert_true(instanceof(o1, Box<NL>))
    assert_false(instanceof(o2, Box<imp.Foo>))

    def Id(b: Box<func(NL): tuple<FooAlias>>): Box<func(list<number>): tuple<Foo>>
      return b
    enddef
    var o4 = Box<func(list<number>): tuple<Foo>>.new()
    assert_true(Id(o4) is o4)
  END
  v9.CheckSourceSuccess(lines)

  # The class Foo in another script is a different type
  lines =<< trim END
    vim9script
    import './XgenericSpelling.vim' as imp
    class Foo
    endclass
    class Box<T>
    endclass
    var o: Box<imp.Foo> = Box<Foo>.new()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<Box<object<Foo>>> but got object<Box<object<Foo>>>', 7)
enddef

" Test for a generic class type created after substituting the type variables
" being the same as when the type is used directly
def Test_generic_class_same_type_after_substitution()
  var lines =<< trim END
    vim9script
    class Pair<A, B>
    endclass
    class Box<T>
      static var n: number = 0
      static def Inc(): number
        n += 1
        return n
      enddef
    endclass
    class Wrap<U>
      static def MkBox(): Box<Pair<U, list<U>>>
        Box<Pair<U, list<U>>>.Inc()
        return Box<Pair<U, list<U>>>.new()
      enddef
      public var p: Box<Pair<U, list<U>>>
    endclass
    def G<X>(): Box<Pair<X, list<X>>>
      Box<Pair<X, list<X>>>.Inc()
      return Box<Pair<X, list<X>>>.new()
    enddef

    Box<Pair<number, list<number>>>.Inc()
    var b1 = Wrap<number>.MkBox()
    var b2 = G<number>()
    var b3: Box<Pair<number, list<number>>> = b1
    b3 = b2
    assert_equal(3, Box<Pair<number, list<number>>>.n)
    var w = Wrap<number>.new()
    w.p = b1
    w.p = b2
    assert_equal('object<Box<object<Pair<number, list<number>>>>>',
          typename(w.p))
    assert_true(instanceof(b2, Box<Pair<number, list<number>>>))
    assert_false(instanceof(b2, Box<Pair<number, list<string>>>))

    def F()
      var x: Box<Pair<number, list<number>>> = G<number>()
      x = Wrap<number>.MkBox()
      assert_equal(5, Box<Pair<number, list<number>>>.n)
    enddef
    F()
    assert_equal(0, Box<Pair<string, list<string>>>.n)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a three level class hierarchy where each class passes different
" type arguments to the parent class
def Test_generic_class_three_level_inheritance()
  var lines =<< trim END
    vim9script
    class A<T>
      static var cnt: number = 0
      var _v: T
      def new(this._v)
      enddef
      def Get(): T
        return this._v
      enddef
      def Who(): string
        return 'A:' .. typename(this._v)
      enddef
      def _Prot(): T
        return this._v
      enddef
      static def Bump(): number
        cnt += 1
        return cnt
      enddef
    endclass
    class B<U> extends A<list<U>>
      def new(this._v)
      enddef
      def Who(): string
        return 'B>' .. super.Who()
      enddef
      def First(): U
        return this._v[0]
      enddef
      def ViaProt(): list<U>
        return this._Prot()
      enddef
      def BumpParent(): number
        return A<list<U>>.Bump()
      enddef
    endclass
    class C extends B<string>
      def new(this._v)
      enddef
      def Who(): string
        return 'C>' .. super.Who()
      enddef
      def Sum(): string
        return join(this.Get(), '+') .. this.First()
      enddef
    endclass

    var c = C.new(['x', 'y'])
    assert_equal('C>B>A:list<string>', c.Who())
    assert_equal('x+yx', c.Sum())
    assert_equal(['x', 'y'], c.ViaProt())
    var a: A<list<string>> = c
    assert_equal('C>B>A:list<string>', a.Who())
    var b: B<string> = c
    assert_equal('x', b.First())
    assert_equal([1, 1, 0, 0], [instanceof(c, A<list<string>>),
          instanceof(c, B<string>), instanceof(c, A<list<number>>),
          instanceof(c, B<number>)])
    c.BumpParent()
    B<string>.new([]).BumpParent()
    assert_equal([2, 0], [A<list<string>>.cnt, A<list<number>>.cnt])

    def Fn(x: A<list<string>>): string
      return x.Who() .. string(x.Get())
    enddef
    assert_equal("C>B>A:list<string>['x', 'y']", Fn(c))
    assert_equal("B>A:list<string>['q']", Fn(B<string>.new(['q'])))
    assert_fails('var bad: A<list<number>> = c', 'E1012: Type mismatch; expected object<A<list<number>>> but got object<C>')
    def Fn2()
      var bad: A<list<number>> = c
    enddef
    assert_fails('Fn2()', 'E1012: Type mismatch; expected object<A<list<number>>> but got object<C>')
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for several classes extending the same concrete generic class
def Test_generic_class_shared_concrete_parent()
  var lines =<< trim END
    vim9script
    class A<T>
      static var cnt: number = 0
      var v: T
      def Inc(): number
        cnt += 1
        return cnt
      enddef
    endclass
    type N = number
    class B1 extends A<number>
    endclass
    class B2<T> extends A<T>
    endclass
    class B3 extends B2<number>
    endclass
    class B4 extends A<N>
    endclass

    var l: list<A<number>> = [B1.new(), B2<number>.new(), B3.new(), B4.new()]
    for o in l
      o.Inc()
    endfor
    assert_equal(4, A<number>.cnt)
    assert_equal(0, A<string>.cnt)
    assert_equal(1, B2<string>.new().Inc())
    assert_equal(1, A<string>.cnt)
    for o in l
      assert_true(instanceof(o, A<N>))
    endfor
    assert_false(instanceof(B2<string>.new(), A<number>))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a generic interface extending a generic interface with different
" type arguments
def Test_generic_interface_extends_other_types()
  var lines =<< trim END
    vim9script
    interface I<T>
      def Get(): T
    endinterface
    interface J<U> extends I<list<U>>
      def Put(u: U)
    endinterface
    class K<V> implements J<V>
      var l: list<V> = []
      def Get(): list<V>
        return this.l
      enddef
      def Put(u: V)
        this.l->add(u)
      enddef
    endclass
    class M implements J<string>
      def Get(): list<string>
        return ['m']
      enddef
      def Put(u: string)
      enddef
    endclass

    var k = K<number>.new()
    var j: J<number> = k
    j.Put(3)
    var i: I<list<number>> = k
    assert_equal([[3], [3]], [i.Get(), j.Get()])
    assert_equal([1, 1, 0], [instanceof(k, I<list<number>>),
          instanceof(k, J<number>), instanceof(k, I<number>)])
    def Fn(x: I<list<number>>): list<number>
      return x.Get()
    enddef
    assert_equal([[3], [3]], [Fn(k), Fn(j)])
    def Fn2(x: I<list<string>>): list<string>
      return x.Get()
    enddef
    assert_equal(['m'], Fn2(M.new()))
  END
  v9.CheckSourceSuccess(lines)

  # The method type is wrong only for some type arguments.  The error is given
  # every time the class is used.
  lines =<< trim END
    vim9script
    interface I<T>
      def Get(): T
    endinterface
    class X<T> implements I<T>
      def Get(): number
        return 1
      enddef
    endclass
    assert_equal(1, X<number>.new().Get())
    var msg = 'E1383: Method "Get": type mismatch, expected func(): string but got func(): number'
    assert_fails('var x = X<string>.new()', msg)
    assert_fails('var x = X<string>.new()', msg)
    def F()
      var x = X<string>.new()
    enddef
    assert_fails('F()', msg)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    interface I<T>
      def Get(): T
    endinterface
    interface J<U> extends I<list<U>>
      def Put(u: U)
    endinterface
    class K<V> implements J<V>
      def Get(): V
        return null_value
      enddef
      def Put(u: V)
      enddef
    endclass
    var k = K<number>.new()
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "Get": type mismatch, expected func(): list<number> but got func(): number', 15)
enddef

" Test for generic classes used in func, tuple and dict types
def Test_generic_class_in_nested_types()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
    endclass
    class Pair<A, B>
    endclass
    type FT = func(Box<number>, ...list<Pair<number, string>>): Box<string>

    var t: tuple<Box<number>, ...list<Box<string>>> = (Box<number>.new(1),
          Box<string>.new('a'))
    var d: dict<Pair<number, list<string>>> = {a: Pair<number, list<string>>.new()}
    def Fn(b: Box<number>, ...l: list<Pair<number, string>>): Box<string>
      return Box<string>.new(string(b.v) .. len(l))
    enddef
    var F: FT = Fn
    assert_equal('12', F(Box<number>.new(1), Pair<number, string>.new(),
          Pair<number, string>.new()).v)
    assert_equal('tuple<object<Box<number>>, ...list<object<Box<string>>>>',
          typename(t))
    assert_equal('dict<object<Pair<number, list<string>>>>', typename(d))
    assert_equal('func(object<Box<number>>, ...list<object<Pair<number, string>>>): object<Box<string>>',
          typename(F))
  END
  v9.CheckSourceSuccess(lines)

  var head = ['vim9script', 'class Box<T>', 'endclass', 'class Pair<A, B>',
        'endclass']
  var errors = [
        ['var t: tuple<Box<number>, ...list<Box<string>>> = (Box<number>.new(), Box<number>.new())',
          'tuple<object<Box<number>>, ...list<object<Box<string>>>> but got tuple<object<Box<number>>, object<Box<number>>>'],
        ['var d: dict<Pair<number, list<string>>> = {a: Pair<number, list<number>>.new()}',
          'dict<object<Pair<number, list<string>>>> but got dict<object<Pair<number, list<number>>>>'],
        ['var F: func(Box<number>): Box<string> = (b: Box<string>): Box<string> => b',
          'func(object<Box<number>>): object<Box<string>> but got func(object<Box<string>>): object<Box<string>>'],
        ['var F: func(Box<number>): Box<string> = (b: Box<number>): Box<number> => b',
          'func(object<Box<number>>): object<Box<string>> but got func(object<Box<number>>): object<Box<number>>'],
        ['var F: func(...list<Pair<number, string>>) = (...l: list<Pair<string, string>>) => 0',
          'func(...list<object<Pair<number, string>>>) but got func(...list<object<Pair<string, string>>>): number'],
      ]
  for [cmd, err] in errors
    v9.CheckSourceFailure(head + [cmd], 'E1012: Type mismatch; expected ' .. err, 6)
    v9.CheckSourceFailure(head + ['def Fn()', '  ' .. cmd, 'enddef', 'Fn()'],
          'E1012: Type mismatch; expected ' .. err, 1)
  endfor
enddef

" Test for recursive generic class types
def Test_generic_class_recursive_types()
  var lines =<< trim END
    vim9script
    class Node<T>
      var val: T
      public var next: Node<T>
      public var kids: list<Node<T>> = []
      public var F: func(Node<T>): Node<T>
      def new(this.val)
      enddef
      def Len(): number
        var cnt = 0
        var p: Node<T> = this
        while p != null
          cnt += 1
          p = p.next
        endwhile
        return cnt
      enddef
      def Map<U>(Fn: func(T): U): Node<U>
        var r = Node<U>.new(Fn(this.val))
        if this.next != null
          r.next = this.next.Map<U>(Fn)
        endif
        return r
      enddef
    endclass
    var n = Node<number>.new(1)
    n.next = Node<number>.new(2)
    n.next.next = Node<number>.new(3)
    n.kids = [Node<number>.new(9)]
    n.F = (x: Node<number>): Node<number> => x.next
    assert_equal([3, 2], [n.Len(), n.F(n).val])
    var s = n.Map<string>((x) => 's' .. x)
    assert_equal([3, 's3'], [s.Len(), s.next.next.val])
    assert_equal('object<Node<string>>', typename(s.next))
    assert_fails('n.next = Node<string>.new("x")', 'E1012: Type mismatch; expected object<Node<number>> but got object<Node<string>>')
    assert_fails('n.kids = [Node<string>.new("x")]', 'E1012: Type mismatch; expected list<object<Node<number>>> but got list<object<Node<string>>>')
    def D()
      var m = Node<string>.new('a')
      m.next = Node<string>.new('b')
      assert_equal([2, 1], [m.Len(), m.Map<number>((x) => len(x)).next.val])
    enddef
    D()
  END
  v9.CheckSourceSuccess(lines)

  # Two generic classes using each other, creating deeper types only when a
  # method is called
  lines =<< trim END
    vim9script
    class A<T>
      var v: T
      def new(this.v)
      enddef
      def MkB(): any
        return B<T>.new(this)
      enddef
    endclass
    class B<T>
      var a: A<T>
      def new(this.a)
      enddef
      def MkA(): A<list<T>>
        return A<list<T>>.new([this.a.v])
      enddef
    endclass
    var b = A<number>.new(1).MkB()
    assert_equal('object<B<number>>', typename(b))
    var a2 = b.MkA()
    assert_equal('object<A<list<number>>>', typename(a2))
    assert_equal('object<B<list<number>>>', typename(a2.MkB()))
    def F()
      var x: B<string> = A<string>.new('s').MkB()
      assert_equal('object<A<list<list<string>>>>',
            typename(x.MkA().MkB().MkA()))
    enddef
    F()

    class Grow<T>
      var v: T
      def new(this.v)
      enddef
      def Up(): any
        return Grow<list<T>>.new([this.v])
      enddef
    endclass
    var g: any = Grow<number>.new(1)
    for i in range(120)
      g = g.Up()
    endfor
    assert_equal(repeat('list<', 120) .. 'number' .. repeat('>', 120),
          typename(g)[12 : -3])
  END
  v9.CheckSourceSuccess(lines)

  # A class passing itself as a type argument to the parent class
  lines =<< trim END
    vim9script
    class Base<T>
      def Self(): T
        return null_object
      enddef
      def Cmp(o: T): bool
        return o != null
      enddef
    endclass
    class Foo<T> extends Base<Foo<T>>
      var v: T
    endclass
    var f = Foo<number>.new(3)
    assert_true(f.Cmp(Foo<number>.new(4)))
    assert_true(instanceof(f, Base<Foo<number>>))
    assert_false(instanceof(f, Base<Foo<string>>))
    assert_equal('object<Foo<number>>', typename(f))
    assert_fails('f.Cmp(Foo<string>.new("a"))', 'E1013: Argument 1: type mismatch, expected object<Foo<number>> but got object<Foo<string>>')
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for overriding a generic method in a generic class hierarchy
def Test_generic_method_override_in_generic_class()
  var lines =<< trim END
    vim9script
    class A<T>
      def Conv<U>(x: T, y: U): list<U>
        return [y]
      enddef
    endclass
    class B<V> extends A<V>
      def Conv<W>(x: V, y: W): list<W>
        return [y, y]
      enddef
    endclass
    class C extends A<number>
      def Conv<Z>(x: number, y: Z): list<Z>
        return [y, y, y]
      enddef
    endclass
    var a: A<number> = B<number>.new()
    assert_equal(['a', 'a'], a.Conv<string>(1, 'a'))
    a = C.new()
    assert_equal(['a', 'a', 'a'], a.Conv<string>(1, 'a'))
    def D(o: A<number>): list<bool>
      return o.Conv<bool>(2, true)
    enddef
    assert_equal([[true, true], [true, true, true], [true]],
          [D(B<number>.new()), D(C.new()), D(A<number>.new())])
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a class variable initializer of a generic class running only once
" for each concrete class
def Test_generic_class_static_init_once()
  var lines =<< trim END
    vim9script
    var calls: list<string> = []
    def Init(s: string): number
      calls->add(s)
      return len(calls)
    enddef
    type S = string
    class Box<T>
      static var id: number = Init(typename(null_object))
    endclass
    var x: Box<string>
    assert_equal([1, 1], [exists('Box<S>'), exists('Box<string>')])
    def F(a: Box<S>): Box<string>
      return a
    enddef
    def G()
      var y: Box<S> = Box<string>.new()
      assert_equal(1, Box<S>.id)
    enddef
    G()
    F(Box<S>.new())
    assert_equal(1, len(calls))
    assert_equal([1, 1], [Box<S>.id, Box<string>.id])
    assert_equal(2, Box<number>.id)
    assert_equal(2, len(calls))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test that the class variable initializers of a generic parent class are not
" run when a generic class extending it is defined, only for the concrete
" classes
def Test_generic_class_parent_static_init()
  var lines =<< trim END
    vim9script
    var calls: list<string> = []
    def Init(s: string): number
      calls->add(s)
      return len(calls)
    enddef
    class A<T>
      static var id: number = Init('A')
      static var l: list<T> = []
    endclass
    class B<T> extends A<T>
      static var bid: number = Init('B')
    endclass
    class C<T> extends B<T>
    endclass
    class D<T> extends A<list<T>>
    endclass
    assert_equal([], calls)

    var c = C<number>.new()
    assert_equal(['A', 'B'], calls)
    var d = D<string>.new()
    assert_equal(['A', 'B', 'A'], calls)
    assert_equal('list<list<string>>', typename(A<list<string>>.l))
    A<number>.l->add(3)
    assert_equal([[3], 2], [A<number>.l, B<number>.bid])
    assert_equal(3, len(calls))
  END
  v9.CheckSourceSuccess(lines)

  # An initializer that fails gives an error when the concrete class is
  # created, not when the class extending the generic class is defined
  lines =<< trim END
    vim9script
    def Fail(): number
      throw 'init failed'
    enddef
    class A<T>
      static var n: number = Fail()
    endclass
    class B<T> extends A<T>
    endclass
    assert_fails('B<number>.new()', 'init failed')
  END
  v9.CheckSourceSuccess(lines)
enddef


" Test for using the type variables of a generic class in a lambda in a class
" or object variable initializer
def Test_generic_class_lambda_in_var_init()
  var lines =<< trim END
    vim9script
    g:X = 1
    class Box<T>
      static var K: func = () => <T>g:X
      static var L: func = () => (() => <list<T>>[g:X])()
      static var M = () => Box<T>.new()
      var F: func = () => <T>g:X
      var G = () => [Box<T>.new()]
      var H: func(): list<Box<T>> = () => [Box<T>.new()]
    endclass

    assert_equal(1, Box<number>.K())
    assert_equal([1], Box<number>.L())
    assert_equal('func(): object<Box<number>>', typename(Box<number>.M))
    assert_equal('object<Box<number>>', typename(Box<number>.M()))
    assert_fails('Box<string>.K()', 'E1012: Type mismatch; expected string but got number')

    var b = Box<number>.new()
    assert_equal(1, b.F())
    assert_equal('func(): list<object<Box<number>>>', typename(b.G))
    var l: list<Box<number>> = b.H()
    assert_equal('list<object<Box<number>>>', typename(l))

    def Fn()
      var o = Box<string>.new()
      assert_equal('func(): list<object<Box<string>>>', typename(o.G))
      var l2: list<Box<string>> = o.H()
      assert_equal('object<Box<string>>', typename(Box<string>.M()))
    enddef
    Fn()
    unlet g:X
  END
  v9.CheckSourceSuccess(lines)

  # Using an imported generic class
  lines =<< trim END
    vim9script
    g:X = 1
    export class Box<T>
      static var K: func = () => <T>g:X
      var G = () => [Box<T>.new()]
    endclass
  END
  writefile(lines, 'XgenericLambda.vim', 'D')
  lines =<< trim END
    vim9script
    import './XgenericLambda.vim' as m
    assert_equal(1, m.Box<number>.K())
    def Fn()
      assert_equal(1, m.Box<number>.K())
      assert_equal('func(): list<object<Box<string>>>',
            typename(m.Box<string>.new().G))
    enddef
    Fn()
    Fn()
    unlet g:X
  END
  v9.CheckSourceSuccess(lines)

  # A block lambda can also use the type variables
  lines =<< trim END
    vim9script
    class Box<T>
      static var F: func = (x) => {
          var y: T = x
          return y
        }
      var G: func = (x) => {
          var y: list<T> = [x]
          return y
        }
    endclass
    assert_equal(1, Box<number>.F(1))
    assert_fails("Box<number>.F('a')",
          'E1012: Type mismatch; expected number but got string')
    assert_equal(['a'], Box<string>.new().G('a'))
    assert_fails('Box<string>.new().G(1)',
          'E1012: Type mismatch; expected list<string> but got list<number>')
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a lambda that uses the type variables of a class and is compiled
" again after the class was freed.  Profiling is used to compile it again,
" run it in another Vim, it cannot be stopped.
def Test_generic_class_lambda_after_class_freed()
  CheckFeature profile

  var lines =<< trim END
    vim9script
    v:testing = 1
    var lib =<< trim LIB
      vim9script
      class Box<T>
        static var E: func = (x) => <T>x
        static var B: func = (x) => {
            var y: T = x
            return y
          }
        def Mk(): func
          return (x) => <T>x
        enddef
      endclass
    LIB
    writefile(lib + ['g:L = [Box<number>.E, Box<number>.B, '
          .. 'Box<number>.new().Mk()]'], 'XgenLambdaLib.vim')
    source XgenLambdaLib.vim
    var result: list<any> = [g:L[0](1), g:L[1](2), g:L[2](3)]

    # Source the script again, the class is freed and the lambdas are kept.
    writefile(lib, 'XgenLambdaLib.vim')
    source XgenLambdaLib.vim
    delete('XgenLambdaLib.vim')
    test_garbagecollect_now()

    # Profiling compiles the lambdas again, the type variables of the freed
    # class cannot be used.
    profile start XgenLambdaProf
    profile func *
    for F in g:L
      try
        result->add(F(4))
      catch
        result->add(matchstr(v:exception, 'E\d\+'))
      endtry
    endfor
    writefile([string(result)], 'XgenLambdaResult')
  END
  writefile(lines, 'Xscript', 'D')
  defer delete('XgenLambdaProf')
  defer delete('XgenLambdaResult')
  g:RunVim([], [], '-u NONE -S Xscript -c qa')
  assert_equal(0, v:shell_error)
  assert_equal(["[1, 2, 3, 'E1010', 'E1010', 'E1010']"],
        readfile('XgenLambdaResult'))
enddef

" Test for the type of the constructor of a generic class used in an object
" variable initializer, before the constructor is compiled
def Test_generic_class_new_type_in_var_init()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
      def new(this.v)
      enddef
      var F = () => Box<T>.new(this.v)
      var G = () => Box<number>.new(1)
    endclass
    var b = Box<string>.new('a')
    assert_equal('func(): object<Box<string>>', typename(b.F))
    assert_equal('func(): object<Box<number>>', typename(b.G))
    assert_equal('a', b.F().v)
    var o: Box<number> = b.G()
    assert_equal(1, o.v)
  END
  v9.CheckSourceSuccess(lines)
enddef


" Test for a concrete class failing a method signature check.  The same error
" must be given each time it is used, other concrete classes can be used.
def Test_generic_class_failed_check_repeated()
  var lines =<< trim END
    vim9script
    class A<T>
      def F(x: T): number
        return 1
      enddef
    endclass
    class B<T> extends A<T>
      def F(x: number): number
        return 2
      enddef
    endclass
    var err = 'E1383: Method "F": type mismatch, expected func(string): number but got func(number): number'
    assert_fails('B<string>.new().F(1)', err)
    assert_fails('B<string>.new().F(1)', err)
    assert_equal(2, B<number>.new().F(3))
    def G(): number
      return B<string>.new().F(1)
    enddef
    assert_fails('G()', err)

    interface I<T>
      def Get(): T
    endinterface
    class C<T> implements I<number>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    err = 'E1383: Method "Get": type mismatch, expected func(): number but got func(): string'
    assert_fails('C<string>.new("a").Get()', err)
    assert_fails('C<string>.new("a").Get()', err)
    var i: I<number> = C<number>.new(9)
    assert_equal(9, i.Get())

    # builtin method with a wrong return type
    class D<T>
      var v: T
      def len(): T
        return this.v
      enddef
    endclass
    err = 'E1383: Method "len": type mismatch, expected func(): number but got func(): string'
    assert_fails('len(D<string>.new("a"))', err)
    assert_fails('len(D<string>.new("a"))', err)
    assert_equal(4, len(D<number>.new(4)))
    assert_equal([0, 1, 0], [exists('D<string>'), exists('D<number>'),
          exists('C<string>')])
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for a concrete class whose class variable initializer fails a few
" times before it succeeds.  The parent class and interface are shared.
func Test_generic_class_parent_after_failed_init()
  let lines =<< trim END
    vim9script
    var n = 0
    def Maybe(): number
      n += 1
      if n < 3
        throw 'nope' .. n
      endif
      return n
    enddef
    interface I<T>
      def Get(): T
    endinterface
    class A<T> implements I<T>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    class B<T> extends A<T>
      static var s: number = Maybe()
    endclass
    g:res = []
    for i in range(4)
      try
        var b = B<number>.new(i)
        var a: A<number> = b
        var ii: I<number> = b
        g:res->add([a.Get(), ii.Get(), B<number>.s,
              instanceof(b, A<number>, I<number>), typename(b)])
      catch
        g:res->add(v:exception)
      endtry
    endfor
    def g:GenericGet(): list<any>
      var b = B<number>.new(9)
      var l: list<I<number>> = [b, A<number>.new(8)]
      return [l[0].Get(), l[1].Get(), B<number>.s]
    enddef
  END
  call writefile(lines, 'XgenericFailedInit.vim', 'D')
  source XgenericFailedInit.vim
  call assert_equal(['nope1', 'nope2', [2, 2, 3, 1, 'object<B<number>>'],
        \ [3, 3, 3, 1, 'object<B<number>>']], g:res)
  call assert_equal([9, 8, 3], g:GenericGet())
  call test_garbagecollect_now()
  call assert_equal([9, 8, 3], g:GenericGet())
  unlet g:res
  delfunc g:GenericGet
endfunc

" Test for :defcompile and :disassemble with a concrete class that cannot be
" created
def Test_generic_class_defcompile_failed()
  var lines =<< trim END
    vim9script
    class Box<T>
      static var bad: T = 'str'
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    var err = 'E1382: Variable "Box<number>.bad": type mismatch, expected number but got string'
    assert_fails('Box<number>.new(1)', err)
    assert_fails('defcompile Box<number>', err)
    assert_fails('disassemble Box<number>.Get', err)
    assert_fails('defcompile Box<number>', err)
    defcompile Box<string>
    assert_match('^\nGet\n    return this.v\n',
          execute('disassemble Box<string>.Get'))
    assert_fails('disassemble Box.Get', "E1588: Type arguments missing for generic class 'Box'")
    assert_equal('z', Box<string>.new('z').Get())
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for sourcing a script again after a generic class definition failed
def Test_generic_class_define_failed_again()
  var bodies = [
    [['class Base<T>', '  var v: T', 'endclass', 'class A<T> extends Base<T>',
      '  var v: T', 'endclass'], 'E1369: Duplicate variable: v'],
    [['class A<T> extends Nosuch<T>', 'endclass'],
      'E1353: Class name not found: Nosuch<T>'],
    [['class A<T> implements Nosuch<T>', 'endclass'],
      'E1346: Interface name not found: Nosuch<T>'],
    [['interface J<T>', 'endinterface', 'class A<T> extends J<T>',
      'endclass'], 'E1354: Cannot extend J<T>'],
    [['class N', 'endclass', 'class A<T> implements N', 'endclass'],
      'E1347: Not a valid interface: N'],
    [['class G<X>', 'endclass', 'class A<T> implements G<T>', 'endclass'],
      'E1347: Not a valid interface: G<T>'],
    [['interface G<X>', 'endinterface', 'class A<T> extends G<list<T>>',
      'endclass'], 'E1354: Cannot extend G<list<T>>'],
    [['class A<T>', '  static var s: list<T> = [1]', '  yyy', 'endclass'],
      'E1318: Not a valid command in a class: yyy'],
    [['class A<T>', '  var s: A<T, T>', 'endclass'],
      "E1590: Too many types specified for generic class 'A'"],
    [['class A<T>', '  def F<T>()', '  enddef', 'endclass'],
      'E1561: Duplicate type variable name: T'],
    [['class A<T> extends A<T>', 'endclass'], 'E1354: Cannot extend A<T>'],
  ]
  var good = ['vim9script', 'class A<T>', '  var v: T',
    '  static var cnt: number = 3', '  def Get(): T', '    return this.v',
    '  enddef', 'endclass', 'g:r = A<string>.new("ok").Get() .. A<number>.cnt']
  for [body, err] in bodies
    writefile(['vim9script'] + body + ['g:r = A<number>.new()'],
      'XgenericDefFail.vim')
    assert_fails('source XgenericDefFail.vim', err)
    assert_fails('source XgenericDefFail.vim', err)
    writefile(good, 'XgenericDefFail.vim')
    g:r = ''
    source XgenericDefFail.vim
    assert_equal('ok3', g:r)
  endfor
  delete('XgenericDefFail.vim')
  unlet g:r
enddef

" Test for a method that fails to compile only for some type arguments, and
" for a function using concrete classes that fails to compile
def Test_generic_class_method_compile_error()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
      def Inc(): T
        var x: T = this.v
        x += 1
        return x
      enddef
      static def Make(x: T): Box<T>
        var r: T = x * 2
        return Box<T>.new(r)
      enddef
    endclass
    var bs = Box<string>.new('a')
    assert_fails('bs.Inc()', 'E1012: Type mismatch; expected string but got number')
    assert_fails('bs.Inc()', 'E1091: Function is not compiled: Inc')
    assert_equal([2, 2.5], [Box<number>.new(1).Inc(),
          Box<float>.new(1.5).Inc()])
    assert_fails('Box<string>.Make("x")', 'E1036: * requires number or float arguments')
    assert_equal(8, Box<number>.Make(4).v)
    def UseBad(): string
      return Box<string>.new('z').Inc()
    enddef
    assert_fails('UseBad()', 'E1191: Call to function that failed to compile: Inc')
    assert_fails('disassemble Box<string>.Inc', 'E1091: Function is not compiled: Box<string>.Inc')
    assert_match('^\nInc\n', execute('disassemble Box<number>.Inc'))

    # concrete classes created by a function that fails to compile can be
    # used in another function
    def Bad()
      var a = Box<list<string>>.new(['a'])
      var b: Box<dict<number>> = Box<dict<number>>.new({})
      var c = Box<string>.new(1)
    enddef
    assert_fails('defcompile Bad', 'E1013: Argument 1: type mismatch, expected string but got number')
    def Good(): string
      var a = Box<list<string>>.new(['a'])
      var b: Box<dict<number>> = Box<dict<number>>.new({k: 2})
      return string(a.v) .. string(b.v)
    enddef
    assert_equal("['a']{'k': 2}", Good())
    assert_equal([1, 1], [exists('Box<list<string>>'),
          exists('Box<dict<number>>')])
    def BadArgs()
      var x = Box<number, number>.new(1)
    enddef
    assert_fails('BadArgs()', "E1590: Too many types specified for generic class 'Box'")
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for reloading a script defining a class used as a type argument.  The
" concrete classes created with the old class are not used for the new one.
func Test_generic_class_reload_type_arg_class()
  let lines =<< trim END
    vim9script
    export class K
      var n: number = 1
    endclass
  END
  call writefile(lines, 'XgenericArgK.vim', 'D')
  let lines =<< trim END
    vim9script
    export interface I<T>
      def Get(): T
    endinterface
    export class A<T> implements I<T>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    export class B1<T> extends A<T>
    endclass
    export class B2<T> extends A<T>
    endclass
  END
  call writefile(lines, 'XgenericArgLib.vim', 'D')
  let lines =<< trim END
    vim9script
    import './XgenericArgK.vim' as mk
    import './XgenericArgLib.vim' as mg
    g:b1 = mg.B1<mk.K>.new(mk.K.new())
    g:b2 = mg.B2<mk.K>.new(mk.K.new())
    g:old_i = <mg.I<mk.K>>g:b1
    source XgenericArgK.vim
    test_garbagecollect_now()
    var nb1 = mg.B1<mk.K>.new(mk.K.new())
    assert_equal([0, 0, 0], [instanceof(g:b1, mg.A<mk.K>),
          instanceof(g:b1, mg.I<mk.K>), instanceof(g:b1, mg.B1<mk.K>)])
    assert_equal([1, 1, 1], [instanceof(nb1, mg.A<mk.K>),
          instanceof(nb1, mg.I<mk.K>), instanceof(nb1, mg.B1<mk.K>)])
    assert_equal([1, 1, 1], [g:b1.Get().n, g:b2.Get().n, g:old_i.Get().n])
    assert_fails('var ii: mg.I<mk.K> = g:b2', 'E1012: Type mismatch; expected object<I<object<K>>> but got object<B2<object<K>>>')
    assert_false(g:b1 == nb1)
    var b4 = mg.A<mk.K>.new(mk.K.new())
    assert_fails('b4 = g:b1', 'E1012: Type mismatch; expected object<A<object<K>>> but got object<B1<object<K>>>')

    source XgenericArgLib.vim
    test_garbagecollect_now()
    assert_equal([1, 1, 0], [g:b1.Get().n, g:old_i.Get().n,
          instanceof(nb1, mg.A<mk.K>)])
  END
  call writefile(lines, 'XgenericArgMain.vim', 'D')
  source XgenericArgMain.vim
  unlet g:b1 g:b2 g:old_i
  call test_garbagecollect_now()
endfunc

" Test for reloading a script defining generic classes while objects, objects
" used through an interface, method partials and lambdas are kept, with
" concrete classes that failed to be created
func Test_generic_class_reload_with_kept_values()
  let lines =<< trim END
    vim9script
    interface I<T>
      def Get(): T
    endinterface
    class A<T> implements I<T>
      var v: T
      static var all: list<A<T>> = []
      def Get(): T
        return this.v
      enddef
    endclass
    class B1<T> extends A<T>
    endclass
    class B2<T> extends A<T>
      static var bad: T = g:Maybe()
    endclass
    class B3 extends A<number>
    endclass
    class Self<T> implements I<Self<T>>
      public var me: Self<T>
      def Get(): Self<T>
        return this.me
      enddef
    endclass
    var b1 = B1<number>.new(1)
    var b3 = B3.new(3)
    g:keep += [b1, b3, A<number>.new(0)]
    try
      g:keep->add(B2<number>.new(2))
    catch /nope/
    endtry
    var s = Self<string>.new()
    s.me = s
    g:keep->add(s)
    var ii: I<number> = b1
    g:keep->add(ii)
    g:keep->add(b3.Get)
    g:keep->add(() => A<list<number>>.new([1]))
    A<number>.all->add(b1)
    A<number>.all->add(b3)
    g:keep->add(A<number>.all)
  END
  call writefile(lines, 'XgenericKeep.vim', 'D')
  let g:keep = []
  let g:n = 0
  func g:Maybe()
    let g:n += 1
    if g:n % 2 == 1
      throw 'nope'
    endif
    return 1
  endfunc
  for i in range(3)
    source XgenericKeep.vim
    if i == 1
      call test_garbagecollect_now()
    endif
  endfor
  " Only the second time creating B2<number> succeeds
  call assert_equal(25, len(g:keep))
  call assert_equal('object<B2<number>>', typename(g:keep[11]))
  call assert_equal([1, 3, 0, 2], g:keep[8 : 11]->mapnew({_, o -> o.Get()}))
  call assert_equal(1, g:keep[0].Get())
  call assert_equal(1, g:keep[4].Get())
  let F = g:keep[5]
  call assert_equal(3, F())
  call assert_equal([1], g:keep[6]().v)
  call assert_true(g:keep[3].Get() is g:keep[3])
  call assert_equal([1, 3], g:keep[16]->mapnew({_, o -> o.Get()}))
  unlet g:keep[0 : 2]
  call test_garbagecollect_now()
  call assert_equal(22, len(g:keep))
  call assert_true(g:keep[0].Get() is g:keep[0])
  let F = g:keep[2]
  call assert_equal(3, F())
  unlet F
  unlet g:keep g:n
  delfunc g:Maybe
  call test_garbagecollect_now()
endfunc

" Test for a class variable initializer failing after other class variables
" were set to objects and lambdas
func Test_generic_class_init_failed_partly_set()
  let lines =<< trim END
    vim9script
    g:Bad = "str"
    class Node
      var next: any
    endclass
    class Box<T>
      static var o: Node = Node.new()
      static var me: Box<T> = Box<T>.new()
      static var l: list<any> = [0, () => Box<T>.new()]
      static var d: dict<any> = {self: Box<T>.new()}
      static var bad: T = g:Bad
      var v: T
    endclass
    g:errs = []
    for i in range(3)
      try
        echo Box<number>.o
      catch
        g:errs->add(v:exception)
      endtry
      test_garbagecollect_now()
    endfor
    g:Bad = 3
    g:res = [Box<number>.me.v, Box<number>.l[1]().v, Box<number>.bad]
  END
  call writefile(lines, 'XgenericPartlySet.vim', 'D')
  source XgenericPartlySet.vim
  call assert_equal(repeat(['Vim(echo):E1382: Variable "Box<number>.bad": type mismatch, expected number but got string'], 3), g:errs)
  call assert_equal([0, 0, 3], g:res)
  call test_garbagecollect_now()
  unlet g:errs g:res g:Bad
endfunc

" Test for comparing objects of different concrete classes
def Test_generic_class_object_compare()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
    endclass
    class Sub<T> extends Box<T>
    endclass
    assert_false(Box<number>.new(1) == Box<string>.new('1'))
    assert_true(Box<number>.new(1) != Box<string>.new('1'))
    assert_true(Box<number>.new(1) isnot Box<string>.new('1'))
    var a: any = Box<number>.new(1)
    var b: any = Box<string>.new('1')
    assert_false(a == b)
    assert_false([a] == [b])
    assert_true({x: a} == {x: Box<number>.new(1)})
    assert_true(Box<list<number>>.new([1]) == Box<list<number>>.new([1]))
    assert_false(Box<number>.new(1) == Sub<number>.new(1))
    assert_fails('echo Box<number>.new(1) == 1', 'E1072: Cannot compare object with number')

    def F()
      var x: Box<number> = null_object
      var y: Box<string> = null_object
      var z: Sub<number> = Sub<number>.new(2)
      var w: Box<number> = z
      var p: any = Box<number>.new(1)
      var q: any = Box<string>.new('1')
      assert_equal([true, true, true, true, true, false, true, true],
        [x == null, x == y, w == z, w is z, x is y, p == q,
         p == Box<number>.new(1), z == Sub<number>.new(2)])
    enddef
    F()
    var n: Box<number> = null_object
    assert_fails('echo n.v', 'E1360: Using a null object')
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for non-generic classes and enums using concrete generic classes
def Test_generic_class_with_non_generic_class()
  var lines =<< trim END
    vim9script
    class Box<T>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    interface I<T>
      def Get(): T
    endinterface
    class NB extends Box<number> implements I<number>
    endclass
    class N
      var x: number = 1
    endclass
    class GB<T> extends N implements I<T>
      var v: T
      def Get(): T
        return this.v
      enddef
    endclass
    enum Color
      Red(Box<string>.new('r')),
      Green(Box<string>.new('g'))
      var b: Box<string>
      def Name(): string
        return this.b.Get()
      enddef
    endenum

    var nb = NB.new(3)
    assert_equal('object<NB>', typename(nb))
    assert_equal([1, 0, 1, 0], [instanceof(nb, Box<number>),
          instanceof(nb, Box<string>), instanceof(nb, I<number>),
          instanceof(nb, I<string>)])
    var bn: Box<number> = nb
    var ii: I<number> = nb
    assert_equal([3, 3], [bn.Get(), ii.Get()])
    var gb = GB<string>.new(1, 'q')
    assert_equal([1, 'q'], [gb.x, gb.Get()])
    assert_equal([1, 1, 0], [instanceof(gb, N), instanceof(gb, GB<string>),
          instanceof(gb, GB<number>)])
    var nn: N = gb
    assert_equal('object<GB<string>>', typename(nn))
    assert_equal(['r', 'object<Box<string>>'], [Color.Red.Name(),
          typename(Color.Green.b)])
    assert_equal('object of NB {v: 3}', string(nb))
    assert_equal("object of GB<string> {x: 1, v: 'q'}", string(gb))
    assert_equal("enum Color.Red {name: 'Red', ordinal: 0, b: object of Box<string> {v: 'r'}}",
          string(Color.Red))
    assert_true(deepcopy(nb) is nb)
    assert_equal([1], deepcopy(Box<list<number>>.new([1])).v)
    assert_fails('json_encode(Box<number>.new(1))', 'E1161: Cannot json encode a object')
    assert_equal(v:t_class, type(Box<number>))
    assert_equal(['class<Box<number>>', 'class<I<number>>', 'class<NB>'],
          [typename(Box<number>), typename(I<number>), typename(NB)])
    def Check(x: I<number>): number
      return x.Get()
    enddef
    assert_equal(3, Check(nb))
    assert_fails('Check(gb)', 'E1013: Argument 1: type mismatch, expected object<I<number>> but got object<GB<string>>')
  END
  v9.CheckSourceSuccess(lines)

  # runtime type checks of concrete classes
  lines =<< trim END
    vim9script
    class Box<T>
      var v: T
    endclass
    class Sub<T> extends Box<T>
    endclass
    def R(x: any): Box<number>
      return x
    enddef
    def RS(x: any): Box<number>
      return <Box<number>>x
    enddef
    def Arg(b: Box<number>): string
      return typename(b)
    enddef
    var errs = [
      ['R(Box<string>.new("s"))', 'E1012: Type mismatch; expected object<Box<number>> but got object<Box<string>>'],
      ['R(Sub<string>.new("x"))', 'E1012: Type mismatch; expected object<Box<number>> but got object<Sub<string>>'],
      ['RS(Box<list<number>>.new([1]))', 'E1012: Type mismatch; expected object<Box<number>> but got object<Box<list<number>>>'],
      ['Arg(<any>Sub<string>.new("s"))', 'E1013: Argument 1: type mismatch, expected object<Box<number>> but got object<Sub<string>>'],
      ['R(Box<number>)', 'E1405: Class "Box<number>" cannot be used as a value'],
      ['R({})', 'E1012: Type mismatch; expected object<Box<number>> but got dict<any>'],
    ]
    for [e, err] in errs
      assert_fails('eval(e)', err)
    endfor
    assert_equal('object<Sub<number>>', typename(R(Sub<number>.new(1))))
    assert_equal('object<Sub<number>>', Arg(Sub<number>.new(1)))

    var d: dict<Box<number>> = {}
    assert_fails('d["a"] = <any>Box<string>.new("a")', 'E1012: Type mismatch; expected object<Box<number>> but got object<Box<string>>')
    var t: tuple<Box<number>, Box<string>> = (Box<number>.new(1), Box<string>.new('a'))
    assert_fails('var t2: tuple<Box<number>, Box<number>> = <any>t', 'E1012: Type mismatch; expected tuple<object<Box<number>>, object<Box<number>>> but got tuple<object<Box<number>>, object<Box<string>>>')
    assert_fails('var Fn2: func(Box<string>): string = <any>Arg', 'E1012: Type mismatch; expected func(object<Box<string>>): string but got func(object<Box<number>>): string')
  END
  v9.CheckSourceSuccess(lines)
enddef


" Test for running out of memory while defining or creating generic classes.
" An error is given and the classes can be used when sourced again.
func Test_generic_class_alloc_fail()
  let lines =<< trim END
    vim9script
    interface I<T>
      def Get(): T
    endinterface
    class A<T>
      var a: T
      var n: number = 1 + 2
      static var cnt: list<T> = []
      def new(this.a)
      enddef
      def GetA(): T
        return this.a
      enddef
      def Wrap<X>(x: X): list<X>
        return [x]
      enddef
    endclass
    class B<T> extends A<T> implements I<T>
      def new(this.a)
      enddef
      def Get(): T
        return this.a
      enddef
    endclass
    class Pair<K, V>
      var k: K
      var v: V
      def Conv(F: func(K): V, ...l: list<K>): func(V): K
        return (x: V): K => this.k
      enddef
    endclass
    enum Color
      Red,
      Green('g')
      var s: string = 'r'
      def new(s: string = 'r')
        this.s = s
      enddef
    endenum
    if exists('g:alloc_id')
      test_alloc_fail(g:alloc_id, g:alloc_cnt, 0)
    endif
    var b = B<number>.new(4)
    var i: I<number> = b
    var p = Pair<list<number>, dict<string>>.new([1], {a: 'x'})
    g:res = [b.Get(), i.Get(), b.Wrap<string>('x'), p.k,
      typename(p.Conv((k) => ({}))), b.n, Color.Green.s, len(Color.values)]
  END
  call writefile(lines, 'XgenericAllocFail.vim', 'D')
  let expected = [4, 4, ['x'], [1], 'func(dict<string>): list<number>', 3,
	\ 'g', 2]
  source XgenericAllocFail.vim
  call assert_equal(expected, g:res)

  " The allocation fails when defining the classes ("def") or when creating
  " the concrete classes ("new").  Every allocation with the ID is made to fail
  " once, until sourcing the script does not fail.
  " The lists must be referenced, test_garbagecollect_now() is used.
  let s:cases = [['type_ptr', 'def'], ['type_ptr', 'new'],
	\ ['func_type_args', 'def'], ['func_type_args', 'new'],
	\ ['copy_func_argtypes', 'def'], ['copy_func_argtypes', 'new'],
	\ ['class_methods', 'def'], ['class_interfaces', 'def'],
	\ ['class_members', 'def'], ['class_add_member', 'def'],
	\ ['expr_concat', 'def'], ['func_this_arg', 'def'],
	\ ['default_new', 'def']]
  for [name, when] in s:cases
    let id = GetAllocId(name)
    let cnt = 0
    while cnt < 200
      if when == 'def'
	call test_alloc_fail(id, cnt, 0)
      else
	let g:alloc_id = id
	let g:alloc_cnt = cnt
      endif
      unlet! g:res
      let failed = 0
      try
	source XgenericAllocFail.vim
      catch /E342:/
	let failed = 1
      endtry
      unlet! g:alloc_id
      if !failed
	" use up the failure that was not used
	call test_alloc_fail(id, 0, 0)
	try
	  source XgenericAllocFail.vim
	catch /E342:/
	endtry
	break
      endif
      call test_garbagecollect_now()
      source XgenericAllocFail.vim
      call assert_equal(expected, g:res, name .. ' ' .. when .. ' ' .. cnt)
      let cnt += 1
    endwhile
    " the allocation failed at least once
    call assert_true(cnt > 0, name .. ' ' .. when)
  endfor
  unlet! g:res g:alloc_cnt s:cases
  call test_garbagecollect_now()
endfunc

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
