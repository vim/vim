vim9script
# VIM_TEST_SETUP hi link vim9LambdaOperator Todo
# VIM_TEST_SETUP hi link vim9LambdaParen Todo


# Vim 9 lambda expressions

var Foo: func
var expr = 0

# without return type

Foo = () => expr
Foo = (_) => expr
Foo = (x) => expr

Foo = (...y) => expr
Foo = (_, ...y) => expr
Foo = (x, ...y) => expr

Foo = (x, y) => expr

Foo = (_: number) => expr
Foo = (x: number) => expr

Foo = (...y: list<number>) => expr
Foo = (_: number, ...y: list<number>) => expr
Foo = (x: number, ...y: list<number>) => expr

Foo = (x: number, y: number) => expr

# with return type

Foo = (): number => expr
Foo = (_): number => expr
Foo = (x): number => expr

Foo = (...y): number => expr
Foo = (_, ...y): number => expr
Foo = (x, ...y): number => expr

Foo = (x, y): number => expr

Foo = (_: number): number => expr
Foo = (x: number): number => expr

Foo = (...y: list<number>): number => expr
Foo = (_: number, ...y: list<number>): number => expr
Foo = (x: number, ...y: list<number>): number => expr

Foo = (x: number, y: number): number => expr

# with compound return type

Foo = (): list<number> => expr
Foo = (_): list<number> => expr
Foo = (x): list<number> => expr

Foo = (...y): list<number> => expr
Foo = (_, ...y): list<number> => expr
Foo = (x, ...y): list<number> => expr

Foo = (x, y): list<number> => expr

Foo = (_: number): list<number> => expr
Foo = (x: number): list<number> => expr

Foo = (...y: list<number>): list<number> => expr
Foo = (_: number, ...y: list<number>): list<number> => expr
Foo = (x: number, ...y: list<number>): list<number> => expr

Foo = (x: number, y: number): list<number> => expr


# post operator comments

Foo = () => # comment
  expr
Foo = () =>
  # comment
  expr
Foo = () =>

  # comment

  expr


# line continuations

Foo = (x: string,
      \ y: number,
      \ z: bool) => expr

Foo = (x: string,
      \ y: number,
      \ z: bool)
      \ => expr

Foo = (x: string,
      \ y: number,
      \ z: bool): number => expr

Foo = (x: string,
      \ y: number,
      \ z: bool): number
      \ => expr

Foo = (x: string,
      \ y: number,
      \ z: bool): 
      \ number => expr


# funcref call

echo (() => 42)()
echo ((x: string): number => 42)("foo")


# :help vim9-lambda

var list = [1, 2, 3]
echo filter(list, (k, v) =>
		v > 0)
echo filter(list, (k,
      \	v)
      \	=> v > 0)

var Callback = (..._) => 'anything'
echo Callback(1, 2, 3)  # displays "anything"

var Lambda = (arg) => {
	g:was_called = 'yes'
	return expr
    }

var count = 0
var timer = timer_start(500, (_) => {
	 count += 1
	 echom 'Handler called ' .. count
     }, {repeat: 3})

var dict = {}
var d = mapnew(dict, (k, v): string => {
     return 'value'
   })


# Issue #15970 (vim9: Restore and extend the recognition of Enum body items)

def Op(): func(func(number, number): number): func(number, Digit): number
    return (F: func(number, number): number) =>
	(x: number, y: Digit): number => F(x, y.value)
enddef ####################### ^ vimCommand?


# Issue #16965 (vim syntax: wrong highlight with lambda, autoload, and false keyword)

autocmd BufRead * timer_start(0, (_) => f#a(false, false))
autocmd

