" Test Vim9 classes

source check.vim
import './vim9.vim' as v9

def Test_class_basic()
  var lines =<< trim END
      class NotWorking
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1316:')

  lines =<< trim END
      vim9script
      class notWorking
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1314:')

  lines =<< trim END
      vim9script
      class Not@working
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1315:')

  lines =<< trim END
      vim9script
      abstract noclass Something
      endclass
  END
  v9.CheckScriptFailure(lines, 'E475:')

  lines =<< trim END
      vim9script
      abstract classy Something
      endclass
  END
  v9.CheckScriptFailure(lines, 'E475:')

  lines =<< trim END
      vim9script
      class Something
      endcl
  END
  v9.CheckScriptFailure(lines, 'E1065:')

  lines =<< trim END
      vim9script
      class Something
      endclass school's out
  END
  v9.CheckScriptFailure(lines, 'E488:')

  lines =<< trim END
      vim9script
      class Something
      endclass | echo 'done'
  END
  v9.CheckScriptFailure(lines, 'E488:')

  lines =<< trim END
      vim9script
      class Something
        this
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this.
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this .count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this. count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this.count: number
        that.count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1318: Not a valid command in a class: that.count')

  lines =<< trim END
      vim9script
      class Something
        this.count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1022:')

  lines =<< trim END
      vim9script
      class Something
        def new()
          this.state = 0
        enddef
      endclass
      var obj = Something.new()
  END
  v9.CheckScriptFailure(lines, 'E1089:')

  lines =<< trim END
      vim9script
      class Something
        this.count : number
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1059:')

  lines =<< trim END
      vim9script
      class Something
        this.count:number
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1069:')

  # Test for unsupported comment specifier
  lines =<< trim END
    vim9script
    class Something
      # comment
      #{
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1170:')

  lines =<< trim END
      vim9script

      class TextPosition
        this.lnum: number
        this.col: number

        # make a nicely formatted string
        def ToString(): string
          return $'({this.lnum}, {this.col})'
        enddef
      endclass

      # use the automatically generated new() method
      var pos = TextPosition.new(2, 12)
      assert_equal(2, pos.lnum)
      assert_equal(12, pos.col)

      # call an object method
      assert_equal('(2, 12)', pos.ToString())

      assert_equal(v:t_class, type(TextPosition))
      assert_equal(v:t_object, type(pos))
      assert_equal('class<TextPosition>', typename(TextPosition))
      assert_equal('object<TextPosition>', typename(pos))
  END
  v9.CheckScriptSuccess(lines)

  # When referencing object methods, space cannot be used after a "."
  lines =<< trim END
    vim9script
    class A
      def Foo(): number
        return 10
      enddef
    endclass
    var a = A.new()
    var v = a. Foo()
  END
  v9.CheckScriptFailure(lines, 'E1202:')

  # Using an object without specifying a method or a member variable
  lines =<< trim END
    vim9script
    class A
      def Foo(): number
        return 10
      enddef
    endclass
    var a = A.new()
    var v = a.
  END
  v9.CheckScriptFailure(lines, 'E15:')

  # Error when parsing the arguments of an object method.
  lines =<< trim END
    vim9script
    class A
      def Foo()
      enddef
    endclass
    var a = A.new()
    var v = a.Foo(,)
  END
  v9.CheckScriptFailure(lines, 'E15:')

  lines =<< trim END
  vim9script
  class A
    this.y = {
      X: 1
    }
  endclass
  var a = A.new()
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_class_defined_twice()
  # class defined twice should fail
  var lines =<< trim END
      vim9script
      class There
      endclass
      class There
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1041: Redefining script item: "There"')

  # one class, reload same script twice is OK
  lines =<< trim END
      vim9script
      class There
      endclass
  END
  writefile(lines, 'XclassTwice.vim', 'D')
  source XclassTwice.vim
  source XclassTwice.vim
enddef

def Test_returning_null_object()
  # this was causing an internal error
  var lines =<< trim END
      vim9script

      class BufferList
          def Current(): any
              return null_object
          enddef
      endclass

      var buffers = BufferList.new()
      echo buffers.Current()
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_using_null_class()
  var lines =<< trim END
      @_ = null_class.member
  END
  v9.CheckDefExecAndScriptFailure(lines, ['E715:', 'E1363:'])
enddef

def Test_class_interface_wrong_end()
  var lines =<< trim END
      vim9script
      abstract class SomeName
        this.member = 'text'
      endinterface
  END
  v9.CheckScriptFailure(lines, 'E476: Invalid command: endinterface, expected endclass')

  lines =<< trim END
      vim9script
      export interface AnotherName
        this.member: string
      endclass
  END
  v9.CheckScriptFailure(lines, 'E476: Invalid command: endclass, expected endinterface')
enddef

def Test_object_not_set()
  var lines =<< trim END
      vim9script

      class State
        this.value = 'xyz'
      endclass

      var state: State
      var db = {'xyz': 789}
      echo db[state.value]
  END
  v9.CheckScriptFailure(lines, 'E1360:')

  lines =<< trim END
      vim9script

      class Class
          this.id: string
          def Method1()
              echo 'Method1' .. this.id
          enddef
      endclass

      var obj: Class
      def Func()
          obj.Method1()
      enddef
      Func()
  END
  v9.CheckScriptFailure(lines, 'E1360:')

  lines =<< trim END
      vim9script

      class Background
        this.background = 'dark'
      endclass

      class Colorscheme
        this._bg: Background

        def GetBackground(): string
          return this._bg.background
        enddef
      endclass

      var bg: Background           # UNINITIALIZED
      echo Colorscheme.new(bg).GetBackground()
  END
  v9.CheckScriptFailure(lines, 'E1360:')

  # TODO: this should not give an error but be handled at runtime
  lines =<< trim END
      vim9script

      class Class
          this.id: string
          def Method1()
              echo 'Method1' .. this.id
          enddef
      endclass

      var obj = null_object
      def Func()
          obj.Method1()
      enddef
      Func()
  END
  v9.CheckScriptFailure(lines, 'E1363:')
enddef

