vim9script
# VIM_TEST_SETUP let g:vimsyn_folding = "cf"
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax
# VIM_TEST_SETUP hi link vim9DefTypeParam Todo
# See: https://github.com/vim/vim/pull/17313#issuecomment-3046696820 (Aliaksei Budavei)

# See https://github.com/vim/vim/issues/14330#issuecomment-2028938515 .
export class Set
    final _elements: dict<number>
    const _Mapper: func(number, string): any
    const ToStringer: func(any): string
    const FromStringer: func(string): any

    static def _Mapper<E>(F: func(string): E): func(number, string): E
	return ((G: func(string): E) => (_: number, v: string): E => G(v))(F)
    enddef

    def new<E>()
	this._elements = {}
	this._Mapper = _Mapper<E>((s: string): E => eval(s))
	this.ToStringer = (a: E): string => string(a)
	this.FromStringer = (s: string): E => eval(s)
    enddef

    def newFromList<E>(elements: list<E>, ToStringer: func(E): string,
					FromStringer: func(string): E)
	this._elements = elements
	    ->reduce(((F: func(E): string) => (d: dict<number>, v: E) =>
		extend({[F(v)]: 1}, d))(ToStringer),
		{})
	this._Mapper = _Mapper<E>(FromStringer)
	this.ToStringer = ToStringer
	this.FromStringer = FromStringer
    enddef

    def _FromList<E>(elements: list<E>): Set
	return Set.newFromList<E>(elements, this.ToStringer, this.FromStringer)
    enddef

    def Contains<E>(element: E): bool
	return has_key(this._elements, this.ToStringer(element))
    enddef

    def Elements<E>(): list<E>
	return keys(this._elements)->mapnew(this._Mapper)
    enddef

    def empty(): bool
	return empty(this._elements)
    enddef

    def len(): number
	return len(this._elements)
    enddef

    def string(): string
	return string(keys(this._elements))
    enddef

    # {1, 2, 3} ⊇ {1, 2}.
    def Superset(that: Set): bool
 	return (len(this._elements) >= len(that._elements)) && that._elements
	    ->keys()
	    ->indexof(((set: Set) => (_: number, v: string) => !set._elements
		->has_key(v))(this)) < 0
    enddef

    # {1, 2} ⊆ {1, 2, 3}.
    def Subset(that: Set): bool
 	return (len(this._elements) <= len(that._elements)) && this._elements
	    ->keys()
	    ->indexof(((set: Set) => (_: number, v: string) => !set._elements
		->has_key(v))(that)) < 0
    enddef

    # {1, 2, 3} ∪ {2, 3, 4} = {1, 2, 3, 4}.
    def Union(that: Set): Set
	return this._FromList<any>({}
	    ->extend(that._elements)
	    ->extend(this._elements)
	    ->keys()
	    ->map(this._Mapper))
    enddef

    # {1, 2, 3} ∩ {2, 3, 4} = {2, 3}.
    def Intersection(that: Set): Set
	return this._FromList<any>(this._elements
	    ->keys()
	    ->filter(((set: Set) => (_: number, v: string) => set._elements
		->has_key(v))(that))
	    ->map(this._Mapper))
    enddef

    # {1, 2, 3} \ {2, 3, 4} = {1}.
    # {2, 3, 4} \ {1, 2, 3} = {4}.
    def SetDifference(that: Set): Set
	return this._FromList<any>(this._elements
	    ->keys()
	    ->filter(((set: Set) => (_: number, v: string) => !set._elements
		->has_key(v))(that))
	    ->map(this._Mapper))
    enddef

    # {1, 2, 3} △ {2, 3, 4} = {1, 4}.
    def SymmetricDifference(that: Set): Set
	return this.Union(that).SetDifference(this.Intersection(that))
    enddef
endclass

############################################################

const ToStr: func(number): string = (s: number) => string(s)
const FromStr: func(string): number = (s: string) => str2nr(s)

echo Set.newFromList<number>([1, 2, 3], ToStr, FromStr)
    .Subset(Set.newFromList<number>([1, 2], ToStr, FromStr))
echo Set.newFromList<number>([1, 2], ToStr, FromStr)
    .Subset(Set.newFromList<number>([1, 2, 3], ToStr, FromStr))
echo Set.newFromList<number>([1, 2], ToStr, FromStr)
    .Superset(Set.newFromList<number>([1, 2, 3], ToStr, FromStr))
echo Set.newFromList<number>([1, 2, 3], ToStr, FromStr)
    .Superset(Set.newFromList<number>([1, 2], ToStr, FromStr))

echo Set.newFromList<number>([1, 2, 3], ToStr, FromStr)
    .Union(Set.newFromList<number>([2, 3, 4], ToStr, FromStr))
    .Elements<number>()
echo Set.newFromList<number>([2, 3, 4], ToStr, FromStr)
    .Union(Set.newFromList<number>([1, 2, 3], ToStr, FromStr))
    .Elements<number>()

echo Set.newFromList<number>([1, 2, 3], ToStr, FromStr)
    .Intersection(Set.newFromList<number>([2, 3, 4], ToStr, FromStr))
    .Elements<number>()
echo Set.newFromList<number>([2, 3, 4], ToStr, FromStr)
    .Intersection(Set.newFromList<number>([1, 2, 3], ToStr, FromStr))
    .Elements<number>()

echo Set.newFromList<number>([1, 2, 3], ToStr, FromStr)
    .SetDifference(Set.newFromList<number>([2, 3, 4], ToStr, FromStr))
    .Elements<number>()
echo Set.newFromList<number>([2, 3, 4], ToStr, FromStr)
    .SetDifference(Set.newFromList<number>([1, 2, 3], ToStr, FromStr))
    .Elements<number>()

echo Set.newFromList<number>([1, 2, 3], ToStr, FromStr)
    .SymmetricDifference(Set.newFromList<number>([2, 3, 4], ToStr, FromStr))
    .Elements<number>()
echo Set.newFromList<number>([2, 3, 4], ToStr, FromStr)
    .SymmetricDifference(Set.newFromList<number>([1, 2, 3], ToStr, FromStr))
    .Elements<number>()

############################################################

const none: Set = Set.new<any>()
echo none.len()
echo none.empty()
echo none.string()
echo string(none.Elements<any>())

const sets: Set = Set.newFromList<Set>(
    [Set.new<any>(), Set.new<any>(), Set.new<any>(), Set.new<any>()],
    (o: Set): string => string(o),
    (_: string): Set => Set.new<any>())
echo sets.len()
echo sets.empty()
echo sets.string()
echo string(sets.Elements<Set>())

const lists: Set = Set.newFromList<list<any>>(
    [[[[[]]]]],
    (o: list<any>): string => string(o),
    (s: string): list<any> => eval(s))
echo lists.len()
echo lists.empty()
echo lists.string()
echo string(lists.Elements<list<any>>())

