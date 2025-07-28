vim9script
# VIM_TEST_SETUP let g:vimsyn_folding = "cfi"
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax
# VIM_TEST_SETUP hi link vim9DefTypeParam Todo
# See: https://github.com/vim/vim/pull/17313#issuecomment-3046696820 (Aliaksei Budavei)


# See https://github.com/vim/vim/pull/16604#issuecomment-265202845 .
export interface Listable
    def Cons<E>(_: E): Listable
    def Reverse<E>(): Listable
    def Rest(): Listable
    def First<E>(): E
    def empty(): bool
    def len(): number
    def string(): string
endinterface

enum EmptyList implements Listable
    INSTANCE

    def Cons<E>(value: E): Listable
	return List.new<E>(value)
    enddef

    def Reverse<E>(): Listable
	return this
    enddef

    def Rest(): Listable
	return this
    enddef

    def First<E>(): E
	return null
    enddef

    def empty(): bool
	return true
    enddef

    def len(): number
	return 0
    enddef

    def string(): string
	return '[]'
    enddef
endenum

class List implements Listable
    const _value: any
    const _size: number
    var _next: Listable

    def new<E>(value: E)
	this._value = value
	this._size = 1
	this._next = EmptyList.INSTANCE
    enddef

    def _newCons<E>(value: E, size: number)
	this._value = value
	this._size = size
    enddef

    def Cons<E>(value: E): Listable
	const list: List = List._newCons<E>(value, (this._size + 1))
	list._next = this
	return list
    enddef

    def Reverse<E>(): Listable
	var result: Listable = List.new<E>(this.First<E>())
	var list: Listable = this.Rest()

	while !list.empty()
	    result = result.Cons<E>(list.First<E>())
	    list = list.Rest()
	endwhile

	return result
    enddef

    def Rest(): Listable
	return this._next
    enddef

    def First<E>(): E
	return this._value
    enddef

    def empty(): bool
	return (this._size == 0)
    enddef

    def len(): number
	return this._size
    enddef

    def string(): string
	if this.empty()
	    return '[]'
	endif

	var text: string = '[' .. string(this.First<any>()) .. ', '
	var list: Listable = this.Rest()

	while !list.empty()
	    text ..= string(list.First<any>()) .. ', '
	    list = list.Rest()
	endwhile

	return strpart(text, 0, (strlen(text) - 2)) .. ']'
    enddef
endclass

export def MakeEmptyList(): Listable
    return EmptyList.INSTANCE
enddef

export def MakeList<E>(value: E): Listable
    return List.new<E>(value)
enddef

export def Map<T, U>(listable: Listable, Mapper: func(T): U): Listable
    var result: Listable = EmptyList.INSTANCE
    var list: Listable = listable

    while !list.empty()
	result = result.Cons<U>(Mapper(list.First<T>()))
	list = list.Rest()
    endwhile

    return result.Reverse<U>()
enddef

export def Filter<T>(listable: Listable, Predicate: func(T): bool): Listable
    var result: Listable = EmptyList.INSTANCE
    var list: Listable = listable

    while !list.empty()
	if Predicate(list.First<T>())
	    result = result.Cons<T>(list.First<T>())
	endif

	list = list.Rest()
    endwhile

    return result.Reverse<T>()
enddef

############################################################

echo MakeEmptyList()

const listX: Listable = MakeEmptyList()
    .Cons<number>(0).Cons<number>(1).Cons<number>(2).Cons<number>(3)
const listY: Listable = MakeList<number>(0)
    .Cons<number>(1).Cons<number>(2).Cons<number>(3)
echo listX == listY
echo listX
echo listX.Reverse<number>()
echo MakeEmptyList().Reverse<any>()
echo Filter<number>(listX, (value: number) => value % 2 != 0)
echo Map<number, string>(listX, (value: number) => nr2char((value + 60), 1))

echo 4 listX.len() listY.len()
echo listX
echo listY

const list3X: Listable = listX.Rest()
const list3Y: Listable = listY.Rest()
echo 3 list3X.len() list3Y.len()
echo list3X
echo list3Y

const list2X: Listable = list3X.Rest()
const list2Y: Listable = list3Y.Rest()
echo 2 list2X.len() list2Y.len()
echo list2X
echo list2Y

const list1X: Listable = list2X.Rest()
const list1Y: Listable = list2Y.Rest()
echo 1 list1X.len() list1Y.len()
echo list1X
echo list1Y

const list0X: Listable = list1X.Rest()
const list0Y: Listable = list1Y.Rest()
echo 0 list0X.len() list0Y.len()
echo list0X
echo list0Y

const list0X_: Listable = list0X.Rest()
const list0Y_: Listable = list0Y.Rest()
echo 0 list0X_.len() list0Y_.len()
echo list0X_
echo list0Y_

const list0X__: Listable = list0X_.Rest()
const list0Y__: Listable = list0Y_.Rest()
echo 0 list0X__.len() list0Y__.len()
echo list0X__
echo list0Y__


const listZ: Listable = MakeList<Listable>(MakeList<number>(-1))
const listZZ: Listable = listZ.Cons<Listable>(MakeList<number>(0))
    .Cons<Listable>(MakeList<number>(1))
    .Cons<Listable>(MakeList<number>(2))
    .Cons<Listable>(MakeList<number>(3))
echo listZZ