def Test_null_object_assign_compare()
  var lines =<< trim END
    vim9script

    var nullo = null_object
    def F(): any
      return nullo
    enddef
    assert_equal('object<Unknown>', typename(F()))

    var o0 = F()
    assert_true(o0 == null_object)
    assert_true(o0 == null)

    var o1: any = nullo
    assert_true(o1 == null_object)
    assert_true(o1 == null)

    def G()
      var x = null_object
    enddef

    class C
    endclass
    var o2: C
    assert_true(o2 == null_object)
    assert_true(o2 == null)

    o2 = null_object
    assert_true(o2 == null)

    o2 = C.new()
    assert_true(o2 != null)

    o2 = null_object
    assert_true(o2 == null)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_class_member_initializer()
  var lines =<< trim END
      vim9script

      class TextPosition
        this.lnum: number = 1
        this.col: number = 1

        # constructor with only the line number
        def new(lnum: number)
          this.lnum = lnum
        enddef
      endclass

      var pos = TextPosition.new(3)
      assert_equal(3, pos.lnum)
      assert_equal(1, pos.col)

      var instr = execute('disassemble TextPosition.new')
      assert_match('new\_s*' ..
            '0 NEW TextPosition size \d\+\_s*' ..
            '\d PUSHNR 1\_s*' ..
            '\d STORE_THIS 0\_s*' ..
            '\d PUSHNR 1\_s*' ..
            '\d STORE_THIS 1\_s*' ..
            'this.lnum = lnum\_s*' ..
            '\d LOAD arg\[-1]\_s*' ..
            '\d PUSHNR 0\_s*' ..
            '\d LOAD $0\_s*' ..
            '\d\+ STOREINDEX object\_s*' ..
            '\d\+ RETURN object.*',
            instr)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_member_any_used_as_object()
  var lines =<< trim END
      vim9script

      class Inner
        this.value: number = 0
      endclass

      class Outer
        this.inner: any
      endclass

      def F(outer: Outer)
        outer.inner.value = 1
      enddef

      var inner_obj = Inner.new(0)
      var outer_obj = Outer.new(inner_obj)
      F(outer_obj)
      assert_equal(1, inner_obj.value)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      class Inner
        this.value: number = 0
      endclass

      class Outer
        this.inner: Inner
      endclass

      def F(outer: Outer)
        outer.inner.value = 1
      enddef

      def Test_assign_to_nested_typed_member()
        var inner = Inner.new(0)
        var outer = Outer.new(inner)
        F(outer)
        assert_equal(1, inner.value)
      enddef

      Test_assign_to_nested_typed_member()
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_assignment_with_operator()
  var lines =<< trim END
      vim9script

      class Foo
        this.x: number

        def Add(n: number)
          this.x += n
        enddef
      endclass

      var f =  Foo.new(3)
      f.Add(17)
      assert_equal(20, f.x)

      def AddToFoo(obj: Foo)
        obj.x += 3
      enddef

      AddToFoo(f)
      assert_equal(23, f.x)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_list_of_objects()
  var lines =<< trim END
      vim9script

      class Foo
        def Add()
        enddef
      endclass

      def ProcessList(fooList: list<Foo>)
        for foo in fooList
          foo.Add()
        endfor
      enddef

      var l: list<Foo> = [Foo.new()]
      ProcessList(l)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_expr_after_using_object()
  var lines =<< trim END
      vim9script

      class Something
        this.label: string = ''
      endclass

      def Foo(): Something
        var v = Something.new()
        echo 'in Foo(): ' .. typename(v)
        return v
      enddef

      Foo()
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_class_default_new()
  var lines =<< trim END
      vim9script

      class TextPosition
        this.lnum: number = 1
        this.col: number = 1
      endclass

      var pos = TextPosition.new()
      assert_equal(1, pos.lnum)
      assert_equal(1, pos.col)

      pos = TextPosition.new(v:none, v:none)
      assert_equal(1, pos.lnum)
      assert_equal(1, pos.col)

      pos = TextPosition.new(3, 22)
      assert_equal(3, pos.lnum)
      assert_equal(22, pos.col)

      pos = TextPosition.new(v:none, 33)
      assert_equal(1, pos.lnum)
      assert_equal(33, pos.col)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      class Person
        this.name: string
        this.age: number = 42
        this.education: string = "unknown"

        def new(this.name, this.age = v:none, this.education = v:none)
        enddef
      endclass

      var piet = Person.new("Piet")
      assert_equal("Piet", piet.name)
      assert_equal(42, piet.age)
      assert_equal("unknown", piet.education)

      var chris = Person.new("Chris", 4, "none")
      assert_equal("Chris", chris.name)
      assert_equal(4, chris.age)
      assert_equal("none", chris.education)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      class Person
        this.name: string
        this.age: number = 42
        this.education: string = "unknown"

        def new(this.name, this.age = v:none, this.education = v:none)
        enddef
      endclass

      var missing = Person.new()
  END
  v9.CheckScriptFailure(lines, 'E119:')
enddef


def Test_class_new_with_object_member()
  var lines =<< trim END
      vim9script

      class C
        this.str: string
        this.num: number
        def new(this.str, this.num)
        enddef
        def newVals(this.str, this.num)
        enddef
      endclass

      def Check()
        try
          var c = C.new('cats', 2)
          assert_equal('cats', c.str)
          assert_equal(2, c.num)

          c = C.newVals('dogs', 4)
          assert_equal('dogs', c.str)
          assert_equal(4, c.num)
        catch
          assert_report($'Unexpected exception was caught: {v:exception}')
        endtry
      enddef

      Check()
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      class C
        this.str: string
        this.num: number
        def new(this.str, this.num)
        enddef
      endclass

      def Check()
        try
          var c = C.new(1, 2)
        catch
          assert_report($'Unexpected exception was caught: {v:exception}')
        endtry
      enddef

      Check()
  END
  v9.CheckScriptFailure(lines, 'E1013:')

  lines =<< trim END
      vim9script

      class C
        this.str: string
        this.num: number
        def newVals(this.str, this.num)
        enddef
      endclass

      def Check()
        try
          var c = C.newVals('dogs', 'apes')
        catch
          assert_report($'Unexpected exception was caught: {v:exception}')
        endtry
      enddef

      Check()
  END
  v9.CheckScriptFailure(lines, 'E1013:')
enddef

def Test_class_object_member_inits()
  var lines =<< trim END
      vim9script
      class TextPosition
        this.lnum: number
        this.col = 1
        this.addcol: number = 2
      endclass

      var pos = TextPosition.new()
      assert_equal(0, pos.lnum)
      assert_equal(1, pos.col)
      assert_equal(2, pos.addcol)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      class TextPosition
        this.lnum
        this.col = 1
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1022:')

  # If the type is not specified for a member, then it should be set during
  # object creation and not when defining the class.
  lines =<< trim END
      vim9script

      var init_count = 0
      def Init(): string
        init_count += 1
        return 'foo'
      enddef

      class A
        this.str1 = Init()
        this.str2: string = Init()
        this.col = 1
      endclass

      assert_equal(init_count, 0)
      var a = A.new()
      assert_equal(init_count, 2)
  END
  v9.CheckScriptSuccess(lines)

  # Test for initializing an object member with an unknown variable/type
  lines =<< trim END
    vim9script
    class A
       this.value = init_val
    endclass
    var a = A.new()
  END
  v9.CheckScriptFailure(lines, 'E1001:')
enddef

def Test_class_object_member_access()
  var lines =<< trim END
      vim9script
      class Triple
         this._one = 1
         this.two = 2
         public this.three = 3

         def GetOne(): number
           return this._one
         enddef
      endclass

      var trip = Triple.new()
      assert_equal(1, trip.GetOne())
      assert_equal(2, trip.two)
      assert_equal(3, trip.three)
      assert_fails('echo trip._one', 'E1333')

      assert_fails('trip._one = 11', 'E1333')
      assert_fails('trip.two = 22', 'E1335')
      trip.three = 33
      assert_equal(33, trip.three)

      assert_fails('trip.four = 4', 'E1334')
  END
  v9.CheckScriptSuccess(lines)

  # Test for a public member variable name beginning with an underscore
  lines =<< trim END
    vim9script
    class A
      public this._val = 10
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1332:')

  lines =<< trim END
      vim9script

      class MyCar
        this.make: string
        this.age = 5

        def new(make_arg: string)
          this.make = make_arg
        enddef

        def GetMake(): string
          return $"make = {this.make}"
        enddef
        def GetAge(): number
          return this.age
        enddef
      endclass

      var c = MyCar.new("abc")
      assert_equal('make = abc', c.GetMake())

      c = MyCar.new("def")
      assert_equal('make = def', c.GetMake())

      var c2 = MyCar.new("123")
      assert_equal('make = 123', c2.GetMake())

      def CheckCar()
        assert_equal("make = def", c.GetMake())
        assert_equal(5, c.GetAge())
      enddef
      CheckCar()
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      class MyCar
        this.make: string

        def new(make_arg: string)
            this.make = make_arg
        enddef
      endclass

      var c = MyCar.new("abc")
      var c = MyCar.new("def")
  END
  v9.CheckScriptFailure(lines, 'E1041:')

  lines =<< trim END
      vim9script

      class Foo
        this.x: list<number> = []

        def Add(n: number): any
          this.x->add(n)
          return this
        enddef
      endclass

      echo Foo.new().Add(1).Add(2).x
      echo Foo.new().Add(1).Add(2)
            .x
      echo Foo.new().Add(1)
            .Add(2).x
      echo Foo.new()
            .Add(1).Add(2).x
      echo Foo.new()
            .Add(1) 
            .Add(2)
            .x
  END
  v9.CheckScriptSuccess(lines)

  # Test for "public" cannot be abbreviated
  lines =<< trim END
    vim9script
    class Something
      pub this.val = 1
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1065:')

  # Test for "public" keyword must be followed by "this" or "static".
  lines =<< trim END
    vim9script
    class Something
      public val = 1
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1331:')

  # Test for "static" cannot be abbreviated
  lines =<< trim END
    vim9script
    class Something
      stat this.val = 1
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1065:')
enddef

