" Test Vim9 type aliases

import './util/vim9.vim' as v9

" Test for :type command to create type aliases
def Test_typealias()
  # Use type alias at script level
  var lines =<< trim END
    vim9script
    type ListOfStrings = list<string>
    def Foo(a: ListOfStrings): ListOfStrings
      return a
    enddef
    var b: ListOfStrings = ['a', 'b']
    assert_equal(['a', 'b'], b)
    assert_equal(['e', 'f'], Foo(['e', 'f']))
    assert_equal('typealias<list<string>>', typename(ListOfStrings))
    assert_equal(v:t_typealias, type(ListOfStrings))
    assert_equal('ListOfStrings', string(ListOfStrings))
    assert_fails('var x = null == ListOfStrings', 'E1403: Type alias "ListOfStrings" cannot be used as a value')
  END
  v9.CheckSourceSuccess(lines)

  # Use type alias at def function level
  lines =<< trim END
    vim9script
    type ListOfStrings = list<string>
    def Foo(a: ListOfStrings): ListOfStrings
      return a
    enddef
    def Bar()
      var c: ListOfStrings = ['c', 'd']
      assert_equal(['c', 'd'], c)
      assert_equal(['e', 'f'], Foo(['e', 'f']))
      assert_equal('typealias<list<string>>', typename(ListOfStrings))
      assert_equal(v:t_typealias, type(ListOfStrings))
      assert_equal('ListOfStrings', string(ListOfStrings))
      #assert_equal(false, null == ListOfStrings)
    enddef
    Bar()
  END
  v9.CheckSourceSuccess(lines)

  # Use :type outside a Vim9 script
  lines =<< trim END
    type Index = number
  END
  v9.CheckSourceFailure(lines, 'E1393: Type can only be defined in Vim9 script', 1)

  # The complete "type" should be specified
  lines =<< trim END
    vim9script
    typ Index = number
  END
  v9.CheckSourceFailure(lines, 'E1065: Command cannot be shortened: typ Index = number', 2)

  # Use :type without any arguments
  lines =<< trim END
    vim9script
    type
  END
  v9.CheckSourceFailure(lines, 'E1397: Missing type alias name', 2)

  # Use :type with a name but no type
  lines =<< trim END
    vim9script
    type MyType
  END
  v9.CheckSourceFailure(lines, "E398: Missing '=': ", 2)

  # Use :type with a name but no type following "="
  lines =<< trim END
    vim9script
    type MyType =
  END
  v9.CheckSourceFailure(lines, 'E1398: Missing type alias type', 2)

  # No space before or after "="
  lines =<< trim END
    vim9script
    type MyType=number
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: MyType=number', 2)

  # No space after "="
  lines =<< trim END
    vim9script
    type MyType =number
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after '=': =number", 2)

  # type alias without "="
  lines =<< trim END
    vim9script
    type Index number
  END
  v9.CheckSourceFailure(lines, "E398: Missing '=': number", 2)

  # type alias for a non-existing type
  lines =<< trim END
    vim9script
    type Index = integer
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: integer', 2)

  # type alias starting with lower-case letter
  lines =<< trim END
    vim9script
    type index = number
  END
  v9.CheckSourceFailure(lines, 'E1394: Type name must start with an uppercase letter: index = number', 2)

  # No white space following the alias name
  lines =<< trim END
    vim9script
    type Index:number
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: Index:number', 2)

  # something following the type alias
  lines =<< trim END
    vim9script
    type ListOfNums = list<number> string
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters:  string', 2)

  # type alias name collides with a variable name
  lines =<< trim END
    vim9script
    var ListOfNums: number = 10
    type ListOfNums = list<number>
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "ListOfNums"', 3)

  # duplicate type alias name
  lines =<< trim END
    vim9script
    type MyList = list<number>
    type MyList = list<string>
  END
  v9.CheckSourceFailure(lines, 'E1396: Type alias "MyList" already exists', 3)

  # def function argument name collision with a type alias
  lines =<< trim END
    vim9script
    type A = list<number>
    def Foo(A: number)
    enddef
  END
  v9.CheckSourceFailure(lines, 'E1168: Argument already declared in the script: A: number)', 3)

  # def function local variable name collision with a type alias
  lines =<< trim END
    vim9script
    type A = list<number>
    def Foo()
      var A: number = 10
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1054: Variable already declared in the script: A', 1)

  # type alias a variable
  lines =<< trim END
    vim9script
    var A: list<number> = []
    type B = A
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: A', 3)

  # type alias a class
  lines =<< trim END
    vim9script
    class C
    endclass
    type AC = C
    assert_equal('class<C>', typename(AC))
  END
  v9.CheckSourceSuccess(lines)

  # another command follows a type alias
  lines =<< trim END
    vim9script
    type MyType = number | var x = 20
    assert_equal(20, x)
  END
  v9.CheckSourceSuccess(lines)

  # Sourcing a script twice (which will free script local variables)
  # Uses "lines" from the previous test
  new
  setline(1, lines)
  :source
  :source
  bw!

  # type alias a type alias
  lines =<< trim END
    vim9script
    type A = string
    type B = A
    var b: B = 'abc'
    assert_equal('abc', b)
    def Foo()
      var c: B = 'def'
      assert_equal('def', c)
    enddef
    Foo()
  END
  v9.CheckSourceSuccess(lines)

  # Assigning to a type alias (script level)
  lines =<< trim END
    vim9script
    type MyType = list<number>
    MyType = [1, 2, 3]
  END
  v9.CheckSourceFailure(lines, 'E1403: Type alias "MyType" cannot be used as a value', 3)

  # Assigning a type alias (def function level)
  lines =<< trim END
    vim9script
    type A = list<string>
    def Foo()
      var x = A
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # Using type alias in an expression (script level)
  lines =<< trim END
    vim9script
    type MyType = list<number>
    assert_fails('var m = MyType', 'E1403: Type alias "MyType" cannot be used as a value')
    assert_fails('var i = MyType + 1', 'E1403: Type alias "MyType" cannot be used as a value')
    assert_fails('var f = 1.0 + MyType', 'E1403: Type alias "MyType" cannot be used as a value')
    assert_fails('MyType += 10', 'E1403: Type alias "MyType" cannot be used as a value')
    assert_fails('var x = $"-{MyType}-"', 'E1403: Type alias "MyType" cannot be used as a value')
    assert_fails('var x = MyType[1]', 'E1403: Type alias "MyType" cannot be used as a value')
  END
  v9.CheckSourceSuccess(lines)

  # Using type alias in an expression (def function level)
  lines =<< trim END
    vim9script
    type MyType = list<number>
    def Foo()
      var x = MyType + 1
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # Using type alias in an expression (def function level)
  lines =<< trim END
    vim9script
    type MyType = list<number>
    def Foo()
      MyType = list<string>
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E46: Cannot change read-only variable "MyType"', 1)

  # Using type alias in an expression (def function level)
  lines =<< trim END
    vim9script
    type MyType = list<number>
    def Foo()
      MyType += 10
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E46: Cannot change read-only variable "MyType"', 1)

  # Convert type alias to a string (def function level)
  lines =<< trim END
    vim9script
    type MyType = list<number>
    def Foo()
      var x = $"-{MyType}-"
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1105: Cannot convert typealias to string', 1)

  # Using type alias as a float
  lines =<< trim END
    vim9script
    type B = number
    sort([1.1, B], 'f')
  END
  v9.CheckSourceFailure(lines, 'E1403: Type alias "B" cannot be used as a value', 3)

  # Creating a typealias in a def function
  lines =<< trim END
    vim9script
    def Foo()
      var n: number = 10
      type A = list<string>
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1399: Type can only be used in a script', 2)

  # json_encode should fail with a type alias
  lines =<< trim END
    vim9script
    type A = list<string>
    var x = json_encode(A)
  END
  v9.CheckSourceFailure(lines, 'E1403: Type alias "A" cannot be used as a value', 3)

  # Comparing type alias with a number (script level)
  lines =<< trim END
    vim9script
    type A = list<string>
    var n: number
    var x = A == n
  END
  v9.CheckSourceFailure(lines, 'E1403: Type alias "A" cannot be used as a value', 4)

  # Comparing type alias with a number (def function level)
  lines =<< trim END
    vim9script
    type A = list<string>
    def Foo()
      var n: number
      var x = A == n
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 2)

  # casting a number to a type alias (script level)
  lines =<< trim END
    vim9script
    type MyType = bool
    assert_equal(true, <MyType>1 == true)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for type checking through a type alias
