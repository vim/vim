vim9script
# Vim :enum command
# VIM_TEST_SETUP hi link vim9EnumValue Todo


interface Interface1
  def Def1()
endinterface
interface Interface2
endinterface

# enum-implements clause with interspersed comments

enum Enum1 implements Interface1, Interface2
  Value1
  def Def1()
  enddef
endenum

enum Enum2
      \ implements Interface1, Interface2
  Value1
  def Def1()
  enddef
endenum

enum Enum3 # comment
      \ implements Interface1, Interface2
  Value1
endenum

enum Enum4
      # comment
      \ implements Interface1, Interface2
  Value1
endenum

enum Enum5
      \ implements Interface1, Interface2 # comment
  Value1
  def Def1()
  enddef
endenum

enum Enum6
      #\ comment
      \ implements Interface1, Interface2
  Value1
  def Def1()
  enddef
endenum

# [enum Enum7.Value1 {name: 'Value1', ordinal: 0, val1: 0, val2: 0}, enum Enum7.Value2 {name: 'Value2', ordinal: 1, val1: 1, val2: 0}, enum Enum7.Value3 {name: 'Value3', ordinal: 2, val1: 2, val2: 0}]
# comment
enum Enum7
      \
      #\ comment
      \
      #\ comment
      \ implements Interface1, Interface2 # comment
    # comment
    # comment
    Value1,        # comment
    # comment
    # comment
    Value2(1 + 0), # comment
    # comment
    # comment
    Value3(   # comment
      # comment
      1       # comment
        # comment
        +     # comment
      # comment
      1       # comment
    )         # comment
    # comment
    var val1: number # comment
    # comment
    def Def1()
    enddef
    # comment
    static def Def2() # comment
      # comment
    enddef # comment
    # comment
    public var val2: number # comment
endenum

# [enum Enum8.implements {name: 'implements', ordinal: 0}]
enum Enum8 implements Interface1, Interface2
  implements
  def Def1()
  enddef
endenum

# [enum Enum9.Value1 {name: 'Value1', ordinal: 0, implements: 0}]
enum Enum9 implements Interface1, Interface2
  Value1
  var implements: number
  def Def1()
  enddef
endenum

# [enum Enum10.implements {name: 'implements', ordinal: 0}]
enum Enum10 implements Interface1, Interface2
  implements
  def Def1()
  enddef
endenum

