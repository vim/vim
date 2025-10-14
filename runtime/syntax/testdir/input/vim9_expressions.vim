vim9script
# Vim9 expressions


# Boolean

echo true false

# Number

# ' separated

echo   1'000'000
echo   1'000'000.00
echo  -1'000'000
echo  -1'000'000.00

echo   0b1000'0000
echo   0o1000'0000
echo   0x1000'0000
echo  -0b1000'0000
echo  -0o1000'0000
echo  -0x1000'0000

# Null

echo null null_blob null_channel null_class null_dict null_function null_job
echo null_list null_object null_partial null_string null_tuple

# Dictionary

echo {}
echo {   foo: 21 * 2 }
echo { -foo-: 21 * 2 }
echo {    42: 21 * 2 }
echo { 'foo': 21 * 2 }
echo { "foo": 21 * 2 }

echo { foo: {   bar: 21 * 2 } }
echo { foo: { -bar-: 21 * 2 } }
echo { foo: {    42: 21 * 2 } }
echo { foo: { 'bar': 21 * 2 } }
echo { foo: { "bar": 21 * 2 } }

echo { -foo-: {   bar: 21 * 2 } }
echo { -foo-: { -bar-: 21 * 2 } }
echo { -foo-: {    42: 21 * 2 } }
echo { -foo-: { 'bar': 21 * 2 } }
echo { -foo-: { "bar": 21 * 2 } }

echo { 42: {   bar: 21 * 2 } }
echo { 42: { -bar-: 21 * 2 } }
echo { 42: {    42: 21 * 2 } }
echo { 42: { 'bar': 21 * 2 } }
echo { 42: { "bar": 21 * 2 } }

echo { 'foo': {   bar: 21 * 2 } }
echo { 'foo': { -bar-: 21 * 2 } }
echo { 'foo': {    42: 21 * 2 } }
echo { 'foo': { "bar": 21 * 2 } }
echo { 'foo': { 'bar': 21 * 2 } }

echo { "foo": {   bar: 21 * 2 } }
echo { "foo": { -bar-: 21 * 2 } }
echo { "foo": {    42: 21 * 2 } }
echo { "foo": { 'bar': 21 * 2 } }
echo { "foo": { "bar": 21 * 2 } }

echo {
  # comment
  foo: {
    bar: 21 * 2
  }
}

# match as keys not scope dictionaries
echo { b: 42, w: 42, t: 42, g: 42, l: 42, s: 42, a: 42, v: 42  }

# Register

echo @" @@
echo @0 @1 @2 @3 @4 @5 @6 @7 @8 @9
echo @a @b @c @d @e @f @g @h @i @j @k @l @m @n @o @p @q @r @s @t @u @v @w @x @y @z
echo @A @B @C @D @E @F @G @H @I @J @K @L @M @N @O @P @Q @R @S @T @U @V @W @X @Y @Z
echo @- @: @. @% @# @= @* @+ @~ @_ @/

# read-only @:, @., @%, @~
@" = "foo" 
@0 = "foo"
@1 = "foo"
@9 = "foo"
@a = "foo"
@k = "foo"
@z = "foo"
@A = "foo"
@K = "foo"
@Z = "foo"
@- = "foo"
@# = "foo"
@= = "foo"
@* = "foo"
@+ = "foo"
@_ = "foo"
@/ = "foo"

# Operators

# Ternary

echo expr ? expr : expr

echo lnum == 1 ? "top" : lnum
echo lnum == 1 ? "top" : lnum == 1000 ? "last" : lnum

echo lnum == 1
	? "top"
	: lnum == 1000
		? "last"
		: lnum
echo lnum == 1 ?
	"top" :
	lnum == 1000 ?
		"last" :
		lnum

echo true ? true : false
echo 1 ? 1 : 0
echo "foo" ? "foo" : "bar"
echo foo ? foo : bar
echo g:foo ? g:foo : g:bar
echo $FOO ? $FOO : $BAR
echo True() ? True() : False()
echo @a ? @a : @b
echo (true) ? (true) : (false)
echo (1) ? (1) : (0)

# Falsy

echo expr ?? expr

echo theList ?? 'list is empty'
echo GetName() ?? 'unknown'

echo theList
      \ ?? 'list is empty'
echo theList ??
      \ 'list is empty'

echo true ?? true
echo 1 ?? 1
echo "foo" ?? "foo"
echo foo ?? foo
echo g:foo ?? g:foo
echo $FOO ?? $FOO
echo True() ?? True()
echo @a ?? @a
echo (true) ?? (true)
echo (1) ?? (1)


# Function calls

Foo(true, false, null)


# Command {expr} arguments

if true
  echo true
elseif false
  echo false
endif

while true
  break
endwhile

def Foo(): bool
  return true
enddef


# Issue #14423 (vim.vim: Opt out of vimSearch*)

:?truthy
const truthy: number = false
    ? (0
    )
    : (1
    )
echo truthy

def Foo()
  :?truthy
  const truthy: number = false
      ? (0
      )
      : (1
      )
  echo truthy
enddef


# Issue #16227 (Vim script ternary expression highlighting)

var foo = 'foo'                         # comment
var bar = foo == 'foo' ? 'bar' : 'baz'
var baz = foo == 'foo'
            \ ? 'baz'
            \ : 'bar'
var qux = foo == 'foo'
            ? 'qux'                     # comment
            : 'qux'                     # comment
echo qux ?? 'quux'

