" Vim :eval command


eval "Foo"->append(0)

eval "Foo"
      \ ->append(
      \ 0
      \ )

eval "Foo"->append(0) | echo "Foo"

echo "Foo" | eval "Foo"->append(0)