def Test_class_object_compare()
  var class_lines =<< trim END
      vim9script
      class Item
        this.nr = 0
        this.name = 'xx'
      endclass
  END

  # used at the script level and in a compiled function
  var test_lines =<< trim END
      var i1 = Item.new()
      assert_equal(i1, i1)
      assert_true(i1 is i1)
      var i2 = Item.new()
      assert_equal(i1, i2)
      assert_false(i1 is i2)
      var i3 = Item.new(0, 'xx')
      assert_equal(i1, i3)

      var io1 = Item.new(1, 'xx')
      assert_notequal(i1, io1)
      var io2 = Item.new(0, 'yy')
      assert_notequal(i1, io2)
  END

  v9.CheckScriptSuccess(class_lines + test_lines)
  v9.CheckScriptSuccess(
      class_lines + ['def Test()'] + test_lines + ['enddef', 'Test()'])

  for op in ['>', '>=', '<', '<=', '=~', '!~']
    var op_lines = [
          'var i1 = Item.new()',
          'var i2 = Item.new()',
          'echo i1 ' .. op .. ' i2',
          ]
    v9.CheckScriptFailure(class_lines + op_lines, 'E1153: Invalid operation for object')
    v9.CheckScriptFailure(class_lines
          + ['def Test()'] + op_lines + ['enddef', 'Test()'], 'E1153: Invalid operation for object')
  endfor
enddef

def Test_object_type()
  var lines =<< trim END
      vim9script

      class One
        this.one = 1
      endclass
      class Two
        this.two = 2
      endclass
      class TwoMore extends Two
        this.more = 9
      endclass

      var o: One = One.new()
      var t: Two = Two.new()
      var m: TwoMore = TwoMore.new()
      var tm: Two = TwoMore.new()

      t = m
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      class One
        this.one = 1
      endclass
      class Two
        this.two = 2
      endclass

      var o: One = Two.new()
  END
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected object<One> but got object<Two>')

  lines =<< trim END
      vim9script

      interface One
        def GetMember(): number
      endinterface
      class Two implements One
        this.one = 1
        def GetMember(): number
          return this.one
        enddef
      endclass

      var o: One = Two.new(5)
      assert_equal(5, o.GetMember())
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      class Num
        this.n: number = 0
      endclass

      def Ref(name: string): func(Num): Num
        return (arg: Num): Num => {
          return eval(name)(arg)
        }
      enddef

      const Fn = Ref('Double')
      var Double = (m: Num): Num => Num.new(m.n * 2)

      echo Fn(Num.new(4))
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_class_member()
  # check access rules
  var lines =<< trim END
      vim9script
      class TextPos
         this.lnum = 1
         this.col = 1
         static counter = 0
         static _secret = 7
         public static  anybody = 42

         static def AddToCounter(nr: number)
           counter += nr
         enddef
      endclass

      assert_equal(0, TextPos.counter)
      TextPos.AddToCounter(3)
      assert_equal(3, TextPos.counter)
      assert_fails('echo TextPos.noSuchMember', 'E1338:')

      def GetCounter(): number
        return TextPos.counter
      enddef
      assert_equal(3, GetCounter())

      assert_fails('TextPos.noSuchMember = 2', 'E1337:')
      assert_fails('TextPos.counter = 5', 'E1335:')
      assert_fails('TextPos.counter += 5', 'E1335:')

      assert_fails('echo TextPos._secret', 'E1333:')
      assert_fails('TextPos._secret = 8', 'E1333:')

      assert_equal(42, TextPos.anybody)
      TextPos.anybody = 12
      assert_equal(12, TextPos.anybody)
      TextPos.anybody += 5
      assert_equal(17, TextPos.anybody)
  END
  v9.CheckScriptSuccess(lines)

  # example in the help
  lines =<< trim END
        vim9script
	class OtherThing
	   this.size: number
	   static totalSize: number

	   def new(this.size)
	      totalSize += this.size
	   enddef
	endclass
        assert_equal(0, OtherThing.totalSize)
        var to3 = OtherThing.new(3)
        assert_equal(3, OtherThing.totalSize)
        var to7 = OtherThing.new(7)
        assert_equal(10, OtherThing.totalSize)
  END
  v9.CheckScriptSuccess(lines)

  # using static class member twice
  lines =<< trim END
      vim9script

      class HTML
        static author: string = 'John Doe'

        static def MacroSubstitute(s: string): string
          return substitute(s, '{{author}}', author, 'gi')
        enddef
      endclass

      assert_equal('some text', HTML.MacroSubstitute('some text'))
      assert_equal('some text', HTML.MacroSubstitute('some text'))
  END
  v9.CheckScriptSuccess(lines)

  # access private member in lambda
  lines =<< trim END
      vim9script

      class Foo
        this._x: number = 0

        def Add(n: number): number
          const F = (): number => this._x + n
          return F()
        enddef
      endclass

      var foo = Foo.new()
      assert_equal(5, foo.Add(5))
  END
  v9.CheckScriptSuccess(lines)

  # access private member in lambda body
  lines =<< trim END
      vim9script

      class Foo
        this._x: number = 6

        def Add(n: number): number
          var Lam = () => {
            this._x = this._x + n
          }
          Lam()
          return this._x
        enddef
      endclass

      var foo = Foo.new()
      assert_equal(13, foo.Add(7))
  END
  v9.CheckScriptSuccess(lines)

  # check shadowing
  lines =<< trim END
      vim9script

      class Some
        static count = 0
        def Method(count: number)
          echo count
        enddef
      endclass

      var s = Some.new()
      s.Method(7)
  END
  v9.CheckScriptFailure(lines, 'E1340: Argument already declared in the class: count')

  lines =<< trim END
      vim9script

      class Some
        static count = 0
        def Method(arg: number)
          var count = 3
          echo arg count
        enddef
      endclass

      var s = Some.new()
      s.Method(7)
  END
  v9.CheckScriptFailure(lines, 'E1341: Variable already declared in the class: count')

  # Test for using an invalid type for a member variable
  lines =<< trim END
    vim9script
    class A
      this.val: xxx
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1010:')

  # Test for no space before or after the '=' when initializing a member
  # variable
  lines =<< trim END
    vim9script
    class A
      this.val: number= 10
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1004:')
  lines =<< trim END
    vim9script
    class A
      this.val: number =10
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1004:')

  # Access a non-existing member
  lines =<< trim END
    vim9script
    class A
    endclass
    var a = A.new()
    var v = a.bar
  END
  v9.CheckScriptFailure(lines, 'E1326:')
enddef

func Test_class_garbagecollect()
  let lines =<< trim END
      vim9script

      class Point
        this.p = [2, 3]
        static pl = ['a', 'b']
        static pd = {a: 'a', b: 'b'}
      endclass

      echo Point.pl Point.pd
      call test_garbagecollect_now()
      echo Point.pl Point.pd
  END
  call v9.CheckScriptSuccess(lines)

  let lines =<< trim END
      vim9script

      interface View
      endinterface

      class Widget
        this.view: View
      endclass

      class MyView implements View
        this.widget: Widget

        def new()
          # this will result in a circular reference to this object
          this.widget = Widget.new(this)
        enddef
      endclass

      var view = MyView.new()

      # overwrite "view", will be garbage-collected next
      view = MyView.new()
      test_garbagecollect_now()
  END
  call v9.CheckScriptSuccess(lines)
endfunc