def Test_typealias_type_check()
  # script variable initialized with a value of the wrong type
  var lines =<< trim END
    vim9script
    type ListOfStr = list<string>
    var x: ListOfStr = [1]
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<string> but got list<number>', 3)

  # argument of the wrong type passed to a def function
  lines =<< trim END
    vim9script
    type ListOfStr = list<string>
    def Foo(l: ListOfStr)
    enddef
    def Bar()
      Foo([1])
    enddef
    Bar()
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected list<string> but got list<number>', 1)

  # returning a value of the wrong type
  lines =<< trim END
    vim9script
    type ListOfStr = list<string>
    def Foo(): ListOfStr
      return [1]
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<string> but got list<number>', 1)

  # type error detected when compiling a def function
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo()
      var l: ListOfNum = []
      l->add('a')
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 2)

  # two type aliases for the same type can be used interchangeably
  lines =<< trim END
    vim9script
    type A = list<number>
    type B = list<number>
    var a: A = [1]
    var b: B = a
    assert_equal([1], b)
    def Foo(x: B): A
      return x
    enddef
    assert_equal([1], Foo(a))
  END
  v9.CheckSourceSuccess(lines)

  # type aliases for different types are not compatible
  lines =<< trim END
    vim9script
    type A = list<number>
    type B = list<string>
    var a: A = [1]
    var b: B = a
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<string> but got list<number>', 5)

  # func, tuple and dict types
  lines =<< trim END
    vim9script
    type NumToStr = func(number): string
    var Fn: NumToStr = (s: string): string => s
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(number): string but got func(string): string', 3)

  lines =<< trim END
    vim9script
    type Pair = tuple<number, string>
    var t: Pair = (1, 2)
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected tuple<number, string> but got tuple<number, number>', 3)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    var d: dict<ListOfNum> = {a: ['x']}
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected dict<list<number>> but got dict<list<string>>', 3)

  # casting to a type alias type
  lines =<< trim END
    vim9script
    type Num = number
    def Foo()
      var x = <Num>'a'
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 1)

  # type alias for an enum
  lines =<< trim END
    vim9script
    enum E1
      X
    endenum
    enum E2
      Y
    endenum
    type EnumAlias = E1
    def Foo(e: EnumAlias)
    enddef
    def Bar()
      Foo(E2.Y)
    enddef
    Bar()
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected enum<E1> but got enum<E2>', 1)

  # type alias in a tuple with a variadic list type
  lines =<< trim END
    vim9script
    type Num = number
    var t: tuple<string, ...list<Num>> = ('a', 1, 2)
    var u: tuple<string, ...list<Num>> = ('a', 'b')
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected tuple<string, ...list<number>> but got tuple<string, string>', 4)

  # interface method signature using a type alias
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    interface I
      def F(x: ListOfNum)
    endinterface
    class C implements I
      def F(x: list<string>)
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "F": type mismatch, expected func(list<number>) but got func(list<string>)', 9)
enddef

" Test for using a type alias in various type contexts
def Test_typealias_in_type_contexts()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>

    var Fn: func(ListOfNum): ListOfNum = (x: ListOfNum): ListOfNum => x
    assert_equal([1], Fn([1]))
    assert_equal('func(list<number>): list<number>', typename(Fn))

    var d: dict<ListOfNum> = {a: [1]}
    assert_equal('dict<list<number>>', typename(d))
    var t: tuple<ListOfNum, string> = ([1], 'a')
    assert_equal('tuple<list<number>, string>', typename(t))

    interface I
      def Get(x: ListOfNum): ListOfNum
    endinterface
    class C implements I
      def Get(x: list<number>): list<number>
        return x
      enddef
    endclass
    assert_equal([2], C.new().Get([2]))

    def G<T>(x: T): T
      return x
    enddef
    assert_equal([3], G<ListOfNum>([3]))

    def Foo(): string
      var l = <ListOfNum>[4]
      return typename(l)
    enddef
    assert_equal('list<number>', Foo())
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for name conflicts between a type alias and other script items
def Test_typealias_name_conflict()
  var lines =<< trim END
    vim9script
    def Foo()
    enddef
    type Foo = number
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Foo"', 4)

  lines =<< trim END
    vim9script
    type Foo = number
    def Foo()
    enddef
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Foo"', 3)

  lines =<< trim END
    vim9script
    export type MyType = number
  END
  writefile(lines, 'Xtypeconflict.vim', 'D')
  lines =<< trim END
    vim9script
    import './Xtypeconflict.vim' as Imp
    type Imp = number
  END
  v9.CheckScriptFailure(lines, 'E1041: Redefining script item: "Imp"', 3)
