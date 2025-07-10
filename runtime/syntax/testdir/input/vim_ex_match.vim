" Vim :match command


match FooGroup /Foo/
match
match none

2match FooGroup /Foo/
2match
2match none

3match FooGroup /Foo/
3match
3match none


" Differentiate match() from :match

call match(haystack, 'needle')
call match (haystack, 'needle')

function Foo()
  match FooGroup /Foo/
  call match(haystack, 'needle')
  call match (haystack, 'needle')
endfunction

def Foo()
  # command
  match FooGroup /Foo/
  # function
  match(haystack, 'needle')
  # Error: bad :match command - trailing characters
  match (haystack, 'needle')
enddef