def Test_class_function()
  var lines =<< trim END
      vim9script
      class Value
        this.value = 0
        static objects = 0

        def new(v: number)
          this.value = v
          ++objects
        enddef

        static def GetCount(): number
          return objects
        enddef
      endclass

      assert_equal(0, Value.GetCount())
      var v1 = Value.new(2)
      assert_equal(1, Value.GetCount())
      var v2 = Value.new(7)
      assert_equal(2, Value.GetCount())
  END
  v9.CheckScriptSuccess(lines)

  # Test for cleaning up after a class definition failure when using class
  # functions.
  lines =<< trim END
    vim9script
    class A
      static def Foo()
      enddef
      aaa
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1318:')
enddef

def Test_class_defcompile()
  var lines =<< trim END
      vim9script

      class C
          def Fo(i: number): string
              return i
          enddef
      endclass

      defcompile C.Fo
  END
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected string but got number')

  lines =<< trim END
      vim9script

      class C
          static def Fc(): number
            return 'x'
          enddef
      endclass

      defcompile C.Fc
  END
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected number but got string')

  # Trying to compile a function using a non-existing class variable
  lines =<< trim END
    vim9script
    defcompile x.Foo()
  END
  v9.CheckScriptFailure(lines, 'E475:')

  # Trying to compile a function using a variable which is not a class
  lines =<< trim END
    vim9script
    var x: number
    defcompile x.Foo()
  END
  v9.CheckScriptFailure(lines, 'E475:')

  # Trying to compile a function without specifying the name
  lines =<< trim END
    vim9script
    class A
    endclass
    defcompile A.
  END
  v9.CheckScriptFailure(lines, 'E475:')

  # Trying to compile a non-existing class object member function
  lines =<< trim END
    vim9script
    class A
    endclass
    var a = A.new()
    defcompile a.Foo()
  END
  v9.CheckScriptFailureList(lines, ['E1334:', 'E475:'])
enddef

def Test_class_object_to_string()
  var lines =<< trim END
      vim9script
      class TextPosition
        this.lnum = 1
        this.col = 22
      endclass

      assert_equal("class TextPosition", string(TextPosition))

      var pos = TextPosition.new()
      assert_equal("object of TextPosition {lnum: 1, col: 22}", string(pos))
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_interface_basics()
  var lines =<< trim END
      vim9script
      interface Something
        this.value: string
        static count: number
        def GetCount(): number
      endinterface
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      interface SomethingWrong
        static count = 7
      endinterface
  END
  v9.CheckScriptFailure(lines, 'E1342:')

  lines =<< trim END
      vim9script

      interface Some
        static count: number
        def Method(count: number)
      endinterface
  END
  v9.CheckScriptFailure(lines, 'E1340: Argument already declared in the class: count', 5)

  lines =<< trim END
      vim9script

      interface Some
        this.value: number
        def Method(value: number)
      endinterface
  END
  # The argument name and the object member name are the same, but this is not a
  # problem because object members are always accessed with the "this." prefix.
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      interface somethingWrong
        static count = 7
      endinterface
  END
  v9.CheckScriptFailure(lines, 'E1343: Interface name must start with an uppercase letter: somethingWrong')

  lines =<< trim END
      vim9script
      interface SomethingWrong
        this.value: string
        static count = 7
        def GetCount(): number
      endinterface
  END
  v9.CheckScriptFailure(lines, 'E1344:')

  lines =<< trim END
      vim9script
      interface SomethingWrong
        this.value: string
        static count: number
        def GetCount(): number
          return 5
        enddef
      endinterface
  END
  v9.CheckScriptFailure(lines, 'E1345: Not a valid command in an interface: return 5')

  lines =<< trim END
      vim9script
      export interface EnterExit
          def Enter(): void
          def Exit(): void
      endinterface
  END
  writefile(lines, 'XdefIntf.vim', 'D')

  lines =<< trim END
      vim9script
      import './XdefIntf.vim' as defIntf
      export def With(ee: defIntf.EnterExit, F: func)
          ee.Enter()
          try
              F()
          finally
              ee.Exit()
          endtry
      enddef
  END
  v9.CheckScriptSuccess(lines)

  var imported =<< trim END
      vim9script
      export abstract class EnterExit
          def Enter(): void
          enddef
          def Exit(): void
          enddef
      endclass
  END
  writefile(imported, 'XdefIntf2.vim', 'D')

  lines[1] = " import './XdefIntf2.vim' as defIntf"
  v9.CheckScriptSuccess(lines)
enddef

def Test_class_implements_interface()
  var lines =<< trim END
      vim9script

      interface Some
        static count: number
        def Method(nr: number)
      endinterface

      class SomeImpl implements Some
        static count: number
        def Method(nr: number)
          echo nr
        enddef
      endclass

      interface Another
        this.member: string
      endinterface

      class AnotherImpl implements Some, Another
        this.member = 'abc'
        static count: number
        def Method(nr: number)
          echo nr
        enddef
      endclass
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      interface Some
        static counter: number
      endinterface

      class SomeImpl implements Some implements Some
        static count: number
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1350:')

  lines =<< trim END
      vim9script

      interface Some
        static counter: number
      endinterface

      class SomeImpl implements Some, Some
        static count: number
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1351: Duplicate interface after "implements": Some')

  lines =<< trim END
      vim9script

      interface Some
        static counter: number
        def Method(nr: number)
      endinterface

      class SomeImpl implements Some
        static count: number
        def Method(nr: number)
          echo nr
        enddef
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1348: Member "counter" of interface "Some" not implemented')

  lines =<< trim END
      vim9script

      interface Some
        static count: number
        def Methods(nr: number)
      endinterface

      class SomeImpl implements Some
        static count: number
        def Method(nr: number)
          echo nr
        enddef
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1349: Function "Methods" of interface "Some" not implemented')

  # Check different order of members in class and interface works.
  lines =<< trim END
      vim9script

      interface Result
        public this.label: string
        this.errpos: number
      endinterface

      # order of members is opposite of interface
      class Failure implements Result
        this.errpos: number = 42
        public this.label: string = 'label'
      endclass

      def Test()
        var result: Result = Failure.new()

        assert_equal('label', result.label)
        assert_equal(42, result.errpos)

        result.label = 'different'
        assert_equal('different', result.label)
        assert_equal(42, result.errpos)
      enddef

      Test()
  END
  v9.CheckScriptSuccess(lines)

  # Interface name after "extends" doesn't end in a space or NUL character
  lines =<< trim END
    vim9script
    interface A
    endinterface
    class B extends A"
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1315:')

  # Trailing characters after a class name
  lines =<< trim END
    vim9script
    class A bbb
    endclass
  END
  v9.CheckScriptFailure(lines, 'E488:')

  # using "implements" with a non-existing class
  lines =<< trim END
    vim9script
    class A implements B
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1346:')

  # using "implements" with a regular class
  lines =<< trim END
    vim9script
    class A
    endclass
    class B implements A
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1347:')

  # using "implements" with a variable
  lines =<< trim END
    vim9script
    var T: number = 10
    class A implements T
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1347:')

  # all the class methods in an "interface" should be implemented
  lines =<< trim END
    vim9script
    interface A
      static def Foo()
    endinterface
    class B implements A
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1349:')

  # implements should be followed by a white space
  lines =<< trim END
    vim9script
    interface A
    endinterface
    class B implements A;
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1315:')

  lines =<< trim END
      vim9script

      interface One
        static matching: bool
        static as_any: any
        static not_matching: number
      endinterface
      class Two implements One
        static not_matching: string
        static as_any: string
        static matching: bool
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1406: Member "not_matching": type mismatch, expected number but got string')

  lines =<< trim END
      vim9script

      interface One
        def IsEven(nr: number): bool
      endinterface
      class Two implements One
        def IsEven(nr: number): string
        enddef
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1407: Member "IsEven": type mismatch, expected func(number): bool but got func(number): string')

  lines =<< trim END
      vim9script

      interface One
        def IsEven(nr: number): bool
      endinterface
      class Two implements One
        def IsEven(nr: bool): bool
        enddef
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1407: Member "IsEven": type mismatch, expected func(number): bool but got func(bool): bool')

  lines =<< trim END
      vim9script

      interface One
        def IsEven(nr: number): bool
      endinterface
      class Two implements One
        def IsEven(nr: number, ...extra: list<number>): bool
        enddef
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1407: Member "IsEven": type mismatch, expected func(number): bool but got func(number, ...list<number>): bool')
enddef

