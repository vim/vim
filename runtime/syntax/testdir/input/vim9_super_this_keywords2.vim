vim9script

# VIM_TEST_SETUP hi link vim9Super Todo
# VIM_TEST_SETUP hi link vim9This Todo
# See: https://github.com/vim/vim/pull/16476#issuecomment-2638191110


class A
  var _value: any

  def new(value: any)
    this._BaseInit(value)
  enddef

  def _BaseInit(value: any)
    this._value = value
  enddef

  def Value(): any
    return this._value
  enddef
endclass

class B extends A
  def new(value: number)
    super._BaseInit(value)
  enddef

  def Value(): number
    echo this
    return super._value
  enddef
endclass

class C extends A
  #### E117
  #   def new(value: string)
  #       super(value)
  #   enddef

  #### E1034
  #   def new(super._value)
  #   enddef

  #### E1034
  #   def new(value: string)
  #       super._value = value
  #   enddef

  #### E1356
  #   def Super(): A
  #       return super
  #   enddef

  def This(): C
    return this
  enddef
endclass

echo 1 == A.new(1).Value()
echo 2 == B.new(2).Value()
defcompile C