enddef

" Test for repeating or retrying a type alias declaration
def Test_typealias_redeclare()
  # A failed declaration does not define the name
  var lines =<< trim END
    vim9script
    try
      type A = NoSuchType
    catch /E1010:/
    endtry
    type A = number
    assert_equal('typealias<number>', typename(A))
  END
  v9.CheckSourceSuccess(lines)

  # A rejected duplicate keeps the original type alias
  lines =<< trim END
    vim9script
    type A = number
    try
      type A = string
    catch /E1396:/
    endtry
    assert_equal('typealias<number>', typename(A))
    var x: A = 1
    assert_fails('x = "a"', 'E1012: Type mismatch; expected number but got string')
  END
  v9.CheckSourceSuccess(lines)

  # A type alias declared in a loop body is local to the loop block
  lines =<< trim END
    vim9script
    var names: list<string>
    for i in range(2)
      type LoopType = list<number>
      add(names, typename(LoopType))
    endfor
    assert_equal(['typealias<list<number>>', 'typealias<list<number>>'], names)
  END
  v9.CheckSourceSuccess(lines)

  # :type executed with :execute in a script
  lines =<< trim END
    vim9script
    execute 'type ExeType = list<string>'
    var x: ExeType = ['a']
    assert_equal('typealias<list<string>>', typename(ExeType))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a type alias for variable arguments and default values
def Test_typealias_func_args()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo(...args: ListOfNum): number
      return len(args)
    enddef
    def Bar(x: ListOfNum = [1]): ListOfNum
      return x
    enddef
    def Baz()
      assert_equal(2, Foo(1, 2))
      assert_equal([1], Bar())
    enddef
    Baz()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo(...args: ListOfNum)
    enddef
    def Bar()
      Foo(1, 'a')
    enddef
    Bar()
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 2: type mismatch, expected number but got string', 1)

  lines =<< trim END
    vim9script
    type Num = number
    def Foo(...args: Num)
    enddef
  END
  v9.CheckSourceFailure(lines, 'E1180: Variable arguments type must be a list: Num', 3)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo(x: ListOfNum = ['a'])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>', 0)
enddef

" Test for using a type alias for a for loop variable
def Test_typealias_for_loop()
  var lines =<< trim END
    vim9script
    type Num = number
    var total = 0
    for n: Num in [1, 2]
      total += n
    endfor
    assert_equal(3, total)
    def Foo(): number
      var sum = 0
      for n: Num in [3, 4]
        sum += n
      endfor
      return sum
    enddef
    assert_equal(7, Foo())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type Num = number
    for n: Num in ['a']
    endfor
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 3)

  lines =<< trim END
    vim9script
    type Num = number
    def Foo()
      for n: Num in ['a']
      endfor
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 1)
enddef

" Test for a null value, a const and a final variable with a type alias type
def Test_typealias_null_const_final()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    var l: ListOfNum = null_list
    assert_true(l == null)
    final f: ListOfNum = [1]
    f->add(2)
    assert_equal([1, 2], f)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    const c: ListOfNum = [1]
    c->add(2)
  END
  v9.CheckSourceFailure(lines, 'E741: Value is locked: add() argument', 4)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    final f: ListOfNum = [1]
    f = []
  END
  v9.CheckSourceFailure(lines, 'E46: Cannot change read-only variable "f"', 4)

  # The type of the list is still checked
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    final f: ListOfNum = [1]
    f->add('a')
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 4)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    var l: ListOfNum = null_list
    l = ['a']
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 4)
enddef

" Test for using a type alias in a class or an interface
def Test_typealias_in_class()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    interface I
      var iv: ListOfNum
    endinterface
    class C implements I
      var iv: list<number> = [0]
      var m: ListOfNum = [1]
      static var s: ListOfNum = [2]
      def Get(x: ListOfNum): ListOfNum
        return this.m + x
      enddef
    endclass
    class D
      var m: ListOfNum
      def new(this.m)
      enddef
    endclass
    var o = C.new()
    assert_equal([1, 3], o.Get([3]))
    assert_equal([2], C.s)
    assert_equal([0], o.iv)
    assert_equal([4], D.new([4]).m)
  END
  v9.CheckSourceSuccess(lines)

  # assigning a value of the wrong type to an object variable
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class C
      public var m: ListOfNum = [1]
    endclass
    var o = C.new()
    o.m = ['a']
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 7)

  # initializing an object variable with a value of the wrong type
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class C
      var m: ListOfNum = ['a']
    endclass
    var o = C.new()
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "m": type mismatch, expected list<number> but got list<string>', 1)

  # assigning a value of the wrong type to a class variable
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class C
      public static var s: ListOfNum = [1]
    endclass
    C.s = ['a']
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 6)

  # passing an argument of the wrong type to a method
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class C
      def Get(x: ListOfNum): ListOfNum
        return x
      enddef
    endclass
    def Foo()
      C.new().Get(['a'])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>', 1)

  # passing an argument of the wrong type to new()
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class C
      var m: ListOfNum
      def new(this.m)
      enddef
    endclass
    def Foo()
      C.new(['a'])
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>', 1)

  # interface variable type does not match
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    interface I
      var iv: ListOfNum
    endinterface
    class C implements I
      var iv: list<string> = []
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "iv": type mismatch, expected list<number> but got list<string>', 8)
enddef

