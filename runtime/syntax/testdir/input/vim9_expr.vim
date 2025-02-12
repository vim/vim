vim9script
# Vim9-script expressions


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


# Issue #16227 (Vimscript ternary expression highlighting)

var foo = 'foo'                         # comment
var bar = foo == 'foo' ? 'bar' : 'baz'
var baz = foo == 'foo'
            \ ? 'baz'
            \ : 'bar'
var qux = foo == 'foo'
            ? 'qux'                     # comment
            : 'qux'                     # comment
echo qux ?? 'quux'

