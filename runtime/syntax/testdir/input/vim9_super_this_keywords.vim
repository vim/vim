vim9script

# Vim9 this and super keywords
# VIM_TEST_SETUP hi link vim9This Todo
# VIM_TEST_SETUP hi link vim9Super Todo

def Echo(...args: list<any>)
  echo args
enddef

class Foo
  var x: number = 42
  var y: number = this.x + 41
  var z: number = this.x + this.y

  def new()
    echo this.x this.y this.z
  enddef

  def newXY(this.x, this.y, this.z)
  enddef

  def Def1(arg = this.x)
    this.y = arg
    this.z += arg
  enddef

  def Def2(arg = (this.x + this.y + this.z))
    Echo(this, this.x, this.y, this.z)
    this->Echo(this.x, this.y, this.z)
  enddef

  def Def3(): Foo
    return this
  enddef

  def Def4(arg: Foo = this): Foo
    return arg
  enddef
endclass

class Bar extends Foo
  def Def1()
    super.Def1()
  enddef

  def Def2()
    var a = super.x * super.y * super.z
    var b = [super.x, super.y, super.z]
    var c = {super: super.x, this: super.y, true: super.z}
    var d = {super: c, this: c}
    echo c.super
    echo c.this
    echo d.super.this
    echo d.this.super
    echo a b c
  enddef

  def Def5()
    var a = this.x * this.y
    var b = (this.x * this.y)
    var c = [this.x, this.y]
    var d = {super: this.x, this: this.y}
    echo a b c d
  enddef

  def Def6()
    var x = this#super#x
    var y = super#this#y
    this#super#Func()
    super#this#Func()
  enddef

  def Def7(arg = super.Def3())
    echo arg
  enddef

  def Def8(): number
    var F = () => this.x
    var G = () => super.x
    return F() + G()
  enddef
endclass

defcompile Foo
defcompile Bar