" Test for using an exported type alias for a class and a type alias in a def
" function in another script
def Test_typealias_import_more()
  var lines =<< trim END
    vim9script
    export class C
      var v = 5
    endclass
    export type ClassAlias = C
    export type ListOfNum = list<number>
  END
  writefile(lines, 'Xtypeexportmore.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xtypeexportmore.vim' as Imp
    var o: Imp.ClassAlias = Imp.ClassAlias.new()
    assert_equal('object<C>', typename(o))
    assert_equal(5, o.v)
    def Foo(): string
      var l: Imp.ListOfNum = [1]
      return typename(Imp.ListOfNum) .. ' ' .. typename(l)
    enddef
    assert_equal('typealias<list<number>> list<number>', Foo())
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for the order of defining a type alias and using it
def Test_typealias_declaration_order()
  # The body of a function is compiled when it is called
  var lines =<< trim END
    vim9script
    def Foo(): string
      var x: Later = 1
      return typename(x)
    enddef
    type Later = number
    assert_equal('number', Foo())
  END
  v9.CheckSourceSuccess(lines)

  # The argument types are parsed when the function is defined
  lines =<< trim END
    vim9script
    def Foo(x: Later)
    enddef
    type Later = number
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: Later', 2)

  lines =<< trim END
    vim9script
    def Foo()
      var x: Later = 1
    enddef
    defcompile Foo
    type Later = number
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: Later', 1)
enddef

" Test for using a type alias in a legacy function
def Test_typealias_legacy_func()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    func LegacyFoo()
      return typename(s:ListOfNum)
    endfunc
    assert_equal('typealias<list<number>>', LegacyFoo())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    func LegacyFoo()
      let x = s:ListOfNum
    endfunc
    call LegacyFoo()
  END
  v9.CheckSourceFailure(lines, 'E1403: Type alias "ListOfNum" cannot be used as a value', 1)
enddef

" Test for errors when defining or using a type alias
def Test_typealias_errors()
  # :type with the "legacy" modifier
  var lines =<< trim END
    vim9script
    legacy type A = number
  END
  v9.CheckSourceFailure(lines, 'E1393: Type can only be defined in Vim9 script', 2)

  # :type in a legacy function in a Vim9 script
  lines =<< trim END
    vim9script
    func LegacyFoo()
      type A = number
    endfunc
    call LegacyFoo()
  END
  v9.CheckSourceFailure(lines, 'E1393: Type can only be defined in Vim9 script', 1)

  # :type in a lambda in a def function
  lines =<< trim END
    vim9script
    def Foo()
      var Fn = () => {
        type A = number
      }
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1399: Type can only be used in a script', 1)

  # Name not starting with an upper case letter, or a missing name
  lines =<< trim END
    vim9script
    type _Foo = number
  END
  v9.CheckSourceFailure(lines, 'E1394: Type name must start with an uppercase letter: _Foo = number', 2)

  lines =<< trim END
    vim9script
    type = number
  END
  v9.CheckSourceFailure(lines, 'E1394: Type name must start with an uppercase letter: = number', 2)

  # Name that cannot be used for a variable
  lines =<< trim END
    vim9script
    type A#b = number
  END
  v9.CheckSourceFailure(lines, 'E461: Illegal variable name: A#b', 2)

  # Unknown type in a compound type
  lines =<< trim END
    vim9script
    type A = list<NoSuchType>
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: NoSuchType', 2)

  lines =<< trim END
    vim9script
    type A = func(NoSuchType): number
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: NoSuchType', 2)

  lines =<< trim END
    vim9script
    type A = list<number>>
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters: >', 2)

  # Name conflicts with a class, enum or interface
  lines =<< trim END
    vim9script
    class Foo
    endclass
    type Foo = number
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Foo"', 4)

  lines =<< trim END
    vim9script
    type Foo = number
    class Foo
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Foo"', 3)

  lines =<< trim END
    vim9script
    enum Foo
      X
    endenum
    type Foo = number
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Foo"', 5)

  lines =<< trim END
    vim9script
    type Foo = number
    enum Foo
      X
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Foo"', 3)

  lines =<< trim END
    vim9script
    type Foo = number
    interface Foo
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Foo"', 3)

  # A type alias cannot be deleted
  lines =<< trim END
    vim9script
    type A = number
    unlet A
  END
  v9.CheckSourceFailure(lines, 'E1081: Cannot unlet A', 3)

  lines =<< trim END
    vim9script
    type A = number
    def Foo()
      unlet A
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1081: Cannot unlet A', 1)

  # Imported type alias used as a value
  lines =<< trim END
    vim9script
    export type MyType = list<number>
  END
  writefile(lines, 'Xtypealiaserr.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xtypealiaserr.vim' as Imp
    var x = Imp.MyType
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "MyType" cannot be used as a value', 3)

  lines =<< trim END
    vim9script
    import './Xtypealiaserr.vim' as Imp
    def Foo()
      var x = Imp.MyType
    enddef
    Foo()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # Type alias for a class that is defined later
  lines =<< trim END
    vim9script
    type ClassAlias = C
    class C
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: C', 2)

  # White space before "<"
  lines =<< trim END
    vim9script
    type ListOfNum = list <number>
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '<':  <number>", 2)

  # Lambda argument with the name of a type alias
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    var Fn = (ListOfNum) => ListOfNum
  END
  v9.CheckSourceFailure(lines, 'E1168: Argument already declared in the script: ListOfNum) => ListOfNum', 3)

  # A type alias cannot refer to itself, it is not defined yet
  lines =<< trim END
    vim9script
    type A = list<A>
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: A', 2)

  lines =<< trim END
    vim9script
    type A = list<B>
    type B = list<A>
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: B', 2)
enddef

" Test for garbage collection with type aliases
func Test_typealias_garbagecollect()
  " Variables with a type alias type
  let lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    type DictOfList = dict<ListOfNum>
    type NumToStr = func(number): string
    type Pair = tuple<number, string>
    var l: ListOfNum = [1, 2]
    var d: DictOfList = {a: [3]}
    var Fn: NumToStr = (n: number): string => string(n)
    var t: Pair = (4, 'x')
    test_garbagecollect_now()
    assert_equal('typealias<list<number>>', typename(ListOfNum))
    assert_equal([1, 2], l)
    assert_equal({a: [3]}, d)
    assert_equal('5', Fn(5))
    assert_equal((4, 'x'), t)
    assert_fails('l = ["a"]', 'E1012: Type mismatch; expected list<number> but got list<string>')
    assert_fails('d = {a: ["b"]}', 'E1012: Type mismatch; expected dict<list<number>> but got dict<list<string>>')
    l->add(3)
    test_garbagecollect_now()
    assert_equal([1, 2, 3], l)
    assert_fails('l->add("b")', 'E1012: Type mismatch; expected number but got string')
  END
  call v9.CheckSourceSuccess(lines)

  " Function arguments and return value with a type alias type
  let lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo(x: ListOfNum): ListOfNum
      return x + [9]
    enddef
    test_garbagecollect_now()
    assert_equal([1, 9], Foo([1]))
    test_garbagecollect_now()
    assert_equal([2, 9], Foo([2]))
    assert_fails('Foo(["a"])', 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>')
  END
  call v9.CheckSourceSuccess(lines)

  " Values and lambdas with a type alias type in a reference cycle are freed
  let lines =<< trim END
    vim9script
    type DictOfAny = dict<any>
    type ListOfAny = list<any>
    var d: DictOfAny = {}
    d.self = d
    var l: ListOfAny = []
    l->add(l)
    var fd: DictOfAny = {}
    fd.Fn = (x: ListOfAny): number => len(x)
    fd.self = fd
    d = {}
    l = []
    fd = {}
    test_garbagecollect_now()
    d.x = 1
    l->add(2)
    assert_equal({x: 1}, d)
    assert_equal([2], l)
    var F = (x: ListOfAny): number => len(x)
    test_garbagecollect_now()
    assert_equal(2, F([1, 2]))
  END
  call v9.CheckSourceSuccess(lines)

  " Type alias for a class
  let lines =<< trim END
    vim9script
    class C
      public var other: any
      public static var cache: list<any> = []
    endclass
    type ClassAlias = C
    var o: ClassAlias = ClassAlias.new()
    o.other = o
    o = null_object
    ClassAlias.cache->add(ClassAlias.new())
    test_garbagecollect_now()
    assert_equal(1, len(ClassAlias.cache))
    var p: ClassAlias = ClassAlias.new()
    assert_equal('object<C>', typename(p))
    assert_true(instanceof(p, ClassAlias))
  END
  call v9.CheckSourceSuccess(lines)

  " Generic function with a type alias type argument
  let lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def G<T>(x: T): T
      return x
    enddef
    test_garbagecollect_now()
    assert_equal([1], G<ListOfNum>([1]))
    test_garbagecollect_now()
    assert_equal([2], G<ListOfNum>([2]))
    assert_fails('G<ListOfNum>(["a"])', 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>')
  END
  call v9.CheckSourceSuccess(lines)

  " Imported type alias
  let lines =<< trim END
    vim9script
    export type ImpList = list<number>
  END
  call writefile(lines, 'Xtypealiasgc.vim', 'D')
  let lines =<< trim END
    vim9script
    import './Xtypealiasgc.vim' as Imp
    var l: Imp.ImpList = [1]
    test_garbagecollect_now()
    assert_equal([1], l)
    assert_equal('typealias<list<number>>', typename(Imp.ImpList))
    assert_fails('l = ["a"]', 'E1012: Type mismatch; expected list<number> but got list<string>')
  END
  call v9.CheckScriptSuccess(lines)
endfunc

" Test for a type alias value in a reference cycle, which is freed by garbage
" collection
func Test_typealias_garbagecollect_refcount()
  let lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    assert_equal(1, test_refcount(ListOfNum))
    var sid = str2nr(matchstr(expand('<SID>'), '\d\+'))
    var vars = getscriptinfo({sid: sid})[0].variables
    assert_equal(2, test_refcount(ListOfNum))
    vars.self = vars
    vars = {}
    assert_equal(2, test_refcount(ListOfNum))
    test_garbagecollect_now()
    assert_equal(1, test_refcount(ListOfNum))
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" Test for a type alias for a class, interface, enum or object type
def Test_typealias_class_like_types()
  var lines =<< trim END
    vim9script
    class C
      static var count = 3
    endclass
    interface I
    endinterface
    enum E
      X
    endenum
    type CA = C
    type IA = I
    type EA = E
    type OA = object<C>
    assert_equal('class<C>', typename(CA))
    assert_equal(v:t_class, type(CA))
    assert_equal('class<I>', typename(IA))
    assert_equal('enum<E>', typename(EA))
    assert_equal('class<C>', typename(OA))
    assert_equal('object<C>', typename(OA.new()))
    assert_equal(3, CA.count)
    assert_equal(E.X, EA.X)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for assignment compatibility between a type alias and other types in
" each direction, checked when compiling and when executing
def Test_typealias_compatibility()
  # type alias to the aliased type and the aliased type to the type alias
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo(): list<number>
      var a: ListOfNum = [1]
      var u: list<number> = a
      return u
    enddef
    def Bar(): ListOfNum
      var u: list<number> = [2]
      var a: ListOfNum = u
      return a
    enddef
    assert_equal([1], Foo())
    assert_equal([2], Bar())
  END
  v9.CheckSourceSuccess(lines)

  # type alias to a type alias for another type
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    type ListOfStr = list<string>
    def Foo()
      var a: ListOfNum = [1]
      var s: ListOfStr = a
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<string> but got list<number>', 2)

  # another type to a type alias
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo()
      var s: list<string> = ['a']
      var a: ListOfNum = s
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 2)

  # A value of type "any" is checked when executing
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo(v: any): ListOfNum
      var a: ListOfNum = v
      return a
    enddef
    assert_equal([3], Foo([3]))
    Foo(['x'])
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 1)
enddef

" Test for the type of a value declared with a type alias, which is used when
" the value is changed and for type inference
def Test_typealias_value_type()
  # Adding an item of the wrong type, checked when executing
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo()
      var l: ListOfNum = [1]
      var x: any = 'a'
      l->add(x)
    enddef
    Foo()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 3)

  lines =<< trim END
    vim9script
    type DictOfNum = dict<number>
    var d: DictOfNum = {}
    var v: any = 'a'
    d.a = v
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 5)

  lines =<< trim END
    vim9script
    type DictOfNum = dict<number>
    var d: DictOfNum = {}
    extend(d, {a: 'x'})
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 2: type mismatch, expected dict<number> but got dict<string> in extend()', 4)

  # The type of a variable is inferred from a variable with a type alias type
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo(): string
      var a: ListOfNum = [1]
      var b = a
      return typename(b)
    enddef
    assert_equal('list<number>', Foo())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Foo()
      var a: ListOfNum = [1]
      var b = a
      b->add('a')
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got string', 3)
enddef

