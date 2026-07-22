vim9script
# Vim9 types
# VIM_TEST_SETUP hi link vimTypeAny Todo
# VIM_TEST_SETUP hi link vim9VariableTypeAny Todo


# builtin types (distinct any highlighting)

var foo: bool
var bar: any

def Foo(arg: bool): bool
enddef

def Bar(arg: any): any
enddef

