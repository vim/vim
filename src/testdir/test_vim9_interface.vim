" Tests for Vim9 interface

import './util/vim9.vim' as v9

" Tests for basic interface declaration and errors
def Test_interface_basics()
  var lines =<< trim END
    vim9script
    interface Something
      var ro_var: list<number>
      def GetCount(): number
    endinterface
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    interface SomethingWrong
      static var count = 7
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1342: Interface can only be defined in Vim9 script', 1)

  lines =<< trim END
    vim9script

    interface Some
      var value: number
      def Method(value: number)
    endinterface
  END
  # The argument name and the object member name are the same, but this is not a
  # problem because object members are always accessed with the "this." prefix.
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    interface somethingWrong
      static var count = 7
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1343: Interface name must start with an uppercase letter: somethingWrong', 2)

  lines =<< trim END
    vim9script
    interface SomethingWrong
      var value: string
      var count = 7
      def GetCount(): number
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1344: Cannot initialize a variable in an interface', 4)

  lines =<< trim END
    vim9script
    interface SomethingWrong
      var value: string
      var count: number
      def GetCount(): number
        return 5
      enddef
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1345: Not a valid command in an interface: return 5', 6)

  # Test for "interface" cannot be abbreviated
  lines =<< trim END
    vim9script
    inte Something
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1065: Command cannot be shortened: inte Something', 2)

  # Test for "endinterface" cannot be abbreviated
  lines =<< trim END
    vim9script
    interface Something
    endin
  END
  v9.CheckSourceFailure(lines, 'E1065: Command cannot be shortened: endin', 3)

  # Additional commands after "interface name"
  lines =<< trim END
    vim9script
    interface Something | var x = 10 | var y = 20
    endinterface
  END
  v9.CheckSourceFailure(lines, "E488: Trailing characters: | var x = 10", 2)

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

def Test_class_interface_wrong_end()
  var lines =<< trim END
    vim9script
    abstract class SomeName
      var member = 'text'
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E476: Invalid command: endinterface, expected endclass', 4)

  lines =<< trim END
    vim9script
    export interface AnotherName
      var member: string
    endclass
  END
  v9.CheckSourceFailure(lines, 'E476: Invalid command: endclass, expected endinterface', 4)
enddef

" Test for using string() with an interface
def Test_interface_to_string()
  var lines =<< trim END
    vim9script
    interface Intf
      def Method(nr: number)
    endinterface
    assert_equal("interface Intf", string(Intf))
  END
  v9.CheckSourceSuccess(lines)
enddef

