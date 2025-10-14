vim9script
# Vim9 generic functions
# VIM_TEST_SETUP let g:vimsyn_folding = "cf"
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax
# VIM_TEST_SETUP hi link vim9DefTypeParam Todo


# :help generic-functions

def MyFunc<T, A, B>(param1: T): T
    var f: A
    var x = param1
    return x
enddef

MyFunc<number, string, list<number>>()


def Flatten<T>(x: list<list<T>>): list<T>
    var result: list<T> = []
    for inner in x
	result += inner
    endfor
    return result
enddef

echo Flatten<number>([[1, 2], [3]])


class A
    def Foo<X, Y>()
    enddef
endclass
var a = A.new()
a.Foo<number, string>()


def MakeEcho<T>(): func(T): T
    return (x: T): T => x
enddef

var EchoNumber = MakeEcho<number>()
echo EchoNumber(123)

var EchoString = MakeEcho<string>()
echo EchoString('abc')

# FIXME: add specific command handling
# defcompile MyFunc<number, list<number>, dict<string>>

# disassemble MyFunc<string, dict<string>>
# disassemble MyFunc<number, list<blob>>


# funcrefs

var Foo = Bar<number>
Execute(Bar<number>)

var Foo = bar.Baz<string>
Execute(bar.Baz<string>)

class Foo
  def _MethodA<T>(arg: T)
    echo arg
  enddef
  def MethodB()
    var F = this._MethodA<number>
    F("text")
  enddef
endclass

class Bar extends Foo
  def MethodC()
    var F = super._MethodA<number>
    F("text")
  enddef
endclass


# Issue: https://github.com/vim/vim/pull/17722#issuecomment-3075531052

export def Id<U>(): func(U): U
    return (X_: U) => X_
enddef

export def Const<U, V>(): func(U): func(V): U
    return (X_: U) => (_: V) => X_
enddef

export def Flip<U, V, W>(): func(func(U): func(V): W): func(V): func(U): W
    return (F_: func(U): func(V): W) => (Y_: V) => (X_: U) => F_(X_)(Y_)
enddef

echo Const<number, any>()(2)(null)
    == Flip<number, any, number>()(Const<number, any>())(null)(2)

