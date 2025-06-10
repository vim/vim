" Vim :debuggreedy command

debuggreedy
0debuggreedy

debuggreedy  " comment
0debuggreedy " comment

debuggreedy  | echo "Foo"
0debuggreedy | echo "Foo"

function Foo()
  debuggreedy
  0debuggreedy

  debuggreedy  " comment
  0debuggreedy " comment

  debuggreedy  | echo "Foo"
  0debuggreedy | echo "Foo"
endfunction

def Bar()
  debuggreedy
  0debuggreedy

  debuggreedy  # comment
  0debuggreedy # comment

  debuggreedy  | echo "Foo"
  0debuggreedy | echo "Foo"
enddef