def Test_class_implements_interface()
  var lines =<< trim END
    vim9script

    interface Some
      var count: number
      def Method(nr: number)
    endinterface

    class SomeImpl implements Some
      var count: number
      def Method(nr: number)
        echo nr
      enddef
    endclass

    interface Another
      var member: string
    endinterface

    class AnotherImpl implements Some, Another
      var member = 'abc'
      var count = 20
      def Method(nr: number)
        echo nr
      enddef
    endclass
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script

    interface Some
      var count: number
    endinterface

    class SomeImpl implements Some implements Some
      var count: number
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1350: Duplicate "implements"', 7)

  lines =<< trim END
    vim9script

    interface Some
      var count: number
    endinterface

    class SomeImpl implements Some, Some
      var count: number
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1351: Duplicate interface after "implements": Some', 7)

  lines =<< trim END
    vim9script

    interface Some
      var counter: number
      def Method(nr: number)
    endinterface

    class SomeImpl implements Some
      var count: number
      def Method(nr: number)
        echo nr
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1348: Variable "counter" of interface "Some" is not implemented', 13)

  lines =<< trim END
    vim9script

    interface Some
      var count: number
      def Methods(nr: number)
    endinterface

    class SomeImpl implements Some
      var count: number
      def Method(nr: number)
        echo nr
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1349: Method "Methods" of interface "Some" is not implemented', 13)

  # Check different order of members in class and interface works.
  lines =<< trim END
    vim9script

      interface Result
        var label: string
        var errpos: number
      endinterface

      # order of members is opposite of interface
      class Failure implements Result
        public var lnum: number = 5
        var errpos: number = 42
        var label: string = 'label'
      endclass

    def Test()
      var result: Result = Failure.new()

        assert_equal('label', result.label)
        assert_equal(42, result.errpos)
      enddef

    Test()
  END
  v9.CheckSourceSuccess(lines)

  # Interface name after "extends" doesn't end in a space or NUL character
  lines =<< trim END
    vim9script
    interface A
    endinterface
    class B extends A"
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: A"', 4)

  # Trailing characters after a class name
  lines =<< trim END
    vim9script
    class A bbb
    endclass
  END
  v9.CheckSourceFailure(lines, 'E488: Trailing characters: bbb', 2)

  # using "implements" with a non-existing class
  lines =<< trim END
    vim9script
    class A implements B
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1346: Interface name not found: B', 3)

  # using "implements" with a regular class
  lines =<< trim END
    vim9script
    class A
    endclass
    class B implements A
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1347: Not a valid interface: A', 5)

  # using "implements" with a variable
  lines =<< trim END
    vim9script
    var T: number = 10
    class A implements T
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1347: Not a valid interface: T', 4)

  # implements should be followed by a white space
  lines =<< trim END
    vim9script
    interface A
    endinterface
    class B implements A;
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: A;', 4)

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
  v9.CheckSourceFailure(lines, 'E1383: Method "IsEven": type mismatch, expected func(number): bool but got func(number): string', 9)

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
  v9.CheckSourceFailure(lines, 'E1383: Method "IsEven": type mismatch, expected func(number): bool but got func(bool): bool', 9)

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
  v9.CheckSourceFailure(lines, 'E1383: Method "IsEven": type mismatch, expected func(number): bool but got func(number, ...list<number>): bool', 9)

  # access superclass interface members from subclass, mix variable order
  lines =<< trim END
    vim9script

    interface I1
      var mvar1: number
      var mvar2: number
    endinterface

    # NOTE: the order is swapped
    class A implements I1
      var mvar2: number
      var mvar1: number
      public static var svar2: number
      public static var svar1: number
      def new()
        svar1 = 11
        svar2 = 12
        this.mvar1 = 111
        this.mvar2 = 112
      enddef
    endclass

    class B extends A
      def new()
        this.mvar1 = 121
        this.mvar2 = 122
      enddef
    endclass

    class C extends B
      def new()
        this.mvar1 = 131
        this.mvar2 = 132
      enddef
    endclass

    def F2(i: I1): list<number>
      return [ i.mvar1, i.mvar2 ]
    enddef

    var oa = A.new()
    var ob = B.new()
    var oc = C.new()

    assert_equal([111, 112], F2(oa))
    assert_equal([121, 122], F2(ob))
    assert_equal([131, 132], F2(oc))
  END
  v9.CheckSourceSuccess(lines)

  # Access superclass interface members from subclass, mix variable order.
  # Two interfaces, one on A, one on B; each has both kinds of variables
  lines =<< trim END
    vim9script

    interface I1
      var mvar1: number
      var mvar2: number
    endinterface

    interface I2
      var mvar3: number
      var mvar4: number
    endinterface

    class A implements I1
      public static var svar1: number
      public static var svar2: number
      var mvar1: number
      var mvar2: number
      def new()
        svar1 = 11
        svar2 = 12
        this.mvar1 = 111
        this.mvar2 = 112
      enddef
    endclass

    class B extends A implements I2
      static var svar3: number
      static var svar4: number
      var mvar3: number
      var mvar4: number
      def new()
        svar3 = 23
        svar4 = 24
        this.mvar1 = 121
        this.mvar2 = 122
        this.mvar3 = 123
        this.mvar4 = 124
      enddef
    endclass

    class C extends B
      public static var svar5: number
      def new()
        svar5 = 1001
        this.mvar1 = 131
        this.mvar2 = 132
        this.mvar3 = 133
        this.mvar4 = 134
      enddef
    endclass

    def F2(i: I1): list<number>
      return [ i.mvar1, i.mvar2 ]
    enddef

    def F4(i: I2): list<number>
      return [ i.mvar3, i.mvar4 ]
    enddef

    var oa = A.new()
    var ob = B.new()
    var oc = C.new()

    assert_equal([[111, 112]], [F2(oa)])
    assert_equal([[121, 122], [123, 124]], [F2(ob), F4(ob)])
    assert_equal([[131, 132], [133, 134]], [F2(oc), F4(oc)])
  END
  v9.CheckSourceSuccess(lines)

  # Using two interface names without a space after the ","
  lines =<< trim END
    vim9script
    interface A
    endinterface
    interface B
    endinterface
    class C implements A,B
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: A,B', 6)

  # No interface name after a comma
  lines =<< trim END
    vim9script
    interface A
    endinterface
    class B implements A,
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1389: Missing name after implements', 4)

  # No interface name after implements
  lines =<< trim END
    vim9script
    class A implements
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1389: Missing name after implements', 2)
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
  v9.CheckSourceSuccess(lines)

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
  v9.CheckSourceSuccess(lines)

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
  v9.CheckSourceSuccess(lines)

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
  v9.CheckSourceSuccess(lines)

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
  v9.CheckSourceSuccess(lines)