def Test_call_interface_method()
  var lines =<< trim END
    vim9script
    interface Base
      def Enter(): void
    endinterface

    class Child implements Base
      def Enter(): void
        g:result ..= 'child'
      enddef
    endclass

    def F(obj: Base)
      obj.Enter()
    enddef

    g:result = ''
    F(Child.new())
    assert_equal('child', g:result)
    unlet g:result
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    class Base
      def Enter(): void
        g:result ..= 'base'
      enddef
    endclass

    class Child extends Base
      def Enter(): void
        g:result ..= 'child'
      enddef
    endclass

    def F(obj: Base)
      obj.Enter()
    enddef

    g:result = ''
    F(Child.new())
    assert_equal('child', g:result)
    unlet g:result
  END
  v9.CheckScriptSuccess(lines)

  # method of interface returns a value
  lines =<< trim END
    vim9script
    interface Base
      def Enter(): string
    endinterface

    class Child implements Base
      def Enter(): string
        g:result ..= 'child'
        return "/resource"
      enddef
    endclass

    def F(obj: Base)
      var r = obj.Enter()
      g:result ..= r
    enddef

    g:result = ''
    F(Child.new())
    assert_equal('child/resource', g:result)
    unlet g:result
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script
    class Base
      def Enter(): string
        return null_string
      enddef
    endclass

    class Child extends Base
      def Enter(): string
        g:result ..= 'child'
        return "/resource"
      enddef
    endclass

    def F(obj: Base)
      var r = obj.Enter()
      g:result ..= r
    enddef

    g:result = ''
    F(Child.new())
    assert_equal('child/resource', g:result)
    unlet g:result
  END
  v9.CheckScriptSuccess(lines)


  # No class that implements the interface.
  lines =<< trim END
      vim9script

      interface IWithEE
          def Enter(): any
          def Exit(): void
      endinterface

      def With1(ee: IWithEE, F: func)
          var r = ee.Enter()
      enddef

      defcompile
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_class_used_as_type()
  var lines =<< trim END
      vim9script

      class Point
        this.x = 0
        this.y = 0
      endclass

      var p: Point
      p = Point.new(2, 33)
      assert_equal(2, p.x)
      assert_equal(33, p.y)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      interface HasX
        this.x: number
      endinterface

      class Point implements HasX
        this.x = 0
        this.y = 0
      endclass

      var p: Point
      p = Point.new(2, 33)
      var hx = p
      assert_equal(2, hx.x)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script

      class Point
        this.x = 0
        this.y = 0
      endclass

      var p: Point
      p = 'text'
  END
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected object<Point> but got string')
enddef

def Test_class_extends()
  var lines =<< trim END
      vim9script
      class Base
        this.one = 1
        def GetOne(): number
          return this.one
        enddef
      endclass
      class Child extends Base
        this.two = 2
        def GetTotal(): number
          return this.one + this.two
        enddef
      endclass
      var o = Child.new()
      assert_equal(1, o.one)
      assert_equal(2, o.two)
      assert_equal(1, o.GetOne())
      assert_equal(3, o.GetTotal())
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      class Base
        this.one = 1
      endclass
      class Child extends Base
        this.two = 2
      endclass
      var o = Child.new(3, 44)
      assert_equal(3, o.one)
      assert_equal(44, o.two)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      class Base
        this.one = 1
      endclass
      class Child extends Base extends Base
        this.two = 2
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1352: Duplicate "extends"')

  lines =<< trim END
      vim9script
      class Child extends BaseClass
        this.two = 2
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1353: Class name not found: BaseClass')

  lines =<< trim END
      vim9script
      var SomeVar = 99
      class Child extends SomeVar
        this.two = 2
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1354: Cannot extend SomeVar')

  lines =<< trim END
      vim9script
      class Base
        this.name: string
        def ToString(): string
          return this.name
        enddef
      endclass

      class Child extends Base
        this.age: number
        def ToString(): string
          return super.ToString() .. ': ' .. this.age
        enddef
      endclass

      var o = Child.new('John', 42)
      assert_equal('John: 42', o.ToString())
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      class Child
        this.age: number
        def ToString(): number
          return this.age
        enddef
        def ToString(): string
          return this.age
        enddef
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1355: Duplicate function: ToString')

  lines =<< trim END
      vim9script
      class Child
        this.age: number
        def ToString(): string
          return super .ToString() .. ': ' .. this.age
        enddef
      endclass
      var o = Child.new(42)
      echo o.ToString()
  END
  v9.CheckScriptFailure(lines, 'E1356:')

  lines =<< trim END
      vim9script
      class Base
        this.name: string
        def ToString(): string
          return this.name
        enddef
      endclass

      var age = 42
      def ToString(): string
        return super.ToString() .. ': ' .. age
      enddef
      echo ToString()
  END
  v9.CheckScriptFailure(lines, 'E1357:')

  lines =<< trim END
      vim9script
      class Child
        this.age: number
        def ToString(): string
          return super.ToString() .. ': ' .. this.age
        enddef
      endclass
      var o = Child.new(42)
      echo o.ToString()
  END
  v9.CheckScriptFailure(lines, 'E1358:')

  lines =<< trim END
      vim9script
      class Base
        this.name: string
        static def ToString(): string
          return 'Base class'
        enddef
      endclass

      class Child extends Base
        this.age: number
        def ToString(): string
          return Base.ToString() .. ': ' .. this.age
        enddef
      endclass

      var o = Child.new('John', 42)
      assert_equal('Base class: 42', o.ToString())
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      class Base
        this.value = 1
        def new(init: number)
          this.value = number + 1
        enddef
      endclass
      class Child extends Base
        def new()
          this.new(3)
        enddef
      endclass
      var c = Child.new()
  END
  v9.CheckScriptFailure(lines, 'E1325: Method not found on class "Child": new(')

  # base class with more than one object member
  lines =<< trim END
      vim9script

      class Result
        this.success: bool
        this.value: any = null
      endclass

      class Success extends Result
        def new(this.value = v:none)
          this.success = true
        enddef
      endclass

      var v = Success.new('asdf')
      assert_equal("object of Success {success: true, value: 'asdf'}", string(v))
  END
  v9.CheckScriptSuccess(lines)

  # class name after "extends" doesn't end in a space or NUL character
  lines =<< trim END
    vim9script
    class A
    endclass
    class B extends A"
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1315:')
enddef

def Test_using_base_class()
  var lines =<< trim END
    vim9script

    class BaseEE
        def Enter(): any
            return null
        enddef
        def Exit(resource: any): void
        enddef
    endclass

    class ChildEE extends BaseEE
        def Enter(): any
            return 42
        enddef

        def Exit(resource: number): void
            g:result ..= '/exit'
        enddef
    endclass

    def With(ee: BaseEE)
        var r = ee.Enter()
        try
            g:result ..= r
        finally
            g:result ..= '/finally'
            ee.Exit(r)
        endtry
    enddef

    g:result = ''
    With(ChildEE.new())
    assert_equal('42/finally/exit', g:result)
  END
  v9.CheckScriptSuccess(lines)
  unlet g:result

  # Using super, Child invokes Base method which has optional arg. #12471
  lines =<< trim END
    vim9script

    class Base
        this.success: bool = false
        def Method(arg = 0)
            this.success = true
        enddef
    endclass

    class Child extends Base
        def new()
            super.Method()
        enddef
    endclass

    var obj = Child.new()
    assert_equal(true, obj.success)
  END
  v9.CheckScriptSuccess(lines)
enddef


