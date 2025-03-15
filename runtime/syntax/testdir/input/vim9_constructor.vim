vim9script

# Vim9 constructor


class A
  static var _instance: A
  var str: string
  def _new(str: string)
    this.str = str
  enddef
  static def GetInstance(str: string): A
    if _instance == null
      _instance = A._new(str)
    endif
    return _instance
  enddef
endclass