enddef

" Test for implementing an imported interface
def Test_implement_imported_interface()
  var lines =<< trim END
    vim9script
    export interface Imp_Intf1
      def Fn1(): number
    endinterface
    export interface Imp_Intf2
      def Fn2(): number
    endinterface
  END
  writefile(lines, 'Ximportinterface.vim', 'D')

  lines =<< trim END
    vim9script
    import './Ximportinterface.vim' as Xintf

    class A implements Xintf.Imp_Intf1, Xintf.Imp_Intf2
      def Fn1(): number
        return 10
      enddef
      def Fn2(): number
        return 20
      enddef
    endclass
    var a = A.new()
    assert_equal(10, a.Fn1())
    assert_equal(20, a.Fn2())
  END
  v9.CheckScriptSuccess(lines)
enddef

" Test for changing the member access of an interface in a implementation class
def Test_change_interface_member_access()
  var lines =<< trim END
    vim9script
    interface A
      var val: number
    endinterface
    class B implements A
      public var val = 10
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1367: Access level of variable "val" of interface "A" is different', 7)

  lines =<< trim END
    vim9script
    interface A
      var val: number
    endinterface
    class B implements A
      public var val = 10
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1367: Access level of variable "val" of interface "A" is different', 7)
enddef

" Test for using a interface method using a child object
def Test_interface_method_from_child()
  var lines =<< trim END
    vim9script

    interface A
      def Foo(): string
    endinterface

    class B implements A
      def Foo(): string
        return 'foo'
      enddef
    endclass

    class C extends B
      def Bar(): string
        return 'bar'
      enddef
    endclass

    def T1(a: A)
      assert_equal('foo', a.Foo())
    enddef

    def T2(b: B)
      assert_equal('foo', b.Foo())
    enddef

    var c = C.new()
    T1(c)
    T2(c)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for using an interface method using a child object when it is overridden
" by the child class.
def Test_interface_overridden_method_from_child()
  var lines =<< trim END
    vim9script

    interface A
      def Foo(): string
    endinterface

    class B implements A
      def Foo(): string
        return 'b-foo'
      enddef
    endclass

    class C extends B
      def Bar(): string
        return 'bar'
      enddef
      def Foo(): string
        return 'c-foo'
      enddef
    endclass

    def T1(a: A)
      assert_equal('c-foo', a.Foo())
    enddef

    def T2(b: B)
      assert_equal('c-foo', b.Foo())
    enddef

    var c = C.new()
    T1(c)
    T2(c)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for interface inheritance
