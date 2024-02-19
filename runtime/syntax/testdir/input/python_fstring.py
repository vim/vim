# Python f-string tests

# This file contains code derived from the CPython project,
# specifically from the file 'Lib/test/test_fstring.py'.
#
# The original code is Copyright (c) 2001-2025 Python Software Foundation;
# All Rights Reserved
#
# It is licensed under the PSF License Agreement:
# https://docs.python.org/3/license.html
#
# Modifications from the original:
# - Test input expressions have been extracted from the test functions and the
#   functions deleted
#
# This inclusion is for testing purposes of Vim's Python syntax highlighting.

# Python-3.12.2/Lib/test/test_fstring.py (reformatted for syntax testing)

## test_ast
f'{a * x()}'

## test_ast_line_numbers_multiple_formattedvalues
f'no formatted values'
f'eggs {a * x()} spam {b + y()}'

## test_ast_line_numbers_nested
f'{a * f"-{x()}-"}'

## test_ast_line_numbers_duplicate_expression
f'{a * x()} {a * x()} {a * x()}'

## test_ast_numbers_fstring_with_formatting
'f"Here is that pesky {xxx:.3f} again"'

## test_ast_line_numbers_multiline_fstring
f'''
  {a
     *
       x()}
non-important content
'''

f'''
          {blech}
'''

## test_ast_line_numbers_with_parentheses
x = (
    f" {test(t)}"
)

x = (
    u'wat',
    u"wat",
    b'wat',
    b"wat",
    f'wat',
    f"wat",
)

y = (
    u'''wat''',
    u"""wat""",
    b'''wat''',
    b"""wat""",
    f'''wat''',
    f"""wat""",
)

x = (
        'PERL_MM_OPT', (
            f'wat'
            f'some_string={f(x)} '
            f'wat'
        ),
)

## test_ast_fstring_empty_format_spec
f'{expr:}'


## test_docstring
def f():
    f'''Not a docstring'''

def g():
    '''Not a docstring''' \
    f''


## test_ast_compile_time_concat
('foo' f'{3}', 'foo3')

## test_literal
(f'', '')
(f'a', 'a')
(f' ', ' ')

## test_double_braces
(f'{{', '{')
(f'a{{', 'a{')
(f'{{b', '{b')
(f'a{{b', 'a{b')
(f'}}', '}')
(f'a}}', 'a}')
(f'}}b', '}b')
(f'a}}b', 'a}b')
(f'{{}}', '{}')
(f'a{{}}', 'a{}')
(f'{{b}}', '{b}')
(f'{{}}c', '{}c')
(f'a{{b}}', 'a{b}')
(f'a{{}}c', 'a{}c')
(f'{{b}}c', '{b}c')
(f'a{{b}}c', 'a{b}c')

(f'{{{10}', '{10')
(f'}}{10}', '}10')
(f'}}{{{10}', '}{10')
(f'}}a{{{10}', '}a{10')

(f'{10}{{', '10{')
(f'{10}}}', '10}')
(f'{10}}}{{', '10}{')
(f'{10}}}a{{' '}', '10}a{}')

# Inside of strings, don't interpret doubled brackets.
(f'{"{{}}"}', '{{}}')

## test_compile_time_concat
x = 'def'
('abc' f'## {x}ghi', 'abc## defghi')
('abc' f'{x}' 'ghi', 'abcdefghi')
('abc' f'{x}' 'gh' f'i{x:4}', 'abcdefghidef ')
('{x}' f'{x}', '{x}def')
('{x' f'{x}', '{xdef')
('{x}' f'{x}', '{x}def')
('{{x}}' f'{x}', '{{x}}def')
('{{x' f'{x}', '{{xdef')
('x}}' f'{x}', 'x}}def')
(f'{x}' 'x}}', 'defx}}')
(f'{x}' '', 'def')
('' f'{x}' '', 'def')
('' f'{x}', 'def')
(f'{x}' '2', 'def2')
('1' f'{x}' '2', '1def2')
('1' f'{x}', '1def')
(f'{x}' f'-{x}', 'def-def')
('' f'', '')
('' f'' '', '')
('' f'' '' f'', '')
(f'', '')
(f'' '', '')
(f'' '' f'', '')
(f'' '' f'' '', '')

