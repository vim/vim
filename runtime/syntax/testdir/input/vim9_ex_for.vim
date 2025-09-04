vim9script
# Vim9 :for command


# :for {var} in {object}

var expr = [42]

for foo in expr
  echo foo
endfor

for foo in expr # comment
  echo foo
endfor

for foo in
      # comment
      expr
  echo foo
endfor

for foo in expr | echo foo | endfor

for foo in [42]
  echo foo
endfor

for foo in [42] | echo foo | endfor

for foo: number in [42] | echo foo | endfor

echo "foo" | for foo in expr
  echo foo
endfor


# :for [{var1}, {var2}, ...] in {listlist}

var expr2 = [[42, 83]]

for [foo, bar] in expr2
  echo foo bar
endfor

for [foo, bar] in expr2 # comment
  echo foo bar
endfor

for [foo, bar] in
      # comment
      expr2
  echo foo bar
endfor

for [foo, bar] in expr2 | echo foo bar | endfor

for [foo, bar] in [[42, 83]]
  echo foo bar
endfor

for [foo, bar] in [[42, 83]] | echo foo bar | endfor

for [foo: number, bar: number] in expr2
  echo foo bar
endfor


# Issue #7961 (Builtin types are not highlighted in item-variable declarations
#              of :for commands)

var m: number
var n: number
for x: number in range(2) | m = x | endfor
###### ^^^^^^
echo m
for [x: number, y: number] in [[0, 0], [1, 1]] | [m, n] = [x, y] | endfor
echo m n

var F: func
for t: tuple<func> in ((function('tolower'),),) | F = t[0] | endfor
###### ^^^^^^^^^^^
echo F('HELLO')
for [L: func, U: func] in [[function('tolower'), function('toupper')]]
    [_, F] = [L, U]
endfor
echo F('hello') F('world')