" Test for a type alias for a type alias for a class, and using type aliases
" for "extends" and "implements"
def Test_typealias_class_alias_chain()
  var lines =<< trim END
    vim9script
    class C
      var v = 1
    endclass
    type ClassAlias = C
    type ClassAlias2 = ClassAlias
    var o: ClassAlias2 = ClassAlias2.new()
    assert_equal('class<C>', typename(ClassAlias2))
    assert_equal(v:t_class, type(ClassAlias2))
    assert_equal('object<C>', typename(o))

    class D extends ClassAlias
    endclass
    assert_equal(1, D.new().v)
    assert_true(instanceof(D.new(), ClassAlias2))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class B extends ListOfNum
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend ListOfNum', 4)

  lines =<< trim END
    vim9script
    class A
    endclass
    type ClassAlias = A
    class B implements ClassAlias
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1347: Not a valid interface: ClassAlias', 6)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class B implements ListOfNum
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1347: Not a valid interface: ListOfNum', 4)
enddef

" Test for using a type alias in an overriding method
def Test_typealias_method_override()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class A
      def F(x: ListOfNum): ListOfNum
        return x
      enddef
    endclass
    class B extends A
      def F(x: list<number>): list<number>
        return x + [9]
      enddef
    endclass
    assert_equal([1, 9], B.new().F([1]))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    class A
      def F(x: ListOfNum): ListOfNum
        return x
      enddef
    endclass
    class B extends A
      def F(x: list<string>): list<number>
        return []
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "F": type mismatch, expected func(list<number>): list<number> but got func(list<string>): list<number>', 12)
enddef

