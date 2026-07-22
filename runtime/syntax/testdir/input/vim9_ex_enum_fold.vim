vim9script
# Vim9 :enum command
# VIM_TEST_SETUP let g:vimsyn_folding = 'efi'
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax
# VIM_TEST_SETUP hi link vim9EnumValue Todo


interface Interface1
endinterface
interface Interface2
endinterface

enum Enum1
endenum

export enum Enum2
endenum

enum Enum3
  Value1,
  Value2,
  Value3
endenum

enum Enum4
  Value1,
  Value2,
  Value3
  def Method1()
  enddef
endenum

enum Enum5 implements Interface1, Interface2
  Value1,
  Value2,
  Value3
  def Method1()
    def Nested()
    enddef
  enddef
endenum

enum Enum6
      #\ comment
      \ implements Interface1, Interface2
  Value1,
  Value2,
  Value3
endenum

