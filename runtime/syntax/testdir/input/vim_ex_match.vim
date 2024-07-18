" Vim :match, :2match and :3match commands

match FooGroup /Foo/
match
match none

2match FooGroup /Foo/
2match
2match none

3match FooGroup /Foo/
3match
3match none


" Differentiate map() from :map

call match(haystack, 'needle')
call match (haystack, 'needle')

function Foo()
  match FooGroup /Foo/
  call match(haystack, 'needle')
  call match (haystack, 'needle')
endfunction

def Foo()
  match FooGroup /Foo/
  match(haystack, 'needle')
  # Error: bad :match command - trailing characters
  match (haystack, 'needle')
enddef
