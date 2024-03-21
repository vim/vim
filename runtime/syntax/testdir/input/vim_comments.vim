" Comments

" line comment
foo() " tail comment

:Foo
      "\ line continuation comment
      \ arg1
      "\ line continuation comment
      \ arg2

" comment
  \ continuing comment
  \ continuing comment

echo "TOP"

" :Foo
      \ arg
      "\ comment
      \ arg

echo "TOP"