" Test for using a type alias for an enum variable
def Test_typealias_in_enum()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    enum E
      X([1]), Y([2, 3])
      var vals: ListOfNum
      def new(this.vals)
      enddef
    endenum
    assert_equal([2, 3], E.Y.vals)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    enum E
      X(['a'])
      var vals: ListOfNum
      def new(this.vals)
      enddef
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>', 8)
enddef

" Test for a type alias used as a type argument in a compound type of a generic
" function parameter
def Test_typealias_generic_param()
  var lines =<< trim END
    vim9script
    type Num = number
    def G<T>(x: list<T>): T
      return x[0]
    enddef
    assert_equal(5, G<Num>([5]))
    G<Num>(['a'])
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected list<number> but got list<string>', 7)
enddef

" Test for type aliases for function types
def Test_typealias_func_types()
  var lines =<< trim END
    vim9script
    type OptFunc = func(?number): number
    def Opt(n: number = 1): number
      return n
    enddef
    var F1: OptFunc = Opt
    assert_equal(1, F1())
    assert_equal(4, F1(4))

    type VarargFunc = func(...list<number>): number
    var F2: VarargFunc = (...l: list<number>): number => len(l)
    assert_equal(3, F2(1, 2, 3))

    type AnyFunc = func
    var F3: AnyFunc = function('len')
    assert_equal(2, F3([1, 2]))

    type StrFunc = func(string): number
    var F4: StrFunc = function('len')
    assert_equal(3, F4('abc'))

    type ListOfNum = list<number>
    type Maker = func(): ListOfNum
    var F5: Maker = (): ListOfNum => [7]
    assert_equal([7], F5())
    assert_equal('func(): list<number>', typename(F5))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type StrFunc = func(string): number
    def G(n: number): number
      return n
    enddef
    var Fn: StrFunc = G
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(string): number but got func(number): number', 6)

  lines =<< trim END
    vim9script
    type NumFunc = func(number): number
    var d: dict<NumFunc> = {inc: (n: number): number => n + 1}
    assert_equal(2, d.inc(1))
    d.bad = (s: string): number => 0
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(number): number but got func(string): number', 5)

  lines =<< trim END
    vim9script
    type OptFunc = func(?number): number
    var Fn: OptFunc = (s: string): number => 0
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(?number): number but got func(string): number', 3)

  lines =<< trim END
    vim9script
    type VarargFunc = func(...list<number>): number
    var Fn: VarargFunc = (...l: list<string>): number => 0
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(...list<number>): number but got func(...list<string>): number', 3)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    type Maker = func(): ListOfNum
    var Fn: Maker = (): list<string> => ['a']
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(): list<number> but got func(): list<string>', 4)
enddef

" Test for type aliases for "any", "blob", "job" and "channel"
def Test_typealias_other_types()
  var lines =<< trim END
    vim9script
    type AnyType = any
    var x: AnyType = 1
    x = 'a'
    x = [1]
    assert_equal('list<number>', typename(x))
    assert_equal('typealias<any>', typename(AnyType))

    type BlobType = blob
    var b: BlobType = 0z0102
    assert_equal('blob', typename(b))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type BlobType = blob
    var b: BlobType = 'x'
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected blob but got string', 3)
enddef

def Test_typealias_job_channel()
  CheckFeature channel
  var lines =<< trim END
    vim9script
    type JobType = job
    type ChannelType = channel
    var j: JobType = null_job
    var c: ChannelType = null_channel
    assert_equal('job', typename(j))
    assert_equal('channel', typename(c))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type JobType = job
    var j: JobType = 1
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected job but got number', 3)

  lines =<< trim END
    vim9script
    type ChannelType = channel
    var c: ChannelType = null_job
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected channel but got job', 3)
enddef

" Test for exporting a type alias for an imported type alias
def Test_typealias_reexport()
  var lines =<< trim END
    vim9script
    export type Inner = list<number>
    export class InnerClass
    endclass
    export type InnerClassAlias = InnerClass
  END
  writefile(lines, 'Xtypealiasinner.vim', 'D')
  lines =<< trim END
    vim9script
    import './Xtypealiasinner.vim' as In
    export type Outer = In.Inner
    export type OuterClass = In.InnerClassAlias
  END
  writefile(lines, 'Xtypealiasouter.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xtypealiasouter.vim' as Out
    var l: Out.Outer = [1]
    var o: Out.OuterClass = Out.OuterClass.new()
    assert_equal('list<number>', typename(l))
    assert_equal('typealias<list<number>>', typename(Out.Outer))
    assert_equal('object<InnerClass>', typename(o))
    var bad: Out.Outer = ['x']
  END
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 8)
enddef

" Test for using a type alias from an autoload script
def Test_typealias_autoload()
  mkdir('Xtypealiasdir/autoload', 'pR')
  var lines =<< trim END
    vim9script
    export type AutoList = list<string>
    g:typealias_autoload_loaded = 1
  END
  writefile(lines, 'Xtypealiasdir/autoload/typealiasauto.vim')
  var save_rtp = &rtp
  exe 'set rtp^=' .. getcwd() .. '/Xtypealiasdir'

  # The autoload script is loaded when the function using the type alias is
  # compiled
  try
    lines =<< trim END
      vim9script
      import autoload 'typealiasauto.vim' as Auto
      def Foo(x: any): string
        var l: Auto.AutoList = x
        return typename(l)
      enddef
      assert_false(exists('g:typealias_autoload_loaded'))
      assert_equal('list<string>', Foo(['a']))
      assert_true(exists('g:typealias_autoload_loaded'))
      assert_fails('Foo([1])', 'E1012: Type mismatch; expected list<string> but got list<number>')
    END
    v9.CheckScriptSuccess(lines)
  finally
    &rtp = save_rtp
    unlet! g:typealias_autoload_loaded
  endtry
enddef

" Test for using a type alias in a nested function and a closure
def Test_typealias_nested_func_and_closure()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Outer(): ListOfNum
      def Inner(x: ListOfNum): ListOfNum
        return x + [2]
      enddef
      return Inner([1])
    enddef
    assert_equal([1, 2], Outer())

    def Make(): func(): ListOfNum
      var l: ListOfNum = [3]
      return (): ListOfNum => l
    enddef
    assert_equal([3], Make()())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    def Make(): func(): ListOfNum
      var s = ['a']
      return (): ListOfNum => s
    enddef
    Make()()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 1)
