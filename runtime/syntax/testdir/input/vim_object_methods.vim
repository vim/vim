vim9script

# VIM_TEST_SETUP hi link vim9MethodName Special
# VIM_TEST_SETUP hi link vim9This Todo


# Vim |builtin-object-methods| and namesake builtin functions.
class PairClassTest
	public const a: any
	public const b: any

	def new(a: any, b: any)
		this.a = a
		this.b = b
	enddef

	def empty(): bool
		return false
	enddef
	def len(): number
		return 2
	enddef
	def string(): string
		return printf('(%s, %s)', this.a, this.b)
	enddef
endclass

enum MarkerEnumTest
	INSTANCE

	def NoOp()
	enddef

	def empty(): bool
		return true
	enddef
	def len(): number
		return 0
	enddef
	def string(): string
		return this.name
	enddef
endenum

const b1: bool = empty(MarkerEnumTest.INSTANCE)
const n1: number = len(MarkerEnumTest.INSTANCE)
const s1: string = string(MarkerEnumTest.INSTANCE)
echo b1 && MarkerEnumTest.INSTANCE.empty()
echo n1 == 0 && MarkerEnumTest.INSTANCE.len() == 0
echo s1 == 'INSTANCE' && MarkerEnumTest.INSTANCE.string() == 'INSTANCE'

const pair: PairClassTest = PairClassTest.new(0, 1)
const b2: bool = !pair.empty()
const n2: number = pair.len()
const s2: string = pair.string()
echo b2 && !empty(pair)
echo n2 == 2 && len(pair) == 2
echo s2 == '(0, 1)' && string(pair) == '(0, 1)'
