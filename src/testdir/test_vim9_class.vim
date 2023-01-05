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


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
