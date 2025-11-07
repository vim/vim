vim9script
# Vim9 object type constructor
# VIM_TEST_SETUP hi link vimTypeObject Todo
# VIM_TEST_SETUP hi link vim9VariableTypeObject Todo


# Issue #18677 (No recognition of object<any> types - Aliaksei Budavei)
￼
interface I
  def string(): string
endinterface

class C implements I
  def string(): string
    return "C"
  enddef
endclass

enum E implements I
  INSTANCE

  def string(): string
    return "E"
  enddef
endenum

var c: object<C> = C.new()
var e: object<E> = E.INSTANCE
var os: tuple<object<any>, object<I>> = (c, e)
echo (c, e) == os

