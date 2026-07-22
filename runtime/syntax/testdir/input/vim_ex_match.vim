" Vim :match command


match FooGroup /Foo/
match
match none

1match FooGroup /Foo/
1match
1match none

2match FooGroup /Foo/
2match
2match none

3match FooGroup /Foo/
3match
3match none

1 match FooGroup /Foo/
2 match FooGroup /Foo/
3 match FooGroup /Foo/


" Differentiate match() from :match

call match(haystack, 'needle')
call match (haystack, 'needle')

let foo = match(haystack, 'needle')
echo match(haystack, 'needle')
echo (match(haystack, 'needle') + 42)
echo (42 + match(haystack, 'needle'))


" Containing functions

function Foo()
  match FooGroup /Foo/
  call match(haystack, 'needle')
  call match (haystack, 'needle')
endfunction

def Foo()
  # command
  match FooGroup /Foo/
  # Error: bad :match command - trailing characters
  match (haystack, 'needle')
  # function
  match(haystack, 'needle')
  call match(haystack, 'needle')
enddef


" Trailing bar and comments

match FooGroup /Foo/ | echo "Foo"
match                | echo "Foo"
match none           | echo "Foo"

match FooGroup /Foo/ " comment
match                " comment
match none           " comment

