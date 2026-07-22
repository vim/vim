" Vim :eval command


eval "Foo"->append(0)

eval "Foo"
      \ ->append(
      \ 0
      \ )

eval "Foo"->append(0) | echo "Foo"

echo "Foo" | eval "Foo"->append(0)

eval "Foo"->append(0) " comment

def Vim9Context()
  eval "Foo"->append(0) # comment
enddef