def Test_interface_inheritance()
  var lines =<< trim END
    vim9script

    interface A
      def A_Fn(): string
    endinterface

    interface B
      def B_Fn(): string
    endinterface

    interface C
      def C_Fn(): string
    endinterface

    class C1 implements A
      def A_Fn(): string
        return 'c1-a'
      enddef
    endclass

    class C2 extends C1 implements B
      def B_Fn(): string
        return 'c2-b'
      enddef
      def A_Fn(): string
        return 'c2-a'
      enddef
    endclass

    class C3 extends C2 implements C
      def C_Fn(): string
        return 'c3-c'
      enddef
      def A_Fn(): string
        return 'c3-a'
      enddef
      def B_Fn(): string
        return 'c3-b'
      enddef
    endclass

    def T1(a: A, s: string)
      assert_equal(s, a.A_Fn())
    enddef

    def T2(b: B, s: string)
      assert_equal(s, b.B_Fn())
    enddef

    def T3(c: C, s: string)
      assert_equal(s, c.C_Fn())
    enddef

    def T4(c1: C1)
      T1(c1, 'c3-a')
    enddef

    def T5(c2: C2)
      T1(c2, 'c3-a')
      T2(c2, 'c3-b')
    enddef

    def T6(c3: C3)
      T1(c3, 'c3-a')
      T2(c3, 'c3-b')
      T3(c3, 'c3-c')
    enddef

    var o3 = C3.new()
    T4(o3)
    T5(o3)
    T6(o3)
  END
  v9.CheckSourceSuccess(lines)

  # Both the parent and child classes implement the same interface
  lines =<< trim END
    vim9script

    interface I
      def Foo(): string
    endinterface

    class A implements I
      def Foo(): string
        return 'A-foo'
      enddef
    endclass

    class B implements I
      def Foo(): string
        return 'B-foo'
      enddef
    endclass

    def Bar(i1: I): string
      return i1.Foo()
    enddef

    var b = B.new()
    assert_equal('B-foo', Bar(b))
  END
  v9.CheckSourceSuccess(lines)
enddef

" A interface cannot have a static variable or a static method or a protected
" variable or a protected method or a public variable
def Test_interface_with_unsupported_members()
  var lines =<< trim END
    vim9script
    interface A
      static var num: number
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1378: Static member not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      static var _num: number
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1378: Static member not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      public static var num: number
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1387: public variable not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      public static var num: number
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1387: public variable not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      static var _num: number
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1378: Static member not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      static def Foo(d: dict<any>): list<string>
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1378: Static member not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      static def _Foo(d: dict<any>): list<string>
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1378: Static member not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      var _Foo: list<string>
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1379: Protected variable not supported in an interface', 3)

  lines =<< trim END
    vim9script
    interface A
      def _Foo(d: dict<any>): list<string>
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1380: Protected method not supported in an interface', 3)
enddef

" Test for extending an interface
def Test_extend_interface()
  var lines =<< trim END
    vim9script
    interface A
      var var1: list<string>
      def Foo()
    endinterface
    interface B extends A
      var var2: dict<string>
      def Bar()
    endinterface
    class C implements A, B
      var var1 = [1, 2]
      def Foo()
      enddef
      var var2 = {a: '1'}
      def Bar()
      enddef
    endclass
  END
  v9.CheckSourceSuccess(lines)

  # extending empty interface
  lines =<< trim END
    vim9script
    interface A
    endinterface
    interface B extends A
    endinterface
    class C implements B
    endclass
  END
  v9.CheckSourceSuccess(lines)

  lines =<< trim END
    vim9script
    interface A
      def Foo()
    endinterface
    interface B extends A
      var var2: dict<string>
    endinterface
    class C implements A, B
      var var2 = {a: '1'}
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1349: Method "Foo" of interface "A" is not implemented', 10)

  lines =<< trim END
    vim9script
    interface A
      def Foo()
    endinterface
    interface B extends A
      var var2: dict<string>
    endinterface
    class C implements A, B
      def Foo()
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1348: Variable "var2" of interface "B" is not implemented', 11)

  # interface cannot extend a class
  lines =<< trim END
    vim9script
    class A
    endclass
    interface B extends A
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend A', 5)

  # class cannot extend an interface
  lines =<< trim END
    vim9script
    interface A
    endinterface
    class B extends A
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1354: Cannot extend A', 5)

  # interface cannot implement another interface
  lines =<< trim END
    vim9script
    interface A
    endinterface
    interface B implements A
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1381: Interface cannot use "implements"', 4)

  # interface cannot extend multiple interfaces
  lines =<< trim END
    vim9script
    interface A
    endinterface
    interface B
    endinterface
    interface C extends A, B
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1315: White space required after name: A, B', 6)

  # Variable type in an extended interface is of different type
  lines =<< trim END
    vim9script
    interface A
      var val1: number
    endinterface
    interface B extends A
      var val2: string
    endinterface
    interface C extends B
      var val1: string
      var val2: number
    endinterface
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "val1": type mismatch, expected number but got string', 11)
enddef