enddef

" Test for white space and a comment in a type alias declaration
def Test_typealias_declaration_syntax()
  var lines =<< trim END
    vim9script
    type   ListOfNum   =   list<number>
    type ListOfStr = list<string> # comment
    var l: ListOfNum = [1]
    var s: ListOfStr = ['a']
    assert_equal('typealias<list<number>>', typename(ListOfNum))
    assert_equal('typealias<list<string>>', typename(ListOfStr))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for type aliases for type aliases at several levels of a compound type.
" A type mismatch at the innermost level must be detected.
def Test_typealias_nested_compound()
  var lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    type DictOfList = dict<ListOfNum>
    type ListOfDict = list<DictOfList>
    var c: ListOfDict = [{k: [1]}]
    assert_equal('list<dict<list<number>>>', typename(c))
    var d: ListOfDict = [{k: ['x']}]
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<dict<list<number>>> but got list<dict<list<string>>>', 7)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    type DictOfList = dict<ListOfNum>
    type ListOfDict = list<DictOfList>
    def Foo()
      var c: ListOfDict = [{k: ['x']}]
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected list<dict<list<number>>> but got list<dict<list<string>>>', 1)

  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    type DictOfList = dict<ListOfNum>
    type ListOfDict = list<DictOfList>
    def Foo()
      var c: ListOfDict = [{k: [1]}]
      c[0].k->add('x')
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 2: type mismatch, expected number but got string', 2)

  # Argument checked when executing
  lines =<< trim END
    vim9script
    type ListOfNum = list<number>
    type DictOfList = dict<ListOfNum>
    def Foo(x: DictOfList): number
      return len(x)
    enddef
    var v: any = {k: ['x']}
    Foo(v)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected dict<list<number>> but got dict<list<string>>', 8)
enddef

" Test for type aliases with the same name in an imported script and in the
" importing script, they are different types
def Test_typealias_same_name_in_imported_script()
  var lines =<< trim END
    vim9script
    export type Same = list<number>
  END
  writefile(lines, 'Xtypealiassame.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xtypealiassame.vim' as Imp
    type Same = list<string>
    var a: Same = ['a']
    var b: Imp.Same = [1]
    assert_equal('typealias<list<string>>', typename(Same))
    assert_equal('typealias<list<number>>', typename(Imp.Same))
    var c: Same = [1]
  END
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected list<string> but got list<number>', 8)

  lines =<< trim END
    vim9script
    import './Xtypealiassame.vim' as Imp
    type Same = list<string>
    var b: Imp.Same = ['a']
  END
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected list<number> but got list<string>', 4)
enddef

" Test for exporting and importing type aliases
def Test_typealias_import()
  var lines =<< trim END
    vim9script
    export type MyType = list<number>
  END
  writefile(lines, 'Xtypeexport.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xtypeexport.vim' as A

    var myList: A.MyType = [1, 2, 3]
    def Foo(l: A.MyType)
      assert_equal([1, 2, 3], l)
    enddef
    Foo(myList)
  END
  v9.CheckScriptSuccess(lines)

  # Use a non existing type alias
  lines =<< trim END
    vim9script
    import './Xtypeexport.vim' as A

    var myNum: A.SomeType = 10
  END
  v9.CheckScriptFailure(lines, 'E1010: Type not recognized: A.SomeType', 4)

  # Use a type alias that is not exported
  lines =<< trim END
    vim9script
    type NewType = dict<string>
  END
  writefile(lines, 'Xtypeexport2.vim', 'D')
  lines =<< trim END
    vim9script
    import './Xtypeexport2.vim' as A

    var myDict: A.NewType = {}
  END
  v9.CheckScriptFailure(lines, 'E1049: Item not exported in script: NewType', 4)

  # Using the same name as an imported type alias
  lines =<< trim END
    vim9script
    export type MyType2 = list<number>
  END
  writefile(lines, 'Xtypeexport3.vim', 'D')
  lines =<< trim END
    vim9script
    import './Xtypeexport3.vim' as A

    type MyType2 = A.MyType2
    var myList1: A.MyType2 = [1, 2, 3]
    var myList2: MyType2 = [4, 5, 6]
    assert_equal([1, 2, 3], myList1)
    assert_equal([4, 5, 6], myList2)
  END
  v9.CheckScriptSuccess(lines)

  # Using an exported class to create a type alias
  lines =<< trim END
    vim9script
    export class MyClass
      var val = 10
    endclass
  END
  writefile(lines, 'Xtypeexport4.vim', 'D')
  lines =<< trim END
    vim9script
    import './Xtypeexport4.vim' as T

    type MyType3 = T.MyClass
    var c: MyType3 = MyType3.new()
    assert_equal(10, c.val)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for using typealias as a def function argument and return type
def Test_typealias_func_argument()
  var lines =<< trim END
    vim9script
    type A = list<number>
    def Foo(l: A): A
      assert_equal([1, 2], l)
      return l
    enddef
    var x: A = [1, 2]
    assert_equal([1, 2], Foo(x))
  END
  v9.CheckScriptSuccess(lines)

  # passing a type alias variable to a function expecting a specific type
  lines =<< trim END
    vim9script
    type A = list<number>
    def Foo(l: list<number>)
      assert_equal([1, 2], l)
    enddef
    var x: A = [1, 2]
    Foo(x)
  END
  v9.CheckScriptSuccess(lines)

  # passing a type alias variable to a function expecting any
  lines =<< trim END
    vim9script
    type A = list<number>
    def Foo(l: any)
      assert_equal([1, 2], l)
    enddef
    var x: A = [1, 2]
    Foo(x)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Using a type alias with a builtin function
def Test_typealias_with_builtin_functions()
  var lines =<< trim END
    vim9script
    type A = list<func>
    var x = empty(A)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "A" cannot be used as a value', 3)

  # Using a type alias with len()
  lines =<< trim END
    vim9script
    type A = list<func>
    var x = len(A)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "A" cannot be used as a value', 3)

  # Using a type alias with len()
  lines =<< trim END
    vim9script
    type A = list<func>
    def Foo()
      var x = len(A)
    enddef
    Foo()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # Using a type alias with eval()
  lines =<< trim END
    vim9script
    type A = number
    def Foo()
      var x = eval("A")
    enddef
    Foo()
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "A" cannot be used as a value', 1)
enddef

" Test for type alias refcount
def Test_typealias_refcount()
  var lines =<< trim END
    vim9script
    type A = list<func>
    assert_equal(1, test_refcount(A))
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    type B = list<number>
    var x: B = []
    assert_equal(1, test_refcount(B))
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for using instanceof() with a type alias
def Test_typealias_instanceof()
  var lines =<< trim END
    vim9script
    class C
    endclass

    type Ctype = C
    var o = C.new()
    assert_equal(1, instanceof(o, Ctype))
    type Ntype = number
    assert_fails('instanceof(o, Ntype)', 'E693: Class or class typealias required for argument 2')
    assert_fails('instanceof(o, Ctype, Ntype)', 'E693: Class or class typealias required for argument 3')

    def F()
      var x = instanceof(o, Ntype)
    enddef
    assert_fails('F()', 'E693: Class or class typealias required for argument 2')

    def G(): bool
      return instanceof(o, Ctype)
    enddef
    assert_equal(1, G())
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for type aliasing a class
def Test_typealias_class()
  var lines =<< trim END
    vim9script
    class C
      var color = 'green'
    endclass
    type MyClass = C
    var o: MyClass = MyClass.new()
    assert_equal('green', o.color)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for typealias as function arg and return value
def Test_type_as_func_argument_or_return_value()
  # check typealias as arg, function call in script level
  var lines =<< trim END
    vim9script
    type A = number
    def Foo(arg: any)
    enddef
    Foo(A)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "A" cannot be used as a value', 5)

  # check typealias as function return, function call in script level
  lines =<< trim END
    vim9script
    type A = number
    def Foo(): any
      return A
    enddef
    Foo()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # check typealias as arg, function call in :def
  lines =<< trim END
    vim9script
    type A = number
    def Foo(arg: any)
    enddef
    def F()
      Foo(A)
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # check typealias as function return, function call in :def
  lines =<< trim END
    vim9script
    type A = number
    def Foo(): any
      return A
    enddef
    def F()
      Foo()
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # check funcref using typealias as arg at script level
  lines =<< trim END
    vim9script
    type A = number
    def F(arg: any)
      echo typename(arg)
    enddef
    var Fref: func(any)
    Fref = F

    Fref(A)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "A" cannot be used as a value', 9)

  # check funcref using typealias as arg in :def
  lines =<< trim END
    vim9script
    type A = number
    def F(arg: any)
      echo typename(arg)
    enddef
    var Fref: func(any)
    Fref = F

    def G()
      Fref(A)
    enddef
    G()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # check funcref using typealias as return
  lines =<< trim END
    vim9script
    type A = number
    def F(): any
      return A
    enddef
    var Fref: func(): any
    Fref = F

    Fref()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)

  # check deferred function using typealias as arg
  lines =<< trim END
    vim9script
    type A = number
    def F(arg: any)
    enddef
    def G()
      defer F(A)
    enddef
    G()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value', 1)
enddef

" Test for class typealias as function arg and return value
def Test_class_as_func_argument_or_return_value()
  # check class typealias as arg, function call in script level
  var lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def Foo(arg: any)
    enddef
    Foo(A)
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 7)

  # check class typealias as function return, function call in script level
  lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def Foo(): any
      return A
    enddef
    Foo()
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 1)

  # check class typealias as arg, function call in :def
  lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def Foo(arg: any)
    enddef
    def F()
      Foo(A)
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 1)

  # check class typealias as function return, function call in :def
  lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def Foo(): any
      return A
    enddef
    def F()
      Foo()
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 1)

  # check funcref using class typealias as arg at script level
  lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def F(arg: any)
      echo typename(arg)
    enddef
    var Fref: func(any)
    Fref = F

    Fref(A)
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 11)

  # check funcref using class typealias as arg in :def
  lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def F(arg: any)
      echo typename(arg)
    enddef
    var Fref: func(any)
    Fref = F

    def G()
      Fref(A)
    enddef
    G()
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 1)

  # check funcref using class typealias as return
  lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def F(): any
      return A
    enddef
    var Fref: func(): any
    Fref = F

    Fref()
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 1)

  # check deferred function using class typealias as arg
  lines =<< trim END
    vim9script
    class C
    endclass
    type A = C
    def F(arg: any)
    enddef
    def G()
      defer F(A)
    enddef
    G()
  END
  v9.CheckScriptFailure(lines, 'E1405: Class "C" cannot be used as a value', 1)
