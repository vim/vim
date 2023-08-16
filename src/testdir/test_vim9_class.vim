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
  v9.CheckScriptFailure(lines, 'E1012: Type mismatch; expected object<Background> but got object<Unknown>')

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

  lines =<< trim END
      vim9script
      class TextPosition
        this.lnum = v:none
        this.col = 1
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1330:')
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

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