# This is not really [f'{'] + [f'}'] since we treat the inside
# of braces as a purely new context, so it is actually f'{ and
# then eval('  f') (a valid expression) and then }' which would
# constitute a valid f-string.
(f'{' f'}', ' f')

## test_comments
# These aren't comments, since they're in strings.
d = {'#': 'hash'}
(f'{"#"}', '#')
(f'{d["#"]}', 'hash')

(f'''A complex trick: {
2  # two
}''', 'A complex trick: 2')

(f'''
{
40 # fourty
+  # plus
2  # two
}''', '\n42')

(f'''
{
40 # fourty
+  # plus
2  # two
}''', '\n42')

(f'''
# this is not a comment
{ # the following operation it's
3 # this is a number
* 2}''', '\n# this is not a comment\n6')

(f'''
{# f'a {comment}'
86 # constant
# nothing more
}''', '\n86')

## test_format_specifier_expressions
width = 10
precision = 4
value = decimal.Decimal('12.34567')
(f'result: {value:{width}.{precision}}', 'result:      12.35')
(f'result: {value:{width!r}.{precision}}', 'result:      12.35')
(f'result: {value:{width:0}.{precision:1}}', 'result:      12.35')
(f'result: {value:{1}{0:0}.{precision:1}}', 'result:      12.35')
(f'result: {value:{ 1}{ 0:0}.{ precision:1}}', 'result:      12.35')
(f'{10:#{1}0x}', '       0xa')
(f'{10:{"#"}1{0}{"x"}}', '       0xa')
(f'{-10:-{"#"}1{0}x}', '      -0xa')
(f'{-10:{"-"}#{1}0{"x"}}', '      -0xa')
(f'{10:#{3 != {4:5} and width}x}', '       0xa')
(f'result: {value:{width:{0}}.{precision:1}}', 'result:      12.35')

## test_parens_in_expressions
(f'{3,}', '(3,)')

## test_backslashes_in_string_part
(f'\t', '\t')
(r'\t', '\\t')
(rf'\t', '\\t')
(f'{2}\t', '2\t')
(f'{2}\t{3}', '2\t3')
(f'\t{3}', '\t3')

(f'\u0394', '\u0394')
(r'\u0394', '\\u0394')
(rf'\u0394', '\\u0394')
(f'{2}\u0394', '2\u0394')
(f'{2}\u0394{3}', '2\u03943')
(f'\u0394{3}', '\u03943')

(f'\U00000394', '\u0394')
(r'\U00000394', '\\U00000394')
(rf'\U00000394', '\\U00000394')
(f'{2}\U00000394', '2\u0394')
(f'{2}\U00000394{3}', '2\u03943')
(f'\U00000394{3}', '\u03943')

(f'\N{GREEK CAPITAL LETTER DELTA}', '\u0394')
(f'{2}\N{GREEK CAPITAL LETTER DELTA}', '2\u0394')
(f'{2}\N{GREEK CAPITAL LETTER DELTA}{3}', '2\u03943')
(f'\N{GREEK CAPITAL LETTER DELTA}{3}', '\u03943')
(f'2\N{GREEK CAPITAL LETTER DELTA}', '2\u0394')
(f'2\N{GREEK CAPITAL LETTER DELTA}3', '2\u03943')
(f'\N{GREEK CAPITAL LETTER DELTA}3', '\u03943')

(f'\x20', ' ')
(r'\x20', '\\x20')
(rf'\x20', '\\x20')
(f'{2}\x20', '2 ')
(f'{2}\x20{3}', '2 3')
(f'\x20{3}', ' 3')

(f'2\x20', '2 ')
(f'2\x203', '2 3')
(f'\x203', ' 3')

