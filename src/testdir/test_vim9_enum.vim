" Tests for Vim9 script enums

import './util/vim9.vim' as v9

" Test for parsing an enum definition {{{1
def Test_enum_parse()
  # enum supported only in a Vim9 script
  var lines =<< trim END
    enum Foo
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1414: Enum can only be defined in Vim9 script', 1)

  # ":enum" and ":endenum"
  lines =<< trim END
    vim9script
    :enum Foo
    :endenum
    assert_equal(v:t_enum, type(Foo))
    assert_equal([], Foo.values)
  END
  v9.CheckSourceSuccess(lines)

  # First character in an enum name should be capitalized.
  lines =<< trim END
    vim9script
    enum foo
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1415: Enum name must start with an uppercase letter: foo', 2)

  # Only alphanumeric characters are supported in an enum name
  lines =<< trim END
    vim9script
    enum Foo@bar
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: Foo@bar', 2)

  # Unsupported keyword (instead of enum)
  lines =<< trim END
    vim9script
    noenum Something
    endenum
  END
  v9.CheckSourceFailure(lines, 'E492: Not an editor command: noenum Something', 2)

  # Only the complete word "enum" should be recognized
  lines =<< trim END
    vim9script
    enums Something
    endenum
  END
  v9.CheckSourceFailure(lines, 'E492: Not an editor command: enums Something', 2)

  # The complete "enum" should be specified.
  lines =<< trim END
    vim9script
    enu Nah
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1065: Command cannot be shortened: enu Nah', 2)

  # The complete "endenum" should be specified.
  lines =<< trim END
    vim9script
    enum Something
    enden
  END
  v9.CheckSourceFailure(lines, 'E1065: Command cannot be shortened: enden', 3)

  # "endenum" cannot be shortened (variant incl. whitespace and colon)
  lines =<< trim END
    vim9script
    enum NoAbbrev
    :	endenu
  END
  v9.CheckSourceFailure(lines, 'E1065: Command cannot be shortened: endenu', 3)

  # Only the complete word "endenum" should be recognized
  lines =<< trim END
    vim9script
    enum Something
    endenums
  END
  v9.CheckSourceFailure(lines, 'E1420: Missing :endenum', 4)

  # all lower case should be used for "enum"
  lines =<< trim END
    vim9script
    Enum Something
    endenum
  END
  v9.CheckSourceFailure(lines, 'E492: Not an editor command: Enum Something', 2)

  # all lower case should be used for "endenum"
  lines =<< trim END
    vim9script
    enum Something
    Endenum
  END
  v9.CheckSourceFailure(lines, 'E1420: Missing :endenum', 4)

  # Additional words after "endenum"
  lines =<< trim END
    vim9script
    enum Something
    endenum school's out
  END
  v9.CheckSourceFailure(lines, "E488: Trailing characters: school's out", 3)

  # Additional commands after "endenum"
  lines =<< trim END
    vim9script
    enum Something
    endenum | echo 'done'
  END
  v9.CheckSourceFailure(lines, "E488: Trailing characters: | echo 'done'", 3)

  # Try to define enum in a single command
  lines =<< trim END
    vim9script
    enum Something | endenum
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters: | endenum', 2)

  # another command follows the enum name
  lines =<< trim END
    vim9script
    enum Something | var x = 10
      Foo
    endenum
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters: | var x = 10', 2)

  # Try to define an enum with the same name as an existing variable
  lines =<< trim END
    vim9script
    var Something: list<number> = [1]
    enum Something
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Something"', 3)

  # Unsupported special character following enum name
  lines =<< trim END
    vim9script
    enum Foo
      first,
      second : 20
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1123: Missing comma before argument: : 20', 4)

  # Try initializing an enum item with a number
  lines =<< trim END
    vim9script
    enum Foo
      first,
      second = 2
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1123: Missing comma before argument: = 2', 4)

  # Try initializing an enum item with a String
  lines =<< trim END
    vim9script
    enum Foo
      first,
      second = 'second'
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1123: Missing comma before argument: = 'second'", 4)

  # Try initializing an enum item with a List
  lines =<< trim END
    vim9script
    enum Foo
      first,
      second = []
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1123: Missing comma before argument: = []', 4)

  # Use a colon after name
  lines =<< trim END
    vim9script
    enum Foo

      # first
      first:
      second
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1123: Missing comma before argument: :', 5)

  # Use a '=='
  lines =<< trim END
    vim9script
    enum Foo
      first == 1
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1123: Missing comma before argument: == 1', 3)

  # Missing comma after an enum item
  lines =<< trim END
    vim9script
    enum Planet
      mercury
      venus
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1419: Not a valid command in an Enum: venus', 4)

  # Comma at the beginning of an item
  lines =<< trim END
    vim9script
    enum Planet
      mercury
      ,venus
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1419: Not a valid command in an Enum: ,venus', 4)
  # Space before comma
  lines =<< trim END
    vim9script
    enum Planet
      mercury ,
      venus
    endenum
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before ','", 3)

  # No space after comma
  lines =<< trim END
    vim9script
    enum Planet
      mercury,venus
    endenum
  END
  v9.CheckSourceFailure(lines, "E1069: White space required after ',': mercury,venus", 3)

  # no comma between items in the same line
  lines =<< trim END
    vim9script
    enum Planet
      mercury venus earth
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1123: Missing comma before argument: venus earth', 3)

  # No space after an item and comment between items
  lines =<< trim END
    vim9script
    enum Planet
      mercury

      # Venus
      venus
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1419: Not a valid command in an Enum: venus', 6)

  # Comma is supported for the last item
  lines =<< trim END
    vim9script
    enum Planet
      mercury,
      venus,
    endenum
    var p: Planet
    p = Planet.mercury
    assert_equal(0, p.ordinal)
    p = Planet.venus
    assert_true(p is Planet.venus)
    assert_equal(['mercury', 'venus'], Planet.values->mapnew((_, v) => v.name))
  END
  v9.CheckSourceSuccess(lines)

  # invalid enum value declaration
  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      $%@
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1418: Invalid enum value declaration: $%@', 4)

  # Additional command after "enumvalue"
  lines =<< trim END
    vim9script
    enum NoAdditionalCmd
      One, | var y = 10
      Two
    endenum
  END
  v9.CheckSourceFailure(lines, "E1418: Invalid enum value declaration: | var y = 10", 3)

  # Duplicate enum value
  lines =<< trim END
    vim9script
    enum A
      Foo,
      Bar,
      Foo
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1428: Duplicate enum value: Foo', 5)

  # Duplicate enum value in the same line
  lines =<< trim END
    vim9script
    enum A
      Foo, Bar, Foo,
      Bar
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1428: Duplicate enum value: Foo', 3)

  # Try extending a class when defining an enum
  lines =<< trim END
    vim9script
    class Foo
    endclass
    enum Bar extends Foo
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1416: Enum cannot extend a class or enum', 4)

  # Try extending an enum
  lines =<< trim END
    vim9script
    enum Foo
    endenum
    enum Bar extends Foo
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1416: Enum cannot extend a class or enum', 4)

  # Try extending an enum using a class
  lines =<< trim END
    vim9script
    enum Foo
    endenum
    class Bar extends Foo
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend Foo', 5)

  # Try implementing an enum using a class
  lines =<< trim END
    vim9script
    enum Foo
    endenum
    class Bar implements Foo
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1347: Not a valid interface: Foo', 5)

  # abstract method is not supported in an enum
  lines =<< trim END
    vim9script
    enum Foo
      Apple
      abstract def Bar()
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1417: Abstract cannot be used in an Enum', 4)

  # Define an enum without any enum values but only with an object variable
  lines =<< trim END
    vim9script
    enum Foo
      final n: number = 10
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1123: Missing comma before argument: n: number = 10', 3)
enddef

