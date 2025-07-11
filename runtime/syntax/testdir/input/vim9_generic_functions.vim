vim9script
# Vim9 generic functions


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