" Test for a child class implementing an interface when some of the methods are
" defined in the parent class.
def Test_child_class_implements_interface()
  var lines =<< trim END
    vim9script

    interface Intf
      def F1(): list<list<number>>
      def F2(): list<list<number>>
      def F3(): list<list<number>>
      var var1: list<dict<number>>
      var var2: list<dict<number>>
      var var3: list<dict<number>>
    endinterface

    class A
      def A1()
      enddef
      def F3(): list<list<number>>
        return [[3]]
      enddef
      var v1: list<list<number>> = [[0]]
      var var3 = [{c: 30}]
    endclass

    class B extends A
      def B1()
      enddef
      def F2(): list<list<number>>
        return [[2]]
      enddef
      var v2: list<list<number>> = [[0]]
      var var2 = [{b: 20}]
    endclass

    class C extends B implements Intf
      def C1()
      enddef
      def F1(): list<list<number>>
        return [[1]]
      enddef
      var v3: list<list<number>> = [[0]]
      var var1 = [{a: 10}]
    endclass

    def T(if: Intf)
      assert_equal([[1]], if.F1())
      assert_equal([[2]], if.F2())
      assert_equal([[3]], if.F3())
      assert_equal([{a: 10}], if.var1)
      assert_equal([{b: 20}], if.var2)
      assert_equal([{c: 30}], if.var3)
    enddef

    var c = C.new()
    T(c)
    assert_equal([[1]], c.F1())
    assert_equal([[2]], c.F2())
    assert_equal([[3]], c.F3())
    assert_equal([{a: 10}], c.var1)
    assert_equal([{b: 20}], c.var2)
    assert_equal([{c: 30}], c.var3)
  END
  v9.CheckSourceSuccess(lines)

  # One of the interface methods is not found
  lines =<< trim END
    vim9script

    interface Intf
      def F1()
      def F2()
      def F3()
    endinterface

    class A
      def A1()
      enddef
    endclass

    class B extends A
      def B1()
      enddef
      def F2()
      enddef
    endclass

    class C extends B implements Intf
      def C1()
      enddef
      def F1()
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1349: Method "F3" of interface "Intf" is not implemented', 26)

  # One of the interface methods is of different type
  lines =<< trim END
    vim9script

    interface Intf
      def F1()
      def F2()
      def F3()
    endinterface

    class A
      def F3(): number
        return 0
      enddef
      def A1()
      enddef
    endclass

    class B extends A
      def B1()
      enddef
      def F2()
      enddef
    endclass

    class C extends B implements Intf
      def C1()
      enddef
      def F1()
      enddef
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1383: Method "F3": type mismatch, expected func() but got func(): number', 29)

  # One of the interface variables is not present
  lines =<< trim END
    vim9script

    interface Intf
      var var1: list<dict<number>>
      var var2: list<dict<number>>
      var var3: list<dict<number>>
    endinterface

    class A
      var v1: list<list<number>> = [[0]]
    endclass

    class B extends A
      var v2: list<list<number>> = [[0]]
      var var2 = [{b: 20}]
    endclass

    class C extends B implements Intf
      var v3: list<list<number>> = [[0]]
      var var1 = [{a: 10}]
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1348: Variable "var3" of interface "Intf" is not implemented', 21)

  # One of the interface variables is of different type
  lines =<< trim END
    vim9script

    interface Intf
      var var1: list<dict<number>>
      var var2: list<dict<number>>
      var var3: list<dict<number>>
    endinterface

    class A
      var v1: list<list<number>> = [[0]]
      var var3: list<dict<string>>
    endclass

    class B extends A
      var v2: list<list<number>> = [[0]]
      var var2 = [{b: 20}]
    endclass

    class C extends B implements Intf
      var v3: list<list<number>> = [[0]]
      var var1 = [{a: 10}]
    endclass
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "var3": type mismatch, expected list<dict<number>> but got list<dict<string>>', 22)
enddef