" Test for basic enum declaration and errors {{{1
def Test_basic_enum()
  # Declare a simple enum
  var lines =<< trim END
    vim9script
    enum Foo
      apple,
      orange
    endenum
    var a: Foo = Foo.apple
    var b: Foo = Foo.orange
    assert_equal(a, Foo.apple)
    assert_equal(b, Foo.orange)
  END
  v9.CheckSourceSuccess(lines)

  # Multiple enums in a single line
  lines =<< trim END
    vim9script
    enum Foo
      apple, orange
    endenum
    assert_equal('enum<Foo>', typename(Foo.apple))
    assert_equal('enum<Foo>', typename(Foo.orange))
  END
  v9.CheckSourceSuccess(lines)

  # Comments and empty lines are supported between enum items
  lines =<< trim END
    vim9script
    enum Foo
      # Apple
      apple,

      # Orange
      orange
    endenum
    def Fn()
      var a: Foo = Foo.apple
      var b: Foo = Foo.orange
      assert_equal(a, Foo.apple)
      assert_equal(b, Foo.orange)
    enddef
  END
  v9.CheckSourceSuccess(lines)

  # Try using a non-existing enum value
  lines =<< trim END
    vim9script
    enum Foo
      apple,
      orange
    endenum
    var a: Foo = Foo.pear
  END
  v9.CheckSourceFailure(lines, 'E1422: Enum value "pear" not found in enum "Foo"', 6)

  # Enum function argument
  lines =<< trim END
    vim9script
    enum Foo
      apple,
      orange
    endenum
    def Fn(a: Foo): Foo
      return a
    enddef
    assert_equal(Foo.apple, Fn(Foo.apple))
  END
  v9.CheckSourceSuccess(lines)

  # Enum function argument
  lines =<< trim END
    vim9script
    enum Foo
      apple,
      orange
    endenum
    def Fn(a: Foo): Foo
      return a
    enddef
    Fn({})
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected enum<Foo> but got dict<any>', 9)

  # Returning an enum in a function returning number
  lines =<< trim END
    vim9script
    enum Foo
      apple,
      orange
    endenum
    def Fn(): number
      return Foo.orange
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got enum<Foo>', 1)

  # Returning a number in a function returning enum
  lines =<< trim END
    vim9script
    enum Foo
      apple,
      orange
    endenum
    def Fn(): Foo
      return 20
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Foo> but got number', 1)

  # Use a List of enums
  lines =<< trim END
    vim9script
    enum Planet
      Mercury,
      Venus,
      Earth
    endenum
    var l1: list<Planet> = [Planet.Mercury, Planet.Venus]
    assert_equal(Planet.Venus, l1[1])
    def Fn()
      var l2: list<Planet> = [Planet.Mercury, Planet.Venus]
      assert_equal(Planet.Venus, l2[1])
    enddef
  END
  v9.CheckSourceSuccess(lines)

  # Try using an enum as a value
  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange
    endenum
    var a = Fruit
  END
  v9.CheckSourceFailure(lines, 'E1421: Enum "Fruit" cannot be used as a value', 6)
enddef