AMPERSAND = 'spam'
# Get the right unicode character (&), or pick up local variable
# depending on the number of backslashes.
(f'\N{AMPERSAND}', '&')
(f'\\N{AMPERSAND}', '\\Nspam')
(fr'\N{AMPERSAND}', '\\Nspam')
(f'\\\N{AMPERSAND}', '\\&')

## test_backslashes_in_expression_part
(f"{(
                1 +
                2
)}", "3")

("\N{LEFT CURLY BRACKET}", '{')
(f'{"\N{LEFT CURLY BRACKET}"}', '{')
(rf'{"\N{LEFT CURLY BRACKET}"}', '{')

## test_no_escapes_for_braces
# Only literal curly braces begin an expression.
# \x7b is '{'.
(f'\x7b1+1}}', '{1+1}')
(f'\x7b1+1', '{1+1')
(f'\u007b1+1', '{1+1')
(f'\N{LEFT CURLY BRACKET}1+1\N{RIGHT CURLY BRACKET}', '{1+1}')

## test_newlines_in_expressions
(f'{0}', '0')
(rf'''{3+
4}''', '7')

## test_lambda
x = 5
(f'{(lambda y:x*y)("8")!r}', "'88888'")
(f'{(lambda y:x*y)("8")!r:10}', "'88888'   ")
(f'{(lambda y:x*y)("8"):10}', "88888     ")

## test_valid_prefixes
(F'{1}', "1")
(FR'{2}', "2")
(fR'{3}', "3")

## test_roundtrip_raw_quotes
(fr"\'", "\\'")
(fr'\"', '\\"')
(fr'\"\'', '\\"\\\'')
(fr'\'\"', '\\\'\\"')
(fr'\"\'\"', '\\"\\\'\\"')
(fr'\'\"\'', '\\\'\\"\\\'')
(fr'\"\'\"\'', '\\"\\\'\\"\\\'')

# test_fstring_backslash_before_double_bracket
deprecated_cases = [
    (r"f'\{{\}}'",   '\\{\\}'),
    (r"f'\{{'",      '\\{'),
    (r"f'\{{{1+1}'", '\\{2'),
    (r"f'\}}{1+1}'", '\\}2'),
    (r"f'{1+1}\}}'", '2\\}')
]
(fr'\{{\}}', '\\{\\}')
(fr'\{{', '\\{')
(fr'\{{{1+1}', '\\{2')
(fr'\}}{1+1}', '\\}2')
(fr'{1+1}\}}', '2\\}')

## test_fstring_backslash_prefix_raw
(f'\\', '\\')
(f'\\\\', '\\\\')
(fr'\\', r'\\')
(fr'\\\\', r'\\\\')
(rf'\\', r'\\')
(rf'\\\\', r'\\\\')
(Rf'\\', R'\\')
(Rf'\\\\', R'\\\\')
(fR'\\', R'\\')
(fR'\\\\', R'\\\\')
(FR'\\', R'\\')
(FR'\\\\', R'\\\\')

## test_fstring_format_spec_greedy_matching
(f"{1:}}}", "1}")
(f"{1:>3{5}}}}", "                                  1}")

## test_yield
# Not terribly useful, but make sure the yield turns
#  a function into a generator
def fn(y):
    f'y:{yield y*2}'
    f'{yield}'

## test_yield_send
def fn(x):
    yield f'x:{yield (lambda i: x * i)}'

## test_expressions_with_triple_quoted_strings
(f"{'''x'''}", 'x')
(f"{'''eric's'''}", "eric's")

# Test concatenation within an expression
(f'{"x" """eric"s""" "y"}', 'xeric"sy')
(f'{"x" """eric"s"""}', 'xeric"s')
(f'{"""eric"s""" "y"}', 'eric"sy')
(f'{"""x""" """eric"s""" "y"}', 'xeric"sy')
(f'{"""x""" """eric"s""" """y"""}', 'xeric"sy')
(f'{r"""x""" """eric"s""" """y"""}', 'xeric"sy')

## test_multiple_vars
x = 98
y = 'abc'
(f'{x}{y}', '98abc')