def Test_class_import()
  var lines =<< trim END
      vim9script
      export class Animal
        this.kind: string
        this.name: string
      endclass
  END
  writefile(lines, 'Xanimal.vim', 'D')

  lines =<< trim END
      vim9script
      import './Xanimal.vim' as animal

      var a: animal.Animal
      a = animal.Animal.new('fish', 'Eric')
      assert_equal('fish', a.kind)
      assert_equal('Eric', a.name)

      var b: animal.Animal = animal.Animal.new('cat', 'Garfield')
      assert_equal('cat', b.kind)
      assert_equal('Garfield', b.name)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_abstract_class()
  var lines =<< trim END
      vim9script
      abstract class Base
        this.name: string
      endclass
      class Person extends Base
        this.age: number
      endclass
      var p: Base = Person.new('Peter', 42)
      assert_equal('Peter', p.name)
      assert_equal(42, p.age)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
      vim9script
      abstract class Base
        this.name: string
      endclass
      class Person extends Base
        this.age: number
      endclass
      var p = Base.new('Peter')
  END
  v9.CheckScriptFailure(lines, 'E1325: Method not found on class "Base": new(')

  lines =<< trim END
      abstract class Base
        this.name: string
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1316:')

  # Abstract class cannot have a "new" function
  lines =<< trim END
    vim9script
    abstract class Base
      def new()
      enddef
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1359:')
enddef

def Test_closure_in_class()
  var lines =<< trim END
      vim9script

      class Foo
        this.y: list<string> = ['B']

        def new()
          g:result = filter(['A', 'B'], (_, v) => index(this.y, v) == -1)
        enddef
      endclass

      Foo.new()
      assert_equal(['A'], g:result)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_call_constructor_from_legacy()
  var lines =<< trim END
      vim9script

      var newCalled = 'false'

      class A
        def new()
          newCalled = 'true'
        enddef
      endclass

      export def F(options = {}): any
        return A
      enddef

      g:p = F()
      legacy call p.new()
      assert_equal('true', newCalled)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_defer_with_object()
  var lines =<< trim END
      vim9script

      class CWithEE
        def Enter()
          g:result ..= "entered/"
        enddef
        def Exit()
          g:result ..= "exited"
        enddef
      endclass

      def With(ee: CWithEE, F: func)
        ee.Enter()
        defer ee.Exit()
        F()
      enddef

      g:result = ''
      var obj = CWithEE.new()
      obj->With(() => {
        g:result ..= "called/"
      })
      assert_equal('entered/called/exited', g:result)
  END
  v9.CheckScriptSuccess(lines)
  unlet g:result

  lines =<< trim END
      vim9script

      class BaseWithEE
        def Enter()
          g:result ..= "entered-base/"
        enddef
        def Exit()
          g:result ..= "exited-base"
        enddef
      endclass

      class CWithEE extends BaseWithEE
        def Enter()
          g:result ..= "entered-child/"
        enddef
        def Exit()
          g:result ..= "exited-child"
        enddef
      endclass

      def With(ee: BaseWithEE, F: func)
        ee.Enter()
        defer ee.Exit()
        F()
      enddef

      g:result = ''
      var obj = CWithEE.new()
      obj->With(() => {
        g:result ..= "called/"
      })
      assert_equal('entered-child/called/exited-child', g:result)
  END
  v9.CheckScriptSuccess(lines)
  unlet g:result
enddef

" The following test used to crash Vim (Github issue #12676)
def Test_extends_method_crashes_vim()
  var lines =<< trim END
    vim9script

    class Observer
    endclass

    class Property
      this.value: any

      def Set(v: any)
        if v != this.value
          this.value = v
        endif
      enddef

      def Register(observer: Observer)
      enddef
    endclass

    class Bool extends Property
      this.value: bool
    endclass

    def Observe(obj: Property, who: Observer)
      obj.Register(who)
    enddef

    var p = Bool.new(false)
    var myObserver = Observer.new()

    Observe(p, myObserver)

    p.Set(true)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for calling a method in a class that is extended
def Test_call_method_in_extended_class()
  var lines =<< trim END
    vim9script

    var prop_init_called = false
    var prop_register_called = false

    class Property
      def Init()
        prop_init_called = true
      enddef

      def Register()
        prop_register_called = true
      enddef
    endclass

    class Bool extends Property
    endclass

    def Observe(obj: Property)
      obj.Register()
    enddef

    var p = Property.new()
    Observe(p)

    p.Init()
    assert_true(prop_init_called)
    assert_true(prop_register_called)
  END
  v9.CheckScriptSuccess(lines)
enddef

def Test_instanceof()
  var lines =<< trim END
    vim9script

    class Base1
    endclass

    class Base2 extends Base1
    endclass

    interface Intf1
    endinterface

    class Mix1 implements Intf1
    endclass

    class Base3 extends Mix1
    endclass

    var b1 = Base1.new()
    var b2 = Base2.new()
    var b3 = Base3.new()

    assert_true(instanceof(b1, Base1))
    assert_true(instanceof(b2, Base1))
    assert_false(instanceof(b1, Base2))
    assert_true(instanceof(b3, Mix1))
    assert_false(instanceof(b3, []))
    assert_true(instanceof(b3, [Base1, Base2, Intf1]))

    def Foo()
      var a1 = Base1.new()
      var a2 = Base2.new()
      var a3 = Base3.new()

      assert_true(instanceof(a1, Base1))
      assert_true(instanceof(a2, Base1))
      assert_false(instanceof(a1, Base2))
      assert_true(instanceof(a3, Mix1))
      assert_false(instanceof(a3, []))
      assert_true(instanceof(a3, [Base1, Base2, Intf1]))
    enddef
    Foo()
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for calling a method in the parent class that is extended partially.
" This used to fail with the 'E118: Too many arguments for function: Text' error
" message (Github issue #12524).
def Test_call_method_in_parent_class()
  var lines =<< trim END
    vim9script

    class Widget
      this._lnum: number = 1

      def SetY(lnum: number)
        this._lnum = lnum
      enddef

      def Text(): string
        return ''
      enddef
    endclass

    class Foo extends Widget
      def Text(): string
        return '<Foo>'
      enddef
    endclass

    def Stack(w1: Widget, w2: Widget): list<Widget>
      w1.SetY(1)
      w2.SetY(2)
      return [w1, w2]
    enddef

    var foo1 = Foo.new()
    var foo2 = Foo.new()
    var l = Stack(foo1, foo2)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for calling methods from three levels of classes
def Test_multi_level_method_call()
  var lines =<< trim END
    vim9script

    var A_func1: number = 0
    var A_func2: number = 0
    var A_func3: number = 0
    var B_func2: number = 0
    var B_func3: number = 0
    var C_func3: number = 0

    class A
      def Func1()
        A_func1 += 1
      enddef

      def Func2()
        A_func2 += 1
      enddef

      def Func3()
        A_func3 += 1
      enddef
    endclass

    class B extends A
      def Func2()
        B_func2 += 1
      enddef

      def Func3()
        B_func3 += 1
      enddef
    endclass

    class C extends B
      def Func3()
        C_func3 += 1
      enddef
    endclass

    def A_CallFuncs(a: A)
      a.Func1()
      a.Func2()
      a.Func3()
    enddef

    def B_CallFuncs(b: B)
      b.Func1()
      b.Func2()
      b.Func3()
    enddef

    def C_CallFuncs(c: C)
      c.Func1()
      c.Func2()
      c.Func3()
    enddef

    var cobj = C.new()
    A_CallFuncs(cobj)
    B_CallFuncs(cobj)
    C_CallFuncs(cobj)
    assert_equal(3, A_func1)
    assert_equal(0, A_func2)
    assert_equal(0, A_func3)
    assert_equal(3, B_func2)
    assert_equal(0, B_func3)
    assert_equal(3, C_func3)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for using members from three levels of classes