" Test for type() and typename() of an enum {{{1
def Test_enum_type()
  var lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange
    endenum
    assert_equal('enum<Fruit>', typename(Fruit))
    assert_equal('enum<Fruit>', typename(Fruit.Apple))
    assert_equal(v:t_enum, type(Fruit))
    assert_equal(v:t_enumvalue, type(Fruit.Apple))
    assert_equal(15, type(Fruit))
    assert_equal(16, type(Fruit.Apple))
  END
  v9.CheckSourceSuccess(lines)

  # Assign an enum to a variable with any type
  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange
    endenum
    var a = Fruit.Apple
    var b: any = Fruit.Orange
    assert_equal('enum<Fruit>', typename(a))
    assert_equal('enum<Fruit>', typename(b))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for trying to modify an enum or an enum item {{{1
def Test_enum_modify()
  # Try assigning an unsupported value to an enum
  var lines =<< trim END
    vim9script
    enum Foo
      apple
    endenum
    var a: Foo = 30
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Foo> but got number', 5)

  # Try assigning an unsupported value to an enum in a function
  lines =<< trim END
    vim9script
    enum Foo
      apple
    endenum
    def Fn()
      var a: Foo = 30
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Foo> but got number', 1)

  # Try assigning a number to an enum
  lines =<< trim END
    vim9script
    enum Foo
      apple,
      orange
    endenum
    Foo = 10
  END
  v9.CheckSourceFailure(lines, 'E1421: Enum "Foo" cannot be used as a value', 6)

  # Try assigning a number to an enum in a function
  lines =<< trim END
    vim9script
    enum Foo
      apple
    endenum
    def Fn()
      Foo = 10
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Foo> but got number', 1)

  # Try assigning a number to an enum value
  lines =<< trim END
    vim9script
    enum Foo
      apple
    endenum
    Foo.apple = 20
  END
  v9.CheckSourceFailure(lines, 'E1423: Enum value "Foo.apple" cannot be modified', 5)

  # Try assigning a number to an enum value in a function
  lines =<< trim END
    vim9script
    enum Foo
      apple
    endenum
    def Fn()
      Foo.apple = 20
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1423: Enum value "Foo.apple" cannot be modified', 1)

  # Try assigning one enum to another
  lines =<< trim END
    vim9script
    enum Foo
    endenum
    enum Bar
    endenum
    Foo = Bar
  END
  v9.CheckSourceFailure(lines, 'E1421: Enum "Bar" cannot be used as a value', 6)

  # Try assigning one enum to another in a function
  lines =<< trim END
    vim9script
    enum Foo
    endenum
    enum Bar
    endenum
    def Fn()
      Foo = Bar
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1421: Enum "Bar" cannot be used as a value', 1)

  # Try assigning one enum item to another enum item
  lines =<< trim END
    vim9script
    enum Foo
      Apple
    endenum
    enum Bar
      Orange
    endenum
    Foo.Apple = Bar.Orange
  END
  v9.CheckSourceFailure(lines, 'E1423: Enum value "Foo.Apple" cannot be modified', 8)

  # Try assigning one enum item to another enum item in a function
  lines =<< trim END
    vim9script
    enum Foo
      Apple
    endenum
    enum Bar
      Orange
    endenum
    def Fn()
      Foo.Apple = Bar.Orange
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1423: Enum value "Foo.Apple" cannot be modified', 1)
enddef

" Test for using enum in an expression {{{1
def Test_enum_expr()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Blue, Green
    endenum
    var a: number = 1 + Color
  END
  v9.CheckSourceFailure(lines, 'E1421: Enum "Color" cannot be used as a value', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Blue, Green
    endenum
    var a: number = 1 + Color.Red
  END
  v9.CheckSourceFailure(lines, 'E1424: Using an Enum "Color" as a Number', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Blue, Green
    endenum
    var s: string = "abc" .. Color
  END
  v9.CheckSourceFailure(lines, 'E1421: Enum "Color" cannot be used as a value', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Blue, Green
    endenum
    var s: string = "abc" .. Color.Red
  END
  v9.CheckSourceFailure(lines, 'E1425: Using an Enum "Color" as a String', 5)
enddef

" Test for using an enum in a lambda function {{{1
def Test_enum_lambda()
  var lines =<< trim END
    vim9script
    enum Planet
      Mercury,
      Venus,
      Earth,
    endenum
    var Fn = (p: Planet): Planet => p
    for [idx, v] in items([Planet.Mercury, Planet.Venus, Planet.Earth])
      assert_equal(idx, Fn(v).ordinal)
    endfor
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for comparison using enums {{{1
def Test_enum_compare()
  var lines =<< trim END
    vim9script
    enum Planet
      Mercury,
      Venus,
      Earth,
    endenum
    enum Fruit
      Apple,
      Orange
    endenum

    var p: Planet = Planet.Venus
    var f: Fruit = Fruit.Orange
    assert_equal(true, p == Planet.Venus)
    assert_equal(false, p == Planet.Earth)
    assert_equal(false, p == f)
    assert_equal(true, Planet.Mercury == Planet.Mercury)
    assert_equal(true, Planet.Venus != Planet.Earth)
    assert_equal(true, Planet.Mercury != Fruit.Apple)

    def Fn1()
      var p2: Planet = Planet.Venus
      var f2: Fruit = Fruit.Orange
      assert_equal(true, p2 == Planet.Venus)
      assert_equal(false, p2 == Planet.Earth)
      assert_equal(false, p2 == f2)
    enddef
    Fn1()

    # comparison using "is" and "isnot"
    assert_equal(true, p is Planet.Venus)
    assert_equal(true, p isnot Planet.Earth)
    assert_equal(false, p is Fruit.Orange)
    assert_equal(true, p isnot Fruit.Orange)
    def Fn2(arg: Planet)
      assert_equal(true, arg is Planet.Venus)
      assert_equal(true, arg isnot Planet.Earth)
      assert_equal(false, arg is Fruit.Orange)
      assert_equal(true, arg isnot Fruit.Orange)
    enddef
    Fn2(p)

    class A
    endclass
    var o: A = A.new()
    assert_equal(false, p == o)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using an enum as a default argument to a function {{{1
def Test_enum_default_arg()
  var lines =<< trim END
    vim9script
    enum Day
      Monday, Tuesday, Wednesday
    endenum
    def Fn(d: Day = Day.Tuesday): Day
      return d
    enddef
    assert_equal(Day.Tuesday, Fn())
    assert_equal(Day.Wednesday, Fn(Day.Wednesday))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for enum garbage collection {{{1
func Test_enum_garbagecollect()
  let lines =<< trim END
    vim9script
    enum Car
      Honda, Ford, Tesla
    endenum
    assert_equal(1, Car.Ford.ordinal)
    call test_garbagecollect_now()
    assert_equal(1, Car.Ford.ordinal)
    var c: Car = Car.Tesla
    assert_equal(2, c.ordinal)
    call test_garbagecollect_now()
    assert_equal(2, c.ordinal)
  END
  call v9.CheckSourceSuccess(lines)

  " garbage collection with a variable of type any
  let lines =<< trim END
    vim9script
    enum Car
      Honda, Ford, Tesla
    endenum
    call test_garbagecollect_now()
    var c: any = Car.Tesla
    call test_garbagecollect_now()
    assert_equal(Car.Tesla, c)
  END
  call v9.CheckSourceSuccess(lines)

  " garbage collection with function arguments and return types
  let lines =<< trim END
    vim9script
    enum Car
      Honda, Ford, Tesla
    endenum
    def Fn(a: Car): Car
      assert_equal(Car.Ford, a)
      return Car.Tesla
    enddef
    call test_garbagecollect_now()
    var b: Car = Car.Ford
    call test_garbagecollect_now()
    assert_equal(Car.Tesla, Fn(b))
    call test_garbagecollect_now()
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" Test for the enum values class variable {{{1
def Test_enum_values()
  var lines =<< trim END
    vim9script
    enum Car
      Honda, Ford, Tesla
    endenum
    var l: list<Car> = Car.values
    assert_equal(Car.Ford, l[1])
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Car
      Honda,
      Ford,
    endenum
    assert_equal([Car.Honda, Car.Ford], Car.values)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Car
      Honda, Ford,
    endenum
    assert_equal([Car.Honda, Car.Ford], Car.values)
  END
  v9.CheckSourceSuccess(lines)

  # empty enum
  lines =<< trim END
    vim9script
    enum Car
    endenum
    assert_equal([], Car.values)
  END
  v9.CheckSourceSuccess(lines)

  # single value
  lines =<< trim END
    vim9script
    enum Car
      Honda
    endenum
    assert_equal([Car.Honda], Car.values)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum A
      Red,
      Blue
      static def GetValues(): list<A>
        return values
      enddef
    endenum
    assert_equal([A.Red, A.Blue], A.GetValues())
  END
  v9.CheckSourceSuccess(lines)

  # Other class variables in an enum should not be added to 'values'
  lines =<< trim END
    vim9script
    enum LogLevel
      Error, Warn
      static const x: number = 22
    endenum
    assert_equal([LogLevel.Error, LogLevel.Warn], LogLevel.values)
  END
  v9.CheckSourceSuccess(lines)

  # Other class variable of enum type should not be added to 'values'
  lines =<< trim END
    vim9script
    enum LogLevel
      Error, Warn
      static const x: LogLevel = LogLevel.Warn
    endenum
    assert_equal([LogLevel.Error, LogLevel.Warn], LogLevel.values)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using comments in enums {{{1
def Test_enum_comments()
  var lines =<< trim END
    vim9script
    enum Car  # cars
      # before enum
      Honda,  # honda
      # before enum
      Ford    # ford
    endenum
    assert_equal(1, Car.Ford.ordinal)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Car   # cars
      # before enum
      Honda(), # honda
      # before enum
      Ford()   # ford
    endenum
    assert_equal(1, Car.Ford.ordinal)
  END
  v9.CheckSourceSuccess(lines)

  # Test for using an unsupported comment
  lines =<< trim END
    vim9script
    enum Car
      Honda,
      Ford,
      #{
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1170: Cannot use #{ to start a comment', 4)
enddef

" Test for trailing whitespace after enum values {{{1
def Test_enum_whitespace()
  var lines =<< trim END
    vim9script
    enum Car
      Honda, 
      Ford   
    endenum
    assert_equal(['Honda', 'Ford'], Car.values->mapnew((_, v) => v.name))
    assert_equal(1, Car.Ford.ordinal)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Car
      Honda(),	
      Ford()   
    endenum
    assert_equal(['Honda', 'Ford'], Car.values->mapnew((_, v) => v.name))
    assert_equal(1, Car.Ford.ordinal)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using string() with enums {{{1
def Test_enum_string()
  var lines =<< trim END
    vim9script
    enum Car
      Honda,
      Ford
    endenum
    assert_equal("enum Car", string(Car))
    assert_equal("enum Car.Honda {name: 'Honda', ordinal: 0}", string(Car.Honda))
  END
  v9.CheckSourceSuccess(lines)

  # customized string function
  lines =<< trim END
    vim9script
    enum Dir
      North,
      South

      def string(): string
        return $'Dir.{this.name}'
      enddef
    endenum
    assert_equal('Dir.North', string(Dir.North))
    assert_equal('Dir.South', string(Dir.South))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for importing an enum {{{1
def Test_enum_import()
  var lines =<< trim END
    vim9script
    export enum Star
      Gemini,
      Orion,
      Pisces
    endenum
  END
  writefile(lines, 'Xenumexport.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xenumexport.vim' as mod

    var s1: mod.Star = mod.Star.Orion
    assert_equal(true, s1 == mod.Star.Orion)
    assert_equal(2, mod.Star.Pisces.ordinal)
    var l1: list<mod.Star> = mod.Star.values
    assert_equal("enum Star.Orion {name: 'Orion', ordinal: 1}", string(l1[1]))
    assert_equal(s1, l1[1])

    def Fn()
      var s2: mod.Star = mod.Star.Orion
      assert_equal(true, s2 == mod.Star.Orion)
      assert_equal(2, mod.Star.Pisces.ordinal)
      var l2: list<mod.Star> = mod.Star.values
      assert_equal("enum Star.Orion {name: 'Orion', ordinal: 1}", string(l2[1]))
      assert_equal(s2, l2[1])
    enddef
    Fn()
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for using test_refcount() with enum {{{1
def Test_enum_refcount()
  var lines =<< trim END
    vim9script
    enum Foo
    endenum
    assert_equal(1, test_refcount(Foo))

    enum Star
      Gemini,
      Orion
    endenum
    assert_equal(3, test_refcount(Star))
    assert_equal(2, test_refcount(Star.Gemini))
    assert_equal(2, test_refcount(Star.Orion))

    var s: Star
    assert_equal(3, test_refcount(Star))
    assert_equal(-1, test_refcount(s))
    s = Star.Orion
    assert_equal(3, test_refcount(Star))
    assert_equal(3, test_refcount(s))
    assert_equal(2, test_refcount(Star.Gemini))
    var t = s
    assert_equal(3, test_refcount(Star))
    assert_equal(4, test_refcount(s))
    assert_equal(4, test_refcount(Star.Orion))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Star
      Gemini,
      Orion,
    endenum

    assert_equal(3, test_refcount(Star))
    assert_equal(2, test_refcount(Star.Gemini))
    assert_equal(2, test_refcount(Star.Orion))

    var x = [Star.Gemini]
    assert_equal(3, test_refcount(Star))
    assert_equal(3, test_refcount(Star.Gemini))

    def Fn()
      var y = [Star.Gemini, Star.Orion]
      assert_equal(6, test_refcount(Star))
      assert_equal(4, test_refcount(Star.Gemini))
    enddef
    Fn()
    # The instruction in the compiled function adds an additional reference
    # to the enum.
    assert_equal(6, test_refcount(Star))
    assert_equal(3, test_refcount(Star.Gemini))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for defining an enum with additional object variables and methods {{{1
def Test_enum_enhanced()
  var lines =<< trim END
    vim9script
    enum Vehicle
      car(4, 5, 400),
      bus(6, 50, 800),
      bicycle(2, 1, 0)

      final tires: number
      final passengers: number
      final carbonPerKilometer: number

      def new(t: number, p: number, cpk: number)
        this.tires = t
        this.passengers = p
        this.carbonPerKilometer = cpk
      enddef

      def CarbonFootprint(): float
        return round(this.carbonPerKilometer / this.passengers)
      enddef

      def IsTwoWheeled(): bool
        return this == Vehicle.bicycle
      enddef

      def CompareTo(other: Vehicle): float
         return this.CarbonFootprint() - other.CarbonFootprint()
      enddef
    endenum

    var v: Vehicle = Vehicle.bus
    assert_equal([6, 50, 800], [v.tires, v.passengers, v.carbonPerKilometer])
    assert_equal(true, Vehicle.bicycle.IsTwoWheeled())
    assert_equal(false, Vehicle.car.IsTwoWheeled())
    assert_equal(16.0, Vehicle.bus.CarbonFootprint())
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for the enum value 'name' variable {{{1
def Test_enum_name()
  # Check the names of enum values
  var lines =<< trim END
    vim9script
    enum Planet
      Mercury,
      Venus,
      Earth
    endenum
    assert_equal('Mercury', Planet.Mercury.name)
    assert_equal('Venus', Planet.Venus.name)
    assert_equal('Earth', Planet.Earth.name)
    assert_equal('string', typename(Planet.Earth.name))
  END
  v9.CheckSourceSuccess(lines)

  # Check the name of enum items in the constructor
  lines =<< trim END
    vim9script
    enum Planet
      Mercury("Mercury"),
      Venus("Venus"),
      Earth("Earth")

      def new(s: string)
        assert_equal(s, this.name)
      enddef
    endenum
    defcompile
  END
  v9.CheckSourceSuccess(lines)

  # Try assigning to the name of an enum
  lines =<< trim END
    vim9script
    enum Fruit
      Apple
    endenum
    Fruit.Apple.name = 'foo'
  END
  v9.CheckSourceFailure(lines, 'E1335: Variable "name" in class "Fruit" is not writable', 5)

  # Try assigning to the name of an enum in a function
  lines =<< trim END
    vim9script
    enum Fruit
      Apple
    endenum
    def Fn()
      Fruit.Apple.name = 'bar'
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1423: Enum value "Fruit.name" cannot be modified', 1)

  # Try to overwrite an enum value name in the enum constructor
  lines =<< trim END
    vim9script
    enum Planet
      Mercury,
      Venus

      def new()
        this.name = 'foo'
      enddef
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1427: Enum "Planet" name cannot be modified', 1)

  # Try to declare an object variable named 'name'
  lines =<< trim END
    vim9script
    enum Planet
      Mercury
      var name: string
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1369: Duplicate variable: name', 4)
enddef

" Test for the enum value 'ordinal' variable {{{1
def Test_enum_ordinal()
  # Check the ordinal values of enum items
  var lines =<< trim END
    vim9script
    enum Planet
      Mercury,
      Venus,
      Earth
    endenum
    assert_equal(0, Planet.Mercury.ordinal)
    assert_equal(1, Planet.Venus.ordinal)
    assert_equal(2, Planet.Earth.ordinal)
    assert_equal('number', typename(Planet.Earth.ordinal))
  END
  v9.CheckSourceSuccess(lines)

  # Check the ordinal value of enum items in the constructor
  lines =<< trim END
    vim9script
    enum Planet
      Mercury(0),
      Venus(1),
      Earth(2)

      def new(v: number)
        assert_equal(v, this.ordinal)
      enddef
    endenum
    defcompile
  END
  v9.CheckSourceSuccess(lines)

  # Try assigning to the ordinal value of an enum
  lines =<< trim END
    vim9script
    enum Fruit
      Apple
    endenum
    Fruit.Apple.ordinal = 20
  END
  v9.CheckSourceFailure(lines, 'E1335: Variable "ordinal" in class "Fruit" is not writable', 5)

  # Try assigning to the ordinal value of an enum in a function
  lines =<< trim END
    vim9script
    enum Fruit
      Apple
    endenum
    def Fn()
      Fruit.Apple.ordinal = 20
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1423: Enum value "Fruit.ordinal" cannot be modified', 1)

  # Try to overwrite an enum value ordinal in the enum constructor
  lines =<< trim END
    vim9script
    enum Planet
      Mercury,
      Venus

      def new()
        this.ordinal = 20
      enddef
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1426: Enum "Planet" ordinal value cannot be modified', 1)

  # Try to declare an object variable named 'ordinal'
  lines =<< trim END
    vim9script
    enum Planet
      Mercury
      var ordinal: number
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1369: Duplicate variable: ordinal', 4)
enddef

" Test for trying to create a new enum object using the constructor {{{1
def Test_enum_invoke_constructor()
  var lines =<< trim END
    vim9script
    enum Foo
    endenum
    var f: Foo = Foo.new()
  END
  v9.CheckSourceFailure(lines, 'E1325: Method "new" not found in class "Foo"', 4)

  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange
    endenum
    var f: Fruit = Fruit.new()
  END
  v9.CheckSourceFailure(lines, 'E1325: Method "new" not found in class "Fruit"', 6)

  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange
      def newFruit()
      enddef
    endenum
    var f: Fruit = Fruit.newFruit()
  END
  v9.CheckSourceFailure(lines, 'E1325: Method "newFruit" not found in class "Fruit"', 8)

  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange
    endenum
    def Fn()
      var f: Fruit = Fruit.new()
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1325: Method "new" not found in class "Fruit"', 1)

  # error in the enum constructor
  lines =<< trim END
    vim9script
    enum Planet
      earth
      def new()
        x = 123
      enddef
    endenum
  END
  v9.CheckSourceFailureList(lines, ['E1100:', 'E1100:'], 1)
enddef

" Test for checking "this" in an enum constructor {{{1
def Test_enum_this_in_constructor()
  var lines =<< trim END
    vim9script
    enum A
      Red("enum A.Red {name: 'Red', ordinal: 0}"),
      Blue("enum A.Blue {name: 'Blue', ordinal: 1}"),
      Green("enum A.Green {name: 'Green', ordinal: 2}")

      def new(s: string)
        assert_equal(s, string(this))
      enddef
    endenum
    defcompile
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using member variables in an enum object {{{1
def Test_enum_object_variable()
  var lines =<< trim END
    vim9script
    enum Planet
      Jupiter(95),
      Saturn(146)

      var moons: number
    endenum
    assert_equal(95, Planet.Jupiter.moons)
    assert_equal(146, Planet.Saturn.moons)
  END
  v9.CheckSourceSuccess(lines)

  # Use a final object variable
  lines =<< trim END
    vim9script
    enum Planet
      Jupiter(95),
      Saturn(146)

      final moons: number
      def new(n: number)
        this.moons = n
      enddef
    endenum
    assert_equal(95, Planet.Jupiter.moons)
    assert_equal(146, Planet.Saturn.moons)
  END
  v9.CheckSourceSuccess(lines)

  # Use a const object variable
  lines =<< trim END
    vim9script
    enum Planet
      Mars(false),
      Jupiter(true)

      const has_ring: bool
      def new(r: bool)
        this.has_ring = r
      enddef
    endenum
    assert_equal(false, Planet.Mars.has_ring)
    assert_equal(true, Planet.Jupiter.has_ring)
  END
  v9.CheckSourceSuccess(lines)

  # Use a regular object variable
  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange

      final farm: string = 'SunValley'
    endenum
    assert_equal('SunValley', Fruit.Apple.farm)
    assert_equal('SunValley', Fruit.Apple.farm)
  END
  v9.CheckSourceSuccess(lines)

  # Invoke the default constructor with an object variable
  lines =<< trim END
    vim9script
    enum Fruit
      Apple('foo'),
      Orange('bar')

      final t: string
    endenum
    assert_equal('foo', Fruit.Apple.t)
    assert_equal('bar', Fruit.Orange.t)
  END
  v9.CheckSourceSuccess(lines)

  # Invoke the default constructor with an argument but without the object
  # variable
  lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange('bar')
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E118: Too many arguments for function: new', 5)

  # Define a default constructor with an argument, but don't pass it in when
  # defining the enum value
  lines =<< trim END
    vim9script
    enum Fruit
      Apple(5),
      Orange

      def new(t: number)
      enddef
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E119: Not enough arguments for function: new', 8)
enddef

" Test for using a custom constructor with an enum {{{1
def Test_enum_custom_constructor()
  # space before "("
  var lines =<< trim END
    vim9script
    enum Fruit
      Apple(10),
      Orange (20)

      def new(t: number)
      enddef
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '(': Orange (20)", 4)

  # no closing ")"
  lines =<< trim END
    vim9script
    enum Fruit
      Apple(10),
      Orange (20

      def new(t: number)
      enddef
    endenum
    defcompile
  END
  v9.CheckSourceFailure(lines, "E1068: No white space allowed before '(': Orange (20", 4)

  # Specify constructor arguments split across multiple lines
  lines =<< trim END
    vim9script
    enum Fruit
      Apple(10,
            'foo'), Orange(20,
            'bar'),
      Pear(30,
           'baz'), Mango(40,
           'qux')

      final n: number
      final s: string
      def new(t: number, str: string)
        this.n = t
        this.s = str
      enddef
    endenum
    defcompile
    assert_equal([10, 'foo'], [Fruit.Apple.n, Fruit.Apple.s])
    assert_equal([20, 'bar'], [Fruit.Orange.n, Fruit.Orange.s])
    assert_equal([30, 'baz'], [Fruit.Pear.n, Fruit.Pear.s])
    assert_equal([40, 'qux'], [Fruit.Mango.n, Fruit.Mango.s])
  END
  v9.CheckSourceSuccess(lines)

  # specify multiple enums with constructor arguments in a single line
  lines =<< trim END
    vim9script
    enum Fruit
      Apple(10, 'foo'), Orange(20, 'bar'), Pear(30, 'baz'), Mango(40, 'qux')
      const n: number
      const s: string
    endenum
    defcompile
    assert_equal([10, 'foo'], [Fruit.Apple.n, Fruit.Apple.s])
    assert_equal([20, 'bar'], [Fruit.Orange.n, Fruit.Orange.s])
    assert_equal([30, 'baz'], [Fruit.Pear.n, Fruit.Pear.s])
    assert_equal([40, 'qux'], [Fruit.Mango.n, Fruit.Mango.s])
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using class variables in an enum class {{{1
def Test_enum_class_variable()
  var lines =<< trim END
    vim9script
    enum Fruit
      Apple,
      Orange

      static var farm: string = 'SunValley'
    endenum
    assert_equal('SunValley', Fruit.farm)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for converting a string to an enum value {{{1
def Test_enum_eval()
  var lines =<< trim END
    vim9script
    enum Color
      Red,
      Blue
    endenum
    var e = eval('Color.Blue')
    assert_equal(Color.Blue, e)
    assert_equal(1, e.ordinal)
    assert_fails("eval('Color.Green')", 'E1422: Enum value "Green" not found in enum "Color"')
    assert_fails("var x = eval('Color')", 'E1421: Enum "Color" cannot be used as a value')
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using "values" in an enum class variable {{{1
def Test_use_enum_values_in_class_variable()
  var lines =<< trim END
    vim9script
    enum Dir
      North, South
      static const dirs: list<Dir> = Dir.values
    endenum
    assert_equal([Dir.North, Dir.South], Dir.dirs)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using lambda block in enums {{{1
def Test_lambda_block_in_enum()
  # This used to crash Vim
  var lines =<< trim END
    vim9script
    enum IdEnum1
      ID1
      const Id: func(number): number = (num: number): number => {
        # Return a ID
        return num / 2
      }
    endenum
    assert_equal(5, IdEnum1.ID1.Id(10))
  END
  v9.CheckScriptSuccess(lines)

  # This used to crash Vim
  lines =<< trim END
    vim9script
    enum IdEnum2
      ID1
      static const Id: func(number): number = (num: number): number => {
        # Return a ID
        return num + 2
      }
    endenum
    assert_equal(12, IdEnum2.Id(10))
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for echoing an enum {{{1
def Test_enum_echo()
  var lines =<< trim END
    vim9script
    enum Demo
      one('tahi'),
      two('rua'),
      three('toru')
      var alias: string
    endenum
    assert_equal('enum Demo.one {name: one, ordinal: 0, alias: tahi}', execute('echo Demo.one')->split("\n")[0])
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for garbage collecting an enum with a complex member variables {{{1
func Test_enum_selfref_gc()
  let lines =<< trim END
    vim9script
    enum Foo
      Red,
      Blue
      static var d = {a: [1, 2]}
      static var l = [{a: 'a', b: 'b'}]
    endenum
    assert_equal(3, test_refcount(Foo))
    test_garbagecollect_now()
    assert_equal(3, test_refcount(Foo))
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" Test for defining an enum in a function {{{1
def Test_enum_defined_in_function()
  var lines =<< trim END
    vim9script
    def Fn()
      var x = 1
      enum Foo
        Red,
      endenum
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1435: Enum can only be used in a script', 2)
enddef

" Test for an enum implementing an interface {{{1
def Test_enum_implements_interface()
  var lines =<< trim END
    vim9script
    interface HasArea
      var sides: number
      def Area(): number
    endinterface

    enum Shape implements HasArea
      Triangle(3),
      Square(4)

      var sides: number
      def Area(): number
        return this.sides * 10
      enddef
    endenum

    # use the enum values through the interface
    var h: HasArea = Shape.Square
    assert_equal(4, h.sides)
    assert_equal(40, h.Area())
    var l: list<HasArea> = [Shape.Triangle, Shape.Square]
    assert_equal([30, 40], l->mapnew((_, v) => v.Area()))
    assert_true(instanceof(Shape.Triangle, HasArea))

    def Total(items: list<HasArea>): number
      var total = 0
      for item in items
        total += item.Area() + item.sides
      endfor
      return total
    enddef
    assert_equal(77, Total(Shape.values))

    def GetSides(item: HasArea): number
      return item.sides
    enddef
    assert_equal(3, GetSides(Shape.Triangle))
  END
  v9.CheckSourceSuccess(lines)

  # enum not implementing an interface method
  lines =<< trim END
    vim9script
    interface HasArea
      def Area(): number
    endinterface
    enum Shape implements HasArea
      Triangle
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1349: Method "Area" of interface "HasArea" is not implemented', 7)

  # enum not implementing an interface variable
  lines =<< trim END
    vim9script
    interface HasArea
      var sides: number
    endinterface
    enum Shape implements HasArea
      Triangle
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1348: Variable "sides" of interface "HasArea" is not implemented', 7)
enddef

" Test for using a value of one enum where another enum type is expected {{{1
def Test_enum_type_mismatch()
  # assignment at script level
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple, Orange
    endenum
    var c: Color = Fruit.Apple
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Color> but got enum<Fruit>', 8)

  # assignment in a def function
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple, Orange
    endenum
    def Fn()
      var c: Color = Color.Red
      c = Fruit.Apple
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Color> but got enum<Fruit>', 2)

  # function argument
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple, Orange
    endenum
    def Fn(c: Color)
    enddef
    Fn(Fruit.Apple)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected enum<Color> but got enum<Fruit>', 10)

  # function return value
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple, Orange
    endenum
    def Fn(): Color
      return Fruit.Apple
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Color> but got enum<Fruit>', 1)

  # value passed through "any" is checked at runtime
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple, Orange
    endenum
    def Fn()
      var a: any = Fruit.Apple
      var c: Color = a
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Color> but got enum<Fruit>', 2)

  # adding a value of another enum to a typed Dict
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple, Orange
    endenum
    var d: dict<Color> = {a: Color.Red}
    d.b = Fruit.Apple
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Color> but got enum<Fruit>', 9)
enddef

" Test for invalid operations with enum values {{{1
def Test_enum_invalid_operations()
  # ordering comparison
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    var b = Color.Red < Color.Green
  END
  v9.CheckSourceFailure(lines, 'E1153: Invalid operation for object', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn(): bool
      return Color.Red > Color.Green
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1153: Invalid operation for object', 1)

  # comparing with a number or a string
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    var b = Color.Red == 0
  END
  v9.CheckSourceFailure(lines, 'E1072: Cannot compare object with number', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn(): bool
      return Color.Red == 'Red'
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1072: Cannot compare object with string', 1)

  # using an enum value as a condition
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    var n = Color.Red ? 1 : 0
  END
  v9.CheckSourceFailure(lines, 'E1424: Using an Enum "Color" as a Number', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn()
      if Color.Red
      endif
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected bool but got enum<Color>', 1)

  # string interpolation
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    var s = $"{Color.Red}"
  END
  v9.CheckSourceFailure(lines, 'E1425: Using an Enum "Color" as a String', 5)

  # arithmetic and string concatenation in a def function (for a script see
  # Test_enum_expr()).  The compiled code gives a generic error.
  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn(): number
      return 1 + Color.Red
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1051: Wrong argument type for +', 1)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn(): string
      return 'abc' .. Color.Red
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1105: Cannot convert object to string', 1)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn(): number
      return -Color.Red
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected number but got enum<Color>', 1)
enddef

" Test that the List of enum values cannot be changed {{{1
def Test_enum_values_list_locked()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    # the same List is returned every time
    assert_true(Color.values is Color.values)

    # a copy of the List can be changed
    var l = copy(Color.values)
    l->add(Color.Red)
    assert_equal([Color.Red, Color.Green, Color.Red], l)
    assert_equal([Color.Red, Color.Green], Color.values)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    Color.values->add(Color.Red)
  END
  v9.CheckSourceFailure(lines, 'E741: Value is locked: add() argument', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn()
      Color.values->add(Color.Red)
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E741: Value is locked: add() argument', 1)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    Color.values[0] = Color.Green
  END
  v9.CheckSourceFailure(lines, 'E741: Value is locked:', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn()
      Color.values[0] = Color.Green
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1119: Cannot change locked list item', 1)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    Color.values = []
  END
  v9.CheckSourceFailure(lines, 'E1335: Variable "values" in class "Color" is not writable', 5)
enddef

" Test for using builtin functions with enum values {{{1
def Test_enum_builtin_functions()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green, Blue
    endenum
    class Paint
    endclass

    assert_true(instanceof(Color.Green, Color))
    assert_false(instanceof(Color.Green, Paint))
    assert_equal(2, index(Color.values, Color.Blue))
    assert_equal(-1, index([Color.Red], Color.Blue))
    assert_equal(1, indexof(Color.values, (_, v) => v.name == 'Green'))
    assert_equal(2, count([Color.Red, Color.Blue, Color.Red], Color.Red))
    assert_equal('list<enum<Color>>', typename(Color.values))

    # an enum value is never copied
    assert_true(copy(Color.Green) is Color.Green)
    assert_true(deepcopy(Color.Green) is Color.Green)
    assert_true(deepcopy([Color.Blue])[0] is Color.Blue)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using a type alias for an enum {{{1
def Test_enum_typealias()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    type Hue = Color

    var h: Hue = Color.Green
    assert_equal('enum<Color>', typename(h))
    assert_true(h is Color.Green)

    def Next(c: Hue): Hue
      return c == Color.Red ? Color.Green : Color.Red
    enddef
    assert_true(Next(Color.Red) is Color.Green)
    var l: list<Hue> = Color.values
    assert_equal(['Red', 'Green'], l->mapnew((_, v) => v.name))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple
    endenum
    type Hue = Color
    var h: Hue = Fruit.Apple
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Color> but got enum<Fruit>', 9)
enddef

" Test for using enum values with a generic function {{{1
def Test_enum_generic_function()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum

    def Identity<T>(x: T): T
      return x
    enddef
    def First<T>(l: list<T>): T
      return l[0]
    enddef

    assert_true(Identity<Color>(Color.Green) is Color.Green)
    assert_true(First<Color>(Color.values) is Color.Red)
    assert_equal('list<enum<Color>>', typename(Identity<list<Color>>(Color.values)))

    def Fn()
      assert_true(Identity<Color>(Color.Red) is Color.Red)
      assert_true(First<Color>([Color.Green]) is Color.Green)
    enddef
    Fn()
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Fruit
      Apple
    endenum
    def Identity<T>(x: T): T
      return x
    enddef
    Identity<Color>(Fruit.Apple)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected enum<Color> but got enum<Fruit>', 11)
enddef

" Test for protected variables and methods in an enum {{{1
def Test_enum_protected_members()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
      var _secret: number = 7
      static var _count: number = 2
      def Peek(): number
        return this._secret + this._Helper() + _count
      enddef
      def _Helper(): number
        return 1
      enddef
    endenum
    assert_equal(10, Color.Red.Peek())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
      var _secret: number = 7
    endenum
    var n = Color.Red._secret
  END
  v9.CheckSourceFailure(lines, 'E1333: Cannot access protected variable "_secret" in class "Color"', 6)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
      var _secret: number = 7
    endenum
    def Fn(): number
      return Color.Red._secret
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1333: Cannot access protected variable "_secret" in class "Color"', 1)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
      static var _count: number = 2
    endenum
    var n = Color._count
  END
  v9.CheckSourceFailure(lines, 'E1333: Cannot access protected variable "_count" in class "Color"', 6)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
      def _Helper(): number
        return 1
      enddef
    endenum
    var n = Color.Red._Helper()
  END
  v9.CheckSourceFailure(lines, 'E1366: Cannot access protected method: _Helper', 8)
enddef

" Test for changing class variables of an enum in a script {{{1
def Test_enum_class_variable_change()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
      public static var level: number = 1
    endenum
    Color.level = 3
    assert_equal(3, Color.level)
    Color.level += 2
    assert_equal(5, Color.level)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
      static var level: number = 1
    endenum
    Color.level = 3
  END
  v9.CheckSourceFailure(lines, 'E1335: Variable "level" in class "Color" is not writable', 6)
enddef

" Test for an enum with many values {{{1
def Test_enum_many_values()
  var names = range(26)->mapnew((_, v) => nr2char(char2nr('A') + v))
  var lines = ['vim9script', 'enum Letter']
  # five values per line
  for i in range(0, 25, 5)
    lines->add('  ' .. names[i : i + 4]->join(', ') .. (i + 5 < 26 ? ',' : ''))
  endfor
  lines += ['endenum',
            'assert_equal(26, len(Letter.values))',
            'assert_equal(0, Letter.A.ordinal)',
            'assert_equal(25, Letter.Z.ordinal)',
            'assert_equal(12, Letter.M.ordinal)',
            'assert_equal(range(26), Letter.values->mapnew((_, v) => v.ordinal))',
            $'assert_equal({names}, Letter.values->mapnew((_, v) => v.name))']
  v9.CheckSourceSuccess(lines)
enddef

" Test for using an enum imported in a legacy script {{{1
def Test_enum_in_legacy_script()
  var lines =<< trim END
    vim9script
    export enum Color
      Red, Green
    endenum
  END
  writefile(lines, 'Xenumlegacy.vim', 'D')

  lines =<< trim END
    import './Xenumlegacy.vim' as m
    call assert_equal('Green', s:m.Color.Green.name)
    call assert_equal(1, s:m.Color.Green.ordinal)
    call assert_equal(2, len(s:m.Color.values))
    call assert_true(s:m.Color.Red is s:m.Color.values[0])
    let s:c = s:m.Color.Red
    call assert_equal(v:t_enumvalue, type(s:c))
    call assert_equal('enum<Color>', typename(s:c))
    call assert_fails('let s:m.Color.Red = s:m.Color.Green',
          \ 'E1423: Enum value "Color.Red" cannot be modified')
  END
  v9.CheckSourceScriptSuccess(lines)
enddef

" Test for the errors when looking up an enum variable or method in the wrong
" way, in a script and in a def function {{{1
def Test_enum_member_lookup_errors()
  var header =<< trim END
    vim9script
    enum Color
      Red, Green
      def Describe(): string
        return this.name
      enddef
      static def Count(): number
        return len(Color.values)
      enddef
    endenum
  END
  var cases = [
    ['Color.name', 'E1376: Object variable "name" accessible only using class "Color" object'],
    ['Color.Red.values', 'E1375: Class variable "values" accessible only using class "Color"'],
    ['Color.Red.Green', 'E1375: Class variable "Green" accessible only using class "Color"'],
    ['Color.Red.Count()', 'E1385: Class method "Count" accessible only using class "Color"'],
    ['Color.Describe()', 'E1386: Object method "Describe" accessible only using class "Color" object'],
    ['Color.red', 'E1422: Enum value "red" not found in enum "Color"'],
    ['Color.Red.nosuch', 'E1326: Variable "nosuch" not found in object "Color"'],
  ]
  for [expr, err] in cases
    # in a script
    v9.CheckSourceFailure(header + [$'var x = {expr}'], err, 11)
    # in a def function
    v9.CheckSourceFailure(header + ['def Fn(): any', $'  return {expr}',
                                    'enddef', 'Fn()'], err, 1)
  endfor

  # unknown enum name
  v9.CheckSourceFailure(header + ['var x = Colr.Red'],
                        'E121: Undefined variable: Colr', 11)
  v9.CheckSourceFailure(header + ['def Fn(): any', '  return Colr.Red',
                                  'enddef', 'Fn()'],
                        'E1001: Variable not found: Colr', 1)
enddef

" Test for a variable with an enum type that is not initialized {{{1
def Test_enum_uninitialized_variable()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    var c: Color
    assert_true(c is null_object)
    assert_true(c == null_object)
    assert_false(c == Color.Red)
    c = Color.Green
    assert_true(c is Color.Green)
    assert_false(c == null_object)

    def Fn(): bool
      var d: Color
      return d == null_object && d != Color.Red
    enddef
    assert_true(Fn())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    var c: Color
    var n = c.name
  END
  v9.CheckSourceFailure(lines, 'E1360: Using a null object', 6)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn(): number
      var c: Color
      return c.ordinal
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1360: Using a null object', 2)
enddef

" Test for function types with enum arguments and return values, and for
" passing enum values through several functions and closures {{{1
def Test_enum_function_types()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green, Blue
    endenum

    def Next(c: Color): Color
      return Color.values[(c.ordinal + 1) % len(Color.values)]
    enddef
    assert_equal('func(enum<Color>): enum<Color>', typename(Next))

    # a funcref with an enum function type
    var F: func(Color): Color = Next
    assert_true(F(Color.Blue) is Color.Red)
    var G: func(Color): string = (c: Color): string => c.name
    assert_equal('Green', G(Color.Green))

    # pass an enum value through several functions
    def Twice(Fn: func(Color): Color, c: Color): Color
      return Fn(Fn(c))
    enddef
    assert_true(Twice(Next, Color.Red) is Color.Blue)
    assert_true(Twice(F, Twice(Next, Color.Green)) is Color.Blue)

    # closures that capture an enum value in a loop
    def MakeNames(): list<func(): string>
      var fns: list<func(): string> = []
      for c in Color.values
        var cc = c
        fns->add(() => cc.name)
      endfor
      return fns
    enddef
    assert_equal(['Red', 'Green', 'Blue'],
                 MakeNames()->mapnew((_, Fn) => Fn()))
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    enum Fruit
      Apple
    endenum
    var F: func(Color): Color = (f: Fruit): Color => Color.Red
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected func(enum<Color>): enum<Color> but got func(enum<Fruit>): enum<Color>', 8)
enddef

" Test for using enum values in control flow statements {{{1
def Test_enum_control_flow()
  var lines =<< trim END
    vim9script
    enum Light
      Red, Yellow, Green
    endenum

    def Action(l: Light): string
      if l == Light.Red
        return 'stop'
      elseif l is Light.Yellow
        return 'wait'
      else
        return 'go'
      endif
    enddef
    assert_equal(['stop', 'wait', 'go'],
                 Light.values->mapnew((_, l) => Action(l)))

    # for loop with continue and break
    def Collect(): list<string>
      var names: list<string> = []
      for l in Light.values
        if l == Light.Red
          continue
        endif
        names->add(l.name)
        if l == Light.Yellow
          break
        endif
      endfor
      return names
    enddef
    assert_equal(['Yellow'], Collect())

    # while loop
    var l = Light.Red
    var count = 0
    while l != Light.Green
      l = Light.values[l.ordinal + 1]
      count += 1
    endwhile
    assert_equal(2, count)

    # throw and catch using the name of an enum value
    def Check(x: Light)
      if x != Light.Green
        throw $'not green: {x.name}'
      endif
    enddef
    var msg = ''
    try
      Check(Light.Yellow)
    catch /not green/
      msg = v:exception
    endtry
    assert_equal('not green: Yellow', msg)
  END
  v9.CheckSourceSuccess(lines)

  # an enum value cannot be thrown
  lines =<< trim END
    vim9script
    enum Light
      Red
    endenum
    throw Light.Red
  END
  v9.CheckSourceFailure(lines, 'E1425: Using an Enum "Light" as a String', 5)

  # Note: the compiled code gives a generic error
  lines =<< trim END
    vim9script
    enum Light
      Red
    endenum
    def Fn()
      throw Light.Red
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1105: Cannot convert object to string', 1)
enddef

" Test for the ignore-case and match-case variants of the comparison operators
" {{{1
def Test_enum_comparison_variants()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    assert_true(Color.Red ==? Color.Red)
    assert_false(Color.Red ==# Color.Green)
    assert_true(Color.Red !=? Color.Green)

    def Fn(): list<bool>
      return [Color.Red ==? Color.Red, Color.Red ==# Color.Green]
    enddef
    assert_equal([true, false], Fn())
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    var b = Color.Red =~ 'Red'
  END
  v9.CheckSourceFailure(lines, 'E1072: Cannot compare object with string', 5)
enddef

" Test for using List functions with enum values {{{1
def Test_enum_list_functions()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green, Blue
    endenum

    var by_ordinal_desc = copy(Color.values)
          ->sort((a, b) => b.ordinal - a.ordinal)
    assert_equal([Color.Blue, Color.Green, Color.Red], by_ordinal_desc)
    assert_equal(['Green', 'Blue'],
          Color.values->copy()->filter((_, v) => v.ordinal > 0)
                      ->map((_, v) => v.name))
    assert_equal('RedGreenBlue', reduce(Color.values, (acc, v) => acc .. v.name, ''))
    assert_equal([Color.Red, Color.Green],
          uniq([Color.Red, Color.Red, Color.Green]))
    var names: dict<Color> = {}
    for c in Color.values
      names[c.name] = c
    endfor
    assert_true(names.Blue is Color.Blue)
  END
  v9.CheckSourceSuccess(lines)

  # an enum value cannot be used as a Dict key
  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    var d = {[Color.Red]: 1}
  END
  v9.CheckSourceFailure(lines, 'E1425: Using an Enum "Color" as a String', 5)
enddef

" Test for type checking enum values in a Tuple and in a nested container {{{1
def Test_enum_container_type_check()
  var lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    enum Fruit
      Apple
    endenum
    var t: tuple<Color, Fruit> = (Color.Red, Fruit.Apple)
    assert_true(t[1] is Fruit.Apple)
    var d: dict<list<Color>> = {a: [Color.Red]}
    assert_true(d.a[0] is Color.Red)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    enum Fruit
      Apple
    endenum
    var t: tuple<Color, Color> = (Color.Red, Fruit.Apple)
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected tuple<enum<Color>, enum<Color>> but got tuple<enum<Color>, enum<Fruit>>', 8)

  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    enum Fruit
      Apple
    endenum
    var d: dict<list<Color>> = {a: [Fruit.Apple]}
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected dict<list<enum<Color>>> but got dict<list<enum<Fruit>>>', 8)
enddef

" Test for using an enum that is not exported {{{1
def Test_enum_not_exported()
  var lines =<< trim END
    vim9script
    enum Hidden
      A
    endenum
    export enum Shown
      B
    endenum
  END
  writefile(lines, 'Xenumnotexported.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xenumnotexported.vim' as m
    assert_equal('B', m.Shown.B.name)
    var x = m.Hidden.A
  END
  v9.CheckSourceFailure(lines, 'E1049: Item not exported in script: Hidden', 4)

  lines =<< trim END
    vim9script
    import './Xenumnotexported.vim' as m
    def Fn(): any
      return m.Hidden.A
    enddef
    Fn()
  END
  v9.CheckSourceFailure(lines, 'E1049: Item not exported in script: Hidden', 1)
enddef

" Test for using an invalid enum type name {{{1
def Test_enum_invalid_type_name()
  var lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    var x: enum<Color> = Color.Red
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: enum<Color>', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    var x: Colr
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: Colr', 5)

  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    var x: object<Colr>
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: Colr', 5)
enddef

" Test for using an enum in a def function defined before the enum {{{1
def Test_enum_declaration_order()
  # the enum can be used in the function body
  var lines =<< trim END
    vim9script
    def Paint(): string
      return Color.Green.name
    enddef
    enum Color
      Red, Green
    endenum
    assert_equal('Green', Paint())
  END
  v9.CheckSourceSuccess(lines)

  # but not as a type in the function signature
  lines =<< trim END
    vim9script
    def Paint(c: Color): string
      return c.name
    enddef
    enum Color
      Red, Green
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1010: Type not recognized: Color', 2)
enddef

" Test for an enum with the same name as another script item {{{1
def Test_enum_name_conflict()
  var before = [
    ['def Color()', 'enddef'],
    ['class Color', 'endclass'],
    ['interface Color', 'endinterface'],
    ['enum Color', '  Blue', 'endenum'],
  ]
  for items in before
    var lines = ['vim9script'] + items + ['enum Color', '  Red', 'endenum']
    v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Color"',
                          len(items) + 2)
  endfor

  var lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    var Color = 1
  END
  v9.CheckSourceFailure(lines, 'E1041: Redefining script item: "Color"', 5)

  # a local variable in a def function cannot have the name of an enum, but
  # it can have the name of an enum value
  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    def Fn()
      var Color = 1
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1054: Variable already declared in the script: Color', 1)

  lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    def Fn(): string
      var Red = Color.Green
      return Red.name .. Color.Red.name
    enddef
    assert_equal('GreenRed', Fn())
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using enum values in class and object variables {{{1
def Test_enum_as_class_member()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum

    class Car
      var color: Color = Color.Red
      static var default: Color = Color.Green
      def Paint(c: Color): Color
        var old = this.color
        this.color = c
        return old
      enddef
    endclass

    var car = Car.new()
    assert_true(car.color is Color.Red)
    assert_true(Car.default is Color.Green)
    assert_true(car.Paint(Color.Green) is Color.Red)
    assert_true(car.color is Color.Green)
    assert_true(Car.new(Color.Green).color is Color.Green)

    def Fn(): Color
      var c = Car.new()
      c.Paint(Car.default)
      return c.color
    enddef
    assert_true(Fn() is Color.Green)
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    enum Color
      Red
    endenum
    enum Fruit
      Apple
    endenum
    class Car
      var color: Color
    endclass
    var car = Car.new(Fruit.Apple)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected enum<Color> but got enum<Fruit>', 11)
enddef

" Test that enums with the same value names are different types {{{1
def Test_enum_same_member_names()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Light
      Red, Green
    endenum
    assert_false(Color.Red == Light.Red)
    assert_false(Color.Red is Light.Red)
    assert_true(Color.Red != Light.Red)
    # the name and ordinal are the same
    assert_equal(Color.Red.name, Light.Red.name)
    assert_equal(Color.Green.ordinal, Light.Green.ordinal)
    assert_equal('enum<Color>', typename(Color.Red))
    assert_equal('enum<Light>', typename(Light.Red))
  END
  v9.CheckSourceSuccess(lines)

  var header =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Light
      Red, Green
    endenum
  END
  v9.CheckSourceFailure(header + ['var c: Color = Light.Red'],
        'E1012: Type mismatch; expected enum<Color> but got enum<Light>', 8)
  v9.CheckSourceFailure(header + ['def Fn()', '  var c: Color = Light.Green',
                                  'enddef', 'defcompile'],
        'E1012: Type mismatch; expected enum<Color> but got enum<Light>', 1)
  # adding to a typed List
  v9.CheckSourceFailure(header + ['var l: list<Color> = []',
                                  'l->add(Light.Red)'],
        'E1012: Type mismatch; expected enum<Color> but got enum<Light>', 9)
  v9.CheckSourceFailure(header + ['def Fn()', '  var l: list<Color> = []',
                                  '  l->add(Light.Red)', 'enddef', 'Fn()'],
        'E1012: Type mismatch; expected enum<Color> but got enum<Light>', 2)
  v9.CheckSourceFailure(header + ['var l: list<Color> = []',
                                  'l->extend([Light.Red])'],
        'E1013: Argument 2: type mismatch, expected list<enum<Color>> but got list<enum<Light>> in extend()', 9)
enddef

" Test the runtime type check of an enum value in a variable of type "any"
" passed as a function argument or returned from a function {{{1
def Test_enum_runtime_type_check()
  var header =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Light
      Red, Green
    endenum
  END
  # return value
  v9.CheckSourceFailure(header + ['def Fn(a: any): Color', '  return a',
                                  'enddef', 'Fn(Light.Red)'],
        'E1012: Type mismatch; expected enum<Color> but got enum<Light>', 1)
  # argument, called from a def function
  v9.CheckSourceFailure(header + ['def Gn(c: Color)', 'enddef', 'def Fn()',
                                  '  var a: any = Light.Red', '  Gn(a)',
                                  'enddef', 'Fn()'],
        'E1013: Argument 1: type mismatch, expected enum<Color> but got enum<Light>', 2)
  # argument, called from a script
  v9.CheckSourceFailure(header + ['def Gn(c: Color)', 'enddef',
                                  'var a: any = Light.Red', 'Gn(a)'],
        'E1013: Argument 1: type mismatch, expected enum<Color> but got enum<Light>', 11)

  # the right enum type and a null object are accepted
  var lines = header + [
    'def Fn(a: any): Color', '  return a', 'enddef',
    'def Gn(c: Color): bool', '  return c == null_object', 'enddef',
    'var value: any = Color.Green',
    'assert_true(Fn(value) is Color.Green)',
    'assert_false(Gn(value))',
    'assert_true(Gn(null_object))',
  ]
  v9.CheckSourceSuccess(lines)
enddef

" Test assigning an enum value to an interface the enum doesn't implement {{{1
def Test_enum_not_implementing_interface()
  var lines =<< trim END
    vim9script
    interface HasArea
    endinterface
    enum Color
      Red
    endenum
    var h: HasArea = Color.Red
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<HasArea> but got enum<Color>', 7)

  lines =<< trim END
    vim9script
    interface HasArea
    endinterface
    enum Color
      Red
    endenum
    def Fn()
      var h: HasArea = Color.Red
    enddef
    defcompile
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected object<HasArea> but got enum<Color>', 1)
enddef

" Test comparing a List, Dict or Tuple with enum values {{{1
def Test_enum_container_equality()
  var lines =<< trim END
    vim9script
    enum Color
      Red, Green
    endenum
    enum Light
      Red, Green
    endenum
    assert_true([Color.Red, Color.Green] == [Color.Red, Color.Green])
    assert_false([Color.Red, Color.Green] == [Color.Green, Color.Red])
    assert_false([Color.Red] == [Light.Red])
    assert_true({a: Color.Red} == {a: Color.Red})
    assert_false({a: Color.Red} == {a: Color.Green})
    assert_true((Color.Red, Light.Red) == (Color.Red, Light.Red))
    assert_false((Color.Red,) == (Light.Red,))

    def Fn(): list<bool>
      return [[Color.Red] == [Color.Red], [Color.Red] == [Light.Red]]
    enddef
    assert_equal([true, false], Fn())
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test that an imported enum and an enum in the script with the same name and
" values are different types {{{1
def Test_enum_imported_same_name()
  var lines =<< trim END
    vim9script
    export enum Color
      Red, Green
    endenum
  END
  writefile(lines, 'Xenumsamename.vim', 'D')

  lines =<< trim END
    vim9script
    import './Xenumsamename.vim' as m
    enum Color
      Red, Green
    endenum
    assert_false(Color.Red == m.Color.Red)
    assert_true(m.Color.Red is m.Color.values[0])
    var c: Color = m.Color.Red
  END
  v9.CheckSourceFailure(lines, 'E1012: Type mismatch; expected enum<Color> but got enum<Color>', 8)

  lines =<< trim END
    vim9script
    import './Xenumsamename.vim' as m
    enum Color
      Red, Green
    endenum
    def Fn(c: m.Color): string
      return c.name
    enddef
    assert_equal('Green', Fn(m.Color.Green))
    Fn(Color.Red)
  END
  v9.CheckSourceFailure(lines, 'E1013: Argument 1: type mismatch, expected enum<Color> but got enum<Color>', 10)
enddef
" }}}
" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