enddef

def Test_passing_typealias_to_builtin()
  # type, typename, string, instanceof are allowed type argument
  var lines =<< trim END
    vim9script
    type T = number
    var x: any
    x = type(T)
    assert_equal(v:t_typealias, x)
    x = typename(T)
    assert_equal('typealias<number>', x)
    x = string(T)
    assert_equal('T', x)
  END
  v9.CheckScriptSuccess(lines)

  # check argument to add at script level
  # Note: add() is special cased in compile_call in vim9expr
  lines =<< trim END
    vim9script
    type T = number
    add([], T)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "T" cannot be used as a value')

  # check argument to add in :def
  lines =<< trim END
    vim9script
    type T = number
    def F()
      add([], T)
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value')

  # check member call argument to add at script level
  lines =<< trim END
    vim9script
    type T = number
    []->add(T)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "T" cannot be used as a value')

  # check member call argument to add in :def
  lines =<< trim END
    vim9script
    type T = number
    def F()
      []->add(T)
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value')

  # Try "empty()" builtin
  # check argument to empty at script level
  lines =<< trim END
    vim9script
    type T = number
    empty(T)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "T" cannot be used as a value')

  # check argument to empty in :def
  lines =<< trim END
    vim9script
    type T = number
    def F()
      empty(T)
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value')

  # check member call argument to empty at script level
  lines =<< trim END
    vim9script
    type T = number
    T->empty()
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "T" cannot be used as a value')

  # check member call argument to empty in :def
  lines =<< trim END
    vim9script
    type T = number
    def F()
      T->empty()
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value')

  # Try "abs()" builtin
  # check argument to abs at script level
  lines =<< trim END
    vim9script
    type T = number
    abs(T)
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "T" cannot be used as a value')

  # check argument to abs in :def
  lines =<< trim END
    vim9script
    type T = number
    def F()
      abs(T)
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value')

  # check member call argument to abs at script level
  lines =<< trim END
    vim9script
    type T = number
    T->abs()
  END
  v9.CheckScriptFailure(lines, 'E1403: Type alias "T" cannot be used as a value')

  # check member call argument to abs in :def
  lines =<< trim END
    vim9script
    type T = number
    def F()
      T->abs()
    enddef
    F()
  END
  v9.CheckScriptFailure(lines, 'E1407: Cannot use a Typealias as a variable or value')
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