def Test_multi_level_member_access()
  var lines =<< trim END
    vim9script

    class A
      this.val1: number = 0
      this.val2: number = 0
      this.val3: number = 0
    endclass

    class B extends A
      this.val2: number = 0
      this.val3: number = 0
    endclass

    class C extends B
      this.val3: number = 0
    endclass

    def A_members(a: A)
      a.val1 += 1
      a.val2 += 1
      a.val3 += 1
    enddef

    def B_members(b: B)
      b.val1 += 1
      b.val2 += 1
      b.val3 += 1
    enddef

    def C_members(c: C)
      c.val1 += 1
      c.val2 += 1
      c.val3 += 1
    enddef

    var cobj = C.new()
    A_members(cobj)
    B_members(cobj)
    C_members(cobj)
    assert_equal(3, cobj.val1)
    assert_equal(3, cobj.val2)
    assert_equal(3, cobj.val3)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test expansion of <stack> with class methods.
def Test_stack_expansion_with_methods()
  var lines =<< trim END
    vim9script

    class C
        def M1()
            F0()
        enddef
    endclass

    def F0()
      assert_match('<SNR>\d\+_F\[1\]\.\.C\.M1\[1\]\.\.<SNR>\d\+_F0\[1\]$', expand('<stack>'))
    enddef

    def F()
        C.new().M1()
    enddef

    F()
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test the return type of the new() constructor
def Test_new_return_type()
  # new() uses the default return type and there is no return statement
  var lines =<< trim END
    vim9script

    class C
      this._bufnr: number

      def new(this._bufnr)
        if !bufexists(this._bufnr)
          this._bufnr = -1
        endif
      enddef
    endclass

    var c = C.new(12345)
    assert_equal('object<C>', typename(c))

    var v1: C
    v1 = C.new(12345)
    assert_equal('object<C>', typename(v1))

    def F()
      var v2: C
      v2 = C.new(12345)
      assert_equal('object<C>', typename(v2))
    enddef
    F()
  END
  v9.CheckScriptSuccess(lines)

  # new() uses the default return type and an empty 'return' statement
  lines =<< trim END
    vim9script

    class C
      this._bufnr: number

      def new(this._bufnr)
        if !bufexists(this._bufnr)
          this._bufnr = -1
          return
        endif
      enddef
    endclass

    var c = C.new(12345)
    assert_equal('object<C>', typename(c))

    var v1: C
    v1 = C.new(12345)
    assert_equal('object<C>', typename(v1))

    def F()
      var v2: C
      v2 = C.new(12345)
      assert_equal('object<C>', typename(v2))
    enddef
    F()
  END
  v9.CheckScriptSuccess(lines)

  # new() uses "any" return type and returns "this"
  lines =<< trim END
    vim9script

    class C
      this._bufnr: number

      def new(this._bufnr): any
        if !bufexists(this._bufnr)
          this._bufnr = -1
          return this
        endif
      enddef
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1365:')

  # new() uses 'Dict' return type and returns a Dict
  lines =<< trim END
    vim9script

    class C
      this._state: dict<any>

      def new(): dict<any>
        this._state = {}
        return this._state
      enddef
    endclass

    var c = C.new()
    assert_equal('object<C>', typename(c))
  END
  v9.CheckScriptFailure(lines, 'E1365:')
enddef

" Test for checking a member initialization type at run time.
def Test_runtime_type_check_for_member_init()
  var lines =<< trim END
    vim9script

    var retnum: bool = false

    def F(): any
        retnum = !retnum
        if retnum
            return 1
        else
            return "hello"
        endif
    enddef

    class C
        this._foo: bool = F()
    endclass

    var c1 = C.new()
    var c2 = C.new()
  END
  v9.CheckScriptFailure(lines, 'E1012:')
enddef

" Test for locking a variable referring to an object and reassigning to another
" object.
def Test_object_lockvar()
  var lines =<< trim END
    vim9script

    class C
      this.val: number
      def new(this.val)
      enddef
    endclass

    var some_dict: dict<C> = { a: C.new(1), b: C.new(2), c: C.new(3), }
    lockvar 2 some_dict

    var current: C
    current = some_dict['c']
    assert_equal(3, current.val)
    current = some_dict['b']
    assert_equal(2, current.val)

    def F()
      current = some_dict['c']
    enddef

    def G()
      current = some_dict['b']
    enddef

    F()
    assert_equal(3, current.val)
    G()
    assert_equal(2, current.val)
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for a private object method
def Test_private_object_method()
  # Try calling a private method using an object (at the script level)
  var lines =<< trim END
    vim9script

    class A
      def _Foo(): number
        return 1234
      enddef
    endclass
    var a = A.new()
    a._Foo()
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Try calling a private method using an object (from a def function)
  lines =<< trim END
    vim9script

    class A
      def _Foo(): number
        return 1234
      enddef
    endclass
    def T()
      var a = A.new()
      a._Foo()
    enddef
    T()
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Use a private method from another object method (in script context)
  lines =<< trim END
    vim9script

    class A
      def _Foo(): number
        return 1234
      enddef
      def Bar(): number
        return this._Foo()
      enddef
    endclass
    var a = A.new()
    assert_equal(1234, a.Bar())
  END
  v9.CheckScriptSuccess(lines)

  # Use a private method from another object method (def function context)
  lines =<< trim END
    vim9script

    class A
      def _Foo(): number
        return 1234
      enddef
      def Bar(): number
        return this._Foo()
      enddef
    endclass
    def T()
      var a = A.new()
      assert_equal(1234, a.Bar())
    enddef
    T()
  END
  v9.CheckScriptSuccess(lines)

  # Try calling a private method without the "this" prefix
  lines =<< trim END
    vim9script

    class A
      def _Foo(): number
        return 1234
      enddef
      def Bar(): number
        return _Foo()
      enddef
    endclass
    var a = A.new()
    a.Bar()
  END
  v9.CheckScriptFailure(lines, 'E117: Unknown function: _Foo')

  # Try calling a private method using the class name
  lines =<< trim END
    vim9script

    class A
      def _Foo(): number
        return 1234
      enddef
    endclass
    A._Foo()
  END
  v9.CheckScriptFailure(lines, 'E1325: Method not found on class "A": _Foo()')

  # Try to use "public" keyword when defining a private method
  lines =<< trim END
    vim9script

    class A
      public def _Foo()
      enddef
    endclass
    var a = A.new()
    a._Foo()
  END
  v9.CheckScriptFailure(lines, 'E1331: Public must be followed by "this" or "static"')

  # Define two private methods with the same name
  lines =<< trim END
    vim9script

    class A
      def _Foo()
      enddef
      def _Foo()
      enddef
    endclass
    var a = A.new()
  END
  v9.CheckScriptFailure(lines, 'E1355: Duplicate function: _Foo')

  # Define a private method and a object method with the same name
  lines =<< trim END
    vim9script

    class A
      def _Foo()
      enddef
      def Foo()
      enddef
    endclass
    var a = A.new()
  END
  v9.CheckScriptFailure(lines, 'E1355: Duplicate function: Foo')

  # Define an object method and a private method with the same name
  lines =<< trim END
    vim9script

    class A
      def Foo()
      enddef
      def _Foo()
      enddef
    endclass
    var a = A.new()
  END
  v9.CheckScriptFailure(lines, 'E1355: Duplicate function: _Foo')

  # Call a public method and a private method from a private method
  lines =<< trim END
    vim9script

    class A
      def Foo(): number
        return 100
      enddef
      def _Bar(): number
        return 200
      enddef
      def _Baz()
        assert_equal(100, this.Foo())
        assert_equal(200, this._Bar())
      enddef
      def T()
        this._Baz()
      enddef
    endclass
    var a = A.new()
    a.T()
  END
  v9.CheckScriptSuccess(lines)

  # Try calling a private method from another class
  lines =<< trim END
    vim9script

    class A
      def _Foo(): number
        return 100
      enddef
    endclass
    class B
      def Foo(): number
        var a = A.new()
        a._Foo()
      enddef
    endclass
    var b = B.new()
    b.Foo()
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Call a private object method from a child class object method
  lines =<< trim END
    vim9script
    class A
      def _Foo(): number
        return 1234
      enddef
    endclass
    class B extends A
      def Bar()
      enddef
    endclass
    class C extends B
      def Baz(): number
        return this._Foo()
      enddef
    endclass
    var c = C.new()
    assert_equal(1234, c.Baz())
  END
  v9.CheckScriptSuccess(lines)

  # Call a private object method from a child class object
  lines =<< trim END
    vim9script
    class A
      def _Foo(): number
        return 1234
      enddef
    endclass
    class B extends A
      def Bar()
      enddef
    endclass
    class C extends B
      def Baz(): number
      enddef
    endclass
    var c = C.new()
    assert_equal(1234, c._Foo())
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Using "_" prefix in a method name should fail outside of a class
  lines =<< trim END
    vim9script
    def _Foo(): number
      return 1234
    enddef
    var a = _Foo()
  END
  v9.CheckScriptFailure(lines, 'E1267: Function name must start with a capital: _Foo(): number')
