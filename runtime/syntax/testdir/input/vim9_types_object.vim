vim9script
# Vim9 object type constructor
# VIM_TEST_SETUP hi link vimTypeObject Todo
# VIM_TEST_SETUP hi link vimTypeObjectBracket Title


interface I
endinterface

var a: object<I>
var b: object<any>
var c: object<object<I>>
var d: object<object<any>>

def Foo(
    arg1: object<I>,
    arg2: object<any>,
    arg3: object<object<I>>,
    arg4: object<object<any>>)
enddef

def Bar(): object<I>
enddef

def Baz(): object<object<I>>
enddef

