vim9script
# Vim9 generic classes

class A<B, C> extends D<E, F> implements G<H, I>, J<K, L>
endclass


# :help vim9-generic


class Box<T>
   var value: T

   def new(this.value)
   enddef

   def Get(): T
      return this.value
   enddef

   def Set(value: T)
      this.value = value
   enddef
endclass


class Pair<K, V>
   var key: K
   var val: V

   def new(this.key, this.val)
   enddef

   def Swap(): Pair<V, K>
      return Pair<V, K>.new(this.val, this.key)
   enddef
endclass


var nb = Box<number>.new(10)
nb.Set(nb.Get() + 5)
echo nb.Get()			# 15
var sb: Box<string> = Box<string>.new('abc')
echo typename(sb)		# object<Box<string>>

var p = Pair<string, number>.new('one', 1)
echo p.Swap().key		# 1


var pairs: list<Pair<string, number>> = [p]
var boxed: Box<Box<number>> = Box<Box<number>>.new(nb)


defcompile Box<number>
disassemble Box<number>.Get


var b = Box.new(1)
# E1588: Type arguments missing for generic class 'Box'
var p = Pair<number>.new()
# E1591: Not enough types specified for generic class 'Pair'


var b1: Box<number> = Box<string>.new('abc')
# E1012: Type mismatch; expected object<Box<number>> but got
#        object<Box<string>>

class Base
endclass
class Child extends Base
endclass
var b2: Box<Base> = Box<Child>.new(Child.new())
# E1012: Type mismatch; expected object<Box<object<Base>>> but got
#        object<Box<object<Child>>>

class Counter<T>
   static var count: number = 0
   var value: T

   def new(this.value)
      Counter<T>.count += 1
   enddef

   static def Of(value: T): Counter<T>
      return Counter<T>.new(value)
   enddef
endclass

Counter<number>.new(1)
Counter<number>.Of(2)
Counter<string>.new('a')
echo Counter<number>.count Counter<string>.count	# 2 1


class Box<T>
   var value: T

   def new(this.value)
   enddef

   def Map<U>(F: func(T): U): Box<U>
      return Box<U>.new(F(this.value))
   enddef
endclass

var b = Box<number>.new(3)
var s = b.Map<string>((n: number): string => repeat('x', n))
echo s.value			# xxx


class Stack<T>
   var items: list<T> = []

   def Push(item: T)
      this.items->add(item)
   enddef
endclass

class LimitedStack<T> extends Stack<T>
   var limit: number

   def new(this.limit)
   enddef

   def Push(item: T)
      if len(this.items) < this.limit
	 super.Push(item)
      endif
   enddef
endclass

class NumberStack extends Stack<number>
   def Sum(): number
      return this.items->reduce((acc, n) => acc + n, 0)
   enddef
endclass

var st: Stack<string> = LimitedStack<string>.new(2)



interface Container<T>
   def Get(): T
endinterface

interface Resettable<T> extends Container<T>
   def Reset(value: T)
endinterface


class Box<T> implements Resettable<T>
   var value: T

   def new(this.value)
   enddef

   def Get(): T
      return this.value
   enddef

   def Reset(value: T)
      this.value = value
   enddef
endclass

class Name implements Container<string>
   def Get(): string
      return 'Vim'
   enddef
endclass


def Describe(c: Container<string>): string
   return $'value: {c.Get()}'
enddef
echo Describe(Box<string>.new('abc'))	# value: abc
echo Describe(Name.new())		# value: Vim

var r: Resettable<number> = Box<number>.new(1)
r.Reset(7)
echo r.Get()				# 7


interface Mapper<T>
   def Map<U>(F: func(T): U): list<U>
endinterface

class Numbers implements Mapper<number>
   var items: list<number>

   def new(this.items)
   enddef

   def Map<U>(F: func(number): U): list<U>
      return this.items->mapnew((_, n) => F(n))
   enddef
endclass

var m: Mapper<number> = Numbers.new([1, 2])
echo m.Map<string>((n: number): string => $'#{n}')	# ['#1', '#2']


class Box<T>
   def Wrap(): Box<list<T>>
      return Box<list<T>>.new()
   enddef
endclass
var b = Box<number>.new()
# E1592: Generic class 'Box' nested too deep