enddef

" Test for an private class method
def Test_private_class_method()
  # Try calling a class private method (at the script level)
  var lines =<< trim END
    vim9script

    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    A._Foo()
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Try calling a class private method (from a def function)
  lines =<< trim END
    vim9script

    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    def T()
      A._Foo()
    enddef
    T()
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Try calling a class private method using an object (at the script level)
  lines =<< trim END
    vim9script

    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    var a = A.new()
    a._Foo()
  END
  v9.CheckScriptFailure(lines, 'E1325: Method not found on class "A": _Foo()')

  # Try calling a class private method using an object (from a def function)
  lines =<< trim END
    vim9script

    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    def T()
      var a = A.new()
      a._Foo()
    enddef
    T()
  END
  v9.CheckScriptFailure(lines, 'E1325: Method not found on class "A": _Foo()')

  # Use a class private method from an object method
  lines =<< trim END
    vim9script

    class A
      static def _Foo(): number
        return 1234
      enddef
      def Bar()
        assert_equal(1234, A._Foo())
      enddef
    endclass
    var a = A.new()
    a.Bar()
  END
  v9.CheckScriptSuccess(lines)

  # Use a class private method from another class private method
  lines =<< trim END
    vim9script

    class A
      static def _Foo1(): number
        return 1234
      enddef
      static def _Foo2()
        assert_equal(1234, A._Foo1())
      enddef
      def Bar()
        A._Foo2()
      enddef
    endclass
    var a = A.new()
    a.Bar()
  END
  v9.CheckScriptSuccess(lines)

  # Declare a class method and a class private method with the same name
  lines =<< trim END
    vim9script

    class A
      static def _Foo()
      enddef
      static def Foo()
      enddef
    endclass
    var a = A.new()
  END
  v9.CheckScriptFailure(lines, 'E1355: Duplicate function: Foo')

  # Try calling a class private method from another class
  lines =<< trim END
    vim9script

    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    class B
      def Foo(): number
        return A._Foo()
      enddef
    endclass
    var b = B.new()
    assert_equal(1234, b.Foo())
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Call a private class method from a child class object method
  lines =<< trim END
    vim9script
    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    class B extends A
      def Bar()
      enddef
    endclass
    class C extends B
      def Baz(): number
        return A._Foo()
      enddef
    endclass
    var c = C.new()
    assert_equal(1234, c.Baz())
  END
  v9.CheckScriptSuccess(lines)

  # Call a private class method from a child class private class method
  lines =<< trim END
    vim9script
    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    class B extends A
      def Bar()
      enddef
    endclass
    class C extends B
      static def Baz(): number
        return A._Foo()
      enddef
    endclass
    assert_equal(1234, C.Baz())
  END
  v9.CheckScriptSuccess(lines)

  # Call a private class method from a child class object
  lines =<< trim END
    vim9script
    class A
      static def _Foo(): number
        return 1234
      enddef
    endclass
    class B extends A
      def Bar()
      enddef
    endclass
    class C extends B
      def Baz(): number
      enddef
    endclass
    var c = C.new()
    assert_equal(1234, C._Foo())
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')
enddef

" Test for an interface private object_method
def Test_interface_private_object_method()
  # Implement an interface private method and use it from a public method
  var lines =<< trim END
    vim9script
    interface Intf
      def _Foo(): number
    endinterface
    class A implements Intf
      def _Foo(): number
        return 1234
      enddef
      def Bar(): number
        return this._Foo()
      enddef
    endclass
    var a = A.new()
    assert_equal(1234, a.Bar())
  END
  v9.CheckScriptSuccess(lines)

  # Call an interface private class method (script context)
  lines =<< trim END
    vim9script
    interface Intf
      def _Foo(): number
    endinterface
    class A implements Intf
      def _Foo(): number
        return 1234
      enddef
    endclass
    var a = A.new()
    assert_equal(1234, a._Foo())
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Call an interface private class method (def context)
  lines =<< trim END
    vim9script
    interface Intf
      def _Foo(): number
    endinterface
    class A implements Intf
      def _Foo(): number
        return 1234
      enddef
    endclass
    def T()
      var a = A.new()
      assert_equal(1234, a._Foo())
    enddef
    T()
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo()')

  # Implement an interface private object method as a private class method
  lines =<< trim END
    vim9script
    interface Intf
      def _Foo(): number
    endinterface
    class A implements Intf
      static def _Foo(): number
        return 1234
      enddef
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1349: Function "_Foo" of interface "Intf" not implemented')
enddef

" Test for an interface private class method
def Test_interface_private_class_method()
  # Implement an interface private class method and use it from a public method
  var lines =<< trim END
    vim9script
    interface Intf
      static def _Foo(): number
    endinterface
    class A implements Intf
      static def _Foo(): number
        return 1234
      enddef
      def Bar(): number
        return A._Foo()
      enddef
    endclass
    var a = A.new()
    assert_equal(1234, a.Bar())
  END
  v9.CheckScriptSuccess(lines)

  # Call an interface private class method (script context)
  lines =<< trim END
    vim9script
    interface Intf
      static def _Foo(): number
    endinterface
    class A implements Intf
      static def _Foo(): number
        return 1234
      enddef
    endclass
    assert_equal(1234, A._Foo())
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo())')

  # Call an interface private class method (def context)
  lines =<< trim END
    vim9script
    interface Intf
      static def _Foo(): number
    endinterface
    class A implements Intf
      static def _Foo(): number
        return 1234
      enddef
    endclass
    def T()
      assert_equal(1234, A._Foo())
    enddef
    T()
  END
  v9.CheckScriptFailure(lines, 'E1366: Cannot access private method: _Foo())')

  # Implement an interface private class method as a private object method
  lines =<< trim END
    vim9script
    interface Intf
      static def _Foo(): number
    endinterface
    class A implements Intf
      def _Foo(): number
        return 1234
      enddef
    endclass
  END
  v9.CheckScriptFailure(lines, 'E1349: Function "_Foo" of interface "Intf" not implemented')
enddef

" Test for using the return value of a class/object method as a function
" argument.
def Test_objmethod_funcarg()
  var lines =<< trim END
    vim9script

    class C
      def Foo(): string
        return 'foo'
      enddef
    endclass

    def Bar(a: number, s: string): string
      return s
    enddef

    def Baz(c: C)
      assert_equal('foo', Bar(10, c.Foo()))
    enddef

    var t = C.new()
    Baz(t)
  END
  v9.CheckScriptSuccess(lines)

  lines =<< trim END
    vim9script

    class C
      static def Foo(): string
        return 'foo'
      enddef
    endclass

    def Bar(a: number, s: string): string
      return s
    enddef

    def Baz()
      assert_equal('foo', Bar(10, C.Foo()))
    enddef

    Baz()
  END
  v9.CheckScriptSuccess(lines)
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
