vim9script
# VIM_TEST_SETUP highlight link vim9UserGenericType Todo
# See: https://github.com/vim/vim/pull/21512#issuecomment-6157916214 (Aliaksei Budavei)

class Object
endclass

class Box<T>
    # E1010
    # final _value: object<T>
    final _value: T

    # E1010
    # def new(value: object<T>)
    def new(value: T)
        this._value = value
    enddef

    # E1010
    # def Value(): object<T>
    def Value(): T
        return this._value
    enddef

    def string(): string
        return id(this._value)
    enddef
endclass


echo Box<Object>.new(Object.new()).Value()

const o: Object = Object.new()
var test1: Box<Object> = Box<Object>.new(o)
var test2: object<Box<Object>> = Box<Object>.new(o)
var test3: object<Box<Object>> = Box<object<Object>>.new(o)
var test4: Box<Box<Object>> = Box<Box<Object>>.new(test1)
var test5: Box<object<Box<Object>>> = Box<Box<Object>>.new(test1)
var test6: Box<Box<Object>> = Box<object<Box<Object>>>.new(test1)
var test7: Box<object<Box<Object>>> = Box<object<Box<Object>>>.new(test1)
var test8: Box<object<Box<object<Object>>>> = Box<object<Box<Object>>>.new(test1)
var test9: Box<object<Box<Object>>> = Box<object<Box<object<Object>>>>.new(test1)
var testa: Box<object<Box<object<Object>>>> = Box<object<Box<object<Object>>>>.new(test1)

echo 2 == [test1, test2, test3, test4, test5, test6, test7, test8, test9, testa]
    ->reduce((d: dict<any>, v: any) =>
        extend({[string(v)]: 1}, d),
        {})
    ->keys()
    ->len()

