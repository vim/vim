" Test Vim9 enums

source check.vim
import './vim9.vim' as v9

" Test for parsing an enum definition
def Test_enum_parse()
  # enum supported only in a Vim9 script
  var lines =<< trim END
    enum Foo
    endenum
  END
  v9.CheckSourceFailure(lines, 'E1414: Enum can only be defined in Vim9 script', 1)

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

  # The complete "endenum" should be specified.
  lines =<< trim END
    vim9script
    enum Something
    enden
  END
  v9.CheckSourceFailure(lines, 'E1065: Command cannot be shortened: enden', 3)

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
    p = Planet.venus
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

" Test for type() and typename() of an enum
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

" Try modifying an enum or an enum item
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

" Test for using enum in an expression
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

" Using an enum in a lambda function
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

" Comparison using enums
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

" Test for using an enum as a default argument to a function
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

" Test for enum garbage collection
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

" Test for the enum values class variable
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

" Test comments in enums
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

" Test string() with enums
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

" Test for importing an enum
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

" Test for using test_refcount() with enum
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
enddef

" Test for defining an enum with additional object variables and methods
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

" Test for the enum value 'name' variable
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

" Test for the enum value 'ordinal' variable
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

" Test for trying to create a new enum object using the constructor
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

" Test for checking "this" in an enum constructor
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

" Test for using member variables in an enum object
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

" Test for using a custom constructor with an enum
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

" Test for using class variables in an enum class
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

" Test for converting a string to an enum value
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

" Test for using "values" in an enum class variable
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

" Test for using lambda block in enums
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

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