(f'X{x}{y}', 'X98abc')
(f'{x}X{y}', '98Xabc')
(f'{x}{y}X', '98abcX')

(f'X{x}Y{y}', 'X98Yabc')
(f'X{x}{y}Y', 'X98abcY')
(f'{x}X{y}Y', '98XabcY')

(f'X{x}Y{y}Z', 'X98YabcZ')

## test_closure
def outer(x):
    def inner():
        return f'x:{x}'
    return inner

(outer('987')(), 'x:987')
(outer(7)(), 'x:7')

## test_arguments
y = 2
def f(x, width):
    return f'x={x*y:{width}}'

(f('foo', 10), 'x=foofoo    ')
x = 'bar'
(f(10, 10), 'x=        20')

## test_locals
value = 123
(f'v:{value}', 'v:123')

## test_missing_format_spec
class O:
    def __format__(self, spec):
        if not spec:
            return '*'
        return spec

(f'{O():x}', 'x')
(f'{O()}', '*')
(f'{O():}', '*')

(f'{3:}', '3')
(f'{3!s:}', '3')

## test_call
def foo(x):
    return 'x=' + str(x)

(f'{foo(10)}', 'x=10')

## test_nested_fstrings
y = 5
(f'{f"{0}"*3}', '000')
(f'{f"{y}"*3}', '555')

## test_leading_trailing_spaces
(f'{ 3}', '3')
(f'{  3}', '3')
(f'{3 }', '3')
(f'{3  }', '3')

(f'expr={ {x: y for x, y in [(1, 2), ]}}',
                 'expr={1: 2}')
(f'expr={ {x: y for x, y in [(1, 2), ]} }',
                 'expr={1: 2}')

## test_not_equal
# There's a special test for this because there's a special
#  case in the f-string parser to look for != as not ending an
#  expression. Normally it would, while looking for !s or !r.

(f'{3!=4}', 'True')
(f'{3!=4:}', 'True')
(f'{3!=4!s}', 'True')
(f'{3!=4!s:.3}', 'Tru')

## test_equal_equal
# Because an expression ending in = has special meaning,
# there's a special test for ==. Make sure it works.

(f'{0==1}', 'False')

## test_conversions
(f'{3.14:10.10}', '      3.14')
(f'{3.14!s:10.10}', '3.14      ')
(f'{3.14!r:10.10}', '3.14      ')
(f'{3.14!a:10.10}', '3.14      ')

(f'{"a"}', 'a')
(f'{"a"!r}', "'a'")
(f'{"a"!a}', "'a'")

# Conversions can have trailing whitespace after them since it
# does not provide any significance
(f"{3!s  }", "3")
(f'{3.14!s  :10.10}', '3.14      ')

# Not a conversion.
(f'{"a!r"}', "a!r")

# Not a conversion, but show that ! is allowed in a format spec.
(f'{3.14:!<10.10}', '3.14!!!!!!')

# But these are just normal strings.
(f'{"{"}', '{')
(f'{"}"}', '}')
(f'{3:{"}"}>10}', '}}}}}}}}}3')
(f'{2:{"{"}>10}', '{{{{{{{{{2')

## test_empty_format_specifier
x = 'test'
(f'{x}', 'test')
(f'{x:}', 'test')
(f'{x!s:}', 'test')
(f'{x!r:}', "'test'")

## test_str_format_differences
d = {'a': 'string',
     0: 'integer',
     }
a = 0
(f'{d[0]}', 'integer')
(f'{d["a"]}', 'string')
(f'{d[a]}', 'integer')
('{d[a]}'.format(d=d), 'string')
('{d[0]}'.format(d=d), 'integer')

## test_dict
d = {'"': 'dquote',
     "'": 'squote',
     'foo': 'bar',
     }
(f'''{d["'"]}''', 'squote')
(f"""{d['"']}""", 'dquote')

(f'{d["foo"]}', 'bar')
(f"{d['foo']}", 'bar')

## test_debug_conversion
x = 'A string'
(f'{x=}', 'x=' + repr(x))
(f'{x =}', 'x =' + repr(x))
(f'{x=!s}', 'x=' + str(x))
(f'{x=!r}', 'x=' + repr(x))
(f'{x=!a}', 'x=' + ascii(x))