" Test for extending an interface with duplicate variables and methods
def Test_interface_extends_with_dup_members()
  var lines =<< trim END
    vim9script
    interface A
      var n1: number
      def Foo1(): number
    endinterface
    interface B extends A
      var n2: number
      var n1: number
      def Foo2(): number
      def Foo1(): number
    endinterface
    class C implements B
      var n1 = 10
      var n2 = 20
      def Foo1(): number
        return 30
      enddef
      def Foo2(): number
        return 40
      enddef
    endclass
    def T1(a: A)
      assert_equal(10, a.n1)
      assert_equal(30, a.Foo1())
    enddef
    def T2(b: B)
      assert_equal(10, b.n1)
      assert_equal(20, b.n2)
      assert_equal(30, b.Foo1())
      assert_equal(40, b.Foo2())
    enddef
    var c = C.new()
    T1(c)
    T2(c)
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for implementing an interface with different ordering for the interface
" member variables.
def Test_implement_interface_with_different_variable_order()
  var lines =<< trim END
    vim9script

    interface IX
      var F: func(): string
    endinterface

    class X implements IX
      var x: number
      var F: func(): string = () => 'ok'
    endclass

    def Foo(ix: IX): string
      return ix.F()
    enddef

    var x0 = X.new(0)
    assert_equal('ok', Foo(x0))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for inheriting interfaces from an imported super class
def Test_interface_inheritance_with_imported_super()
  var lines =<< trim END
    vim9script

    export interface I
      def F(): string
    endinterface

    export class A implements I
      def F(): string
        return 'A'
      enddef
    endclass
  END
  writefile(lines, 'Xinheritintfimportclass.vim', 'D')

  lines =<< trim END
    vim9script

    import './Xinheritintfimportclass.vim' as i_imp

    # class C extends i_imp.A
    class C extends i_imp.A implements i_imp.I
      def F(): string
        return 'C'
      enddef
    endclass

    def TestI(i: i_imp.I): string
        return i.F()
    enddef

    assert_equal('C', TestI(C.new()))
  END
  v9.CheckSourceSuccess(lines)
enddef

" Test for defining an interface in a function
def Test_interface_defined_in_function()
  var lines =<< trim END
    vim9script
    def Fn()
      var x = 1
      interface Foo
      endinterface
    enddef
    defcompile
  END
  v9.CheckScriptFailure(lines, 'E1436: Interface can only be used in a script', 2)
enddef

" Test for using "any" type for a variable in a sub-class while it has a
" concrete type in the interface
def Test_implements_using_var_type_any()
  var lines =<< trim END
    vim9script
    interface A
      var val: list<dict<string>>
    endinterface
    class B implements A
      var val = [{a: '1'}, {b: '2'}]
    endclass
    var b = B.new()
    assert_equal([{a: '1'}, {b: '2'}], b.val)
  END
  v9.CheckSourceSuccess(lines)

  # initialize instance variable using a different type
  lines =<< trim END
    vim9script
    interface A
      var val: list<dict<string>>
    endinterface
    class B implements A
      var val = {a: 1, b: 2}
    endclass
    var b = B.new()
  END
  v9.CheckSourceFailure(lines, 'E1382: Variable "val": type mismatch, expected list<dict<string>> but got dict<number>', 1)
enddef

" Test interface garbage collection
func Test_interface_garbagecollect()
  let lines =<< trim END
    vim9script

    interface I
      var ro_obj_var: number

      def ObjFoo(): number
    endinterface

    class A implements I
      static var ro_class_var: number = 10
      public static var rw_class_var: number = 20
      static var _priv_class_var: number = 30
      var ro_obj_var: number = 40
      var _priv_obj_var: number = 60

      static def _ClassBar(): number
        return _priv_class_var
      enddef

      static def ClassFoo(): number
        return ro_class_var + rw_class_var + A._ClassBar()
      enddef

      def _ObjBar(): number
        return this._priv_obj_var
      enddef

      def ObjFoo(): number
        return this.ro_obj_var + this._ObjBar()
      enddef
    endclass

    assert_equal(60, A.ClassFoo())
    var o = A.new()
    assert_equal(100, o.ObjFoo())
    test_garbagecollect_now()
    assert_equal(60, A.ClassFoo())
    assert_equal(100, o.ObjFoo())
  END
  call v9.CheckSourceSuccess(lines)
endfunc

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