x = 2.71828
(f'{x=:.2f}', 'x=' + format(x, '.2f'))
(f'{x=:}', 'x=' + format(x, ''))
(f'{x=!r:^20}', 'x=' + format(repr(x), '^20'))
(f'{x=!s:^20}', 'x=' + format(str(x), '^20'))
(f'{x=!a:^20}', 'x=' + format(ascii(x), '^20'))

x = 9
(f'{3*x+15=}', '3*x+15=42')

# There is code in ast.c that deals with non-ascii expression values.  So,
# use a unicode identifier to trigger that.
tenπ = 31.4
(f'{tenπ=:.2f}', 'tenπ=31.40')

# Also test with Unicode in non-identifiers.
(f'{"Σ"=}', '"Σ"=\'Σ\'')

# Make sure nested fstrings still work.
(f'{f"{3.1415=:.1f}":*^20}', '*****3.1415=3.1*****')

# Make sure text before and after an expression with = works
# correctly.
pi = 'π'
(f'alpha α {pi=} ω omega', "alpha α pi='π' ω omega")

# Check multi-line expressions.
(f'''{
3
=}''', '\n3\n=3')

# Since = is handled specially, make sure all existing uses of
# it still work.

(f'{0==1}', 'False')
(f'{0!=1}', 'True')
(f'{0<=1}', 'True')
(f'{0>=1}', 'False')
(f'{(x:="5")}', '5')
(x, '5')
(f'{(x:=5)}', '5')
(x, 5)
(f'{"="}', '=')

x = 20
# This isn't an assignment expression, it's 'x', with a format
# spec of '=10'.  See test_walrus: you need to use parens.
(f'{x:=10}', '        20')

# Test named function parameters, to make sure '=' parsing works
# there.
def f(a):
    nonlocal x
    oldx = x
    x = a
    return oldx
x = 0
(f'{f(a="3=")}', '0')
(x, '3=')
(f'{f(a=4)}', '3=')
(x, 4)

# Make sure __format__ is being called.
class C:
    def __format__(self, s):
        return f'FORMAT-{s}'
    def __repr__(self):
        return 'REPR'

(f'{C()=}', 'C()=REPR')
(f'{C()=!r}', 'C()=REPR')
(f'{C()=:}', 'C()=FORMAT-')
(f'{C()=: }', 'C()=FORMAT- ')
(f'{C()=:x}', 'C()=FORMAT-x')
(f'{C()=!r:*^20}', 'C()=********REPR********')

        self.assertRaises(SyntaxError, eval, "f'{C=]'")

# Make sure leading and following text works.
x = 'foo'
(f'X{x=}Y', 'Xx='+repr(x)+'Y')

# Make sure whitespace around the = works.
(f'X{x  =}Y', 'Xx  ='+repr(x)+'Y')
(f'X{x=  }Y', 'Xx=  '+repr(x)+'Y')
(f'X{x  =  }Y', 'Xx  =  '+repr(x)+'Y')
(f"sadsd {1 + 1 =  :{1 + 1:1d}f}", "sadsd 1 + 1 =  2.000000")

(f"{1+2 = # my comment
  }", '1+2 = \n  3')

# These next lines contains tabs.  Backslash escapes don't
# work in f-strings.
# patchcheck doesn't like these tabs.  So the only way to test
# this will be to dynamically created and exec the f-strings.  But
# that's such a hassle I'll save it for another day.  For now, convert
# the tabs to spaces just to shut up patchcheck.
(f'X{x	=}Y', 'Xx\t='+repr(x)+'Y')
(f'X{x	=       }Y', 'Xx\t=\t'+repr(x)+'Y')

## test_walrus
x = 20
# This isn't an assignment expression, it's 'x', with a format
# spec of '=10'.
(f'{x:=10}', '        20')

# This is an assignment expression, which requires parens.
(f'{(x:=10)}', '10')
(x, 10)

